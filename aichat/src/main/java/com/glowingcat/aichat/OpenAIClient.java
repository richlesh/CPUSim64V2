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
 * LLMClient implementation for OpenAI-compatible APIs (OpenAI, DeepSeek,
 * Cerebras, Groq, Google, Meta, Mistral, Moonshot AI, Ollama, Perplexity, xAI,
 * Alibaba, and Generic OpenAI API).
 */
public class OpenAIClient implements LLMClient {

    private final String baseUrl;
    private final String apiKey;
    private final String model;
    private final Duration requestTimeout;

    public OpenAIClient(String baseUrl, String apiKey, String model) {
        this(baseUrl, apiKey, model, Duration.ofSeconds(600));
    }

    public OpenAIClient(String baseUrl, String apiKey, String model, Duration requestTimeout) {
        this.baseUrl = baseUrl;
        this.apiKey = apiKey;
        this.model = model;
        this.requestTimeout = requestTimeout;
    }

    @Override
    public String chat(List<Map<String, String>> messages, String systemPrompt) throws Exception {
        StringBuilder body = new StringBuilder();
        body.append("{\"model\":\"").append(model).append("\",\"max_completion_tokens\":128000,\"messages\":[");
        for (int i = 0; i < messages.size(); i++) {
            if (i > 0) body.append(",");
            body.append("{\"role\":\"").append(messages.get(i).get("role"))
                .append("\",\"content\":").append(jsonString(messages.get(i).get("content"))).append("}");
        }
        body.append("]}");

        HttpRequest.Builder reqBuilder = HttpRequest.newBuilder()
            .uri(URI.create(baseUrl + "/chat/completions"))
            .header("Content-Type", "application/json")
            .timeout(requestTimeout)
            .POST(HttpRequest.BodyPublishers.ofString(body.toString()));

        if (apiKey != null && !apiKey.isEmpty()) {
            reqBuilder.header("Authorization", "Bearer " + apiKey);
        }

        HttpResponse<String> resp = HttpClient.newBuilder()
            .connectTimeout(Duration.ofSeconds(30))
            .build()
            .send(reqBuilder.build(), HttpResponse.BodyHandlers.ofString());

        String respBody = resp.body();
        String content = extractJsonValue(respBody, "content");
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
