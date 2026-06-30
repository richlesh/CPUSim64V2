// SPDX-License-Identifier: Apache-2.0
import java.awt.Color;
import java.awt.Font;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;

public class AppSettings {
    private static final Path SETTINGS_FILE =
        Path.of(System.getProperty("user.home"), ".cpusim64-settings.json");

    public String fontName = "Monospaced";
    public int fontSize = 14;
    public String aiFontName = detectAIFont();
    public int aiFontSize = 14;

    private static String detectAIFont() {
        String os = System.getProperty("os.name", "").toLowerCase();
        String[] candidates;
        if (os.contains("linux")) candidates = new String[]{"DejaVu Sans", "Arial", "Helvetica", "SansSerif"};
        else if (os.contains("win")) candidates = new String[]{"Calibri", "Arial", "Helvetica", "SansSerif"};
        else candidates = new String[]{"Arial", "Helvetica", "SansSerif"};
        for (String name : candidates) {
            Font f = new Font(name, Font.PLAIN, 14);
            if (!f.getFamily().equals("Dialog")) return name;
        }
        return "SansSerif";
    }
    public String licenseEmail = null;
    public String licenseKey = null;
    public String llmVendor = "OpenAI";
    public String llmModel = "gpt-4o";
    public String llmApiKey = null;
    public Color userPromptColor = new Color(0x00, 0xAA, 0x00);
    public Color aiResponseColor = new Color(0x33, 0x99, 0xFF);
    public Color consoleFg = new Color(0xBB, 0xBB, 0xBB);
    public Color consoleBg = new Color(30, 30, 30);
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
    public int debugMainDivider = -1;
    public int debugRegStackDivider = -1;
    public int[] debugRegColWidths = null;
    public int[] debugStackColWidths = null;
    public int debugWindowWidth = 1100;
    public int debugWindowHeight = 700;
    public double heapSizeMiB = 1.0;
    public double stackSizeKiB = 8.0;

