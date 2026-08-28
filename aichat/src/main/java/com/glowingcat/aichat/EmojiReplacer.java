/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

/**
 * Replaces Unicode supplementary characters (code points above U+FFFF, including emoji)
 * with inline {@code <img>} tags referencing Twemoji SVG images from a CDN.
 * <p>
 * This works around JavaFX WebView's inability to reliably render non-BMP characters,
 * even when the HTML is loaded from a UTF-8 file.
 * <p>
 * Uses Twemoji v14.0.2 SVGs from jsDelivr CDN. Handles both single code point emoji
 * and multi-code-point sequences (ZWJ sequences, skin tone modifiers, flag sequences).
 */
public class EmojiReplacer {

    private static final String TWEMOJI_BASE =
            "https://cdn.jsdelivr.net/gh/twitter/twemoji@14.0.2/assets/svg/";

    private EmojiReplacer() {}

    /**
     * Replaces all non-BMP characters in the given HTML string with Twemoji {@code <img>} tags.
     * Handles surrogate pairs, ZWJ sequences (U+200D), variation selectors (U+FE0F),
     * and skin tone modifiers (U+1F3FB–U+1F3FF).
     *
     * @param html the HTML string potentially containing supplementary characters
     * @return the HTML with supplementary characters replaced by img tags
     */
    public static String replaceEmoji(String html) {
        if (html == null || html.isEmpty()) return html;

        StringBuilder result = new StringBuilder(html.length());
        int i = 0;
        while (i < html.length()) {
            char c = html.charAt(i);

            // Check for a supplementary character (high surrogate)
            if (Character.isHighSurrogate(c) && i + 1 < html.length()
                    && Character.isLowSurrogate(html.charAt(i + 1))) {
                // Start collecting an emoji sequence
                StringBuilder emojiChars = new StringBuilder();
                StringBuilder codePoints = new StringBuilder();

                int codePoint = Character.toCodePoint(c, html.charAt(i + 1));
                emojiChars.appendCodePoint(codePoint);
                codePoints.append(Integer.toHexString(codePoint));
                i += 2;

                // Continue collecting joined characters (ZWJ sequences, modifiers, etc.)
                while (i < html.length()) {
                    int nextCp = -1;
                    int advance = 0;

                    if (i < html.length() && Character.isHighSurrogate(html.charAt(i))
                            && i + 1 < html.length() && Character.isLowSurrogate(html.charAt(i + 1))) {
                        nextCp = Character.toCodePoint(html.charAt(i), html.charAt(i + 1));
                        advance = 2;
                    } else if (i < html.length()) {
                        char next = html.charAt(i);
                        // ZWJ (U+200D), Variation Selector-16 (U+FE0F), skin tone modifiers handled as BMP
                        if (next == '\u200D' || next == '\uFE0F' || next == '\uFE0E'
                                || (next >= '\u20E3' && next <= '\u20E3')) {
                            nextCp = next;
                            advance = 1;
                        } else {
                            break;
                        }
                    }

                    if (nextCp == -1) break;

                    // Is this a continuation character?
                    if (nextCp == 0x200D || nextCp == 0xFE0F || nextCp == 0xFE0E
                            || (nextCp >= 0x1F3FB && nextCp <= 0x1F3FF)  // skin tones
                            || (nextCp >= 0xE0020 && nextCp <= 0xE007F)  // tag characters
                            || (nextCp >= 0x1F1E6 && nextCp <= 0x1F1FF)  // regional indicators
                            || isEmojiContinuation(codePoints.toString(), nextCp)) {
                        emojiChars.appendCodePoint(nextCp);
                        // Twemoji file names omit FE0F in most cases, but include it for some.
                        // We include it in the filename and fall back without it.
                        codePoints.append("-").append(Integer.toHexString(nextCp));
                        i += advance;
                    } else {
                        break;
                    }
                }

                // Build the img tag
                String filename = codePoints.toString();
                String alt = emojiChars.toString();
                result.append("<img src=\"").append(TWEMOJI_BASE).append(filename).append(".svg\"")
                      .append(" alt=\"").append(escapeAttr(alt)).append("\"")
                      .append(" class=\"emoji\"")
                      .append(" draggable=\"false\"")
                      .append(" onerror=\"this.onerror=null;")
                      .append("this.src='").append(TWEMOJI_BASE)
                      .append(filename.replace("-fe0f", "")).append(".svg';\"")
                      .append(">");
            } else {
                result.append(c);
                i++;
            }
        }
        return result.toString();
    }

    /**
     * Returns CSS rules for inline emoji images.
     */
    public static String emojiCss() {
        return "img.emoji { height: 1.2em; width: 1.2em; vertical-align: -0.2em; "
                + "margin: 0 0.05em; display: inline-block; }";
    }

    /**
     * Checks whether the next code point is a valid continuation of an emoji sequence.
     * After a ZWJ, nearly any emoji can follow, so this is permissive.
     */
    private static boolean isEmojiContinuation(String preceding, int nextCp) {
        // After a ZWJ (the preceding ends with the ZWJ hex), allow any supplementary char
        if (preceding.endsWith("200d")) {
            return nextCp > 0xFFFF || (nextCp >= 0x2600 && nextCp <= 0x27BF)
                    || (nextCp >= 0x2700 && nextCp <= 0x27BF);
        }
        // Regional indicator flags: pairs of U+1F1E6..U+1F1FF
        if (nextCp >= 0x1F1E6 && nextCp <= 0x1F1FF) {
            return true;
        }
        return false;
    }

    private static String escapeAttr(String s) {
        return s.replace("&", "&amp;").replace("\"", "&quot;").replace("<", "&lt;").replace(">", "&gt;");
    }
}
