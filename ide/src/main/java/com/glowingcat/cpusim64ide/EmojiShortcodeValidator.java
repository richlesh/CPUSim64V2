/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.cpusim64ide;

import com.glowingcat.aichat.MarkdownExtras;
import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import java.lang.reflect.Type;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * Validates GitHub-style emoji shortcodes (e.g. {@code :smile:}) against the known emoji set
 * and suggests similar valid shortcodes for an unknown one.
 * <p>
 * The authoritative set of valid shortcodes is {@link MarkdownExtras#knownShortcodes()} — the
 * same gemoji-backed dataset the renderer uses — so validation is fully offline and always
 * consistent with what actually renders. {@link #refreshFromGitHub()} can optionally pull the
 * live list from {@code https://api.github.com/emojis} and register any newly added shortcodes,
 * keeping the set current without a rebuild; it is best-effort and never required.
 * <p>
 * Suggestions for an unknown shortcode combine three strategies, ranked best-first:
 * <ol>
 *   <li><b>prefix</b> — known codes that start with the typed text;</li>
 *   <li><b>substring</b> — known codes that contain the typed text;</li>
 *   <li><b>spelled-like</b> — known codes within a small Levenshtein edit distance.</li>
 * </ol>
 */
public final class EmojiShortcodeValidator {

    /** GitHub's public emoji list; keys are shortcodes (without colons), values are image URLs. */
    private static final String GITHUB_EMOJIS_URL = "https://api.github.com/emojis";

    private EmojiShortcodeValidator() {}

    /**
     * Returns whether the shortcode (without surrounding colons) is a valid, known emoji.
     * Case-insensitive; null/blank returns {@code false}.
     *
     * @param code shortcode without colons (e.g. {@code "tada"})
     * @return true if known
     */
    public static boolean isValid(String code) {
        return MarkdownExtras.isKnownShortcode(code);
    }

    /**
     * Suggests up to {@code limit} valid shortcodes similar to the given (unknown) one.
     * Ordering: exact-prefix matches first, then substring matches, then edit-distance
     * ("spelled like") matches; within each group, shorter codes and closer matches rank
     * higher, then alphabetical. The query itself is never returned.
     *
     * @param code  the typed shortcode without colons (may be null/blank → empty result)
     * @param limit maximum number of suggestions (values &lt; 1 yield an empty list)
     * @return similar valid shortcodes, best first
     */
    public static List<String> suggest(String code, int limit) {
        if (code == null || code.isBlank() || limit < 1) return List.of();
        String q = code.toLowerCase(Locale.US);
        Set<String> known = MarkdownExtras.knownShortcodes();

        // Score each candidate; lower score = better. Unqualified candidates get Integer.MAX_VALUE.
        record Scored(String code, int tier, int distance) {}
        List<Scored> scored = new ArrayList<>();
        int maxEditDistance = Math.clamp(q.length() / 2 + 1, 1, 3);

        for (String cand : known) {
            if (cand.equals(q)) continue; // never suggest the exact (but here unknown) query
            int tier;
            int distance;
            if (cand.startsWith(q)) {
                tier = 0;
                distance = cand.length() - q.length(); // closer length ranks higher
            } else if (cand.contains(q)) {
                tier = 1;
                distance = cand.length() - q.length();
            } else {
                int d = levenshtein(q, cand, maxEditDistance);
                if (d > maxEditDistance) continue;
                tier = 2;
                distance = d;
            }
            scored.add(new Scored(cand, tier, distance));
        }

        scored.sort(Comparator
                .comparingInt(Scored::tier)
                .thenComparingInt(Scored::distance)
                .thenComparingInt(s -> s.code().length())
                .thenComparing(Scored::code));

        List<String> out = new ArrayList<>(Math.min(limit, scored.size()));
        for (Scored s : scored) {
            out.add(s.code());
            if (out.size() >= limit) break;
        }
        return out;
    }

    /**
     * Computes the Levenshtein edit distance between {@code a} and {@code b}, short-circuiting
     * once the running minimum of a row exceeds {@code max} (returns {@code max + 1} then). This
     * keeps the suggestion scan cheap over the full ~1,900-entry dataset.
     */
    static int levenshtein(String a, String b, int max) {
        int la = a.length();
        int lb = b.length();
        if (Math.abs(la - lb) > max) return max + 1;
        int[] prev = new int[lb + 1];
        int[] curr = new int[lb + 1];
        for (int j = 0; j <= lb; j++) prev[j] = j;
        for (int i = 1; i <= la; i++) {
            curr[0] = i;
            int rowMin = curr[0];
            char ca = a.charAt(i - 1);
            for (int j = 1; j <= lb; j++) {
                int cost = ca == b.charAt(j - 1) ? 0 : 1;
                curr[j] = Math.min(Math.min(prev[j] + 1, curr[j - 1] + 1), prev[j - 1] + cost);
                rowMin = Math.min(rowMin, curr[j]);
            }
            if (rowMin > max) return max + 1;
            int[] tmp = prev;
            prev = curr;
            curr = tmp;
        }
        return prev[lb];
    }

    /**
     * Fetches the current emoji list from GitHub and registers any shortcodes not already known
     * as image-only emoji, so validation recognizes recently added ones. Best-effort and
     * blocking — call off the EDT (e.g. from a background thread). Returns the number of newly
     * registered shortcodes, or 0 on any failure.
     *
     * @return count of newly added shortcodes (0 if none or on error)
     */
    public static int refreshFromGitHub() {
        try {
            HttpClient client = HttpClient.newBuilder()
                    .connectTimeout(Duration.ofSeconds(10))
                    .followRedirects(HttpClient.Redirect.NORMAL)
                    .build();
            HttpRequest request = HttpRequest.newBuilder()
                    .uri(URI.create(GITHUB_EMOJIS_URL))
                    .timeout(Duration.ofSeconds(15))
                    .header("Accept", "application/vnd.github+json")
                    .header("User-Agent", "PurplePlatypus")
                    .GET()
                    .build();
            HttpResponse<String> resp = client.send(request, HttpResponse.BodyHandlers.ofString());
            if (resp.statusCode() != 200 || resp.body() == null || resp.body().isBlank()) {
                return 0;
            }
            Type mapType = new TypeToken<Map<String, String>>() {}.getType();
            Map<String, String> emojis = new Gson().fromJson(resp.body(), mapType);
            if (emojis == null || emojis.isEmpty()) return 0;

            Map<String, String> additions = new HashMap<>();
            for (Map.Entry<String, String> e : emojis.entrySet()) {
                String code = e.getKey();
                String url = e.getValue();
                if (code == null || url == null) continue;
                if (!MarkdownExtras.isKnownShortcode(code)) {
                    additions.put(code.toLowerCase(Locale.US), url);
                }
            }
            if (!additions.isEmpty()) {
                MarkdownExtras.registerCustomEmojiImages(additions);
            }
            return additions.size();
        } catch (Exception ex) {
            // Best-effort: offline, rate-limited, or malformed response — keep the bundled set.
            return 0;
        }
    }
}
