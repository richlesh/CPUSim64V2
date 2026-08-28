/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.io.*;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

/**
 * RAG-based document retriever. Loads documentation from classpath resources,
 * chunks them, and provides relevant chunks for a given query.
 *
 * <p>Uses embedding-based retrieval when the vendor supports embeddings,
 * falls back to BM25 keyword retrieval otherwise.
 *
 * <p>Caches indexes to &lt;cacheBaseDir&gt;/embeddings/ or &lt;cacheBaseDir&gt;/keywords/
 * and invalidates when the source jar is newer than the cache.
 */
public class DocumentRetriever {

    private static final int CHUNK_SIZE = 64*1024;  // target chars per chunk
    private static final int CHUNK_OVERLAP = 1024;  // overlap between chunks

    private final Path cacheBaseDir;
    private final List<String> indexPaths;
    private final int topK;
    private EmbeddingIndex embeddingIndex;
    private KeywordIndex keywordIndex;
    private EmbeddingClient embeddingClient;
    private boolean useEmbeddings;
    private boolean initialized = false;
    private String initializedVendor = null;
    private boolean embeddingFailed = false;

    /**
     * Create a retriever with the specified cache base directory and index files.
     * Each index file is a classpath resource containing document paths (one per line)
     * to load, chunk, and index for retrieval.
     *
     * @param cacheBaseDir base directory for cache files (e.g. ~/.myapp)
     * @param indexPaths   classpath resource paths to index files (e.g. "/documentation/doc-index.txt")
     * @param topK         maximum number of chunks to return on retrieval
     */
    public DocumentRetriever(Path cacheBaseDir, List<String> indexPaths, int topK) {
        this.cacheBaseDir = cacheBaseDir;
        this.indexPaths = List.copyOf(indexPaths);
        this.topK = topK;
    }

    /**
     * Check if the retriever needs (re-)initialization for the given vendor.
     * Returns true if the index doesn't exist or is out of date.
     * This is a lightweight check — does not load indexes into memory.
     */
    public boolean needsInitialization(AIChatPreferences prefs) {
        String vendor = prefs.getLlmVendor();

        // Already initialized for this vendor
        if (initialized && vendor.equals(initializedVendor)) return false;

        // Check if cache exists and is current
        String safeVendor = vendor.replaceAll("[^a-zA-Z0-9_-]", "_");
        boolean supportsEmb = EmbeddingClient.supportsEmbeddings(vendor);
        Path cachePath = getCachePath(supportsEmb ? "embeddings" : "keywords", safeVendor);
        if (!Files.exists(cachePath)) return true;

        try {
            long cacheTime = Files.getLastModifiedTime(cachePath).toMillis();
            long sourceTime = getDocumentSourceTimestamp();
            if (sourceTime > cacheTime) return true;
        } catch (Exception e) {
            return true;
        }

        // Cache exists and is current — still needs initialization (loading into memory)
        return true;
    }

    /**
     * Initialize the retriever for the given vendor preferences.
     * This will load or build the appropriate index (embedding or keyword).
     * If a valid cache exists, loads from cache. Otherwise builds the index
     * (which may make API calls if embeddings need to be computed).
     */
    public void initialize(AIChatPreferences prefs) {
        String vendor = prefs.getLlmVendor();
        useEmbeddings = EmbeddingClient.supportsEmbeddings(vendor);
        embeddingFailed = false;
        String safeVendor = vendor.replaceAll("[^a-zA-Z0-9_-]", "_");

        // Try loading from a valid cache first (fast path)
        Path cachePath = getCachePath(useEmbeddings ? "embeddings" : "keywords", safeVendor);
        if (Files.exists(cachePath)) {
            try {
                long cacheTime = Files.getLastModifiedTime(cachePath).toMillis();
                long sourceTime = getDocumentSourceTimestamp();
                if (cacheTime >= sourceTime) {
                    if (useEmbeddings) {
                        embeddingIndex = EmbeddingIndex.load(cachePath);
                        embeddingClient = EmbeddingClient.fromPreferences(prefs);
                        // Also build keyword index as fallback
                        List<DocumentChunk> chunks = loadAndChunkDocuments();
                        if (!chunks.isEmpty()) {
                            keywordIndex = new KeywordIndex(chunks);
                        }
                    } else {
                        keywordIndex = KeywordIndex.load(cachePath);
                    }
                    initialized = true;
                    initializedVendor = vendor;
                    return;
                }
            } catch (Exception e) {
                // Cache corrupt or load failed — fall through to full rebuild
            }
        }

        // Full rebuild: load documents, chunk, and index
        List<DocumentChunk> chunks = loadAndChunkDocuments();
        if (chunks.isEmpty()) {
            initialized = true;
            initializedVendor = vendor;
            return;
        }

        long sourceTimestamp = getDocumentSourceTimestamp();

        if (useEmbeddings) {
            embeddingClient = EmbeddingClient.fromPreferences(prefs);
            if (embeddingClient == null) {
                // Fall back to keywords if we can't create embedding client
                useEmbeddings = false;
                initKeywordIndex(chunks, safeVendor, sourceTimestamp);
            } else {
                initEmbeddingIndex(chunks, safeVendor, sourceTimestamp);
            }
        } else {
            initKeywordIndex(chunks, safeVendor, sourceTimestamp);
        }

        initialized = true;
        initializedVendor = vendor;
    }

