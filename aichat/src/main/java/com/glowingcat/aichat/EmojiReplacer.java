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

    /**
     * GitHub's hosted emoji image base. GitHub names Unicode emoji by their code points joined
     * with '-', omitting the ZWJ (U+200D) and variation selectors (U+FE0F), e.g.
     * {@code .../unicode/1f426-2b1b.png}. Used as a last-resort fallback for ZWJ sequences that
     * Twemoji v14.0.2 does not include (such as 🐦‍⬛ black_bird, a Unicode 15.1 emoji).
     */
    private static final String GITHUB_EMOJI_BASE =
            "https://github.githubassets.com/images/icons/emoji/unicode/";

    private EmojiReplacer() {}

    // --------------------------------------------------------------------------------------
    // BMP (<= U+FFFF) emoji code points. These are NOT surrogate pairs, so without special
    // handling they pass through as raw characters and the platform font decides color vs.
    // black-and-white — the latter for Unicode "text-default" symbols (e.g. ✔ U+2714,
    // ☹ U+2639). Routing them through Twemoji guarantees consistent color rendering.
    //
    // Two tiers, matching Unicode's emoji-vs-text presentation model, so ordinary prose
    // symbols are never turned into images:
    //   • EMOJI_DEFAULT_BMP — emoji presentation by default; rendered as an image whether or
    //     not a following U+FE0F is present (e.g. ✅ ⭐ ⚡ ☕).
    //   • TEXT_DEFAULT_BMP — text presentation by default; rendered as an image ONLY when
    //     immediately followed by U+FE0F (the explicit emoji-presentation selector). This
    //     covers ✔️ ☹️ ⚠️ ✏️ etc. while leaving bare © ™ ⚠ ↔ in text untouched.
    // Both arrays are sorted for binary search and were generated from the bundled gemoji
    // dataset (single-code-point BMP emoji, and BMP base + FE0F emoji, respectively).
    // --------------------------------------------------------------------------------------
    private static final int[] EMOJI_DEFAULT_BMP = {
        0x231A, 0x231B, 0x23E9, 0x23EA, 0x23EB, 0x23EC, 0x23F0, 0x23F3, 0x25FD, 0x25FE,
        0x2614, 0x2615, 0x2648, 0x2649, 0x264A, 0x264B, 0x264C, 0x264D, 0x264E, 0x264F,
        0x2650, 0x2651, 0x2652, 0x2653, 0x267F, 0x2693, 0x26A1, 0x26AA, 0x26AB, 0x26BD,
        0x26BE, 0x26C4, 0x26C5, 0x26CE, 0x26D4, 0x26EA, 0x26F2, 0x26F3, 0x26F5, 0x26FA,
        0x26FD, 0x2705, 0x270A, 0x270B, 0x2728, 0x274C, 0x274E, 0x2753, 0x2754, 0x2755,
        0x2757, 0x2795, 0x2796, 0x2797, 0x27B0, 0x27BF, 0x2B1B, 0x2B1C, 0x2B50, 0x2B55
    };
    private static final int[] TEXT_DEFAULT_BMP = {
        0x00A9, 0x00AE, 0x203C, 0x2049, 0x2122, 0x2139, 0x2194, 0x2195, 0x2196, 0x2197,
        0x2198, 0x2199, 0x21A9, 0x21AA, 0x2328, 0x23CF, 0x23ED, 0x23EE, 0x23EF, 0x23F1,
        0x23F2, 0x23F8, 0x23F9, 0x23FA, 0x24C2, 0x25AA, 0x25AB, 0x25B6, 0x25C0, 0x25FB,
        0x25FC, 0x2600, 0x2601, 0x2602, 0x2603, 0x2604, 0x260E, 0x2611, 0x2618, 0x261D,
        0x2620, 0x2622, 0x2623, 0x2626, 0x262A, 0x262E, 0x262F, 0x2638, 0x2639, 0x263A,
        0x2640, 0x2642, 0x265F, 0x2660, 0x2663, 0x2665, 0x2666, 0x2668, 0x267B, 0x267E,
        0x2692, 0x2694, 0x2695, 0x2696, 0x2697, 0x2699, 0x269B, 0x269C, 0x26A0, 0x26A7,
        0x26B0, 0x26B1, 0x26C8, 0x26CF, 0x26D1, 0x26D3, 0x26E9, 0x26F0, 0x26F1, 0x26F4,
        0x26F7, 0x26F8, 0x26F9, 0x2702, 0x2708, 0x2709, 0x270C, 0x270D, 0x270F, 0x2712,
        0x2714, 0x2716, 0x271D, 0x2721, 0x2733, 0x2734, 0x2744, 0x2747, 0x2763, 0x2764,
        0x27A1, 0x2934, 0x2935, 0x2B05, 0x2B06, 0x2B07, 0x3030, 0x303D, 0x3297, 0x3299
    };

    private static boolean inSet(int[] sortedSet, int cp) {
        return java.util.Arrays.binarySearch(sortedSet, cp) >= 0;
    }

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
            int cp = html.codePointAt(i);
            int cpWidth = Character.charCount(cp);
            boolean nextIsFe0f = i + cpWidth < html.length() && html.charAt(i + cpWidth) == '\uFE0F';

            if (startsEmojiSequence(cp, nextIsFe0f)) {
                // Collect the full emoji sequence: base + optional FE0F, then any
                // ZWJ-joined parts (BMP or non-BMP), skin tones, keycaps, tag chars,
                // and regional-indicator pairs. The filename keeps FE0F (Twemoji names
                // some sequences with it); the onerror fallback strips it.
                StringBuilder alt = new StringBuilder();
                StringBuilder codePoints = new StringBuilder();
                int pos = i;

                // Base code point.
                alt.appendCodePoint(cp);
                codePoints.append(Integer.toHexString(cp));
                pos += cpWidth;
                // Optional emoji-presentation selector on the base.
                if (pos < html.length() && html.charAt(pos) == '\uFE0F') {
                    alt.append('\uFE0F');
                    codePoints.append("-fe0f");
                    pos++;
                }

                // Greedily consume continuation parts.
                while (pos < html.length()) {
                    int ncp = html.codePointAt(pos);
                    int nWidth = Character.charCount(ncp);

                    boolean isJoinerOrModifier =
                            ncp == 0x200D                         // ZWJ
                            || ncp == 0xFE0F || ncp == 0xFE0E     // variation selectors
                            || ncp == 0x20E3                      // combining enclosing keycap
                            || (ncp >= 0x1F3FB && ncp <= 0x1F3FF) // skin tones
                            || (ncp >= 0xE0020 && ncp <= 0xE007F) // tag characters
                            || (ncp >= 0x1F1E6 && ncp <= 0x1F1FF);// regional indicators

                    boolean afterZwj = codePoints.toString().endsWith("200d");

                    if (isJoinerOrModifier) {
                        alt.appendCodePoint(ncp);
                        codePoints.append('-').append(Integer.toHexString(ncp));
                        pos += nWidth;
                    } else if (afterZwj && isEmojiBaseCodePoint(ncp)) {
                        // After a ZWJ, accept the next emoji base regardless of plane — this is
                        // what joins e.g. 🐈 + ⬛ (black_cat) or ⛹ + ♂ (basketball_man) into one.
                        alt.appendCodePoint(ncp);
                        codePoints.append('-').append(Integer.toHexString(ncp));
                        pos += nWidth;
                    } else {
                        break;
                    }
                }

                String filename = codePoints.toString();
                // Twemoji keeps FE0F in the filename for ZWJ sequences (e.g.
                // 26f9-fe0f-200d-2642-fe0f) but omits it for standalone symbols
                // (2714, not 2714-fe0f). Choose the primary accordingly; the onerror
                // fallback always uses the FE0F-stripped form as a safety net.
                String primary = filename.contains("200d") ? filename : filename.replace("-fe0f", "");
                // Ordered fallback chain, tried left-to-right when an image fails to load:
                //   1. Twemoji with FE0F stripped (covers standalone symbols whose ZWJ-kept
                //      name 404s), then
                //   2. GitHub's hosted PNG, built from the code points with ZWJ and the
                //      variation selectors removed — this rescues ZWJ sequences Twemoji lacks
                //      (e.g. 🐦‍⬛ black_bird).
                java.util.List<String> fallbacks = new java.util.ArrayList<>(2);
                String twemojiStripped = TWEMOJI_BASE + filename.replace("-fe0f", "") + ".svg";
                if (!twemojiStripped.equals(TWEMOJI_BASE + primary + ".svg")) {
                    fallbacks.add(twemojiStripped);
                }
                String githubName = githubCodePoints(filename);
                if (!githubName.isEmpty()) {
                    fallbacks.add(GITHUB_EMOJI_BASE + githubName + ".png");
                }
                result.append("<img src=\"").append(TWEMOJI_BASE).append(primary).append(".svg\"")
                      .append(" alt=\"").append(escapeAttr(alt.toString())).append("\"")
                      .append(" class=\"emoji\"")
                      .append(" draggable=\"false\"");
                if (!fallbacks.isEmpty()) {
                    // Store remaining URLs in a data attribute; onerror pops the next one.
                    // Keep the tag parser-safe: store fallback URLs in a data attribute and
                    // delegate the stepping logic to a tiny named handler (defined once per page
                    // by emojiFallbackScript()). Inline multi-statement JS with quotes and '|'
                    // in an attribute is fragile across HTML parsers/WebViews; a single function
                    // call is not.
                    result.append(" data-fallbacks=\"").append(escapeAttr(String.join("|", fallbacks))).append("\"")
                          .append(" onerror=\"emojiFallback(this)\"");
                }
                result.append(">");
                i = pos;
            } else {
                result.appendCodePoint(cp);
                i += cpWidth;
            }
        }
        return result.toString();
    }

    /**
     * Whether an emoji sequence begins at this code point. Non-BMP code points in the emoji
     * planes always start one; BMP code points start one if they are emoji-presentation by
     * default, or text-default symbols immediately followed by the FE0F selector.
     */
    private static boolean startsEmojiSequence(int cp, boolean nextIsFe0f) {
        if (cp > 0xFFFF) {
            return isNonBmpEmoji(cp);
        }
        if (inSet(EMOJI_DEFAULT_BMP, cp)) return true;
        return nextIsFe0f && inSet(TEXT_DEFAULT_BMP, cp);
    }

    /**
     * Whether a code point may be an emoji base reached after a ZWJ (any plane). Accepts the
     * known BMP emoji sets plus non-BMP emoji; permissive by design since, after a ZWJ, the
     * sequence is already known to be an emoji.
     */
    private static boolean isEmojiBaseCodePoint(int cp) {
        if (cp > 0xFFFF) return isNonBmpEmoji(cp);
        return inSet(EMOJI_DEFAULT_BMP, cp) || inSet(TEXT_DEFAULT_BMP, cp);
    }

    /**
     * Rough test for a non-BMP (supplementary) emoji code point. Covers the main emoji blocks
     * (Misc Symbols &amp; Pictographs, Emoticons, Transport, Supplemental &amp; Extended-A,
     * and Dingbats/Symbols that live above U+FFFF). Non-emoji supplementary text is rare in
     * rendered markdown, so this is intentionally broad.
     */
    private static boolean isNonBmpEmoji(int cp) {
        return (cp >= 0x1F000 && cp <= 0x1FAFF)   // symbols & pictographs incl. Extended-A/B
            || (cp >= 0x1F1E6 && cp <= 0x1F1FF)   // regional indicators
            || (cp >= 0x1FC00 && cp <= 0x1FFFD);  // symbols & pictographs Extended-B tail
    }

    /**
     * Returns CSS rules for inline emoji images.
     */
    public static String emojiCss() {
        return "img.emoji { height: 1.2em; width: 1.2em; vertical-align: -0.2em; "
                + "margin: 0 0.05em; display: inline-block; }";
    }

    /**
     * Returns the JavaScript that powers the emoji image fallback chain, to be embedded once
     * per page (inside a {@code <script>} element). Each emoji {@code <img>} carries a
     * {@code data-fallbacks} attribute (a '|'-separated URL list) and {@code onerror=
     * "emojiFallback(this)"}; when an image fails to load, this handler swaps in the next
     * fallback URL, and clears the handler once the list is exhausted.
     * <p>
     * Delegating to a named function keeps the generated {@code <img>} tags trivial for any
     * HTML parser, avoiding fragile multi-statement inline event handlers.
     */
    public static String emojiFallbackScript() {
        return "function emojiFallback(img){"
             + "var a=img.getAttribute('data-fallbacks')||'';"
             + "var f=a.length?a.split('|'):[];"
             + "if(f.length){img.src=f.shift();img.setAttribute('data-fallbacks',f.join('|'));}"
             + "else{img.onerror=null;}"
             + "}";
    }

    private static String escapeAttr(String s) {
        return s.replace("&", "&amp;").replace("\"", "&quot;").replace("<", "&lt;").replace(">", "&gt;");
    }

    /**
     * Converts a Twemoji-style code point filename (hyphen-joined lowercase hex, possibly
     * containing {@code 200d} ZWJ and {@code fe0f}/{@code fe0e} selectors) into GitHub's
     * naming, which joins the remaining code points with '-' and drops the joiner and
     * variation selectors. Returns "" if nothing remains.
     *
     * <p>Examples: {@code 1f426-200d-2b1b} → {@code 1f426-2b1b};
     * {@code 26f9-fe0f-200d-2642-fe0f} → {@code 26f9-2642}; {@code 2714-fe0f} → {@code 2714}.
     */
    private static String githubCodePoints(String twemojiFilename) {
        StringBuilder sb = new StringBuilder(twemojiFilename.length());
        for (String part : twemojiFilename.split("-")) {
            if (part.isEmpty() || part.equals("200d") || part.equals("fe0f") || part.equals("fe0e")) {
                continue;
            }
            if (sb.length() > 0) sb.append('-');
            sb.append(part);
        }
        return sb.toString();
    }
}
