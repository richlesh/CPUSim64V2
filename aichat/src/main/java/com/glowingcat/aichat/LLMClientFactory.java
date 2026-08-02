/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

/**
 * Factory that creates the appropriate LLMClient based on AIChatPreferences.
 */
public class LLMClientFactory {

    private LLMClientFactory() {}

    /**
     * Create an LLMClient for the currently configured vendor.
     *
     * @param prefs the AI chat preferences
     * @return an appropriate LLMClient implementation
     */
    public static LLMClient create(AIChatPreferences prefs) {
        String vendor = prefs.getLlmVendor();
        String apiKey = prefs.getLlmApiKey();
        String model = prefs.getLlmModel();

        VendorRegistry.VendorInfo info = VendorRegistry.getVendor(vendor);

        if (info != null && "generic".equals(info.clientType())) {
            return new GenericClient(apiKey, model);
        }

        if (info != null && "anthropic".equals(info.clientType())) {
            return new AnthropicClient(apiKey, model);
        }

        // OpenAI-compatible client
        String baseUrl;
        if ("Generic OpenAI API".equals(vendor)) {
            String ep = prefs.getLlmEndpoint();
            if (ep == null || ep.isBlank()) throw new RuntimeException("No endpoint configured for Generic OpenAI API");
            if (ep.endsWith("/")) ep = ep.substring(0, ep.length() - 1);
            baseUrl = ep;
        } else if (info != null && !info.baseUrl().isEmpty()) {
            baseUrl = info.baseUrl();
        } else {
            baseUrl = "https://api.openai.com/v1";
        }

        return new OpenAIClient(baseUrl, apiKey, model);
    }
}
