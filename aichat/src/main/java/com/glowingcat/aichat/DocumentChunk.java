/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.io.Serializable;

/**
 * A chunk of document text with its source identifier.
 */
record DocumentChunk(String source, String text) implements Serializable {
    private static final long serialVersionUID = 1L;
}
