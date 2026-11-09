/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.awt.Font;
import java.awt.GraphicsEnvironment;
import java.awt.Shape;
import java.awt.font.FontRenderContext;
import java.awt.font.GlyphVector;
import java.awt.geom.AffineTransform;
import java.awt.geom.PathIterator;
import java.awt.geom.Rectangle2D;
import java.util.LinkedHashMap;
import java.util.Locale;
import java.util.Map;

/**
 * Replaces certain BMP "symbol" characters — notably the macOS keyboard modifier
 * glyphs such as ⌘ (U+2318), ⇧ (U+21E7), ⌥ (U+2325) and ⌃ (U+2303) — with inline
 * {@code <svg>} vector images built from the actual system-font glyph outlines.
 * <p>
 * <b>Why this exists.</b> These glyphs are not present in most text fonts. On screen
 * JavaFX WebView silently falls back to a system font (e.g. "Apple Symbols") that
 * contains them, so they display correctly. However, WebView's <i>print</i> pipeline
 * (used for File &gt; Print and Export &gt; PDF) does not perform the same font
 * substitution, so the glyphs vanish from printed / exported output. Naming the
 * fallback font in CSS ({@code font-family: ..., 'Apple Symbols', ...}) does not fix
 * this because the print path does not honour the fallback the same way the screen
 * renderer does.
 * <p>
 * The robust fix — mirroring what {@link EmojiReplacer} does for non-BMP emoji — is to
 * embed the glyph as an {@code <img>} whose source is a {@code data:image/svg+xml} URI built
 * from the actual system-font glyph outline. We take the glyph {@link Shape outline} from an
 * AWT {@link Font} that can display the code point and serialise it as an SVG path.
 * <p>
 * <b>Why {@code <img>} and not inline {@code <svg>}.</b> An earlier version emitted an inline
 * {@code <svg>} element. That rendered correctly on screen and in exported HTML, but JavaFX
 * WebKit's <i>print</i> pipeline renders inline {@code <svg>} as blank, so the glyphs still
 * vanished from Print / Export &gt; PDF output. WebKit's printer <i>does</i> rasterise
 * {@code <img>} elements (including data-URI SVG sources), which is exactly how
 * {@link EmojiReplacer} survives the print path. The glyph fill is baked to black (correct on
 * a white printed page); {@link #symbolCss()} inverts it for dark-mode on-screen preview.
 * <p>
 * Java2D user space is y-down, the same convention as SVG, so no vertical flip is needed.
 */
public final class SymbolReplacer {

    /** Code points that are replaced with inline SVG. Keyboard modifier / editing symbols. */
    private static final int[] SYMBOL_CODE_POINTS = {
            0x2318, // ⌘ PLACE OF INTEREST SIGN (Command)
            0x21E7, // ⇧ UPWARDS WHITE ARROW (Shift)
            0x2325, // ⌥ OPTION KEY (Option / Alt)
            0x2303, // ⌃ UP ARROWHEAD (Control)
            0x21A9, // ↩ LEFTWARDS ARROW WITH HOOK (Return)
            0x23CE, // ⏎ RETURN SYMBOL
            0x232B, // ⌫ ERASE TO THE LEFT (Backspace/Delete)
            0x2326, // ⌦ ERASE TO THE RIGHT (Forward Delete)
            0x238B, // ⎋ BROKEN CIRCLE WITH NORTHWEST ARROW (Escape)
            0x21E5, // ⇥ RIGHTWARDS ARROW TO BAR (Tab)
            0x21E4, // ⇤ LEFTWARDS ARROW TO BAR (Backtab)
            0x2190, // ← LEFTWARDS ARROW
            0x2191, // ↑ UPWARDS ARROW
            0x2192, // → RIGHTWARDS ARROW
            0x2193, // ↓ DOWNWARDS ARROW
            0x21DE, // ⇞ UPWARDS ARROW WITH DOUBLE STROKE (Page Up)
            0x21DF, // ⇟ DOWNWARDS ARROW WITH DOUBLE STROKE (Page Down)
            0x229E, // ⊞ SQUARED PLUS (Windows / Super key)
            0x21EA, // ⇪ UPWARDS WHITE ARROW FROM BAR (Caps Lock)
            0x2324, // ⌤ UP ARROWHEAD BETWEEN TWO HORIZONTAL BARS (Enter)
            0x23CF, // ⏏ EJECT SYMBOL (Eject)
    };

    /** Fonts to try, in order, when resolving a glyph outline. */
    private static final String[] CANDIDATE_FONTS = {
            "Apple Symbols",       // macOS
            "Segoe UI Symbol",     // Windows
            "Noto Sans Symbols2",  // Linux (Noto)
            "Noto Sans Symbols",
            "Arial Unicode MS",
            "DejaVu Sans",
            Font.SANS_SERIF,       // last-ditch logical font
    };

    private static final FontRenderContext FRC = new FontRenderContext(null, true, true);

