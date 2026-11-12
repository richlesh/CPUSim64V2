/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.cpusim64ide;

import com.glowingcat.aichat.MarkdownExtras;
import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.lang.reflect.Type;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.zip.GZIPInputStream;

/**
 * Loads the bundled gemoji dataset ({@code /emoji.json.gz}) and registers every shortcode
 * alias with {@link MarkdownExtras}, giving full GitHub emoji-shortcode coverage ({@code
 * :tada:} → 🎉) offline.
 * <p>
 * The dataset is GitHub's own <a href="https://github.com/github/gemoji">gemoji</a>
 * {@code db/emoji.json}, gzip-compressed (~34&nbsp;KB vs ~413&nbsp;KB raw) and read back as a
 * {@link GZIPInputStream}. It is bundled rather than fetched at runtime so the app works
 * offline and builds stay reproducible; refresh it deliberately via the Maven
 * {@code update-emoji} profile.
 * <p>
 * Loading is best-effort: any failure leaves {@link MarkdownExtras}'s built-in fallback
 * subset in place and logs to stderr, so a missing/corrupt resource never breaks startup.
 */
public final class EmojiShortcodes {

    private static final String RESOURCE = "/emoji.json.gz";
    private static final String CUSTOM_RESOURCE = "/github-emoji.json";
    private static volatile boolean loaded = false;

    private EmojiShortcodes() {}

    /** One entry of the gemoji dataset (only the fields we need). */
    private static final class Entry {
        String emoji;
        List<String> aliases;
    }

    /**
     * Loads and registers the full shortcode set. Safe to call multiple times; only the first
     * call does work. Never throws — failures are logged and the built-in fallback remains.
     */
    public static synchronized void load() {
        if (loaded) return;
        loaded = true; // set first so a failure doesn't cause repeated retries
        try (InputStream raw = EmojiShortcodes.class.getResourceAsStream(RESOURCE)) {
            if (raw == null) {
                System.err.println("EmojiShortcodes: resource " + RESOURCE + " not found; using built-in subset.");
                return;
            }
            try (Reader reader = new InputStreamReader(new GZIPInputStream(raw), StandardCharsets.UTF_8)) {
                Type listType = new TypeToken<List<Entry>>() {}.getType();
                List<Entry> entries = new Gson().fromJson(reader, listType);
                if (entries == null) return;
                Map<String, String> map = new HashMap<>(entries.size() * 2);
                for (Entry e : entries) {
                    if (e == null || e.emoji == null || e.emoji.isEmpty() || e.aliases == null) continue;
                    for (String alias : e.aliases) {
                        if (alias != null && !alias.isEmpty()) {
                            map.put(alias, e.emoji);
                        }
                    }
                }
                MarkdownExtras.registerEmojiShortcodes(map);
            }
        } catch (Exception ex) {
            System.err.println("EmojiShortcodes: failed to load " + RESOURCE + ": " + ex.getMessage()
                    + " — using built-in subset.");
        }
        loadCustomImages();
    }

    /**
     * Loads the small set of image-only GitHub-custom shortcodes ({@code :shipit:},
     * {@code :octocat:}, …) from {@link #CUSTOM_RESOURCE} — a plain JSON object mapping
     * shortcode → image URL — and registers them as image emoji. Best-effort.
     */
    private static void loadCustomImages() {
        try (InputStream raw = EmojiShortcodes.class.getResourceAsStream(CUSTOM_RESOURCE)) {
            if (raw == null) return; // optional
            try (Reader reader = new InputStreamReader(raw, StandardCharsets.UTF_8)) {
                Type mapType = new TypeToken<Map<String, String>>() {}.getType();
                Map<String, String> custom = new Gson().fromJson(reader, mapType);
                if (custom != null) {
                    MarkdownExtras.registerCustomEmojiImages(custom);
                }
            }
        } catch (Exception ex) {
            System.err.println("EmojiShortcodes: failed to load " + CUSTOM_RESOURCE + ": " + ex.getMessage());
        }
    }
}
