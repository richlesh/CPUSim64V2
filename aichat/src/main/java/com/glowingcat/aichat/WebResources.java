/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.io.*;
import java.nio.file.*;

/**
 * Extracts bundled JavaScript/CSS libraries to a temporary directory and
 * provides file:// URIs for use in JavaFX WebView HTML pages.
 * <p>
 * This allows the application to work behind corporate firewalls that block
 * CDN downloads from within the WebView.
 */
public class WebResources {

    private static final String[] RESOURCE_NAMES = {
        "highlight.min.js",
        "github.min.css",
        "github-dark.min.css",
        "mermaid.min.js",
        "tex-svg.js"
    };

    // Bundled JS/CSS loaded from classpath resources (avoids CDN dependency)
    private static final String MERMAID_JS = loadResourceAsString("/mermaid.min.js");
    private static final String HIGHLIGHT_JS = loadResourceAsString("/highlight.min.js");
    private static final String HLJS_GITHUB_CSS = loadResourceAsString("/hljs-github.min.css");
    private static final String HLJS_GITHUB_DARK_CSS = loadResourceAsString("/hljs-github-dark.min.css");
    private static final String TEX_SVG_JS = loadResourceAsString("/tex-svg.js");

    private static String loadResourceAsString(String path) {
        try (var is = AIChatPanel.class.getResourceAsStream(path)) {
            if (is != null) {
                return new String(is.readAllBytes(), java.nio.charset.StandardCharsets.UTF_8);
            }
        } catch (Exception e) {
            System.err.println("Failed to load resource: " + path + " - " + e.getMessage());
        }
        return "";
    }

    /** Returns the file:// URI for highlight.min.js */
    public static String highlightJs() {
        return HIGHLIGHT_JS;
    }

    /** Returns the file:// URI for the highlight.js CSS theme */
    public static String highlightCss(boolean dark) {
        return dark ? HLJS_GITHUB_DARK_CSS : HLJS_GITHUB_CSS;
    }

    /** Returns the file:// URI for mermaid.min.js */
    public static String mermaidJs() {
        return MERMAID_JS;
    }

    /** Returns the file:// URI for MathJax tex-svg.js */
    public static String mathjaxJs() {
        return TEX_SVG_JS;
    }
}