    /**
     * Retrieve the top-K most relevant document chunks for a user query.
     *
     * @param query the user's message text
     * @return list of relevant chunks formatted as "source:\ntext"
     */
    public List<String> retrieve(String query) {
        if (!initialized) return List.of();

        List<DocumentChunk> results;
        if (useEmbeddings && embeddingIndex != null && embeddingClient != null) {
            try {
                float[] queryVec = embeddingClient.embed(query);
                results = embeddingIndex.retrieve(queryVec, topK);
            } catch (Exception e) {
                // Fall back to keyword search on embedding failure
                if (keywordIndex != null) {
                    results = keywordIndex.retrieve(query, topK);
                } else {
                    return List.of();
                }
            }
        } else if (keywordIndex != null) {
            results = keywordIndex.retrieve(query, topK);
        } else {
            return List.of();
        }

        return results.stream()
            .map(c -> c.source() + ":\n" + c.text())
            .toList();
    }

    public boolean isInitialized() {
        return initialized;
    }

    /**
     * Returns true if embedding indexing was attempted but failed,
     * causing a fallback to keyword-based retrieval.
     */
    public boolean didEmbeddingFail() {
        return embeddingFailed;
    }

    // --- Index initialization ---

    private void initEmbeddingIndex(List<DocumentChunk> chunks, String vendor, long sourceTimestamp) {
        Path cachePath = getCachePath("embeddings", vendor);

        // Try loading from cache
        if (Files.exists(cachePath)) {
            try {
                long cacheTime = Files.getLastModifiedTime(cachePath).toMillis();
                if (cacheTime > sourceTimestamp) {
                    embeddingIndex = EmbeddingIndex.load(cachePath);
                    // Also build keyword index as fallback
                    keywordIndex = new KeywordIndex(chunks);
                    return;
                }
            } catch (Exception e) {
                // Cache corrupt, rebuild
            }
        }

        // Compute embeddings in batches
        try {
            List<String> texts = chunks.stream().map(DocumentChunk::text).toList();
            float[][] allEmbeddings = new float[texts.size()][];

            int batchSize = 100;
            for (int i = 0; i < texts.size(); i += batchSize) {
                int end = Math.min(i + batchSize, texts.size());
                List<String> batch = texts.subList(i, end);
                List<float[]> batchResults = embeddingClient.embedBatch(batch);
                for (int j = 0; j < batchResults.size(); j++) {
                    allEmbeddings[i + j] = batchResults.get(j);
                }
            }

            embeddingIndex = new EmbeddingIndex(chunks, allEmbeddings);
            embeddingIndex.save(cachePath);
        } catch (Exception e) {
            // Embedding failed, fall back to keywords
            useEmbeddings = false;
            embeddingIndex = null;
            embeddingFailed = true;
        }

        // Always build keyword index as fallback
        keywordIndex = new KeywordIndex(chunks);
    }

    private void initKeywordIndex(List<DocumentChunk> chunks, String vendor, long sourceTimestamp) {
        Path cachePath = getCachePath("keywords", vendor);

        // Try loading from cache
        if (Files.exists(cachePath)) {
            try {
                long cacheTime = Files.getLastModifiedTime(cachePath).toMillis();
                if (cacheTime > sourceTimestamp) {
                    keywordIndex = KeywordIndex.load(cachePath);
                    return;
                }
            } catch (Exception e) {
                // Cache corrupt, rebuild
            }
        }

        // Build and save
        keywordIndex = new KeywordIndex(chunks);
        try {
            keywordIndex.save(cachePath);
        } catch (IOException e) {
            // Non-fatal: index works in memory
        }
    }

    // --- Document loading and chunking ---

