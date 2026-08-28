/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

/**
 * Abstraction over the document editor, allowing AIChatPanel to read and
 * write document text without depending on a specific editor component.
 */
public interface DocumentEditor {

    /** Get the full text content of the document. */
    String getText();

    /** Replace the entire document text. */
    void setText(String text);

    /**
     * Get the file path of the current document, or null if unsaved/untitled.
     */
    default String getFilepath() {
        return null;
    }

    /**
     * Get document text suitable for sending as LLM context.
     * May be truncated for large documents. Defaults to full text.
     */
    default String getContextText() {
        return getText();
    }
}
