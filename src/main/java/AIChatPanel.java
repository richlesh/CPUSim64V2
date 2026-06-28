// SPDX-License-Identifier: Apache-2.0
import javax.swing.*;
import javax.swing.text.*;
import javax.swing.text.html.HTMLDocument;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.RoundRectangle2D;
import java.io.*;
import java.net.*;
import java.net.http.*;
import java.nio.file.*;
import java.util.*;
import java.util.List;
import java.util.stream.*;

public class AIChatPanel extends JPanel {
    private final JPanel chatPanel;
    private final JScrollPane chatScroll;
    private final JTextArea inputArea;
    private final JButton sendBtn;
    private final JTextPane codeEditor;
    private final JTextArea console;
    private final AppSettings settings;
    private final List<Map<String, String>> messages = new ArrayList<>();
    private String systemPrompt;
    private final ImageIcon humanIcon;
    private final ImageIcon aiIcon;
    private JLabel pulsingAiLabel;
    private javax.swing.Timer pulseTimer;
    private Runnable onCodeChanged;
    private Runnable statusUpdater;
    private volatile Thread currentThread;

    public AIChatPanel(JTextPane codeEditor, JTextArea console, AppSettings settings) {
        super(new BorderLayout());
        this.codeEditor = codeEditor;
        this.console = console;
        this.settings = settings;
        this.systemPrompt = buildSystemPrompt();

        // Load icons
        var humanUrl = AIChatPanel.class.getResource("/human.png");
        var aiUrl = AIChatPanel.class.getResource("/AI.png");
        humanIcon = humanUrl != null ? new ImageIcon(new ImageIcon(humanUrl).getImage().getScaledInstance(28, 28, Image.SCALE_SMOOTH)) : null;
        aiIcon = aiUrl != null ? new ImageIcon(new ImageIcon(aiUrl).getImage().getScaledInstance(28, 28, Image.SCALE_SMOOTH)) : null;

        setPreferredSize(new Dimension(380, 0));
        setBorder(BorderFactory.createTitledBorder("AI Assistant"));

        chatPanel = new JPanel() {
            @Override public Dimension getPreferredSize() {
                // Constrain width to viewport so text wraps
                if (getParent() != null) {
                    int w = getParent().getWidth();
                    if (w > 0) {
                        Dimension d = super.getPreferredSize();
                        return new Dimension(w, d.height);
                    }
                }
                return super.getPreferredSize();
            }
        };
        chatPanel.setLayout(new BoxLayout(chatPanel, BoxLayout.Y_AXIS));
        chatPanel.setBackground(new Color(245, 245, 245));
        chatScroll = new JScrollPane(chatPanel);
        chatScroll.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
        chatScroll.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        chatScroll.getVerticalScrollBar().setUnitIncrement(16);

        inputArea = new JTextArea(3, 20);
        inputArea.setFont(new Font(settings.fontName, Font.PLAIN, settings.fontSize));
        inputArea.setLineWrap(true);
        inputArea.setWrapStyleWord(true);
        inputArea.addKeyListener(new KeyAdapter() {
            @Override public void keyPressed(KeyEvent e) {
                if (e.getKeyCode() == KeyEvent.VK_ENTER && !e.isShiftDown()) {
                    e.consume();
                    sendMessage();
                }
            }
        });
        JScrollPane inputScroll = new JScrollPane(inputArea);
        inputScroll.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
        sendBtn = new JButton("Send");
        JButton clearBtn = new JButton("Clear");
        JPanel btnPanel = new JPanel(new GridLayout(1, 2, 4, 0));
        btnPanel.add(sendBtn);
        btnPanel.add(clearBtn);
        JPanel inputPanel = new JPanel(new BorderLayout(4, 0));
        inputPanel.setBorder(BorderFactory.createEmptyBorder(4, 4, 4, 4));
        inputPanel.add(inputScroll, BorderLayout.CENTER);
        inputPanel.add(btnPanel, BorderLayout.EAST);

        JLabel statusBar = new JLabel(" ");
        statusBar.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 10));
        statusBar.setBorder(BorderFactory.createEmptyBorder(2, 4, 2, 4));
        this.statusUpdater = () -> {
            int sp = systemPrompt.length();
            int prog = codeEditor.getText().length();
            int out = console.getText().length();
            statusBar.setText(String.format("LLM System Prompt: %,d chars    Current Program: %,d chars    Current Output: %,d chars", sp, prog, out));
        };
        statusUpdater.run();

        JPanel southPanel = new JPanel(new BorderLayout());
        southPanel.add(inputPanel, BorderLayout.CENTER);
        southPanel.add(statusBar, BorderLayout.SOUTH);

        add(chatScroll, BorderLayout.CENTER);
        add(southPanel, BorderLayout.SOUTH);

        sendBtn.addActionListener(e -> sendMessage());
        clearBtn.addActionListener(e -> { messages.clear(); chatPanel.removeAll(); chatPanel.revalidate(); chatPanel.repaint(); });
    }

    public void setOnCodeChanged(Runnable callback) {
        this.onCodeChanged = callback;
    }

    public void updateFont() {
        Font font = new Font(settings.fontName, Font.PLAIN, settings.fontSize);
        inputArea.setFont(font);
        for (Component c : chatPanel.getComponents()) {
            updateFontRecursive(c, font);
        }
        chatPanel.revalidate();
        chatPanel.repaint();
    }

    private void updateFontRecursive(Component c, Font font) {
        if (c instanceof JTextArea) c.setFont(font);
        if (c instanceof JTextPane) c.setFont(font);
        if (c instanceof Container) {
            for (Component child : ((Container) c).getComponents()) updateFontRecursive(child, font);
        }
    }

    private void sendMessage() {
        String text = inputArea.getText().trim();
        if (text.isEmpty()) return;
        inputArea.setText("");
        addUserBubble(text);
        statusUpdater.run();

        String context = "Current source code:\n```\n" + codeEditor.getText() + "\n```\n\n"
            + "Console output:\n```\n" + console.getText() + "\n```";

        if (messages.isEmpty()) {
            messages.add(Map.of("role", "system", "content", systemPrompt));
        }
        messages.add(Map.of("role", "user", "content", context + "\n\nUser request: " + text));

        sendBtn.setEnabled(false);
        startPulse();
        currentThread = new Thread(() -> {
            try {
                String response = callLLM();
                SwingUtilities.invokeLater(() -> {
                    stopPulse();
                    processResponse(response);
                    sendBtn.setEnabled(true);
                });
            } catch (Exception ex) {
                SwingUtilities.invokeLater(() -> {
                    stopPulse();
                    if (!Thread.currentThread().isInterrupted())
                        addAiBubble("Error: " + ex.getMessage());
                    sendBtn.setEnabled(true);
                });
            }
        });
        currentThread.start();
    }

    private void addUserBubble(String text) {
        Color uColor = settings.userPromptColor;
        JPanel bubble = new JPanel(new BorderLayout(8, 0)) {
            @Override protected void paintComponent(Graphics g) {
                Graphics2D g2 = (Graphics2D) g.create();
                g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                g2.setColor(uColor);
                g2.fill(new RoundRectangle2D.Float(0, 0, getWidth(), getHeight(), 16, 16));
                g2.dispose();
                super.paintComponent(g);
            }
        };
        bubble.setOpaque(false);
        bubble.setBorder(BorderFactory.createEmptyBorder(8, 8, 8, 12));

        JLabel icon = new JLabel(humanIcon);
        icon.setVerticalAlignment(SwingConstants.TOP);
        bubble.add(icon, BorderLayout.WEST);

        JTextArea msg = new JTextArea(text);
        msg.setFont(new Font(settings.fontName, Font.PLAIN, settings.fontSize));
        msg.setForeground(Color.WHITE);
        msg.setOpaque(false);
        msg.setEditable(false);
        msg.setLineWrap(true);
        msg.setWrapStyleWord(true);
        bubble.add(msg, BorderLayout.CENTER);

        JPanel row = new JPanel(new BorderLayout());
        row.setOpaque(false);
        row.setBorder(BorderFactory.createEmptyBorder(6, 6, 6, 6));
        row.add(bubble, BorderLayout.CENTER);
        row.setMaximumSize(new Dimension(Integer.MAX_VALUE, row.getPreferredSize().height));

        chatPanel.add(row);
        chatPanel.revalidate();
        scrollToBottom();
    }

    private void addAiBubble(String text) {
        Color aiColor = settings.aiResponseColor;
        JPanel bubble = new JPanel(new BorderLayout(8, 0)) {
            @Override protected void paintComponent(Graphics g) {
                Graphics2D g2 = (Graphics2D) g.create();
                g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                g2.setColor(aiColor);
                g2.fill(new RoundRectangle2D.Float(0, 0, getWidth(), getHeight(), 16, 16));
                g2.dispose();
                super.paintComponent(g);
            }
        };
        bubble.setOpaque(false);
        bubble.setBorder(BorderFactory.createEmptyBorder(8, 8, 8, 12));

        JLabel icon = new JLabel(aiIcon);
        icon.setVerticalAlignment(SwingConstants.TOP);
        bubble.add(icon, BorderLayout.WEST);

        JTextPane msg = new JTextPane();
        msg.setOpaque(false);
        msg.setEditable(false);
        msg.setFont(new Font(settings.fontName, Font.PLAIN, settings.fontSize));
        renderStyledMessage(msg, text, settings.fontName, settings.fontSize);
        bubble.add(msg, BorderLayout.CENTER);

        JPanel row = new JPanel(new BorderLayout());
        row.setOpaque(false);
        row.setBorder(BorderFactory.createEmptyBorder(6, 6, 6, 6));
        row.add(bubble, BorderLayout.CENTER);

        pulsingAiLabel = icon;
        chatPanel.add(row);
        chatPanel.revalidate();
        scrollToBottom();
    }

    private void addCodeApprovalBubble(String explanation, String newCode) {
        // Show explanation in a normal AI bubble
        if (!explanation.isEmpty()) {
            addAiBubble(explanation);
        }

        // Add Allow/Reject buttons directly below
        JPanel btnRow = new JPanel(new FlowLayout(FlowLayout.LEFT, 8, 0));
        btnRow.setOpaque(false);
        btnRow.setBorder(BorderFactory.createEmptyBorder(2, 14, 6, 6));
        btnRow.setMaximumSize(new Dimension(Integer.MAX_VALUE, 36));
        JLabel prompt = new JLabel("Apply code changes?");
        prompt.setFont(new Font(settings.fontName, Font.BOLD, settings.fontSize));
        JButton allowBtn = new JButton("Allow");
        JButton rejectBtn = new JButton("Reject");
        allowBtn.addActionListener(e -> {
            codeEditor.setText(newCode);
            if (onCodeChanged != null) onCodeChanged.run();
            allowBtn.setEnabled(false);
            rejectBtn.setEnabled(false);
            prompt.setText("Code applied.");
        });
        rejectBtn.addActionListener(e -> {
            allowBtn.setEnabled(false);
            rejectBtn.setEnabled(false);
            prompt.setText("Changes rejected.");
        });
        btnRow.add(prompt);
        btnRow.add(allowBtn);
        btnRow.add(rejectBtn);

        chatPanel.add(btnRow);
        chatPanel.revalidate();
        scrollToBottom();
    }

    private float pulseAlpha = 0f;

    private void startPulse() {
        // Add a placeholder AI row with pulsing icon
        JPanel row = new JPanel(new BorderLayout(8, 0));
        row.setOpaque(false);
        row.setBorder(BorderFactory.createEmptyBorder(6, 6, 6, 6));
        row.setMaximumSize(new Dimension(Integer.MAX_VALUE, 44));

        pulsingAiLabel = new JLabel(aiIcon) {
            @Override protected void paintComponent(Graphics g) {
                if (pulseAlpha > 0) {
                    Graphics2D g2 = (Graphics2D) g.create();
                    g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                    int cx = getWidth() / 2, cy = getHeight() / 2, r = Math.max(getWidth(), getHeight()) / 2 + 4;
                    float[] dist = {0.3f, 1.0f};
                    Color[] colors = {new Color(50, 130, 255, (int)(pulseAlpha * 160)), new Color(50, 130, 255, 0)};
                    g2.setPaint(new RadialGradientPaint(cx, cy, r, dist, colors));
                    g2.fillOval(cx - r, cy - r, r * 2, r * 2);
                    g2.dispose();
                }
                super.paintComponent(g);
            }
        };
        pulsingAiLabel.setVerticalAlignment(SwingConstants.TOP);
        row.add(pulsingAiLabel, BorderLayout.WEST);
        JLabel thinking = new JLabel("Thinking...");
        thinking.setFont(new Font(settings.fontName, Font.ITALIC, settings.fontSize));
        thinking.setForeground(Color.GRAY);
        row.add(thinking, BorderLayout.CENTER);
        JButton cancelBtn = new JButton("✕");
        cancelBtn.setForeground(Color.RED);
        cancelBtn.setFont(cancelBtn.getFont().deriveFont(Font.BOLD, 14f));
        cancelBtn.setBorderPainted(false);
        cancelBtn.setContentAreaFilled(false);
        cancelBtn.setFocusPainted(false);
        cancelBtn.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        cancelBtn.setToolTipText("Cancel");
        cancelBtn.addActionListener(e -> {
            if (currentThread != null) currentThread.interrupt();
            stopPulse();
            sendBtn.setEnabled(true);
        });
        row.add(cancelBtn, BorderLayout.EAST);

        chatPanel.add(row);
        chatPanel.revalidate();
        scrollToBottom();

        // Pulse animation
        pulseTimer = new javax.swing.Timer(80, new ActionListener() {
            boolean increasing = true;
            @Override public void actionPerformed(ActionEvent e) {
                if (increasing) { pulseAlpha += 0.08f; if (pulseAlpha >= 1f) { pulseAlpha = 1f; increasing = false; } }
                else { pulseAlpha -= 0.08f; if (pulseAlpha <= 0f) { pulseAlpha = 0f; increasing = true; } }
                if (pulsingAiLabel != null) pulsingAiLabel.repaint();
            }
        });
        pulseTimer.start();
    }

    private void stopPulse() {
        if (pulseTimer != null) { pulseTimer.stop(); pulseTimer = null; }
        pulseAlpha = 0f;
        if (pulsingAiLabel != null) { pulsingAiLabel.repaint(); }
        // Remove the "Thinking..." placeholder (last component)
        int count = chatPanel.getComponentCount();
        if (count > 0) chatPanel.remove(count - 1);
        chatPanel.revalidate();
        chatPanel.repaint();
    }

    private void scrollToBottom() {
        SwingUtilities.invokeLater(() -> {
            JScrollBar v = chatScroll.getVerticalScrollBar();
            v.setValue(v.getMaximum());
        });
    }

    private String callLLM() throws Exception {
        String vendor = settings.llmVendor;
        String apiKey = settings.llmApiKey;
        String model = settings.llmModel;

        String baseUrl = switch (vendor) {
            case "Alibaba" -> "https://dashscope-us.aliyuncs.com/compatible-mode/v1";
            case "Anthropic" -> "https://api.anthropic.com/v1";
            case "DeepSeek" -> "https://api.deepseek.com/v1";
            case "Google" -> "https://generativelanguage.googleapis.com/v1beta/openai";
            case "Ollama" -> "http://localhost:11434/v1";
            case "OpenAI" -> "https://api.openai.com/v1";
            default -> "https://api.openai.com/v1";
        };

        if ("Anthropic".equals(vendor)) {
            return callAnthropic(apiKey, model);
        }

        StringBuilder body = new StringBuilder();
        body.append("{\"model\":\"").append(model).append("\",\"messages\":[");
        for (int i = 0; i < messages.size(); i++) {
            if (i > 0) body.append(",");
            body.append("{\"role\":\"").append(messages.get(i).get("role"))
                .append("\",\"content\":").append(jsonString(messages.get(i).get("content"))).append("}");
        }
        body.append("]}");

        HttpRequest.Builder reqBuilder = HttpRequest.newBuilder()
            .uri(URI.create(baseUrl + "/chat/completions"))
            .header("Content-Type", "application/json")
            .POST(HttpRequest.BodyPublishers.ofString(body.toString()));

        if (apiKey != null && !apiKey.isEmpty()) {
            reqBuilder.header("Authorization", "Bearer " + apiKey);
        }

        HttpResponse<String> resp = HttpClient.newHttpClient()
            .send(reqBuilder.build(), HttpResponse.BodyHandlers.ofString());

        String respBody = resp.body();
        String content = extractJsonValue(respBody, "content");
        if (content == null) throw new RuntimeException("Unexpected response: " + respBody.substring(0, Math.min(300, respBody.length())));
        messages.add(Map.of("role", "assistant", "content", content));
        return content;
    }

    private String callAnthropic(String apiKey, String model) throws Exception {
        StringBuilder body = new StringBuilder();
        body.append("{\"model\":\"").append(model).append("\",\"max_tokens\":4096,");
        String sys = messages.stream().filter(m -> "system".equals(m.get("role"))).map(m -> m.get("content")).findFirst().orElse("");
        body.append("\"system\":").append(jsonString(sys)).append(",\"messages\":[");
        boolean first = true;
        for (var m : messages) {
            if ("system".equals(m.get("role"))) continue;
            if (!first) body.append(",");
            body.append("{\"role\":\"").append(m.get("role"))
                .append("\",\"content\":").append(jsonString(m.get("content"))).append("}");
            first = false;
        }
        body.append("]}");

        HttpRequest req = HttpRequest.newBuilder()
            .uri(URI.create("https://api.anthropic.com/v1/messages"))
            .header("Content-Type", "application/json")
            .header("x-api-key", apiKey)
            .header("anthropic-version", "2023-06-01")
            .POST(HttpRequest.BodyPublishers.ofString(body.toString()))
            .build();

        HttpResponse<String> resp = HttpClient.newHttpClient().send(req, HttpResponse.BodyHandlers.ofString());
        String respBody = resp.body();
        String content = extractJsonValue(respBody, "text");
        if (content == null) throw new RuntimeException("Unexpected response: " + respBody.substring(0, Math.min(300, respBody.length())));
        messages.add(Map.of("role", "assistant", "content", content));
        return content;
    }

    private void processResponse(String response) {
        int codeStart = response.indexOf("```asm\n");
        if (codeStart < 0) codeStart = response.indexOf("```assembly\n");
        if (codeStart < 0) codeStart = response.indexOf("```\n");

        if (codeStart >= 0) {
            int blockStart = response.indexOf("\n", codeStart) + 1;
            int blockEnd = response.indexOf("\n```", blockStart);
            if (blockEnd > blockStart) {
                String newCode = response.substring(blockStart, blockEnd);
                String explanation = response.substring(0, codeStart).trim();
                if (blockEnd + 4 < response.length()) {
                    String after = response.substring(blockEnd + 4).trim();
                    if (!after.isEmpty()) explanation += (explanation.isEmpty() ? "" : "\n") + after;
                }
                addCodeApprovalBubble(explanation, newCode);
                return;
            }
        }
        addAiBubble(response);
    }

    private String buildSystemPrompt() {
        StringBuilder sb = new StringBuilder();
        sb.append("You are an AI assistant embedded in the CPUSim64 IDE. ");
        sb.append("You help users write and debug CPUSim64 assembly programs. ");
        sb.append("When you want to provide code changes, wrap the COMPLETE modified source in a ```asm code block. ");
        sb.append("You have access to the user's current source code and console output.\n\n");
        sb.append("=== CPUSim64 Documentation ===\n");
        try {
            for (String name : readIndex("/documentation/doc-index.txt")) {
                String content = readResource("/documentation/" + name);
                if (content == null) continue;
                String text = content.replaceAll("<[^>]+>", " ").replaceAll("\\s+", " ").trim();
                if (text.length() > 100000) text = text.substring(0, 100000);
                sb.append("\n--- ").append(name).append(" ---\n").append(text).append("\n");
            }
            for (String name : readIndex("/documentation/examples-index.txt")) {
                String content = readResource("/documentation/" + name);
                if (content == null) continue;
                if (content.length() > 10000) content = content.substring(0, 10000);
                sb.append("\n--- Example: ").append(name.substring(name.lastIndexOf('/') + 1)).append(" ---\n").append(content).append("\n");
            }
            for (String name : readIndex("/documentation/projects-index.txt")) {
                String content = readResource("/documentation/" + name);
                if (content == null) continue;
                if (content.length() > 10000) content = content.substring(0, 10000);
                sb.append("\n--- Project: ").append(name.substring(name.lastIndexOf('/') + 1)).append(" ---\n").append(content).append("\n");
            }
        } catch (Exception ignored) {}
        return sb.toString();
    }

    private List<String> readIndex(String path) {
        try (var in = getClass().getResourceAsStream(path)) {
            if (in == null) return List.of();
            return new BufferedReader(new InputStreamReader(in)).lines()
                    .filter(l -> !l.isBlank()).collect(Collectors.toList());
        } catch (Exception e) { return List.of(); }
    }

    private String readResource(String path) {
        try (var in = getClass().getResourceAsStream(path)) {
            if (in == null) return null;
            return new String(in.readAllBytes(), java.nio.charset.StandardCharsets.UTF_8);
        } catch (Exception e) { return null; }
    }

    private static void renderStyledMessage(JTextPane pane, String text, String fontName, int fontSize) {
        StyledDocument doc = pane.getStyledDocument();
        // Define styles
        javax.swing.text.Style normal = doc.addStyle("normal", null);
        StyleConstants.setFontFamily(normal, fontName);
        StyleConstants.setFontSize(normal, fontSize);

        javax.swing.text.Style bold = doc.addStyle("bold", normal);
        StyleConstants.setBold(bold, true);

        javax.swing.text.Style italic = doc.addStyle("italic", normal);
        StyleConstants.setItalic(italic, true);

        javax.swing.text.Style code = doc.addStyle("code", normal);
        StyleConstants.setFontFamily(code, Font.MONOSPACED);
        StyleConstants.setBackground(code, new Color(0xe8, 0xe8, 0xe8));

        javax.swing.text.Style header = doc.addStyle("header", normal);
        StyleConstants.setBold(header, true);
        StyleConstants.setFontSize(header, fontSize + 4);

        boolean inCodeBlock = false;
        String[] lines = text.split("\n");
        for (int li = 0; li < lines.length; li++) {
            String line = lines[li];
            if (line.startsWith("```")) {
                inCodeBlock = !inCodeBlock;
                continue;
            }
            if (inCodeBlock) {
                try { doc.insertString(doc.getLength(), line + "\n", code); } catch (Exception ignored) {}
                continue;
            }
            // Headers
            if (line.startsWith("### ")) { insertLine(doc, line.substring(4) + "\n", header); continue; }
            if (line.startsWith("## ")) { insertLine(doc, line.substring(3) + "\n", header); continue; }
            if (line.startsWith("# ")) { insertLine(doc, line.substring(2) + "\n", header); continue; }
            // List items
            String content = line;
            if (line.startsWith("- ") || line.startsWith("* ")) content = "\u2022 " + line.substring(2);
            // Detect bare LaTeX (no delimiters but contains LaTeX commands)
            if (looksLikeLatex(content) && !content.contains("$")) {
                try {
                    ImageIcon icon = renderLatexToIcon(content.trim(), fontSize);
                    try { doc.insertString(doc.getLength(), " ", normal); } catch (Exception ignored) {}
                    pane.setCaretPosition(doc.getLength());
                    pane.insertIcon(icon);
                    try { doc.insertString(doc.getLength(), "\n", normal); } catch (Exception ignored) {}
                    continue;
                } catch (Exception ignored) {
                    // Fall through to normal rendering
                }
            }
            // Render inline formatting and LaTeX
            renderInline(pane, doc, content, normal, bold, italic, code, fontSize);
            try { doc.insertString(doc.getLength(), "\n", normal); } catch (Exception ignored) {}
        }
    }

    private static void renderInline(JTextPane pane, StyledDocument doc, String text,
                                     javax.swing.text.Style normal, javax.swing.text.Style bold,
                                     javax.swing.text.Style italic, javax.swing.text.Style code, int fontSize) {
        int i = 0;
        while (i < text.length()) {
            // LaTeX: $...$ or \(...\) or \[...\]
            if (text.charAt(i) == '$') {
                int end = text.indexOf('$', i + 1);
                if (end > i) {
                    String latex = text.substring(i + 1, end);
                    try {
                        ImageIcon icon = renderLatexToIcon(latex, fontSize);
                        try { doc.insertString(doc.getLength(), " ", normal); } catch (Exception ignored) {}
                        pane.setCaretPosition(doc.getLength());
                        pane.insertIcon(icon);
                        i = end + 1;
                        continue;
                    } catch (Exception ex) {
                        // Fall through to render as code
                        try { doc.insertString(doc.getLength(), latex, code); } catch (Exception ignored) {}
                        i = end + 1;
                        continue;
                    }
                }
            }
            if (i + 1 < text.length() && text.charAt(i) == '\\' && (text.charAt(i + 1) == '(' || text.charAt(i + 1) == '[')) {
                char open = text.charAt(i + 1);
                String close = open == '(' ? "\\)" : "\\]";
                int end = text.indexOf(close, i + 2);
                if (end > i) {
                    String latex = text.substring(i + 2, end);
                    try {
                        ImageIcon icon = renderLatexToIcon(latex, fontSize);
                        try { doc.insertString(doc.getLength(), " ", normal); } catch (Exception ignored) {}
                        pane.setCaretPosition(doc.getLength());
                        pane.insertIcon(icon);
                        i = end + close.length();
                        continue;
                    } catch (Exception ex) {
                        try { doc.insertString(doc.getLength(), latex, code); } catch (Exception ignored) {}
                        i = end + close.length();
                        continue;
                    }
                }
            }
            // Inline code: `...`
            if (text.charAt(i) == '`') {
                int end = text.indexOf('`', i + 1);
                if (end > i) {
                    try { doc.insertString(doc.getLength(), text.substring(i + 1, end), code); } catch (Exception ignored) {}
                    i = end + 1;
                    continue;
                }
            }
            // Bold: **...**
            if (i + 1 < text.length() && text.charAt(i) == '*' && text.charAt(i + 1) == '*') {
                int end = text.indexOf("**", i + 2);
                if (end > i) {
                    try { doc.insertString(doc.getLength(), text.substring(i + 2, end), bold); } catch (Exception ignored) {}
                    i = end + 2;
                    continue;
                }
            }
            // Italic: *...*
            if (text.charAt(i) == '*') {
                int end = text.indexOf('*', i + 1);
                if (end > i && !(i + 1 < text.length() && text.charAt(i + 1) == '*')) {
                    try { doc.insertString(doc.getLength(), text.substring(i + 1, end), italic); } catch (Exception ignored) {}
                    i = end + 1;
                    continue;
                }
            }
            // Plain text - collect until next special char
            // Check if this is a LaTeX command starting with backslash
            if (text.charAt(i) == '\\' && i + 1 < text.length() && Character.isLetter(text.charAt(i + 1))) {
                // Try to find extent of LaTeX expression (ends at end of line or double newline)
                // Heuristic: grab until end of balanced braces or end of line
                int latexEnd = findLatexEnd(text, i);
                String latex = text.substring(i, latexEnd);
                try {
                    ImageIcon icon = renderLatexToIcon(latex, fontSize);
                    try { doc.insertString(doc.getLength(), " ", normal); } catch (Exception ignored) {}
                    pane.setCaretPosition(doc.getLength());
                    pane.insertIcon(icon);
                    i = latexEnd;
                    continue;
                } catch (Exception ex) {
                    // Not valid LaTeX, output as plain text
                }
            }
            int next = text.length();
            for (int j = i + 1; j < text.length(); j++) {
                char c = text.charAt(j);
                if (c == '$' || c == '`' || c == '*' || (c == '\\' && j + 1 < text.length() && Character.isLetter(text.charAt(j + 1)))) { next = j; break; }
            }
            try { doc.insertString(doc.getLength(), text.substring(i, next), normal); } catch (Exception ignored) {}
            i = next;
        }
    }

    private static void insertLine(StyledDocument doc, String text, javax.swing.text.Style style) {
        try { doc.insertString(doc.getLength(), text, style); } catch (Exception ignored) {}
    }

    private static boolean looksLikeLatex(String text) {
        String t = text.trim();
        return t.contains("\\frac") || t.contains("\\sqrt") || t.contains("\\sum") ||
               t.contains("\\int") || t.contains("\\lim") || t.contains("\\prod") ||
               (t.contains("\\") && (t.contains("^") || t.contains("_") || t.contains("{")));
    }

    private static int findLatexEnd(String text, int start) {
        // Greedily consume LaTeX: track brace depth, include everything that looks like math
        int depth = 0;
        int lastGood = start;
        int i = start;
        while (i < text.length()) {
            char c = text.charAt(i);
            if (c == '{') { depth++; i++; continue; }
            if (c == '}') { depth--; i++; if (depth <= 0) lastGood = i; continue; }
            if (depth > 0) { i++; continue; } // inside braces, consume everything
            // At depth 0, decide if we should continue
            if (c == '\\' && i + 1 < text.length() && Character.isLetter(text.charAt(i + 1))) {
                // Another LaTeX command, keep going
                i++;
                while (i < text.length() && Character.isLetter(text.charAt(i))) i++;
                lastGood = i;
                continue;
            }
            if (c == '^' || c == '_' || c == '=' || c == '+' || c == '-' || c == '(' || c == ')' ||
                c == ' ' || c == ',' || Character.isDigit(c) || c == '.') {
                i++;
                lastGood = i;
                continue;
            }
            // Anything else (regular letter word) — stop unless it's a single char variable
            if (Character.isLetter(c) && (i + 1 >= text.length() || !Character.isLetter(text.charAt(i + 1)))) {
                i++;
                lastGood = i;
                continue;
            }
            break;
        }
        return lastGood > start ? lastGood : text.length();
    }

    private static String markdownToHtml(String md, String fontName, int fontSize, List<ImageIcon> mathIcons) {
        StringBuilder html = new StringBuilder();
        html.append("<html><body style='font-family:").append(fontName)
            .append(";font-size:").append(fontSize).append("px;margin:0;padding:0;'>");
        boolean inCode = false;
        for (String line : md.split("\n")) {
            if (line.startsWith("```")) {
                if (inCode) { html.append("</pre>"); inCode = false; }
                else { html.append("<pre style='background:#f0f0f0;padding:4px;font-size:").append(fontSize - 1).append("px;'>"); inCode = true; }
                continue;
            }
            if (inCode) { html.append(esc(line)).append("\n"); continue; }
            if (line.startsWith("### ")) { html.append("<h4>").append(esc(line.substring(4))).append("</h4>"); continue; }
            if (line.startsWith("## ")) { html.append("<h3>").append(esc(line.substring(3))).append("</h3>"); continue; }
            if (line.startsWith("# ")) { html.append("<h2>").append(esc(line.substring(2))).append("</h2>"); continue; }
            if (line.startsWith("- ") || line.startsWith("* ")) { html.append("&bull; ").append(inlineFormat(line.substring(2), mathIcons, fontSize)).append("<br>"); continue; }
            if (line.matches("^\\d+\\.\\s.*")) { html.append(inlineFormat(line, mathIcons, fontSize)).append("<br>"); continue; }
            if (line.trim().isEmpty()) { html.append("<br>"); continue; }
            html.append(inlineFormat(line, mathIcons, fontSize)).append("<br>");
        }
        if (inCode) html.append("</pre>");
        html.append("</body></html>");
        return html.toString();
    }

    private static String inlineFormat(String text, List<ImageIcon> mathIcons, int fontSize) {
        String s = esc(text);
        // LaTeX math: $...$  — replace with placeholder snowman character
        s = renderLatexInline(s, mathIcons, fontSize);
        // Code spans
        s = s.replaceAll("`([^`]+)`", "<code style='background:#e8e8e8;'>$1</code>");
        // Bold
        s = s.replaceAll("\\*\\*(.+?)\\*\\*", "<b>$1</b>");
        s = s.replaceAll("__(.+?)__", "<b>$1</b>");
        // Italic
        s = s.replaceAll("\\*(.+?)\\*", "<i>$1</i>");
        s = s.replaceAll("_(.+?)_", "<i>$1</i>");
        return s;
    }

    private static String renderLatexInline(String html, List<ImageIcon> mathIcons, int fontSize) {
        StringBuilder result = new StringBuilder();
        int i = 0;
        while (i < html.length()) {
            int start = html.indexOf("$", i);
            if (start < 0) { result.append(html.substring(i)); break; }
            int end = html.indexOf("$", start + 1);
            if (end < 0) { result.append(html.substring(i)); break; }
            result.append(html, i, start);
            String latex = html.substring(start + 1, end)
                .replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">");
            try {
                ImageIcon icon = renderLatexToIcon(latex, fontSize);
                mathIcons.add(icon);
                result.append("\u2603"); // snowman placeholder
            } catch (Exception ex) {
                result.append("<code>").append(esc(latex)).append("</code>");
            }
            i = end + 1;
        }
        return result.toString();
    }

    private static ImageIcon renderLatexToIcon(String latex, int fontSize) {
        org.scilab.forge.jlatexmath.TeXFormula formula = new org.scilab.forge.jlatexmath.TeXFormula(latex);
        org.scilab.forge.jlatexmath.TeXIcon texIcon = formula.createTeXIcon(org.scilab.forge.jlatexmath.TeXConstants.STYLE_DISPLAY, fontSize);
        java.awt.image.BufferedImage img = new java.awt.image.BufferedImage(
            Math.max(texIcon.getIconWidth(), 1), Math.max(texIcon.getIconHeight(), 1),
            java.awt.image.BufferedImage.TYPE_INT_ARGB);
        Graphics2D g2 = img.createGraphics();
        g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        texIcon.paintIcon(null, g2, 0, 0);
        g2.dispose();
        return new ImageIcon(img);
    }
    private static String esc(String s) {
        return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
    }

    private static String jsonString(String s) {
        return "\"" + s.replace("\\", "\\\\").replace("\"", "\\\"")
            .replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t") + "\"";
    }

    private static String extractJsonValue(String json, String key) {
        String pattern = "\"" + key + "\"";
        int idx = json.lastIndexOf(pattern);
        if (idx < 0) return null;
        int colonIdx = json.indexOf(':', idx + pattern.length());
        if (colonIdx < 0) return null;
        int i = colonIdx + 1;
        while (i < json.length() && Character.isWhitespace(json.charAt(i))) i++;
        if (i >= json.length() || json.charAt(i) != '"') return null;
        i++;
        StringBuilder sb = new StringBuilder();
        while (i < json.length()) {
            char c = json.charAt(i);
            if (c == '\\' && i + 1 < json.length()) {
                char next = json.charAt(i + 1);
                switch (next) {
                    case 'n': sb.append('\n'); break;
                    case 'r': sb.append('\r'); break;
                    case 't': sb.append('\t'); break;
                    case '"': sb.append('"'); break;
                    case '\\': sb.append('\\'); break;
                    case '/': sb.append('/'); break;
                    case 'u':
                        if (i + 5 < json.length()) {
                            sb.append((char) Integer.parseInt(json.substring(i + 2, i + 6), 16));
                            i += 4;
                        }
                        break;
                    default: sb.append('\\').append(next); break;
                }
                i += 2;
            } else if (c == '"') {
                return sb.toString();
            } else {
                sb.append(c);
                i++;
            }
        }
        return sb.toString();
    }
}
