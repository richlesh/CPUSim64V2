/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.*;
import java.util.stream.*;

/**
 * Keyword-based (BM25-style) retrieval index for document chunks.
 * Used as a fallback when the LLM vendor doesn't support embeddings.
 */
class KeywordIndex implements Serializable {
    private static final long serialVersionUID = 1L;

    private final List<DocumentChunk> chunks;
    /** Inverted index: term → set of chunk indices */
    private final Map<String, List<Integer>> invertedIndex;
    /** Document frequency: term → number of chunks containing it */
    private final Map<String, Integer> docFrequency;
    /** Number of words per chunk */
    private final int[] chunkLengths;
    /** Average chunk length */
    private final double avgLength;

    private static final double K1 = 1.5;
    private static final double B = 0.75;

    KeywordIndex(List<DocumentChunk> chunks) {
        this.chunks = new ArrayList<>(chunks);
        this.invertedIndex = new HashMap<>();
        this.docFrequency = new HashMap<>();
        this.chunkLengths = new int[chunks.size()];

        long totalLength = 0;
        for (int i = 0; i < chunks.size(); i++) {
            List<String> terms = tokenize(chunks.get(i).text());
            chunkLengths[i] = terms.size();
            totalLength += terms.size();

            Set<String> seen = new HashSet<>();
            for (String term : terms) {
                invertedIndex.computeIfAbsent(term, k -> new ArrayList<>()).add(i);
                if (seen.add(term)) {
                    docFrequency.merge(term, 1, Integer::sum);
                }
            }
        }
        this.avgLength = chunks.isEmpty() ? 1.0 : (double) totalLength / chunks.size();
    }

    /**
     * Retrieve the top-K most relevant chunks for a query using BM25 scoring.
     */
    List<DocumentChunk> retrieve(String query, int topK) {
        List<String> queryTerms = tokenize(query);
        double[] scores = new double[chunks.size()];
        int n = chunks.size();

        for (String term : queryTerms) {
            Integer df = docFrequency.get(term);
            if (df == null) continue;

            // IDF component
            double idf = Math.log((n - df + 0.5) / (df + 0.5) + 1.0);

            List<Integer> postings = invertedIndex.get(term);
            if (postings == null) continue;

            // Count term frequency in each chunk
            Map<Integer, Integer> tfMap = new HashMap<>();
            for (int idx : postings) {
                tfMap.merge(idx, 1, Integer::sum);
            }

            for (var entry : tfMap.entrySet()) {
                int idx = entry.getKey();
                int tf = entry.getValue();
                double lengthNorm = chunkLengths[idx] / avgLength;
                double tfNorm = (tf * (K1 + 1)) / (tf + K1 * (1 - B + B * lengthNorm));
                scores[idx] += idf * tfNorm;
            }
        }

        // Get top-K indices by score
        return IntStream.range(0, scores.length)
            .filter(i -> scores[i] > 0)
            .boxed()
            .sorted((a, b) -> Double.compare(scores[b], scores[a]))
            .limit(topK)
            .map(chunks::get)
            .collect(Collectors.toList());
    }

    List<DocumentChunk> getChunks() {
        return Collections.unmodifiableList(chunks);
    }

    /**
     * Tokenize text into lowercase terms, stripping punctuation.
     */
    private static List<String> tokenize(String text) {
        List<String> terms = new ArrayList<>();
        String lower = text.toLowerCase();
        StringBuilder word = new StringBuilder();
        for (int i = 0; i < lower.length(); i++) {
            char c = lower.charAt(i);
            if (Character.isLetterOrDigit(c) || c == '_' || c == '#') {
                word.append(c);
            } else {
                if (!word.isEmpty()) {
                    terms.add(word.toString());
                    word.setLength(0);
                }
            }
        }
        if (!word.isEmpty()) terms.add(word.toString());
        return terms;
    }

    /** Save index to file. */
    void save(Path path) throws IOException {
        Files.createDirectories(path.getParent());
        try (ObjectOutputStream oos = new ObjectOutputStream(
                new BufferedOutputStream(Files.newOutputStream(path)))) {
            oos.writeObject(this);
        }
    }

    /** Load index from file. */
    static KeywordIndex load(Path path) throws IOException, ClassNotFoundException {
        try (ObjectInputStream ois = new ObjectInputStream(
                new BufferedInputStream(Files.newInputStream(path)))) {
            return (KeywordIndex) ois.readObject();
        }
    }
}
