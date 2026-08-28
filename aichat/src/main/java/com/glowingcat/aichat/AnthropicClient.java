/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.net.URI;
import java.net.http.*;
import java.time.Duration;
import java.util.List;
import java.util.Map;

/**
 * LLMClient implementation for the Anthropic Messages API.
 */
public class AnthropicClient implements LLMClient {

    private final String apiKey;
    private final String model;
    private final String baseUrl;

    public AnthropicClient(String apiKey, String model) {
        this.apiKey = apiKey;
        this.model = model;
        VendorRegistry.VendorInfo info = VendorRegistry.getVendor("Anthropic");
        this.baseUrl = (info != null && !info.baseUrl().isEmpty()) ? info.baseUrl() : "https://api.anthropic.com/v1";
    }

    @Override
    public String chat(List<Map<String, String>> messages, String systemPrompt) throws Exception {
        StringBuilder body = new StringBuilder();
        body.append("{\"model\":\"").append(model).append("\",\"max_tokens\":128000,");
        String sys = messages.stream()
            .filter(m -> "system".equals(m.get("role")))
            .map(m -> m.get("content"))
            .findFirst().orElse("");
        body.append("\"system\":").append(jsonString(sys)).append(",\"messages\":[");
        boolean first = true;
        for (var m : messages) {
            if ("system".equals(m.get("role"))) continue;
            if (!first) body.append(",");
            body.append("{\"role\":\"").append(m.get("role"))
                .append("\",\"content\":").append(jsonString(m.get("content"))).append("}");
            first = false;
        }
        body.append("]}");

        HttpRequest req = HttpRequest.newBuilder()
            .uri(URI.create(baseUrl + "/messages"))
            .header("Content-Type", "application/json")
            .header("x-api-key", apiKey)
            .header("anthropic-version", "2023-06-01")
            .timeout(Duration.ofSeconds(600))
            .POST(HttpRequest.BodyPublishers.ofString(body.toString()))
            .build();

        HttpResponse<String> resp = HttpClient.newBuilder()
            .connectTimeout(Duration.ofSeconds(30))
            .build()
            .send(req, HttpResponse.BodyHandlers.ofString());

        String respBody = resp.body();
        String content = extractJsonValue(respBody, "text");
        if (content == null) {
            throw new RuntimeException("Unexpected response: " + respBody.substring(0, Math.min(300, respBody.length())));
        }
        return content;
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
                case '\b' -> sb.append("\\b");
                case '\f' -> sb.append("\\f");
                default -> {
                    if (c < 0x20) {
                        sb.append(String.format("\\u%04x", (int) c));
                    } else {
                        sb.append(c);
                    }
                }
            }
        }
        sb.append("\"");
        return sb.toString();
    }

    private static String extractJsonValue(String json, String key) {
        String pattern = "\"" + key + "\"";
        int idx = json.lastIndexOf(pattern);
        if (idx < 0) return null;
        int colonIdx = json.indexOf(':', idx + pattern.length());
        if (colonIdx < 0) return null;
        int i = colonIdx + 1;
        while (i < json.length() && Character.isWhitespace(json.charAt(i))) i++;
        if (i >= json.length() || json.charAt(i) != '"') return null;
        i++;
        StringBuilder sb = new StringBuilder();
        while (i < json.length()) {
            char c = json.charAt(i);
            if (c == '\\' && i + 1 < json.length()) {
                char next = json.charAt(i + 1);
                switch (next) {
                    case 'n' -> sb.append('\n');
                    case 'r' -> sb.append('\r');
                    case 't' -> sb.append('\t');
                    case '"' -> sb.append('"');
                    case '\\' -> sb.append('\\');
                    case '/' -> sb.append('/');
                    case 'u' -> {
                        if (i + 5 < json.length()) {
                            sb.append((char) Integer.parseInt(json.substring(i + 2, i + 6), 16));
                            i += 4;
                        }
                    }
                    default -> { sb.append('\\'); sb.append(next); }
                }
                i += 2;
            } else if (c == '"') {
                return sb.toString();
            } else {
                sb.append(c);
                i++;
            }
        }
        return sb.toString();
    }
}
