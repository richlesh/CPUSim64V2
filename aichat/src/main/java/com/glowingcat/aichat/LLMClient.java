/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.util.List;
import java.util.Map;

/**
 * Strategy interface for calling LLM APIs. Implementations handle
 * vendor-specific request formatting, authentication, and response parsing.
 */
public interface LLMClient {

    /**
     * Send a chat completion request to the LLM.
     *
     * @param messages     the full conversation history (list of role/content maps)
     * @param systemPrompt the system prompt text
     * @return the assistant's response content
     * @throws Exception if the request fails
     */
    String chat(List<Map<String, String>> messages, String systemPrompt) throws Exception;
}
