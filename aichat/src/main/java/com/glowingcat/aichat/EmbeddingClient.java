/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.net.URI;
import java.net.http.*;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.*;

/**
 * Client for calling OpenAI-compatible embedding APIs.
 * Supports batching multiple inputs in a single request.
 */
public class EmbeddingClient {

    private final String baseUrl;
    private final String apiKey;
    private final String model;

    /** Default embedding models per vendor. */
    public static String defaultModel(String vendor) {
        VendorRegistry.VendorInfo info = VendorRegistry.getVendor(vendor);
        if (info != null && info.embeddingModel() != null) {
            return info.embeddingModel();
        }
        return "text-embedding-3-small";
    }

    /** Returns true if the vendor supports embeddings. */
    public static boolean supportsEmbeddings(String vendor) {
        if ("Generic".equals(vendor)) {
            // Check the Generic vendor's YAML config for Embedding section
            GenericVendorConfig config = new GenericVendorConfig();
            config.load();
            return config.hasEmbeddingConfig();
        }
        VendorRegistry.VendorInfo info = VendorRegistry.getVendor(vendor);
        return info != null && info.supportsEmbeddings();
    }

    public EmbeddingClient(String baseUrl, String apiKey, String model) {
        this.baseUrl = baseUrl;
        this.apiKey = apiKey;
        this.model = model;
    }

    /**
     * Create an EmbeddingClient from AIChatPreferences.
     * Returns null if the vendor doesn't support embeddings.
     */
    public static EmbeddingClient fromPreferences(AIChatPreferences prefs) {
        String vendor = prefs.getLlmVendor();
        if (!supportsEmbeddings(vendor)) return null;

        // Handle Generic vendor — use its YAML-configured embedding endpoint
        if ("Generic".equals(vendor)) {
            GenericVendorConfig config = new GenericVendorConfig();
            config.load();
            if (!config.hasEmbeddingConfig()) return null;
            String uri = config.getEmbeddingUri();
            // Strip trailing path to get base URL for EmbeddingClient
            // The URI in config is the full embeddings endpoint
            String baseUrl = uri.endsWith("/embeddings")
                ? uri.substring(0, uri.length() - "/embeddings".length())
                : uri;
            return new EmbeddingClient(baseUrl, prefs.getLlmApiKey(), config.getEmbeddingModel());
        }

        VendorRegistry.VendorInfo info = VendorRegistry.getVendor(vendor);
        String baseUrl;

        if ("Generic OpenAI API".equals(vendor)) {
            String ep = prefs.getLlmEndpoint();
            if (ep == null || ep.isBlank()) return null;
            if (ep.endsWith("/")) ep = ep.substring(0, ep.length() - 1);
            baseUrl = ep;
        } else if (info != null && !info.baseUrl().isEmpty()) {
            baseUrl = info.baseUrl();
        } else {
            baseUrl = "https://api.openai.com/v1";
        }

        return new EmbeddingClient(baseUrl, prefs.getLlmApiKey(), defaultModel(vendor));
    }

    /**
     * Embed a single text input.
     */
    public float[] embed(String text) throws Exception {
        List<float[]> results = embedBatch(List.of(text));
        return results.isEmpty() ? new float[0] : results.get(0);
    }

    /**
     * Embed a batch of text inputs in a single API call.
     * Returns a list of embedding vectors in the same order as inputs.
     */
    public List<float[]> embedBatch(List<String> texts) throws Exception {
        // Build JSON request body
        StringBuilder body = new StringBuilder();
        body.append("{\"model\":\"").append(model).append("\",\"input\":[");
        for (int i = 0; i < texts.size(); i++) {
            if (i > 0) body.append(",");
            body.append(jsonString(texts.get(i)));
        }
        body.append("]}");

        HttpRequest.Builder reqBuilder = HttpRequest.newBuilder()
            .uri(URI.create(baseUrl + "/embeddings"))
            .header("Content-Type", "application/json")
            .timeout(Duration.ofSeconds(60))
            .POST(HttpRequest.BodyPublishers.ofString(body.toString(), StandardCharsets.UTF_8));

        if (apiKey != null && !apiKey.isEmpty()) {
            reqBuilder.header("Authorization", "Bearer " + apiKey);
        }

        HttpResponse<String> resp = HttpClient.newBuilder()
            .connectTimeout(Duration.ofSeconds(30))
            .build()
            .send(reqBuilder.build(), HttpResponse.BodyHandlers.ofString());

        if (resp.statusCode() != 200) {
            throw new RuntimeException("Embedding API error " + resp.statusCode() + ": " +
                resp.body().substring(0, Math.min(200, resp.body().length())));
        }

        return parseEmbeddingResponse(resp.body());
    }

    /**
     * Parse the embedding response JSON to extract float[] vectors.
     * Response format: {"data": [{"embedding": [0.1, 0.2, ...], "index": 0}, ...]}
     */
    private List<float[]> parseEmbeddingResponse(String json) {
        List<float[]> results = new ArrayList<>();
        // Find each "embedding" array
        Pattern embPat = Pattern.compile("\"embedding\"\\s*:\\s*\\[([^\\]]+)]");
        Matcher m = embPat.matcher(json);
        while (m.find()) {
            String[] nums = m.group(1).split(",");
            float[] vec = new float[nums.length];
            for (int i = 0; i < nums.length; i++) {
                vec[i] = Float.parseFloat(nums[i].trim());
            }
            results.add(vec);
        }
        return results;
    }

    private static String jsonString(String s) {
        StringBuilder sb = new StringBuilder("\"");
        for (int i = 0; i < s.length(); i++) {
            char c = s.charAt(i);
            switch (c) {
                case '\\' -> sb.append("\\\\");
                case '"' -> sb.append("\\\"");
                case '\n' -> sb.append("\\n");
                case '\r' -> sb.append("\\r");
                case '\t' -> sb.append("\\t");
                default -> {
                    if (c < 0x20) sb.append(String.format("\\u%04x", (int) c));
                    else sb.append(c);
                }
            }
        }
        sb.append("\"");
        return sb.toString();
    }
}