    private List<DocumentChunk> loadAndChunkDocuments() {
        List<DocumentChunk> chunks = new ArrayList<>();

        for (String indexPath : indexPaths) {
            // Determine the base directory from the index file path
            String baseDir = indexPath.substring(0, indexPath.lastIndexOf('/') + 1);
            List<String> files = readIndexFile(indexPath);
            for (String filename : files) {
                String content = loadResource(baseDir + filename);
                if (content == null || content.isEmpty()) continue;

                // HTML files: split at heading boundaries, then chunk each section
                if (filename.endsWith(".html") || filename.endsWith(".htm")) {
                    chunks.addAll(chunkHtmlBySections(filename, content));
                    continue;
                }

                // Always keep .asm files as whole units (example programs)
                if (filename.endsWith(".asm") || content.length() <= CHUNK_SIZE * 2) {
                    chunks.add(new DocumentChunk(filename, content));
                } else {
                    chunks.addAll(chunkText(filename, content));
                }
            }
        }

        return chunks;
    }

    /**
     * Split an HTML document into sections at heading boundaries (h1–h4),
     * strip HTML from each section, and chunk only if a section still exceeds CHUNK_SIZE.
     * This keeps related content (e.g., a library's function table) together.
     */
    private List<DocumentChunk> chunkHtmlBySections(String filename, String html) {
        List<DocumentChunk> chunks = new ArrayList<>();

        // Split at heading tags (h1-h4), keeping the heading with its section
        Pattern headingPattern = Pattern.compile("(?=<h[1-4][^>]*>)", Pattern.CASE_INSENSITIVE);
        String[] sections = headingPattern.split(html);

        for (String section : sections) {
            String text = stripHtml(section).trim();
            if (text.isEmpty()) continue;

            // Extract heading text for the section label
            String label = filename;
            Matcher hm = Pattern.compile("<h[1-4][^>]*>(.*?)</h[1-4]>", Pattern.CASE_INSENSITIVE | Pattern.DOTALL).matcher(section);
            if (hm.find()) {
                String heading = hm.group(1).replaceAll("<[^>]+>", "").trim();
                if (!heading.isEmpty()) {
                    label = filename + " — " + heading;
                }
            }

            // Keep each section as a whole unit — heading-based splitting
            // already produces semantically meaningful chunks
            chunks.add(new DocumentChunk(label, text));
        }

        return chunks;
    }

    /**
     * Split text into overlapping chunks of approximately CHUNK_SIZE characters.
     * Splits at paragraph/line boundaries when possible.
     */
    private List<DocumentChunk> chunkText(String source, String text) {
        List<DocumentChunk> chunks = new ArrayList<>();
        if (text.length() <= CHUNK_SIZE) {
            chunks.add(new DocumentChunk(source, text));
            return chunks;
        }

        int pos = 0;
        int chunkNum = 1;
        while (pos < text.length()) {
            int end = Math.min(pos + CHUNK_SIZE, text.length());

            // Try to break at a paragraph or line boundary
            if (end < text.length()) {
                int paraBreak = text.lastIndexOf("\n\n", end);
                if (paraBreak > pos) {
                    end = paraBreak + 2;
                } else {
                    int lineBreak = text.lastIndexOf('\n', end);
                    if (lineBreak > pos) {
                        end = lineBreak + 1;
                    }
                }
            }

            String chunk = text.substring(pos, end).trim();
            if (!chunk.isEmpty()) {
                String label = chunks.isEmpty() ? source : source + " (part " + chunkNum + ")";
                chunks.add(new DocumentChunk(label, chunk));
                chunkNum++;
            }

            // Advance: use overlap only if there's enough remaining text
            int nextPos = end - CHUNK_OVERLAP;
            if (nextPos <= pos) nextPos = end; // always advance forward
            pos = nextPos;
        }

        return chunks;
    }

    // --- Utility methods ---

    private Path getCachePath(String type, String vendor) {
        return cacheBaseDir.resolve(type).resolve(vendor + ".cache");
    }

    /**
     * Get the modification timestamp of the JAR/directory containing the documentation resources.
     */
    private long getDocumentSourceTimestamp() {
        try {
            if (indexPaths.isEmpty()) return 0;
            URL url = getClass().getResource(indexPaths.get(0));
            if (url == null) return 0;

            String protocol = url.getProtocol();
            if ("jar".equals(protocol)) {
                // Extract jar path from jar:file:/path/to/file.jar!/documentation/doc-index.txt
                String jarPath = url.getPath();
                int bangIdx = jarPath.indexOf('!');
                if (bangIdx > 0) {
                    jarPath = jarPath.substring(0, bangIdx);
                    if (jarPath.startsWith("file:")) {
                        jarPath = jarPath.substring(5);
                    }
                    return Files.getLastModifiedTime(Path.of(jarPath)).toMillis();
                }
            } else if ("file".equals(protocol)) {
                // Running from exploded classes directory
                Path filePath = Path.of(url.toURI());
                return Files.getLastModifiedTime(filePath).toMillis();
            }
        } catch (Exception e) {
            // Fall through
        }
        return 0;
    }

