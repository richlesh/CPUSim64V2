/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

/**
 * Provides chat bubble colors for the AI Chat panel.
 * Implement this interface in the host application to control
 * the appearance of user and AI message bubbles.
 */
public interface ChatColors {

    /** Background color for user prompt bubbles (CSS hex string, e.g. "#9B59B6"). */
    String getUserPromptColor();

    /** Text color for user prompt bubbles (CSS hex string, e.g. "#FFFFFF"). */
    String getUserTextColor();

    /** Background color for AI response bubbles (CSS hex string, e.g. "#6C3483"). */
    String getAiResponseColor();

    /** Text color for AI response bubbles (CSS hex string, e.g. "#FFFFFF"). */
    String getAiTextColor();
}
