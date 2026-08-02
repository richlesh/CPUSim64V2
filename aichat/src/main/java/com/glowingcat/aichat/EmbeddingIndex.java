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
 * Embedding-based vector retrieval index for document chunks.
 * Stores pre-computed embeddings and performs cosine similarity search.
 */
class EmbeddingIndex implements Serializable {
    private static final long serialVersionUID = 1L;

    private final List<DocumentChunk> chunks;
    private final float[][] embeddings;

    EmbeddingIndex(List<DocumentChunk> chunks, float[][] embeddings) {
        this.chunks = new ArrayList<>(chunks);
        this.embeddings = embeddings;
    }

    /**
     * Retrieve the top-K most relevant chunks for a query embedding using cosine similarity.
     */
    List<DocumentChunk> retrieve(float[] queryEmbedding, int topK) {
        double[] scores = new double[chunks.size()];
        for (int i = 0; i < embeddings.length; i++) {
            scores[i] = cosineSimilarity(queryEmbedding, embeddings[i]);
        }

        return IntStream.range(0, scores.length)
            .boxed()
            .sorted((a, b) -> Double.compare(scores[b], scores[a]))
            .limit(topK)
            .map(chunks::get)
            .collect(Collectors.toList());
    }

    List<DocumentChunk> getChunks() {
        return Collections.unmodifiableList(chunks);
    }

    private static double cosineSimilarity(float[] a, float[] b) {
        if (a.length != b.length) return 0;
        double dot = 0, normA = 0, normB = 0;
        for (int i = 0; i < a.length; i++) {
            dot += a[i] * b[i];
            normA += a[i] * a[i];
            normB += b[i] * b[i];
        }
        double denom = Math.sqrt(normA) * Math.sqrt(normB);
        return denom == 0 ? 0 : dot / denom;
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
    static EmbeddingIndex load(Path path) throws IOException, ClassNotFoundException {
        try (ObjectInputStream ois = new ObjectInputStream(
                new BufferedInputStream(Files.newInputStream(path)))) {
            return (EmbeddingIndex) ois.readObject();
        }
    }
}
