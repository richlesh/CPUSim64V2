/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import org.yaml.snakeyaml.Yaml;

import java.io.*;
import java.net.URI;
import java.net.http.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.time.Duration;
import java.util.*;
import java.util.regex.*;

/**
 * Manages a Generic vendor YAML configuration for custom LLM API endpoints.
 * The YAML is stored at ~/.glowingcat-generic.yml and defines how to
 * call a chat/prompt API and a models-listing API with configurable request
 * format, headers, and response parsing via JSONPath-like expressions.
 *
 * Supports two conversation modes:
 *   - single-shot: sends only the latest user prompt with a conversation GUID
 *   - multi-turn: sends the full message history array
 */
public class GenericVendorConfig {

    private static final String CONFIG_FILENAME = ".glowingcat-generic.yml";
    private static final Path CONFIG_PATH = Paths.get(System.getProperty("user.home"), CONFIG_FILENAME);

    // Parsed config sections
    private Map<String, Object> promptConfig;
    private Map<String, Object> modelsConfig;
    private Map<String, Object> authConfig;
    private Map<String, Object> embeddingConfig;

    // Conversation GUID — generated once per session/clear
    private String conversationGuid = UUID.randomUUID().toString();

    // Cached auth token and expiry
    private String cachedAccessToken = null;
    private long tokenExpiryTime = 0;

    /** Default YAML template for new configurations, loaded from resources/generic_vendor.yml. */
    public static final String DEFAULT_YAML = loadDefaultYaml();

    private static String loadDefaultYaml() {
        try (var is = GenericVendorConfig.class.getResourceAsStream("/generic_vendor.yml")) {
            if (is != null) {
                return new String(is.readAllBytes(), StandardCharsets.UTF_8);
            }
        } catch (Exception e) {
            // Fall through to hardcoded fallback
        }
        return "# Generic LLM Vendor Configuration\n# See documentation for setup instructions.\n";
    }

    /**
     * Apply TrustStore settings from the YAML config as JVM system properties.
     * Must be called early at application startup, before any HTTPS connections.
     * Changes require a restart to take effect.
     */
    @SuppressWarnings("unchecked")
    public static void applyTrustStore() {
        try {
            String yamlContent = loadYamlString();
            Yaml yaml = new Yaml();
            Map<String, Object> root = yaml.load(yamlContent);
            if (root == null) return;

            Map<String, Object> trustStoreConfig = (Map<String, Object>) root.get("TrustStore");
            if (trustStoreConfig == null) return;

            String path = trustStoreConfig.get("Path") != null ? trustStoreConfig.get("Path").toString() : null;
            String password = trustStoreConfig.get("Password") != null ? trustStoreConfig.get("Password").toString() : null;
            String type = trustStoreConfig.get("Type") != null ? trustStoreConfig.get("Type").toString() : "JKS";

            if (path == null || path.isBlank()) return;

            // Expand ~ to user home directory
            if (path.startsWith("~")) {
                path = System.getProperty("user.home") + path.substring(1);
            }

            // Only apply if the file exists
            if (!Files.exists(Paths.get(path))) return;

            System.setProperty("javax.net.ssl.trustStore", path);
            if (password != null && !password.isBlank()) {
                System.setProperty("javax.net.ssl.trustStorePassword", password);
            }
            System.setProperty("javax.net.ssl.trustStoreType", type);
        } catch (Exception e) {
            // Silently fail — don't prevent app from launching
        }
    }

    public GenericVendorConfig() {
        load();
    }

    /** Reset conversation GUID (called on Clear). */
    public void resetGuid() {
        conversationGuid = UUID.randomUUID().toString();
    }

    /** Get the current conversation GUID. */
    public String getGuid() {
        return conversationGuid;
    }

    /** Load config from disk; if missing or invalid, use defaults. */
    @SuppressWarnings("unchecked")
    public void load() {
        String yamlContent = loadYamlString();
        try {
            Yaml yaml = new Yaml();
            Map<String, Object> root = yaml.load(yamlContent);
            if (root != null) {
                promptConfig = (Map<String, Object>) root.get("Prompt");
                modelsConfig = (Map<String, Object>) root.get("Models");
                authConfig = (Map<String, Object>) root.get("Auth");
                embeddingConfig = (Map<String, Object>) root.get("Embedding");
            }
        } catch (Exception e) {
            // If parsing fails, leave configs null — calls will fail gracefully
            promptConfig = null;
            modelsConfig = null;
            authConfig = null;
            embeddingConfig = null;
        }
    }