    /** Lazily-built cache of code point -> inline SVG markup (or empty string if unavailable). */
    private static final Map<Integer, String> CACHE = new LinkedHashMap<>();

    /** Set of code points we handle, for fast membership tests. */
    private static final java.util.Set<Integer> HANDLED = new java.util.HashSet<>();
    static {
        for (int cp : SYMBOL_CODE_POINTS) HANDLED.add(cp);
    }

    private SymbolReplacer() {}

    /**
     * Replaces every handled symbol code point in {@code html} with an inline SVG image.
     * Characters that are not handled, or for which no glyph could be resolved, are left
     * unchanged.
     *
     * @param html HTML fragment (may be {@code null})
     * @return the HTML with symbol characters replaced by inline SVG, or the input unchanged
     */
    public static String replaceSymbols(String html) {
        if (html == null || html.isEmpty()) return html;

        // Fast path: nothing to do if none of our symbols are present.
        boolean any = false;
        for (int i = 0; i < html.length(); i++) {
            if (HANDLED.contains((int) html.charAt(i))) { any = true; break; }
        }
        if (!any) return html;

        StringBuilder out = new StringBuilder(html.length() + 64);
        // Only rewrite symbols that appear in text content, never inside an HTML tag
        // (e.g. a symbol character sitting in another element's attribute such as
        // <img alt="⏏️">). Rewriting inside a tag would corrupt it and spill the rest of
        // the tag as visible text. We track a simple in-tag flag by watching for '<' / '>'.
        boolean inTag = false;
        for (int i = 0; i < html.length(); i++) {
            char c = html.charAt(i);
            if (c == '<') {
                inTag = true;
                out.append(c);
                continue;
            }
            if (c == '>') {
                inTag = false;
                out.append(c);
                continue;
            }
            int cp = c;
            if (!inTag && HANDLED.contains(cp)) {
                String svg = svgFor(cp);
                if (svg != null && !svg.isEmpty()) {
                    out.append(svg);
                    continue;
                }
            }
            out.append(c);
        }
        return out.toString();
    }

    /** Returns cached inline SVG for a code point, building it on first use. */
    private static synchronized String svgFor(int codePoint) {
        String cached = CACHE.get(codePoint);
        if (cached != null) return cached;
        String svg = buildSvg(codePoint);
        CACHE.put(codePoint, svg == null ? "" : svg);
        return svg;
    }

    /**
     * Builds an inline SVG {@code <svg>} element for the given code point using the first
     * candidate font that can display it. Returns {@code null} if no font can render it
     * (e.g. in a headless environment with no symbol fonts installed).
     */
    private static String buildSvg(int codePoint) {
        Font font = resolveFont(codePoint);
        if (font == null) return null;

        String text = new String(Character.toChars(codePoint));
        GlyphVector gv = font.createGlyphVector(FRC, text);
        Shape outline = gv.getOutline();
        Rectangle2D b = outline.getBounds2D();
        if (b.getWidth() <= 0 || b.getHeight() <= 0) return null;

        // Render the glyph centered inside a *square* viewBox whose side is the larger of the
        // glyph's width or height. This gives every symbol a uniform square footprint (so they
        // sit in consistent boxes regardless of their natural aspect ratio) with the glyph
        // centered in that square.
        //
        // Baseline handling: with vertical-align:baseline the image's bottom edge rests on the
        // text baseline. Centering the glyph in a taller-than-glyph square adds empty padding
        // below the glyph equal to (side - h) / 2, which would lift the visible glyph off the
        // baseline. We cancel that by shifting the image down (negative vertical-align) by the
        // same amount, so the glyph's own bottom still lands on the baseline like a letter.
        double emSize = font.getSize2D();
        double w = b.getWidth();
        double h = b.getHeight();
        double side = Math.max(w, h);
        double padX = (side - w) / 2.0;
        double padY = (side - h) / 2.0;

        // Translate the outline so the tight glyph bounds are centered in the square viewBox.
        AffineTransform tx = AffineTransform.getTranslateInstance(-b.getX() + padX, -b.getY() + padY);
        Shape norm = tx.createTransformedShape(outline);
        String path = toSvgPath(norm);
        if (path.isEmpty()) return null;

        String alt = htmlEscape(text);

        // Emit the glyph as an <img> whose source is a data-URI SVG, rather than an inline
        // <svg> element. This mirrors EmojiReplacer's <img> approach and is required for the
        // PDF/print path: JavaFX WebKit's print pipeline (used by File > Print and
        // Export > PDF) renders inline <svg> elements as blank, but correctly rasterises
        // <img> elements — including data-URI SVG sources. Inline SVG works on screen and in
        // exported HTML but silently vanishes in printed output.
        //
        // The fill is baked to black (correct for print, on a white page); dark-mode
        // on-screen preview inverts it via symbolCss().
        String svgDoc = String.format(Locale.US,
                "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %.2f %.2f\">"
                        + "<path d=\"%s\" fill=\"black\"/></svg>",
                side, side, path);
        String dataUri = "data:image/svg+xml," + urlEncodeSvg(svgDoc);

        // The image is a square of side/emSize em, with the glyph centered inside it. Every
        // symbol uses the SAME vertical placement — vertical-align:middle — so short glyphs
        // (⌥ ⌃ arrows) and tall glyphs (⌘ ⇧ ↩) sit identically, centered on the text's mid
        // line. An earlier version shifted each glyph by its own square padding (padY), which
        // moved short glyphs down relative to tall ones; a single uniform rule avoids that.
        double sideEm = side / emSize;
        return String.format(Locale.US,
                "<img class=\"kbd-symbol\" role=\"img\" alt=\"%s\" aria-label=\"%s\" src=\"%s\" "
                        + "style=\"height:%.3fem;width:%.3fem;vertical-align:middle;display:inline-block\"/>",
                alt, alt, dataUri, sideEm, sideEm);
    }

