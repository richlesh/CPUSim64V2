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
}
