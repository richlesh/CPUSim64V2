/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import java.awt.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;

/**
 * AI Chat preferences stored in ~/.glowingcat-ai-settings.json.
 * This is a standalone settings class shared across all Glowing Cat apps.
 */
public class AIChatPreferences {

    private static final String SETTINGS_FILENAME = ".glowingcat-ai-settings.json";

    /** LLM vendor name. */
    private String llmVendor = "OpenAI";

    /** LLM model identifier. */
    private String llmModel = "gpt-4o";

    /** LLM API key (null means not configured). */
    private String llmApiKey = null;

    /** Custom LLM endpoint URL for Generic OpenAI API vendor. */
    private String llmEndpoint = null;

    /** Font family for the AI chat panel. */
    private String aiFontFamily = detectAIFont();

    /** Font size for the AI chat panel. */
    private int aiFontSize = 14;

    /** Font family for code blocks and inline code in the AI chat panel (monospaced). */
    private String aiCodeFontFamily = detectCodeFont();

    /** Font size for code blocks and inline code in the AI chat panel. */
    private int aiCodeFontSize = 13;

    /** Background color for user prompt chat bubbles (hex string). */
    private String userPromptColor = "#88FF88";

    /** Text color for user prompt chat bubbles (hex string). */
    private String userTextColor = "#555555";

    /** Background color for AI response chat bubbles (hex string). */
    private String aiResponseColor = "#33BB00";

    /** Text color for AI response chat bubbles (hex string). */
    private String aiTextColor = "#DDFFDD";

    // --- Getters and Setters ---

    public String getLlmVendor() { return llmVendor; }
    public void setLlmVendor(String llmVendor) { this.llmVendor = llmVendor; }

    public String getLlmModel() { return llmModel; }
    public void setLlmModel(String llmModel) { this.llmModel = llmModel; }

    public String getLlmApiKey() { return llmApiKey; }
    public void setLlmApiKey(String llmApiKey) { this.llmApiKey = llmApiKey; }

    public String getLlmEndpoint() { return llmEndpoint; }
    public void setLlmEndpoint(String llmEndpoint) { this.llmEndpoint = llmEndpoint; }

    public String getAiFontFamily() { return aiFontFamily; }
    public void setAiFontFamily(String aiFontFamily) { this.aiFontFamily = aiFontFamily; }

    public int getAiFontSize() { return aiFontSize; }
    public void setAiFontSize(int aiFontSize) { this.aiFontSize = aiFontSize; }

    public String getAiCodeFontFamily() { return aiCodeFontFamily; }
    public void setAiCodeFontFamily(String aiCodeFontFamily) { this.aiCodeFontFamily = aiCodeFontFamily; }

    public int getAiCodeFontSize() { return aiCodeFontSize; }
    public void setAiCodeFontSize(int aiCodeFontSize) { this.aiCodeFontSize = aiCodeFontSize; }

    public Color getUserPromptColorObj() { return Color.decode(userPromptColor); }
    public void setUserPromptColor(Color color) { this.userPromptColor = toHex(color); }
    public String getUserPromptColor() { return userPromptColor; }
    public void setUserPromptColor(String hex) { this.userPromptColor = hex; }

    public Color getUserTextColorObj() { return Color.decode(userTextColor); }
    public void setUserTextColor(Color color) { this.userTextColor = toHex(color); }
    public String getUserTextColor() { return userTextColor; }
    public void setUserTextColor(String hex) { this.userTextColor = hex; }

    public Color getAiResponseColorObj() { return Color.decode(aiResponseColor); }
    public void setAiResponseColor(Color color) { this.aiResponseColor = toHex(color); }
    public String getAiResponseColor() { return aiResponseColor; }
    public void setAiResponseColor(String hex) { this.aiResponseColor = hex; }

    public Color getAiTextColorObj() { return Color.decode(aiTextColor); }
    public void setAiTextColor(Color color) { this.aiTextColor = toHex(color); }
    public String getAiTextColor() { return aiTextColor; }
    public void setAiTextColor(String hex) { this.aiTextColor = hex; }

    // --- Load / Save ---

    private static Path getSettingsPath() {
        return Paths.get(System.getProperty("user.home"), SETTINGS_FILENAME);
    }

    public static AIChatPreferences load() {
        Path path = getSettingsPath();
        if (Files.exists(path)) {
            try (Reader reader = Files.newBufferedReader(path, StandardCharsets.UTF_8)) {
                Gson gson = new Gson();
                AIChatPreferences prefs = gson.fromJson(reader, AIChatPreferences.class);
                if (prefs != null) {
                    return prefs;
                }
            } catch (IOException | com.google.gson.JsonSyntaxException e) {
                // Fall through to return defaults
            }
        }
        return new AIChatPreferences();
    }

    public void save() {
        Path path = getSettingsPath();
        Gson gson = new GsonBuilder().setPrettyPrinting().create();
        try (Writer writer = Files.newBufferedWriter(path, StandardCharsets.UTF_8)) {
            gson.toJson(this, writer);
        } catch (IOException e) {
            // Silently fail - preferences are non-critical
        }
    }

    // --- Helpers ---

    private static String toHex(Color color) {
        return String.format("#%02x%02x%02x", color.getRed(), color.getGreen(), color.getBlue());
    }

    private static String detectAIFont() {
        String os = System.getProperty("os.name", "").toLowerCase();
        String[] candidates;
        if (os.contains("linux")) candidates = new String[]{"DejaVu Sans", "Arial", "Helvetica", "SansSerif"};
        else if (os.contains("win")) candidates = new String[]{"Calibri", "Arial", "Helvetica", "SansSerif"};
        else candidates = new String[]{"Calibri", "DejaVu Sans", "Arial", "Helvetica", "SansSerif"};
        for (String name : candidates) {
            Font f = new Font(name, Font.PLAIN, 14);
            if (!f.getFamily().equals("Dialog")) return name;
        }
        return "SansSerif";
    }

    private static String detectCodeFont() {
        String os = System.getProperty("os.name", "").toLowerCase();
        String[] candidates;
        if (os.contains("mac")) candidates = new String[]{"JetBrains Mono", "Consolas", "Menlo", "SF Mono", "Monaco", "Monospaced"};
        else if (os.contains("win")) candidates = new String[]{"JetBrains Mono", "Cascadia Mono", "Consolas", "Courier New", "Monospaced"};
        else candidates = new String[]{"JetBrains Mono", "DejaVu Sans Mono", "Liberation Mono", "Monospaced"};
        for (String name : candidates) {
            Font f = new Font(name, Font.PLAIN, 13);
            if (!f.getFamily().equals("Dialog")) return name;
        }
        return "Monospaced";
    }
}