    /**
     * CSS for the symbol images. In dark mode the baked-black glyph is inverted to white so
     * it stays visible against a dark background on screen. (Print/PDF output is on a white
     * page, where the un-inverted black glyph is correct.)
     *
     * @return a CSS snippet to inject into the preview {@code <head>}
     */
    public static String symbolCss() {
        // These !important-free overrides beat the generic `img { margin:12px 0; display:inline;
        // max-width:75% }` rule in preview.css by class specificity. Without them the symbol
        // images inherit that 12px vertical margin and generic display, which lifts them off
        // the text baseline (they render fine in isolation but not inside the preview page).
        // We pin the box back to a baseline-resting inline replaced element with no margin.
        return "img.kbd-symbol{"
                + "margin:0;"
                + "max-width:none;"
                + "display:inline-block;"
                + "vertical-align:baseline;"
                + "background:transparent;"
                + "}"
                + "body.dark-mode img.kbd-symbol{filter:invert(1);}";
    }

    /**
     * URL-encodes an SVG document for use in a {@code data:image/svg+xml,...} URI.
     * Percent-encodes the characters that are unsafe in an unquoted-ish data URI context
     * ({@code # % " < > &} and whitespace) while leaving the rest readable.
     */
    private static String urlEncodeSvg(String svg) {
        StringBuilder sb = new StringBuilder(svg.length() + 32);
        for (int i = 0; i < svg.length(); i++) {
            char c = svg.charAt(i);
            switch (c) {
                case '"':  sb.append("%22"); break;
                case '#':  sb.append("%23"); break;
                case '%':  sb.append("%25"); break;
                case '<':  sb.append("%3C"); break;
                case '>':  sb.append("%3E"); break;
                case '&':  sb.append("%26"); break;
                case '\n':
                case '\r':
                case '\t':
                case ' ':  sb.append("%20"); break;
                default:   sb.append(c); break;
            }
        }
        return sb.toString();
    }

    /** Finds a font that can display the code point, preferring the candidate list. */
    private static Font resolveFont(int codePoint) {
        for (String name : CANDIDATE_FONTS) {
            Font f = new Font(name, Font.PLAIN, 100);
            // A logical font ("SansSerif") always reports its family as the logical name;
            // canDisplay still reflects real glyph coverage via the underlying physical font.
            if (f.canDisplay(codePoint)) {
                return f;
            }
        }
        // Last resort: scan installed fonts (may be empty in some headless setups).
        try {
            GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
            for (Font f : ge.getAllFonts()) {
                if (f.canDisplay(codePoint)) {
                    return f.deriveFont(100f);
                }
            }
        } catch (Throwable ignore) {
            // Headless / no fonts — fall through.
        }
        return null;
    }

    /** Serialises a {@link Shape} to an SVG path {@code d} attribute string. */
    private static String toSvgPath(Shape shape) {
        StringBuilder sb = new StringBuilder();
        PathIterator it = shape.getPathIterator(null);
        double[] c = new double[6];
        while (!it.isDone()) {
            int type = it.currentSegment(c);
            switch (type) {
                case PathIterator.SEG_MOVETO:
                    sb.append(String.format(Locale.US, "M%.2f %.2f ", c[0], c[1]));
                    break;
                case PathIterator.SEG_LINETO:
                    sb.append(String.format(Locale.US, "L%.2f %.2f ", c[0], c[1]));
                    break;
                case PathIterator.SEG_QUADTO:
                    sb.append(String.format(Locale.US, "Q%.2f %.2f %.2f %.2f ", c[0], c[1], c[2], c[3]));
                    break;
                case PathIterator.SEG_CUBICTO:
                    sb.append(String.format(Locale.US, "C%.2f %.2f %.2f %.2f %.2f %.2f ",
                            c[0], c[1], c[2], c[3], c[4], c[5]));
                    break;
                case PathIterator.SEG_CLOSE:
                    sb.append("Z ");
                    break;
                default:
                    break;
            }
            it.next();
        }
        return sb.toString().trim();
    }

    private static String htmlEscape(String s) {
        return s.replace("&", "&amp;").replace("\"", "&quot;")
                .replace("<", "&lt;").replace(">", "&gt;");
    }
}
