/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.util.List;
import java.util.Map;

/**
 * LLMClient implementation for the Generic (YAML-configured) vendor.
 * Delegates to GenericVendorConfig for request building and response parsing.
 */
public class GenericClient implements LLMClient {

    private final String apiKey;
    private final String model;
    private GenericVendorConfig config;

    public GenericClient(String apiKey, String model) {
        this.apiKey = apiKey;
        this.model = model;
    }

    /** Get the underlying config (for resetGuid on clear). */
    public GenericVendorConfig getConfig() {
        return config;
    }

    @Override
    public String chat(List<Map<String, String>> messages, String systemPrompt) throws Exception {
        if (config == null) {
            config = new GenericVendorConfig();
        } else {
            config.load(); // Reload in case user edited config
        }
        if (!config.isValid()) {
            throw new RuntimeException("Generic vendor not configured. Use Configure... in Preferences.");
        }

        // For single-shot mode, send just the latest user prompt
        String lastPrompt = "";
        for (int i = messages.size() - 1; i >= 0; i--) {
            if ("user".equals(messages.get(i).get("role"))) {
                lastPrompt = messages.get(i).get("content");
                break;
            }
        }

        String content = config.callPrompt(apiKey, model, lastPrompt, messages);
        if (content == null || content.isBlank()) {
            throw new RuntimeException("Empty response from Generic vendor");
        }
        return content;
    }
}
