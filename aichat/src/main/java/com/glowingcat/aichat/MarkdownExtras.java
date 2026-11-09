/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.util.Locale;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Adds three GitHub-Flavored-Markdown features that the base CommonMark configuration does
 * not provide, implemented without a custom CommonMark extension so they can be shared by
 * every render path (preview, HTML/PDF export, and AI chat):
 *
 * <ol>
 *   <li><b>Emoji shortcodes</b> — {@code :tada:} → 🎉. Handled in {@link #preprocessMarkdown}
 *       by substituting the shortcode with the actual Unicode emoji <i>before</i> parsing, so
 *       the existing {@link EmojiReplacer} (Twemoji images) renders it downstream.</li>
 *   <li><b>Subscript / superscript</b> — {@code ~text~} → {@code <sub>text</sub>} and
 *       {@code ^text^} → {@code <sup>text</sup>}. Also done in {@link #preprocessMarkdown}.
 *       Single {@code ~} only: double {@code ~~} (strikethrough) is left untouched.</li>
 *   <li><b>Alerts / callouts</b> — GitHub blockquote admonitions such as
 *       {@code > [!NOTE]}. Handled in {@link #postProcessHtml} by rewriting the rendered
 *       {@code <blockquote>} whose first line is {@code [!TYPE]} into a styled callout
 *       {@code <div class="markdown-alert markdown-alert-note">…</div>}. The CSS lives in
 *       preview.css / preview_dark.css (class {@code .markdown-alert}).</li>
 * </ol>
 *
 * All methods are null-safe and return the input unchanged when there is nothing to do.
 */
public final class MarkdownExtras {

    private MarkdownExtras() {}

    // ---------------------------------------------------------------------------------------
    // 1. Emoji shortcodes
    // ---------------------------------------------------------------------------------------

    /**
     * A focused set of common GitHub emoji shortcodes. This is intentionally not the full
     * ~1800-entry GitHub set; it covers the shortcodes people actually type in prose. Add
     * entries here as needed. Keys are the shortcode without the surrounding colons.
     */
    private static final Map<String, String> EMOJI = new java.util.concurrent.ConcurrentHashMap<>();
    static {
        EMOJI.put("smile", "\uD83D\uDE04");
        EMOJI.put("grin", "\uD83D\uDE01");
        EMOJI.put("laughing", "\uD83D\uDE06");
        EMOJI.put("wink", "\uD83D\uDE09");
        EMOJI.put("blush", "\uD83D\uDE0A");
        EMOJI.put("heart", "\u2764\uFE0F");
        EMOJI.put("thumbsup", "\uD83D\uDC4D");
        EMOJI.put("+1", "\uD83D\uDC4D");
        EMOJI.put("thumbsdown", "\uD83D\uDC4E");
        EMOJI.put("-1", "\uD83D\uDC4E");
        EMOJI.put("tada", "\uD83C\uDF89");
        EMOJI.put("rocket", "\uD83D\uDE80");
        EMOJI.put("fire", "\uD83D\uDD25");
        EMOJI.put("star", "\u2B50");
        EMOJI.put("sparkles", "\u2728");
        EMOJI.put("100", "\uD83D\uDCAF");
        EMOJI.put("check", "\u2705");
        EMOJI.put("white_check_mark", "\u2705");
        EMOJI.put("x", "\u274C");
        EMOJI.put("warning", "\u26A0\uFE0F");
        EMOJI.put("bulb", "\uD83D\uDCA1");
        EMOJI.put("bug", "\uD83D\uDC1B");
        EMOJI.put("eyes", "\uD83D\uDC40");
        EMOJI.put("wave", "\uD83D\uDC4B");
        EMOJI.put("clap", "\uD83D\uDC4F");
        EMOJI.put("pray", "\uD83D\uDE4F");
        EMOJI.put("thinking", "\uD83E\uDD14");
        EMOJI.put("joy", "\uD83D\uDE02");
        EMOJI.put("cry", "\uD83D\uDE22");
        EMOJI.put("sob", "\uD83D\uDE2D");
        EMOJI.put("sweat_smile", "\uD83D\uDE05");
        EMOJI.put("tired_face", "\uD83D\uDE29");
        EMOJI.put("sunglasses", "\uD83D\uDE0E");
        EMOJI.put("point_right", "\uD83D\uDC49");
        EMOJI.put("point_left", "\uD83D\uDC48");
        EMOJI.put("point_up", "\uD83D\uDC46");
        EMOJI.put("point_down", "\uD83D\uDC47");
        EMOJI.put("zap", "\u26A1");
        EMOJI.put("boom", "\uD83D\uDCA5");
        EMOJI.put("book", "\uD83D\uDCD6");
        EMOJI.put("memo", "\uD83D\uDCDD");
        EMOJI.put("pencil", "\u270F\uFE0F");
        EMOJI.put("lock", "\uD83D\uDD12");
        EMOJI.put("key", "\uD83D\uDD11");
        EMOJI.put("mag", "\uD83D\uDD0D");
        EMOJI.put("gear", "\u2699\uFE0F");
        EMOJI.put("wrench", "\uD83D\uDD27");
        EMOJI.put("hammer", "\uD83D\uDD28");
        EMOJI.put("package", "\uD83D\uDCE6");
        EMOJI.put("computer", "\uD83D\uDCBB");
        EMOJI.put("email", "\uD83D\uDCE7");
        EMOJI.put("calendar", "\uD83D\uDCC5");
        EMOJI.put("clock", "\uD83D\uDD52");
        EMOJI.put("hourglass", "\u231B");
        EMOJI.put("coffee", "\u2615");
        EMOJI.put("beer", "\uD83C\uDF7A");
        EMOJI.put("pizza", "\uD83C\uDF55");
        EMOJI.put("sun", "\u2600\uFE0F");
        EMOJI.put("moon", "\uD83C\uDF19");
        EMOJI.put("cloud", "\u2601\uFE0F");
        EMOJI.put("snowflake", "\u2744\uFE0F");
        EMOJI.put("earth_americas", "\uD83C\uDF0E");
        EMOJI.put("green_heart", "\uD83D\uDC9A");
        EMOJI.put("blue_heart", "\uD83D\uDC99");
        EMOJI.put("yellow_heart", "\uD83D\uDC9B");
        EMOJI.put("purple_heart", "\uD83D\uDC9C");
        EMOJI.put("broken_heart", "\uD83D\uDC94");
        EMOJI.put("exclamation", "\u2757");
        EMOJI.put("question", "\u2753");
        EMOJI.put("information_source", "\u2139\uFE0F");
        EMOJI.put("no_entry", "\u26D4");
        EMOJI.put("recycle", "\u267B\uFE0F");
        EMOJI.put("arrow_right", "\u27A1\uFE0F");
        EMOJI.put("arrow_left", "\u2B05\uFE0F");
        EMOJI.put("arrow_up", "\u2B06\uFE0F");
        EMOJI.put("arrow_down", "\u2B07\uFE0F");
    }

    /**
     * Registers additional (or overriding) emoji shortcode mappings. Keys are shortcodes
     * <i>without</i> the surrounding colons (e.g. {@code "tada"}); values are the emoji
     * characters. Intended to be called once at startup by the application to load the full
     * gemoji dataset over the built-in fallback subset. Existing keys are overwritten, so the
     * full dataset supersedes the fallback. Null-safe and thread-safe.
     *
     * @param mappings shortcode → emoji character mappings to add
     */
    public static void registerEmojiShortcodes(Map<String, String> mappings) {
        if (mappings == null || mappings.isEmpty()) return;
        for (Map.Entry<String, String> e : mappings.entrySet()) {
            if (e.getKey() != null && e.getValue() != null && !e.getValue().isEmpty()) {
                EMOJI.put(e.getKey().toLowerCase(Locale.US), e.getValue());
            }
        }
    }

    /** @return the number of registered emoji shortcodes (for diagnostics/tests). */
    public static int emojiShortcodeCount() {
        return EMOJI.size();
    }

    /**
     * Returns whether the given shortcode (without surrounding colons) is a known, valid
     * emoji shortcode — either a Unicode emoji or an image-only GitHub-custom one. The lookup
     * is case-insensitive. Null/blank input returns {@code false}.
     *
     * @param code the shortcode without colons (e.g. {@code "tada"})
     * @return true if the shortcode resolves to an emoji
     */
    public static boolean isKnownShortcode(String code) {
        if (code == null || code.isEmpty()) return false;
        String key = code.toLowerCase(Locale.US);
        return EMOJI.containsKey(key) || CUSTOM_EMOJI.containsKey(key);
    }

    /**
     * Returns an immutable snapshot of every known emoji shortcode (without colons), combining
     * Unicode and image-only GitHub-custom shortcodes. Useful for validation and for computing
     * similar-shortcode suggestions. The returned set is a copy and is safe to iterate.
     *
     * @return all known shortcodes, lower-cased
     */
    public static java.util.Set<String> knownShortcodes() {
        java.util.Set<String> all = new java.util.HashSet<>(EMOJI.size() + CUSTOM_EMOJI.size());
        all.addAll(EMOJI.keySet());
        all.addAll(CUSTOM_EMOJI.keySet());
        return java.util.Collections.unmodifiableSet(all);
    }

    /**
     * Image-only GitHub-custom shortcodes (e.g. {@code :shipit:}, {@code :octocat:}) that have
     * no Unicode character. Maps shortcode → image URL; rendered as an {@code <img>} directly
     * during preprocessing since there is no character for {@link EmojiReplacer} to handle.
     */
    private static final Map<String, String> CUSTOM_EMOJI = new java.util.concurrent.ConcurrentHashMap<>();

    /**
     * Registers image-only (non-Unicode) emoji shortcodes such as GitHub's {@code :shipit:}.
     * Keys are shortcodes without colons; values are image URLs. These render as inline
     * {@code <img class="emoji">} tags. Null-safe and thread-safe.
     *
     * @param mappings shortcode → image URL mappings
     */
    public static void registerCustomEmojiImages(Map<String, String> mappings) {
        if (mappings == null || mappings.isEmpty()) return;
        for (Map.Entry<String, String> e : mappings.entrySet()) {
            if (e.getKey() != null && e.getValue() != null && !e.getValue().isEmpty()) {
                CUSTOM_EMOJI.put(e.getKey().toLowerCase(Locale.US), e.getValue());
            }
        }
    }

    /** @return the number of registered image-only (custom) emoji shortcodes. */
    public static int customEmojiCount() {
        return CUSTOM_EMOJI.size();
    }

    // Matches :shortcode: where the shortcode is word chars, +, - only. Non-greedy body.
    private static final Pattern SHORTCODE = Pattern.compile(":([a-z0-9_+\\-]+):", Pattern.CASE_INSENSITIVE);

    // ---------------------------------------------------------------------------------------
    // 2. Subscript / superscript
    //    ~x~  -> <sub>x</sub>     (single tilde only; ~~x~~ strikethrough is preserved)
    //    ^x^  -> <sup>x</sup>
    //    Bodies may not contain whitespace or the delimiter itself, matching GitHub behaviour
    //    and avoiding accidental matches across large spans.
    // ---------------------------------------------------------------------------------------

    // (?<!~) / (?!~) guards ensure we don't touch the ~~ strikethrough delimiter.
    private static final Pattern SUB = Pattern.compile("(?<!~)~(?!~)([^~\\s]+)(?<!~)~(?!~)");
    private static final Pattern SUP = Pattern.compile("\\^([^\\^\\s]+)\\^");

    /**
     * Applies the text-level transforms (emoji shortcodes, subscript, superscript) that must
     * run <i>before</i> CommonMark parsing. Fenced/inline code is protected so shortcodes and
     * sub/sup markers inside code spans are left literal.
     *
     * @param markdown the raw markdown (may be {@code null})
     * @return the transformed markdown
     */
    public static String preprocessMarkdown(String markdown) {
        if (markdown == null || markdown.isEmpty()) return markdown;
        return protectingCode(markdown, MarkdownExtras::applyInlineTransforms);
    }

    private static String applyInlineTransforms(String text) {
        text = replaceEmojiShortcodes(text);
        text = SUB.matcher(text).replaceAll("<sub>$1</sub>");
        text = SUP.matcher(text).replaceAll("<sup>$1</sup>");
        return text;
    }

    private static String replaceEmojiShortcodes(String text) {
        Matcher m = SHORTCODE.matcher(text);
        StringBuilder sb = new StringBuilder(text.length());
        while (m.find()) {
            String code = m.group(1).toLowerCase(Locale.US);
            String emoji = EMOJI.get(code);
            String replacement;
            if (emoji != null) {
                // Unicode emoji: substitute the character (EmojiReplacer renders it downstream).
                replacement = emoji;
            } else {
                String imgUrl = CUSTOM_EMOJI.get(code);
                if (imgUrl != null) {
                    // Image-only GitHub-custom emoji: emit an <img> directly. CommonMark passes
                    // inline HTML through, so it survives parsing.
                    replacement = "<img class=\"emoji\" title=\":" + code + ":\" alt=\":" + code + ":\" src=\""
                            + imgUrl + "\" height=\"20\" width=\"20\">";
                } else {
                    // Unknown shortcode: leave the literal :code: untouched.
                    replacement = m.group(0);
                }
            }
            m.appendReplacement(sb, Matcher.quoteReplacement(replacement));
        }
        m.appendTail(sb);
        return sb.toString();
    }

    /**
     * Runs {@code fn} over the markdown while leaving fenced code blocks (``` … ```) and
     * inline code spans (`code`) untouched, so the transforms never mangle code samples.
     */
    private static String protectingCode(String markdown, java.util.function.Function<String, String> fn) {
        // Split on fenced code blocks first (multi-line), applying fn only outside them.
        Pattern fence = Pattern.compile("(?s)(```.*?```|~~~.*?~~~)");
        Matcher fm = fence.matcher(markdown);
        StringBuilder out = new StringBuilder(markdown.length() + 32);
        int last = 0;
        while (fm.find()) {
            out.append(protectingInlineCode(markdown.substring(last, fm.start()), fn));
            out.append(fm.group(1)); // leave fenced block verbatim
            last = fm.end();
        }
        out.append(protectingInlineCode(markdown.substring(last), fn));
        return out.toString();
    }

    private static String protectingInlineCode(String text, java.util.function.Function<String, String> fn) {
        Pattern inline = Pattern.compile("(`+)(.+?)\\1");
        Matcher im = inline.matcher(text);
        StringBuilder out = new StringBuilder(text.length() + 16);
        int last = 0;
        while (im.find()) {
            out.append(fn.apply(text.substring(last, im.start())));
            out.append(im.group()); // leave inline code verbatim
            last = im.end();
        }
        out.append(fn.apply(text.substring(last)));
        return out.toString();
    }

    // ---------------------------------------------------------------------------------------
    // 3. Alerts / callouts  ( > [!NOTE] etc. )
    // ---------------------------------------------------------------------------------------

    /** The alert types GitHub recognises. */
    private static final String[] ALERT_TYPES = {"NOTE", "TIP", "IMPORTANT", "WARNING", "CAUTION"};

    // Matches a rendered blockquote whose first line (after optional <p>) is [!TYPE].
    // commonmark renders  > [!NOTE]\n> body  as  <blockquote>\n<p>[!NOTE]<br />body</p>\n</blockquote>
    // or, when body is a new paragraph, <blockquote><p>[!NOTE]</p><p>body</p></blockquote>.
    private static final Pattern ALERT_BLOCKQUOTE = Pattern.compile(
            "(?is)<blockquote>\\s*<p>\\s*\\[!(" + String.join("|", ALERT_TYPES) + ")\\]\\s*(?:<br\\s*/?>)?\\s*(.*?)</blockquote>");

    /**
     * Rewrites GitHub-style alert blockquotes in rendered HTML into styled callout divs.
     * Non-alert blockquotes are left unchanged.
     *
     * @param html rendered HTML (may be {@code null})
     * @return HTML with alert blockquotes converted to {@code .markdown-alert} divs
     */
    public static String postProcessHtml(String html) {
        if (html == null || html.isEmpty() || !html.contains("[!")) return html;
        Matcher m = ALERT_BLOCKQUOTE.matcher(html);
        StringBuilder sb = new StringBuilder(html.length() + 64);
        while (m.find()) {
            String type = m.group(1).toUpperCase(Locale.US);
            String inner = m.group(2); // remaining blockquote inner HTML up to (not incl) </blockquote>
            String title = type.charAt(0) + type.substring(1).toLowerCase(Locale.US);
            String replacement =
                    "<div class=\"markdown-alert markdown-alert-" + type.toLowerCase(Locale.US) + "\">"
                    + "<p class=\"markdown-alert-title\">" + title + "</p>"
                    + "<p>" + inner
                    + "</div>";
            m.appendReplacement(sb, Matcher.quoteReplacement(replacement));
        }
        m.appendTail(sb);
        return sb.toString();
    }
}
