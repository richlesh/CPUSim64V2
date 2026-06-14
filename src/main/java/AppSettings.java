// SPDX-License-Identifier: Apache-2.0
import java.awt.Color;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

public class AppSettings {
    private static final Path SETTINGS_FILE =
        Path.of(System.getProperty("user.home"), ".cpusim64-settings.json");

    public String fontName = "Monospaced";
    public int fontSize = 14;
    public String licenseEmail = null;
    public String licenseKey = null;
    public Color[] colors = {
        Color.BLACK,              // normal
        new Color(0, 0, 180),     // keyword
        new Color(128, 0, 128),   // directive
        new Color(0x99, 0x99, 0x99),   // comment
        new Color(163, 21, 21),   // string
        new Color(180, 100, 0),   // number
        new Color(200, 0, 100),   // register
        new Color(0, 100, 100),   // label
        new Color(100, 0, 150)    // condition
    };

    public void save() {
        try {
            StringBuilder sb = new StringBuilder("{\n");
            sb.append("  \"fontName\": \"").append(escape(fontName)).append("\",\n");
            sb.append("  \"fontSize\": ").append(fontSize).append(",\n");
            if (licenseEmail != null) sb.append("  \"licenseEmail\": \"").append(escape(licenseEmail)).append("\",\n");
            if (licenseKey != null) sb.append("  \"licenseKey\": \"").append(escape(licenseKey)).append("\",\n");
            sb.append("  \"colors\": [");
            for (int i = 0; i < colors.length; i++) {
                if (i > 0) sb.append(", ");
                sb.append("\"").append(colorToHex(colors[i])).append("\"");
            }
            sb.append("]\n}");
            Files.writeString(SETTINGS_FILE, sb.toString());
        } catch (IOException e) {
            // silently ignore save errors
        }
    }

    public static AppSettings load() {
        AppSettings s = new AppSettings();
        try {
            if (!Files.exists(SETTINGS_FILE)) return s;
            String json = Files.readString(SETTINGS_FILE);

            String font = extractString(json, "fontName");
            if (font != null) s.fontName = font;

            Integer size = extractInt(json, "fontSize");
            if (size != null) s.fontSize = size;

            s.licenseEmail = extractString(json, "licenseEmail");
            s.licenseKey = extractString(json, "licenseKey");

            List<String> colorList = extractArray(json, "colors");
            if (colorList != null) {
                for (int i = 0; i < Math.min(colorList.size(), s.colors.length); i++) {
                    s.colors[i] = hexToColor(colorList.get(i));
                }
            }
        } catch (Exception e) {
            // return defaults on any parse error
        }
        return s;
    }

    private static String colorToHex(Color c) {
        return String.format("#%02x%02x%02x", c.getRed(), c.getGreen(), c.getBlue());
    }

    private static Color hexToColor(String hex) {
        hex = hex.trim().replace("\"", "");
        return Color.decode(hex);
    }

    private static String escape(String s) {
        return s.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    private static String extractString(String json, String key) {
        Matcher m = Pattern.compile("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"").matcher(json);
        return m.find() ? m.group(1) : null;
    }

    private static Integer extractInt(String json, String key) {
        Matcher m = Pattern.compile("\"" + key + "\"\\s*:\\s*(\\d+)").matcher(json);
        return m.find() ? Integer.parseInt(m.group(1)) : null;
    }

    private static List<String> extractArray(String json, String key) {
        Matcher m = Pattern.compile("\"" + key + "\"\\s*:\\s*\\[([^\\]]+)]").matcher(json);
        if (!m.find()) return null;
        String[] items = m.group(1).split(",");
        List<String> result = new ArrayList<>();
        for (String item : items) {
            result.add(item.trim().replace("\"", ""));
        }
        return result;
    }
}
