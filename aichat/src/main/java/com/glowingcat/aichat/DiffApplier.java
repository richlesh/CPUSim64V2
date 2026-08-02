/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import java.util.ArrayList;
import java.util.List;

/**
 * Applies unified diff patches to text content.
 * Supports standard unified diff format with @@ hunk headers.
 */
public class DiffApplier {

    /**
     * Apply a unified diff to the original text.
     *
     * @param original the original document text
     * @param diff     the unified diff content (without the --- and +++ file headers)
     * @return the patched text
     * @throws DiffException if the diff cannot be applied cleanly
     */
    public static String apply(String original, String diff) throws DiffException {
        String[] originalLines = original.split("\n", -1);
        List<Hunk> hunks = parseHunks(diff);

        // Apply hunks in reverse order so line numbers remain valid
        List<String> result = new ArrayList<>(List.of(originalLines));

        // Sort hunks by start line descending
        hunks.sort((a, b) -> Integer.compare(b.originalStart, a.originalStart));

        for (Hunk hunk : hunks) {
            applyHunk(result, hunk);
        }

        return String.join("\n", result);
    }

    /**
     * Parse unified diff content into hunks.
     */
    static List<Hunk> parseHunks(String diff) throws DiffException {
        List<Hunk> hunks = new ArrayList<>();
        String[] lines = diff.split("\n");
        int i = 0;

        // Regex for standard unified diff hunk header: @@ -start[,count] +start[,count] @@
        java.util.regex.Pattern hunkPattern = java.util.regex.Pattern.compile(
            "^@@\\s+-([\\d]+)(?:,([\\d]+))?\\s+\\+([\\d]+)(?:,([\\d]+))?\\s+@@");

        while (i < lines.length) {
            String line = lines[i];

            // Skip --- and +++ headers if present
            if (line.startsWith("---") || line.startsWith("+++")) {
                i++;
                continue;
            }

            // Look for @@ hunk header using regex
            java.util.regex.Matcher m = hunkPattern.matcher(line);
            if (m.find()) {
                Hunk hunk = new Hunk();
                hunk.originalStart = Integer.parseInt(m.group(1));
                hunk.originalCount = m.group(2) != null ? Integer.parseInt(m.group(2)) : 1;
                hunk.newStart = Integer.parseInt(m.group(3));
                hunk.newCount = m.group(4) != null ? Integer.parseInt(m.group(4)) : 1;
                i++;

                // Collect hunk lines
                while (i < lines.length) {
                    String hunkLine = lines[i];
                    // Stop at next hunk header
                    if (hunkPattern.matcher(hunkLine).find()) break;
                    // Stop at --- / +++ (next file in multi-file diff)
                    if (hunkLine.startsWith("---") || hunkLine.startsWith("+++")) break;

                    if (hunkLine.startsWith("-")) {
                        hunk.removedLines.add(hunkLine.substring(1));
                        hunk.operations.add(new Operation(OpType.REMOVE, hunkLine.substring(1)));
                    } else if (hunkLine.startsWith("+")) {
                        hunk.addedLines.add(hunkLine.substring(1));
                        hunk.operations.add(new Operation(OpType.ADD, hunkLine.substring(1)));
                    } else if (hunkLine.startsWith(" ")) {
                        hunk.operations.add(new Operation(OpType.CONTEXT, hunkLine.substring(1)));
                    } else if (hunkLine.isEmpty()) {
                        // Empty line in diff = context line that was empty
                        hunk.operations.add(new Operation(OpType.CONTEXT, ""));
                    } else {
                        // Treat as context (some diffs omit the leading space)
                        hunk.operations.add(new Operation(OpType.CONTEXT, hunkLine));
                    }
                    i++;
                }

                hunks.add(hunk);
            } else {
                i++;
            }
        }

        // If no hunks found with @@ headers, try treating the whole diff as a single hunk
        if (hunks.isEmpty()) {
            hunks = parseFreeformDiff(lines);
        }

        return hunks;
    }