    /** Load the raw YAML string from disk; returns default if file doesn't exist. */
    public static String loadYamlString() {
        if (Files.exists(CONFIG_PATH)) {
            try {
                return Files.readString(CONFIG_PATH, StandardCharsets.UTF_8);
            } catch (IOException e) {
                return DEFAULT_YAML;
            }
        }
        return DEFAULT_YAML;
    }

    /** Save the given YAML string to disk. */
    public static void saveYamlString(String yaml) {
        try {
            Files.writeString(CONFIG_PATH, yaml, StandardCharsets.UTF_8);
        } catch (IOException e) {
            // Silently fail — non-critical
        }
    }

    /** Whether the configuration is valid (has at least a Prompt section). */
    public boolean isValid() {
        return promptConfig != null;
    }

    /** Returns true if embedding configuration is present with a URI and Model. */
    public boolean hasEmbeddingConfig() {
        if (embeddingConfig == null) return false;
        String uri = getEmbeddingUri();
        String model = getEmbeddingModel();
        return uri != null && !uri.isBlank() && model != null && !model.isBlank();
    }

    /** Get the embedding API URI, or null if not configured. */
    public String getEmbeddingUri() {
        if (embeddingConfig == null) return null;
        Object val = embeddingConfig.get("URI");
        return val != null ? val.toString().trim() : null;
    }

    /** Get the embedding model name, or null if not configured. */
    public String getEmbeddingModel() {
        if (embeddingConfig == null) return null;
        Object val = embeddingConfig.get("Model");
        return val != null ? val.toString().trim() : null;
    }

    /** Get the conversation mode: "single-shot" or "multi-turn". */
    public String getConversationMode() {
        if (promptConfig == null) return "single-shot";
        Object mode = promptConfig.get("ConversationMode");
        if (mode instanceof String s) {
            return s.trim().toLowerCase();
        }
        return "single-shot";
    }

    /**
     * Call the prompt/chat endpoint.
     *
     * @param authToken  the API key / auth token from preferences
     * @param model      the selected model
     * @param prompt     the current user prompt text
     * @param messages   full conversation history (list of role/content maps)
     * @return the extracted response content
     */
    @SuppressWarnings("unchecked")
    public String callPrompt(String authToken, String model, String prompt,
                             List<Map<String, String>> messages) throws Exception {
        if (promptConfig == null) {
            throw new RuntimeException("Generic vendor not configured. Use Configure... in Preferences.");
        }

        // Resolve token (performs exchange if Auth section is configured)
        String resolvedToken = resolveAuthToken(authToken);

        String uri = substituteVars(getString(promptConfig, "URI"), resolvedToken, model, prompt, messages);
        String method = getString(promptConfig, "Method");
        if (method == null) method = "POST";

        Map<String, String> headers = getHeaders(promptConfig, resolvedToken, model, prompt, messages);
        String body = substituteVars(getString(promptConfig, "Body"), resolvedToken, model, prompt, messages);

        HttpRequest.Builder reqBuilder = HttpRequest.newBuilder()
                .uri(URI.create(uri))
                .timeout(Duration.ofSeconds(120));

        for (Map.Entry<String, String> h : headers.entrySet()) {
            reqBuilder.header(h.getKey(), h.getValue());
        }

        if ("POST".equalsIgnoreCase(method)) {
            reqBuilder.POST(HttpRequest.BodyPublishers.ofString(body != null ? body : "", StandardCharsets.UTF_8));
        } else {
            reqBuilder.GET();
        }

        HttpResponse<String> resp = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(30))
                .build()
                .send(reqBuilder.build(), HttpResponse.BodyHandlers.ofString());

        if (resp.statusCode() != 200) {
            throw new RuntimeException("HTTP " + resp.statusCode() + ": " +
                    resp.body().substring(0, Math.min(300, resp.body().length())));
        }

        // Extract content using ContentPath
        Map<String, Object> responseConfig = (Map<String, Object>) promptConfig.get("Response");
        if (responseConfig == null) {
            throw new RuntimeException("No Response section in Generic vendor config");
        }
        String contentPath = (String) responseConfig.get("ContentPath");
        if (contentPath == null || contentPath.isBlank()) {
            // If no path specified, return the whole body
            return resp.body();
        }