    /**
     * Load HTML documents listed in the given index files, strip their HTML tags,
     * and concatenate the resulting plain text. Intended for embedding documentation
     * directly into the system prompt (as opposed to RAG indexing).
     *
     * @param indexPaths classpath resource paths to index files (e.g. "/documentation/doc-index.txt")
     * @return concatenated plain-text content of all listed documents
     */
    public static String loadAndStripDocuments(List<String> indexPaths) {
        StringBuilder sb = new StringBuilder();
        for (String indexPath : indexPaths) {
            String baseDir = indexPath.substring(0, indexPath.lastIndexOf('/') + 1);
            List<String> files = readIndexFile(indexPath);
            for (String filename : files) {
                String content = loadResource(baseDir + filename);
                if (content == null || content.isEmpty()) continue;
                String text;
                if (filename.endsWith(".html") || filename.endsWith(".htm")) {
                    text = stripHtml(content);
                } else {
                    text = content;
                }
                if (!text.isBlank()) {
                    sb.append("\n\n## ").append(filename).append("\n\n");
                    sb.append(text.trim());
                }
            }
        }
        return sb.toString();
    }

    private static List<String> readIndexFile(String resourcePath) {
        List<String> files = new ArrayList<>();
        try (var is = DocumentRetriever.class.getResourceAsStream(resourcePath)) {
            if (is != null) {
                String content = new String(is.readAllBytes(), StandardCharsets.UTF_8);
                for (String line : content.split("\n")) {
                    line = line.trim();
                    if (!line.isEmpty() && !line.startsWith("#")) {
                        files.add(line);
                    }
                }
            }
        } catch (Exception ignored) {}
        return files;
    }

    private static String loadResource(String resourcePath) {
        try (var is = DocumentRetriever.class.getResourceAsStream(resourcePath)) {
            if (is != null) {
                return new String(is.readAllBytes(), StandardCharsets.UTF_8);
            }
        } catch (Exception ignored) {}
        return null;
    }

    /**
     * Strip HTML tags and return clean body text.
     */
    private static String stripHtml(String html) {
        // Remove everything before <body>
        int bodyStart = html.toLowerCase().indexOf("<body");
        if (bodyStart >= 0) {
            int bodyTagEnd = html.indexOf('>', bodyStart);
            if (bodyTagEnd >= 0) html = html.substring(bodyTagEnd + 1);
        }
        int bodyEnd = html.toLowerCase().indexOf("</body>");
        if (bodyEnd >= 0) html = html.substring(0, bodyEnd);

        // Remove script, style, nav, footer blocks
        html = html.replaceAll("(?si)<script[^>]*>.*?</script>", "");
        html = html.replaceAll("(?si)<style[^>]*>.*?</style>", "");
        html = html.replaceAll("(?si)<nav[^>]*>.*?</nav>", "");
        html = html.replaceAll("(?si)<footer[^>]*>.*?</footer>", "");

        // Replace block-level tags with newlines
        html = html.replaceAll("(?i)<br[^>]*>", "\n");
        html = html.replaceAll("(?i)</(p|div|h[1-6]|li|tr|dt|dd|blockquote|pre|section|article)>", "\n");
        html = html.replaceAll("(?i)<(p|div|h[1-6]|li|tr|dt|dd|blockquote|section|article)[^>]*>", "\n");

        // Remove all remaining tags (replace with space to preserve word boundaries)
        html = html.replaceAll("<[^>]+>", " ");

        // Decode HTML entities
        html = html.replaceAll("&amp;", "&");
        html = html.replaceAll("&lt;", "<");
        html = html.replaceAll("&gt;", ">");
        html = html.replaceAll("&quot;", "\"");
        html = html.replaceAll("&apos;", "'");
        html = html.replaceAll("&nbsp;", " ");
        html = html.replaceAll("&#(\\d+);", "");
        html = html.replaceAll("&\\w+;", "");

        // Collapse whitespace
        html = html.replaceAll("[ \\t]+", " ");
        html = html.replaceAll(" *\\n *", "\n");
        html = html.replaceAll("\\n{3,}", "\n\n");

        return html.trim();
    }
}