    /**
     * Fallback parser for diffs without @@ hunk headers.
     * Treats the entire diff as a single hunk starting at line 1.
     */
    private static List<Hunk> parseFreeformDiff(String[] lines) {
        Hunk hunk = new Hunk();
        hunk.originalStart = 1;
        hunk.originalCount = 0;
        hunk.newStart = 1;
        hunk.newCount = 0;
        boolean hasChanges = false;

        for (String line : lines) {
            if (line.startsWith("---") || line.startsWith("+++")) continue;
            if (line.startsWith("-")) {
                hunk.removedLines.add(line.substring(1));
                hunk.operations.add(new Operation(OpType.REMOVE, line.substring(1)));
                hunk.originalCount++;
                hasChanges = true;
            } else if (line.startsWith("+")) {
                hunk.addedLines.add(line.substring(1));
                hunk.operations.add(new Operation(OpType.ADD, line.substring(1)));
                hunk.newCount++;
                hasChanges = true;
            } else if (line.startsWith(" ")) {
                hunk.operations.add(new Operation(OpType.CONTEXT, line.substring(1)));
                hunk.originalCount++;
                hunk.newCount++;
            } else if (!line.isEmpty()) {
                hunk.operations.add(new Operation(OpType.CONTEXT, line));
                hunk.originalCount++;
                hunk.newCount++;
            }
        }

        List<Hunk> hunks = new ArrayList<>();
        if (hasChanges) hunks.add(hunk);
        return hunks;
    }

    /**
     * Apply a single hunk to the result lines.
     */
    private static void applyHunk(List<String> result, Hunk hunk) throws DiffException {
        // Convert to 0-based index
        int startIdx = hunk.originalStart - 1;

        // Find the actual position by matching context if possible
        int matchIdx = findHunkPosition(result, hunk, startIdx);

        // Apply operations at the found position
        int pos = matchIdx;
        List<String> newLines = new ArrayList<>();

        for (Operation op : hunk.operations) {
            switch (op.type) {
                case CONTEXT:
                    if (pos < result.size()) {
                        newLines.add(result.get(pos));
                        pos++;
                    }
                    break;
                case REMOVE:
                    if (pos < result.size()) {
                        pos++; // Skip the removed line
                    }
                    break;
                case ADD:
                    newLines.add(op.content);
                    break;
            }
        }

        // Replace the affected range in result
        int endPos = pos;
        for (int i = startIdx; i < matchIdx; i++) {
            // Lines before the match point stay
        }

        // Remove old lines and insert new ones
        int removeFrom = matchIdx;
        int removeTo = endPos;
        if (removeFrom >= 0 && removeTo <= result.size()) {
            result.subList(removeFrom, removeTo).clear();
            result.addAll(removeFrom, newLines);
        }
    }

    /**
     * Find the best position to apply a hunk by matching context lines.
     * Falls back to the line number in the hunk header.
     */
    private static int findHunkPosition(List<String> lines, Hunk hunk, int suggestedStart) {
        // Get the first few context/remove lines from operations to match
        List<String> matchLines = new ArrayList<>();
        for (Operation op : hunk.operations) {
            if (op.type == OpType.CONTEXT || op.type == OpType.REMOVE) {
                matchLines.add(op.content);
                if (matchLines.size() >= 3) break;
            }
        }

        if (matchLines.isEmpty()) return Math.max(0, Math.min(suggestedStart, lines.size()));

        // Try exact position first
        if (matchesAt(lines, suggestedStart, matchLines)) {
            return suggestedStart;
        }

        // Search nearby (within 50 lines)
        for (int offset = 1; offset <= 50; offset++) {
            if (suggestedStart - offset >= 0 && matchesAt(lines, suggestedStart - offset, matchLines)) {
                return suggestedStart - offset;
            }
            if (suggestedStart + offset < lines.size() && matchesAt(lines, suggestedStart + offset, matchLines)) {
                return suggestedStart + offset;
            }
        }

        // Fall back to suggested position
        return Math.max(0, Math.min(suggestedStart, lines.size()));
    }

    private static boolean matchesAt(List<String> lines, int pos, List<String> matchLines) {
        if (pos < 0 || pos + matchLines.size() > lines.size()) return false;
        for (int i = 0; i < matchLines.size(); i++) {
            if (!lines.get(pos + i).stripTrailing().equals(matchLines.get(i).stripTrailing())) return false;
        }
        return true;
    }

    // --- Internal types ---

    static class Hunk {
        int originalStart;
        int originalCount;
        int newStart;
        int newCount;
        List<String> removedLines = new ArrayList<>();
        List<String> addedLines = new ArrayList<>();
        List<Operation> operations = new ArrayList<>();
    }

    enum OpType { CONTEXT, REMOVE, ADD }

    static class Operation {
        final OpType type;
        final String content;

        Operation(OpType type, String content) {
            this.type = type;
            this.content = content;
        }
    }

    /** Exception thrown when a diff cannot be applied. */
    public static class DiffException extends Exception {
        public DiffException(String message) {
            super(message);
        }
    }
}