        return evaluateJsonPath(resp.body(), contentPath);
    }

    /**
     * Fetch the list of available models from the configured endpoint.
     *
     * @param authToken the API key / auth token
     * @return list of model ID strings
     */
    @SuppressWarnings("unchecked")
    public List<String> fetchModels(String authToken) {
        List<String> result = new ArrayList<>();
        if (modelsConfig == null) return result;

        try {
            // Resolve token (performs exchange if Auth section is configured)
            String resolvedToken = resolveAuthToken(authToken);

            String uri = substituteVars(getString(modelsConfig, "URI"), resolvedToken, "", "", null);
            String method = getString(modelsConfig, "Method");
            if (method == null) method = "GET";

            Map<String, String> headers = getHeaders(modelsConfig, resolvedToken, "", "", null);

            HttpRequest.Builder reqBuilder = HttpRequest.newBuilder()
                    .uri(URI.create(uri))
                    .timeout(Duration.ofSeconds(15));

            for (Map.Entry<String, String> h : headers.entrySet()) {
                reqBuilder.header(h.getKey(), h.getValue());
            }

            if ("POST".equalsIgnoreCase(method)) {
                String body = substituteVars(getString(modelsConfig, "Body"), resolvedToken, "", "", null);
                reqBuilder.POST(HttpRequest.BodyPublishers.ofString(body != null ? body : ""));
            } else {
                reqBuilder.GET();
            }

            HttpResponse<String> resp = HttpClient.newBuilder()
                    .connectTimeout(Duration.ofSeconds(10))
                    .build()
                    .send(reqBuilder.build(), HttpResponse.BodyHandlers.ofString());

            if (resp.statusCode() != 200) return result;

            Map<String, Object> responseConfig = (Map<String, Object>) modelsConfig.get("Response");
            if (responseConfig == null) return result;

            String listPath = (String) responseConfig.get("ListPath");
            String idField = (String) responseConfig.get("IdField");
            if (idField == null) idField = "id";

            String body = resp.body().trim();

            // Navigate to the array using ListPath
            String arrayJson;
            if (listPath == null || listPath.isBlank()) {
                arrayJson = body;
            } else {
                arrayJson = evaluateJsonPath(body, listPath);
            }

            // Extract model IDs from the JSON array
            if (arrayJson != null && arrayJson.trim().startsWith("[")) {
                // Simple regex extraction of the id field from each object
                Pattern p = Pattern.compile("\"" + Pattern.quote(idField) + "\"\\s*:\\s*\"([^\"]+)\"");
                Matcher m = p.matcher(arrayJson);
                while (m.find()) {
                    result.add(m.group(1));
                }
            }
        } catch (Exception e) {
            // Return empty list on any failure
        }

        return result;
    }

    // --- Private helpers ---

    /**
     * Resolve the auth token. If an Auth section is configured, performs a token
     * exchange (e.g., OAuth/IAM) using the raw API key and caches the result.
     * If no Auth section, returns the raw authToken as-is.
     *
     * @param rawAuthToken the API key / credential from Preferences
     * @return the resolved bearer/access token to use in API calls
     */
    @SuppressWarnings("unchecked")
    private String resolveAuthToken(String rawAuthToken) throws Exception {
        if (authConfig == null) {
            return rawAuthToken; // No token exchange configured
        }

        // Return cached token if still valid (with 60-second buffer)
        if (cachedAccessToken != null && System.currentTimeMillis() < (tokenExpiryTime - 60_000)) {
            return cachedAccessToken;
        }

        String tokenUri = getString(authConfig, "TokenURI");
        if (tokenUri == null || tokenUri.isBlank()) {
            return rawAuthToken;
        }

        String method = getString(authConfig, "Method");
        if (method == null) method = "POST";

        // Substitute ${AUTH_TOKEN} in the auth request body and headers
        // (using raw token since we haven't resolved yet)
        String body = getString(authConfig, "Body");
        if (body != null) {
            body = body.replace("${AUTH_TOKEN}", rawAuthToken != null ? rawAuthToken : "");
        }

        // Build headers
        Map<String, String> headers = new LinkedHashMap<>();
        Object headersObj = authConfig.get("Headers");
        if (headersObj instanceof Map) {
            Map<String, Object> headersMap = (Map<String, Object>) headersObj;
            for (Map.Entry<String, Object> entry : headersMap.entrySet()) {
                String val = entry.getValue() != null ? entry.getValue().toString() : "";
                val = val.replace("${AUTH_TOKEN}", rawAuthToken != null ? rawAuthToken : "");
                headers.put(entry.getKey(), val);
            }
        }

        HttpRequest.Builder reqBuilder = HttpRequest.newBuilder()
                .uri(URI.create(tokenUri))
                .timeout(Duration.ofSeconds(15));

        for (Map.Entry<String, String> h : headers.entrySet()) {
            reqBuilder.header(h.getKey(), h.getValue());
        }

        if ("POST".equalsIgnoreCase(method)) {
            reqBuilder.POST(HttpRequest.BodyPublishers.ofString(body != null ? body : "", StandardCharsets.UTF_8));
        } else {
            reqBuilder.GET();
        }

        HttpResponse<String> resp = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(10))
                .build()
                .send(reqBuilder.build(), HttpResponse.BodyHandlers.ofString());

        if (resp.statusCode() != 200) {
            throw new RuntimeException("Auth token exchange failed (HTTP " + resp.statusCode() + "): " +
                    resp.body().substring(0, Math.min(200, resp.body().length())));
        }

        // Extract the token from the response
        Map<String, Object> responseConfig = (Map<String, Object>) authConfig.get("Response");
        if (responseConfig == null) {
            throw new RuntimeException("Auth section missing Response configuration");
        }

        String tokenPath = (String) responseConfig.get("TokenPath");
        if (tokenPath == null || tokenPath.isBlank()) {
            throw new RuntimeException("Auth section missing Response.TokenPath");
        }

        String token = evaluateJsonPath(resp.body(), tokenPath);
        if (token == null || token.isBlank()) {
            throw new RuntimeException("Failed to extract token from auth response");
        }

        cachedAccessToken = token;

        // Check for expiry info — look for "expires_in" (seconds) or "expiration" (epoch)
        String expiresInPath = (String) responseConfig.get("ExpiresInPath");
        if (expiresInPath != null && !expiresInPath.isBlank()) {
            try {
                String expiresStr = evaluateJsonPath(resp.body(), expiresInPath);
                if (expiresStr != null) {
                    long expiresIn = Long.parseLong(expiresStr.trim());
                    // If value > 1_000_000_000, treat as epoch seconds; otherwise as duration in seconds
                    if (expiresIn > 1_000_000_000L) {
                        tokenExpiryTime = expiresIn * 1000; // epoch seconds to millis
                    } else {
                        tokenExpiryTime = System.currentTimeMillis() + (expiresIn * 1000);
                    }
                }
            } catch (NumberFormatException e) {
                // Default to 50 minutes if we can't parse
                tokenExpiryTime = System.currentTimeMillis() + (50 * 60 * 1000);
            }
        } else {
            // Default: assume token is valid for 50 minutes
            tokenExpiryTime = System.currentTimeMillis() + (50 * 60 * 1000);
        }

        return cachedAccessToken;
    }

    @SuppressWarnings("unchecked")
    private Map<String, String> getHeaders(Map<String, Object> config, String authToken,
                                           String model, String prompt,
                                           List<Map<String, String>> messages) {
        Map<String, String> result = new LinkedHashMap<>();
        Object headersObj = config.get("Headers");
        if (headersObj instanceof Map) {
            Map<String, Object> headersMap = (Map<String, Object>) headersObj;
            for (Map.Entry<String, Object> entry : headersMap.entrySet()) {
                String val = entry.getValue() != null ? entry.getValue().toString() : "";
                result.put(entry.getKey(), substituteVars(val, authToken, model, prompt, messages));
            }
        }
        return result;
    }

    private String getString(Map<String, Object> map, String key) {
        Object val = map.get(key);
        return val != null ? val.toString() : null;
    }

    /**
     * Substitute ${AUTH_TOKEN}, ${MODEL}, ${PROMPT}, ${GUID}, ${MESSAGES},
     * ${MESSAGES_NO_SYSTEM}, and ${SYSTEM_PROMPT} in a template string.
     * ${MESSAGES} expands to a JSON array of all message objects.
     * ${MESSAGES_NO_SYSTEM} expands to a JSON array excluding system-role messages.
     * ${SYSTEM_PROMPT} expands to the content of the first system message (unescaped for embedding in JSON).
     */
    private String substituteVars(String template, String authToken, String model,
                                  String prompt, List<Map<String, String>> messages) {
        if (template == null) return null;
        String result = template;

        // Replace simple scalar variables first
        result = result.replace("${AUTH_TOKEN}", authToken != null ? authToken : "");
        result = result.replace("${MODEL}", model != null ? model : "");
        result = result.replace("${PROMPT}", jsonEscape(prompt != null ? prompt : ""));
        result = result.replace("${GUID}", conversationGuid);

        // Replace ${SYSTEM_PROMPT} before message arrays, since the system prompt
        // is a simple scalar and won't contain template variable patterns.
        if (messages != null) {
            if (template.contains("${SYSTEM_PROMPT}")) {
                String sysContent = messages.stream()
                    .filter(m -> "system".equals(m.get("role")))
                    .map(m -> m.get("content"))
                    .findFirst().orElse("");
                result = result.replace("${SYSTEM_PROMPT}", jsonEscape(sysContent));
            }

            // Replace message arrays LAST. These inject full user content which may
            // contain literal "${VAR}" text (e.g. documentation about template variables).
            // By doing these last, no further replacements will corrupt the injected content.
            if (template.contains("${MESSAGES}")) {
                StringBuilder sb = new StringBuilder("[");
                for (int i = 0; i < messages.size(); i++) {
                    if (i > 0) sb.append(",");
                    Map<String, String> msg = messages.get(i);
                    sb.append("{\"role\":\"").append(jsonEscape(msg.get("role")))
                      .append("\",\"content\":\"").append(jsonEscape(msg.get("content"))).append("\"}");
                }
                sb.append("]");
                result = result.replace("${MESSAGES}", sb.toString());
            }

            if (template.contains("${MESSAGES_NO_SYSTEM}")) {
                StringBuilder sb = new StringBuilder("[");
                boolean first = true;
                for (Map<String, String> msg : messages) {
                    if ("system".equals(msg.get("role"))) continue;
                    if (!first) sb.append(",");
                    sb.append("{\"role\":\"").append(jsonEscape(msg.get("role")))
                      .append("\",\"content\":\"").append(jsonEscape(msg.get("content"))).append("\"}");
                    first = false;
                }
                sb.append("]");
                result = result.replace("${MESSAGES_NO_SYSTEM}", sb.toString());
            }
        }

        return result;
    }

    private static String jsonEscape(String s) {
        StringBuilder sb = new StringBuilder();
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
        return sb.toString();
    }

    /**
     * Evaluate a simple JSONPath expression against a JSON string.
     * Supports: field.field, field[index].field, etc.
     * Examples: "choices[0].message.content", "data[0].id", "result"
     */
    static String evaluateJsonPath(String json, String path) {
        String current = json.trim();

        // Split path into segments: "choices[0].message.content" -> ["choices", "[0]", "message", "content"]
        List<String> segments = new ArrayList<>();
        Matcher m = Pattern.compile("([^.\\[]+)|\\[(\\d+)]").matcher(path);
        while (m.find()) {
            if (m.group(1) != null) segments.add(m.group(1));
            else if (m.group(2) != null) segments.add("[" + m.group(2) + "]");
        }

        for (String seg : segments) {
            if (current == null) return null;
            current = current.trim();

            if (seg.startsWith("[") && seg.endsWith("]")) {
                // Array index access
                int index = Integer.parseInt(seg.substring(1, seg.length() - 1));
                current = getArrayElement(current, index);
            } else {
                // Object field access
                current = getObjectField(current, seg);
            }
        }

        if (current == null) return null;
        current = current.trim();

        // If the result is a JSON string, unwrap the quotes and unescape
        if (current.startsWith("\"")) {
            return unescapeJsonString(current);
        }
        return current;
    }

    /** Get the value of a field in a JSON object string. */
    private static String getObjectField(String json, String field) {
        if (!json.startsWith("{")) return null;

        String pattern = "\"" + field + "\"";
        int idx = -1;
        int searchFrom = 0;

        // Find the field key at the correct nesting level
        while (searchFrom < json.length()) {
            int candidate = json.indexOf(pattern, searchFrom);
            if (candidate < 0) return null;

            // Check that this is at the top level of this object
            int depth = 0;
            boolean inString = false;
            boolean valid = true;
            for (int i = 1; i < candidate; i++) { // start after opening {
                char c = json.charAt(i);
                if (inString) {
                    if (c == '\\') { i++; continue; }
                    if (c == '"') inString = false;
                } else {
                    if (c == '"') inString = true;
                    else if (c == '{' || c == '[') depth++;
                    else if (c == '}' || c == ']') depth--;
                }
            }
            if (depth == 0) {
                idx = candidate;
                break;
            }
            searchFrom = candidate + 1;
        }

        if (idx < 0) return null;

        // Find the colon after the key
        int colonIdx = json.indexOf(':', idx + pattern.length());
        if (colonIdx < 0) return null;

        // Find the start of the value
        int valStart = colonIdx + 1;
        while (valStart < json.length() && Character.isWhitespace(json.charAt(valStart))) valStart++;
        if (valStart >= json.length()) return null;

        return extractJsonValue(json, valStart);
    }

    /** Get an element from a JSON array string by index. */
    private static String getArrayElement(String json, int index) {
        if (!json.startsWith("[")) return null;

        int pos = 1; // after opening [
        int currentIndex = 0;

        while (pos < json.length()) {
            while (pos < json.length() && Character.isWhitespace(json.charAt(pos))) pos++;
            if (pos >= json.length() || json.charAt(pos) == ']') return null;

            int valueStart = pos;
            int valueEnd = findValueEnd(json, pos);
            if (valueEnd < 0) return null;

            if (currentIndex == index) {
                return json.substring(valueStart, valueEnd).trim();
            }

            pos = valueEnd;
            while (pos < json.length() && Character.isWhitespace(json.charAt(pos))) pos++;
            if (pos < json.length() && json.charAt(pos) == ',') pos++;
            currentIndex++;
        }

        return null;
    }

    /** Extract a JSON value starting at the given position. Returns the value substring. */
    private static String extractJsonValue(String json, int start) {
        int end = findValueEnd(json, start);
        if (end < 0) return null;
        return json.substring(start, end).trim();
    }

    /** Find the end position of a JSON value starting at pos. */
    private static int findValueEnd(String json, int pos) {
        while (pos < json.length() && Character.isWhitespace(json.charAt(pos))) pos++;
        if (pos >= json.length()) return -1;

        char c = json.charAt(pos);

        if (c == '"') {
            // String
            int i = pos + 1;
            while (i < json.length()) {
                if (json.charAt(i) == '\\') { i += 2; continue; }
                if (json.charAt(i) == '"') return i + 1;
                i++;
            }
            return -1;
        } else if (c == '{' || c == '[') {
            // Object or Array — find matching close
            char open = c;
            char close = (c == '{') ? '}' : ']';
            int depth = 1;
            int i = pos + 1;
            boolean inStr = false;
            while (i < json.length() && depth > 0) {
                char ch = json.charAt(i);
                if (inStr) {
                    if (ch == '\\') { i++; }
                    else if (ch == '"') inStr = false;
                } else {
                    if (ch == '"') inStr = true;
                    else if (ch == open) depth++;
                    else if (ch == close) depth--;
                }
                i++;
            }
            return i;
        } else {
            // Number, boolean, null
            int i = pos;
            while (i < json.length()) {
                char ch = json.charAt(i);
                if (ch == ',' || ch == '}' || ch == ']' || Character.isWhitespace(ch)) break;
                i++;
            }
            return i;
        }
    }

    /** Unescape a JSON string value (remove surrounding quotes, process escape sequences). */
    private static String unescapeJsonString(String jsonStr) {
        if (jsonStr.length() < 2) return jsonStr;
        String inner = jsonStr.substring(1, jsonStr.length() - 1);
        StringBuilder sb = new StringBuilder();
        int i = 0;
        while (i < inner.length()) {
            char c = inner.charAt(i);
            if (c == '\\' && i + 1 < inner.length()) {
                char next = inner.charAt(i + 1);
                switch (next) {
                    case 'n' -> sb.append('\n');
                    case 'r' -> sb.append('\r');
                    case 't' -> sb.append('\t');
                    case '"' -> sb.append('"');
                    case '\\' -> sb.append('\\');
                    case '/' -> sb.append('/');
                    case 'u' -> {
                        if (i + 5 < inner.length()) {
                            sb.append((char) Integer.parseInt(inner.substring(i + 2, i + 6), 16));
                            i += 4;
                        }
                    }
                    default -> { sb.append('\\'); sb.append(next); }
                }
                i += 2;
            } else {
                sb.append(c);
                i++;
            }
        }
        return sb.toString();
    }
}
