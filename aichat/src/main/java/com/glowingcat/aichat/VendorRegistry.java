/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import org.yaml.snakeyaml.Yaml;

import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.*;

/**
 * Registry of AI vendor configurations loaded from ai_vendors.yml.
 * Provides vendor metadata (API key URLs, base URLs, embedding models)
 * to LLMClientFactory, EmbeddingClient, and AIChatPreferencesDialog.
 */
public class VendorRegistry {

    private static final List<VendorInfo> vendors = new ArrayList<>();
    private static final Map<String, VendorInfo> vendorMap = new LinkedHashMap<>();

    static {
        load();
    }

    @SuppressWarnings("unchecked")
    private static void load() {
        try (InputStream is = VendorRegistry.class.getResourceAsStream("/ai_vendors.yml")) {
            if (is == null) return;
            Yaml yaml = new Yaml();
            Map<String, Object> root = yaml.load(is);
            List<Map<String, Object>> vendorList = (List<Map<String, Object>>) root.get("vendors");
            if (vendorList == null) return;

            for (Map<String, Object> entry : vendorList) {
                String name = (String) entry.getOrDefault("name", "");
                String apiKeyUrl = (String) entry.getOrDefault("apiKeyUrl", "");
                String baseUrl = (String) entry.getOrDefault("baseUrl", "");
                String clientType = (String) entry.getOrDefault("clientType", "openai");
                String embeddingModel = (String) entry.getOrDefault("embeddingModel", null);

                VendorInfo info = new VendorInfo(name, apiKeyUrl, baseUrl, clientType, embeddingModel);
                vendors.add(info);
                vendorMap.put(name, info);
            }
        } catch (Exception e) {
            // Fall back to empty registry
        }
    }

    /** Get all registered vendors in order. */
    public static List<VendorInfo> getVendors() {
        return Collections.unmodifiableList(vendors);
    }

    /** Get vendor info by name, or null if not found. */
    public static VendorInfo getVendor(String name) {
        return vendorMap.get(name);
    }

    /** Get all vendor names in order. */
    public static String[] getVendorNames() {
        return vendors.stream().map(VendorInfo::name).toArray(String[]::new);
    }

    /** Vendor configuration record. */
    public record VendorInfo(
        String name,
        String apiKeyUrl,
        String baseUrl,
        String clientType,
        String embeddingModel
    ) {
        /** Returns true if this vendor supports embeddings (has an embedding model configured). */
        public boolean supportsEmbeddings() {
            return embeddingModel != null && !embeddingModel.isEmpty();
        }
    }
}
