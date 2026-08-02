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

    private static final int CHUNK_SIZE = 800;  // target chars per chunk
    private static final int CHUNK_OVERLAP = 100; // overlap between chunks

    private final Path cacheBaseDir;
    private final List<String> indexPaths;
    private final int topK;
    private EmbeddingIndex embeddingIndex;
    private KeywordIndex keywordIndex;
    private EmbeddingClient embeddingClient;
    private boolean useEmbeddings;
    private boolean initialized = false;
    private String initializedVendor = null;

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
     */
    public boolean needsInitialization(AIChatPreferences prefs) {
        String vendor = prefs.getLlmVendor();
        String safeVendor = vendor.replaceAll("[^a-zA-Z0-9_-]", "_");

        // Already initialized for this vendor
        if (initialized && vendor.equals(initializedVendor)) return false;

        // Check if cache exists and is current
        boolean supportsEmb = EmbeddingClient.supportsEmbeddings(vendor);
        Path cachePath = getCachePath(supportsEmb ? "embeddings" : "keywords", safeVendor);
        if (!Files.exists(cachePath)) return true;

        try {
            long cacheTime = Files.getLastModifiedTime(cachePath).toMillis();
            long sourceTime = getDocumentSourceTimestamp();
            return sourceTime > cacheTime;
        } catch (Exception e) {
            return true;
        }
    }

    /**
     * Initialize the retriever for the given vendor preferences.
     * This will load or build the appropriate index (embedding or keyword).
     * May make API calls if embeddings need to be computed.
     */
    public void initialize(AIChatPreferences prefs) {
        String vendor = prefs.getLlmVendor();
        useEmbeddings = EmbeddingClient.supportsEmbeddings(vendor);

        List<DocumentChunk> chunks = loadAndChunkDocuments();
        if (chunks.isEmpty()) {
            initialized = true;
            initializedVendor = vendor;
            return;
        }

        long sourceTimestamp = getDocumentSourceTimestamp();
        String safeVendor = vendor.replaceAll("[^a-zA-Z0-9_-]", "_");

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

                // Strip HTML if the file appears to be HTML
                String text = content;
                if (filename.endsWith(".html") || filename.endsWith(".htm")) {
                    text = stripHtml(content);
                    if (text.isEmpty()) continue;
                }

                // Don't chunk small files — keep whole
                if (text.length() <= CHUNK_SIZE * 2) {
                    chunks.add(new DocumentChunk(filename, text));
                } else {
                    chunks.addAll(chunkText(filename, text));
                }
            }
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

        // Remove all remaining tags
        html = html.replaceAll("<[^>]+>", "");

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
