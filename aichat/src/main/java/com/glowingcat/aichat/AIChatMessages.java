/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.text.MessageFormat;
import java.util.Locale;
import java.util.MissingResourceException;
import java.util.ResourceBundle;

/**
 * Provides localized strings for the AI Chat panel UI.
 * <p>
 * Usage: {@code AIChatMessages.get("aichat.send")} returns the localized string.
 * The locale should be set by the host application via {@link #setLocale(Locale)}
 * before the AI chat panel is created.
 */
public class AIChatMessages {

    private static final String BUNDLE_NAME = "com.glowingcat.aichat.AIChatMessages";
    private static ResourceBundle bundle;

    static {
        setLocale(Locale.getDefault());
    }

    private AIChatMessages() {}

    /**
     * Sets the locale for AI chat UI strings.
     *
     * @param locale the desired locale
     */
    public static void setLocale(Locale locale) {
        bundle = ResourceBundle.getBundle(BUNDLE_NAME, locale);
    }

    /**
     * Returns the localized string for the given key.
     *
     * @param key the resource key
     * @return the localized string, or the key itself if not found
     */
    public static String get(String key) {
        try {
            return bundle.getString(key);
        } catch (MissingResourceException e) {
            return key;
        }
    }

    /**
     * Returns the localized string with parameter substitution.
     *
     * @param key  the resource key
     * @param args values to substitute for {0}, {1}, etc.
     * @return the formatted localized string
     */
    public static String get(String key, Object... args) {
        String pattern = get(key);
        try {
            return MessageFormat.format(pattern, args);
        } catch (IllegalArgumentException e) {
            return pattern;
        }
    }
}