    public void save() {
        try {
            StringBuilder sb = new StringBuilder("{\n");
            sb.append("  \"fontName\": \"").append(escape(fontName)).append("\",\n");
            sb.append("  \"fontSize\": ").append(fontSize).append(",\n");
            sb.append("  \"aiFontName\": \"").append(escape(aiFontName)).append("\",\n");
            sb.append("  \"aiFontSize\": ").append(aiFontSize).append(",\n");
            if (licenseEmail != null) sb.append("  \"licenseEmail\": \"").append(escape(licenseEmail)).append("\",\n");
            if (licenseKey != null) sb.append("  \"licenseKey\": \"").append(escape(licenseKey)).append("\",\n");
            if (llmVendor != null) sb.append("  \"llmVendor\": \"").append(escape(llmVendor)).append("\",\n");
            if (llmModel != null) sb.append("  \"llmModel\": \"").append(escape(llmModel)).append("\",\n");
            if (llmApiKey != null) sb.append("  \"llmApiKey\": \"").append(escape(llmApiKey)).append("\",\n");
            sb.append("  \"userPromptColor\": \"").append(colorToHex(userPromptColor)).append("\",\n");
            sb.append("  \"aiResponseColor\": \"").append(colorToHex(aiResponseColor)).append("\",\n");
            sb.append("  \"consoleFg\": \"").append(colorToHex(consoleFg)).append("\",\n");
            sb.append("  \"consoleBg\": \"").append(colorToHex(consoleBg)).append("\",\n");
            sb.append("  \"colors\": [");
            for (int i = 0; i < colors.length; i++) {
                if (i > 0) sb.append(", ");
                sb.append("\"").append(colorToHex(colors[i])).append("\"");
            }
            sb.append("],\n");
            sb.append("  \"debugMainDivider\": ").append(debugMainDivider).append(",\n");
            sb.append("  \"debugRegStackDivider\": ").append(debugRegStackDivider).append(",\n");
            sb.append("  \"debugWindowWidth\": ").append(debugWindowWidth).append(",\n");
            sb.append("  \"debugWindowHeight\": ").append(debugWindowHeight).append(",\n");
            if (debugRegColWidths != null) {
                sb.append("  \"debugRegColWidths\": [");
                for (int i = 0; i < debugRegColWidths.length; i++) {
                    if (i > 0) sb.append(", ");
                    sb.append(debugRegColWidths[i]);
                }
                sb.append("],\n");
            }
            if (debugStackColWidths != null) {
                sb.append("  \"debugStackColWidths\": [");
                for (int i = 0; i < debugStackColWidths.length; i++) {
                    if (i > 0) sb.append(", ");
                    sb.append(debugStackColWidths[i]);
                }
                sb.append("],\n");
            }
            sb.append("  \"heapSizeMiB\": ").append(heapSizeMiB).append(",\n");
            sb.append("  \"stackSizeKiB\": ").append(stackSizeKiB).append(",\n");
            sb.setLength(sb.length() - 2); // remove trailing comma+newline
            sb.append("\n}");
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

            String aiFont = extractString(json, "aiFontName");
            if (aiFont != null) s.aiFontName = aiFont;

            Integer aiSize = extractInt(json, "aiFontSize");
            if (aiSize != null) s.aiFontSize = aiSize;

            s.licenseEmail = extractString(json, "licenseEmail");
            s.licenseKey = extractString(json, "licenseKey");

            String vendor = extractString(json, "llmVendor");
            if (vendor != null) s.llmVendor = vendor;
            String model = extractString(json, "llmModel");
            if (model != null) s.llmModel = model;
            s.llmApiKey = extractString(json, "llmApiKey");

            String upc = extractString(json, "userPromptColor");
            if (upc != null) s.userPromptColor = hexToColor(upc);
            String arc = extractString(json, "aiResponseColor");
            if (arc != null) s.aiResponseColor = hexToColor(arc);

            String cfg = extractString(json, "consoleFg");
            if (cfg != null) s.consoleFg = hexToColor(cfg);
            String cbg = extractString(json, "consoleBg");
            if (cbg != null) s.consoleBg = hexToColor(cbg);

            List<String> colorList = extractArray(json, "colors");
            if (colorList != null) {
                for (int i = 0; i < Math.min(colorList.size(), s.colors.length); i++) {
                    s.colors[i] = hexToColor(colorList.get(i));
                }
            }

            Integer dmd = extractInt(json, "debugMainDivider");
            if (dmd != null) s.debugMainDivider = dmd;
            Integer drsd = extractInt(json, "debugRegStackDivider");
            if (drsd != null) s.debugRegStackDivider = drsd;
            Integer dww = extractInt(json, "debugWindowWidth");
            if (dww != null) s.debugWindowWidth = dww;
            Integer dwh = extractInt(json, "debugWindowHeight");
            if (dwh != null) s.debugWindowHeight = dwh;

            List<String> rcw = extractArray(json, "debugRegColWidths");
            if (rcw != null) {
                s.debugRegColWidths = new int[rcw.size()];
                for (int i = 0; i < rcw.size(); i++) s.debugRegColWidths[i] = Integer.parseInt(rcw.get(i).trim());
            }
            List<String> scw = extractArray(json, "debugStackColWidths");
            if (scw != null) {
                s.debugStackColWidths = new int[scw.size()];
                for (int i = 0; i < scw.size(); i++) s.debugStackColWidths[i] = Integer.parseInt(scw.get(i).trim());
            }

            Integer hs = extractInt(json, "heapSize");
            if (hs != null) s.heapSizeMiB = hs / (1024.0 * 1024.0);
            Integer ss = extractInt(json, "stackSize");
            if (ss != null) s.stackSizeKiB = ss / 1024.0;
            Double hm = extractDouble(json, "heapSizeMiB");
            if (hm != null) s.heapSizeMiB = hm;
            Double sk = extractDouble(json, "stackSizeKiB");
            if (sk != null) s.stackSizeKiB = sk;
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

    private static Double extractDouble(String json, String key) {
        Matcher m = Pattern.compile("\"" + key + "\"\\s*:\\s*([\\d.eE+-]+)").matcher(json);
        return m.find() ? Double.parseDouble(m.group(1)) : null;
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
