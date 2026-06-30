// SPDX-License-Identifier: Apache-2.0
import javax.swing.*;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;

public class SettingsDialog {

    public static void show(JFrame parent, JTextPane editor, TerminalPanel console,
                            AsmSyntaxHighlighter highlighter, AppSettings settings) {
        JDialog dialog = new JDialog(parent, "Settings", true);
        dialog.setLayout(new BorderLayout(10, 10));

        JPanel panel = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.anchor = GridBagConstraints.WEST;

        // Code Font selector (monospaced only)
        gbc.gridx = 0; gbc.gridy = 0;
        panel.add(new JLabel("Code Font:"), gbc);

        String[] monoFonts = getMonospacedFonts();
        JComboBox<String> fontCombo = new JComboBox<>(monoFonts);
        fontCombo.setSelectedItem(editor.getFont().getFamily());
        gbc.gridx = 1;
        panel.add(fontCombo, gbc);

        // Code Size selector
        gbc.gridx = 0; gbc.gridy = 1;
        panel.add(new JLabel("Code Size:"), gbc);

        int[] sizeValues = {9, 10, 11, 12, 13, 14, 16, 18, 20, 24, 28, 32};
        String[] sizes = new String[sizeValues.length];
        for (int i = 0; i < sizeValues.length; i++) sizes[i] = sizeValues[i] + "pt";
        JComboBox<String> sizeCombo = new JComboBox<>(sizes);
        int currentSize = editor.getFont().getSize();
        for (int i = 0; i < sizeValues.length; i++) { if (sizeValues[i] == currentSize) { sizeCombo.setSelectedIndex(i); break; } }
        gbc.gridx = 1;
        panel.add(sizeCombo, gbc);

        // AI Font selector
        gbc.gridx = 0; gbc.gridy = 2;
        panel.add(new JLabel("AI Font:"), gbc);

        String[] allFonts = GraphicsEnvironment.getLocalGraphicsEnvironment().getAvailableFontFamilyNames();
        JComboBox<String> aiFontCombo = new JComboBox<>(allFonts);
        aiFontCombo.setSelectedItem(settings.aiFontName);
        gbc.gridx = 1;
        panel.add(aiFontCombo, gbc);

        // AI Size selector
        gbc.gridx = 0; gbc.gridy = 3;
        panel.add(new JLabel("AI Size:"), gbc);

        JComboBox<String> aiSizeCombo = new JComboBox<>(sizes);
        for (int i = 0; i < sizeValues.length; i++) { if (sizeValues[i] == settings.aiFontSize) { aiSizeCombo.setSelectedIndex(i); break; } }
        gbc.gridx = 1;
        panel.add(aiSizeCombo, gbc);

        // Color settings
        gbc.gridx = 0; gbc.gridy = 4;
        gbc.gridwidth = 2;
        panel.add(new JSeparator(), gbc);
        gbc.gridwidth = 1;

        // Syntax Colors group
        JPanel colorsPanel = new JPanel(new GridBagLayout());
        colorsPanel.setBorder(BorderFactory.createTitledBorder("Syntax Colors"));
        GridBagConstraints cgbc = new GridBagConstraints();
        cgbc.insets = new Insets(3, 5, 3, 5);
        cgbc.anchor = GridBagConstraints.WEST;

        JButton[] colorButtons = new JButton[AsmSyntaxHighlighter.CATEGORY_NAMES.length];
        JTextField[] colorFields = new JTextField[AsmSyntaxHighlighter.CATEGORY_NAMES.length];
        Color[] colors = new Color[AsmSyntaxHighlighter.CATEGORY_NAMES.length];

        int colCount = 2;
        for (int i = 0; i < AsmSyntaxHighlighter.CATEGORY_NAMES.length; i++) {
            int col = i % colCount;
            int row = i / colCount;
            cgbc.gridy = row;
            cgbc.gridx = col * 2;
            cgbc.fill = GridBagConstraints.NONE;
            cgbc.weightx = 0;
            colorsPanel.add(new JLabel(AsmSyntaxHighlighter.CATEGORY_NAMES[i] + ":"), cgbc);

            colors[i] = highlighter.getColor(i);
            JPanel colorPanel = new JPanel(new BorderLayout(4, 0));
            JTextField field = new JTextField(String.format("#%02x%02x%02x", colors[i].getRed(), colors[i].getGreen(), colors[i].getBlue()), 7);
            field.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 12));
            JPanel swatch = new JPanel();
            swatch.setBackground(colors[i]);
            swatch.setPreferredSize(new Dimension(24, 24));
            swatch.setBorder(BorderFactory.createLineBorder(Color.DARK_GRAY));
            swatch.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
            colorPanel.add(field, BorderLayout.CENTER);
            colorPanel.add(swatch, BorderLayout.EAST);

            final int idx = i;
            field.getDocument().addDocumentListener(new javax.swing.event.DocumentListener() {
                public void insertUpdate(javax.swing.event.DocumentEvent e) { update(); }
                public void removeUpdate(javax.swing.event.DocumentEvent e) { update(); }
                public void changedUpdate(javax.swing.event.DocumentEvent e) {}
                private void update() {
                    try {
                        Color c = Color.decode(field.getText().trim());
                        colors[idx] = c;
                        swatch.setBackground(c);
                    } catch (NumberFormatException ignored) {}
                }
            });
            swatch.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    Color c = JColorChooser.showDialog(dialog, "Choose Color", colors[idx]);
                    if (c != null) {
                        colors[idx] = c;
                        swatch.setBackground(c);
                        field.setText(String.format("#%02x%02x%02x", c.getRed(), c.getGreen(), c.getBlue()));
                    }
                }
            });

            colorFields[i] = field;
            cgbc.gridx = col * 2 + 1;
            cgbc.fill = GridBagConstraints.HORIZONTAL;
            cgbc.weightx = 0.5;
            colorsPanel.add(colorPanel, cgbc);
        }

        gbc.gridy = 5; gbc.gridx = 0; gbc.gridwidth = 2;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(colorsPanel, gbc);
        gbc.gridwidth = 1;

        // LLM Settings group
        JPanel llmPanel = new JPanel(new GridBagLayout());
        llmPanel.setBorder(BorderFactory.createTitledBorder("LLM Settings"));
        GridBagConstraints lgbc = new GridBagConstraints();
        lgbc.insets = new Insets(4, 5, 4, 5);
        lgbc.anchor = GridBagConstraints.WEST;

        String[][] vendorData = {
            {"Alibaba", "https://www.alibabacloud.com/help/en/model-studio/get-api-key", "https://dashscope-us.aliyuncs.com/compatible-mode/v1"},
            {"Anthropic", "https://console.anthropic.com/settings/keys", "https://api.anthropic.com/v1"},
            {"DeepSeek", "https://platform.deepseek.com/api_keys", "https://api.deepseek.com/v1"},
            {"Google", "https://aistudio.google.com/apikey", "https://generativelanguage.googleapis.com/v1beta/openai"},
            {"Ollama", "https://ollama.com", "http://localhost:11434/v1"},
            {"OpenAI", "https://platform.openai.com/api-keys", "https://api.openai.com/v1"},
        };

        lgbc.gridx = 0; lgbc.gridy = 0;
        llmPanel.add(new JLabel("Vendor:"), lgbc);
        String[] vendorNames = new String[vendorData.length];
        for (int i = 0; i < vendorData.length; i++) vendorNames[i] = vendorData[i][0];
        JComboBox<String> vendorCombo = new JComboBox<>(vendorNames);
        if (settings.llmVendor != null) vendorCombo.setSelectedItem(settings.llmVendor);
        lgbc.gridx = 1; lgbc.fill = GridBagConstraints.HORIZONTAL; lgbc.weightx = 1;
        llmPanel.add(vendorCombo, lgbc);

        lgbc.gridx = 0; lgbc.gridy = 1; lgbc.fill = GridBagConstraints.NONE; lgbc.weightx = 0;
        llmPanel.add(new JLabel("Model:"), lgbc);
        JComboBox<String> modelCombo = new JComboBox<>();
        modelCombo.setEditable(true);
        lgbc.gridx = 1; lgbc.fill = GridBagConstraints.HORIZONTAL; lgbc.weightx = 1;
        llmPanel.add(modelCombo, lgbc);

        lgbc.gridx = 0; lgbc.gridy = 2; lgbc.fill = GridBagConstraints.NONE; lgbc.weightx = 0;
        llmPanel.add(new JLabel("API Key:"), lgbc);
        JPasswordField apiKeyField = new JPasswordField(settings.llmApiKey != null ? settings.llmApiKey : "", 20);
        lgbc.gridx = 1; lgbc.fill = GridBagConstraints.HORIZONTAL; lgbc.weightx = 1;
        llmPanel.add(apiKeyField, lgbc);

        JLabel apiKeyLink = new JLabel("<html><a href=''>Get API key...</a></html>");
        apiKeyLink.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        lgbc.gridx = 1; lgbc.gridy = 3;
        llmPanel.add(apiKeyLink, lgbc);

        // Fetch models from vendor API
        Runnable fetchModels = () -> {
            int vi = vendorCombo.getSelectedIndex();
            String apiKey = new String(apiKeyField.getPassword()).trim();
            String baseUrl = vendorData[vi][2];
            modelCombo.removeAllItems();
            if (apiKey.isEmpty() && !"Ollama".equals(vendorData[vi][0])) {
                if (settings.llmModel != null) modelCombo.addItem(settings.llmModel);
                return;
            }
            new Thread(() -> {
                try {
                    java.net.http.HttpRequest.Builder reqBuilder = java.net.http.HttpRequest.newBuilder()
                        .uri(java.net.URI.create(baseUrl + "/models"))
                        .header("Content-Type", "application/json")
                        .GET();
                    if ("Anthropic".equals(vendorData[vi][0])) {
                        reqBuilder.header("x-api-key", apiKey);
                        reqBuilder.header("anthropic-version", "2023-06-01");
                    } else if (!apiKey.isEmpty()) {
                        reqBuilder.header("Authorization", "Bearer " + apiKey);
                    }
                    java.net.http.HttpResponse<String> resp = java.net.http.HttpClient.newHttpClient()
                        .send(reqBuilder.build(), java.net.http.HttpResponse.BodyHandlers.ofString());
                    String body = resp.body();
                    // Parse model IDs from JSON response
                    java.util.List<String> models = new java.util.ArrayList<>();
                    java.util.regex.Matcher m = java.util.regex.Pattern.compile("\"id\"\\s*:\\s*\"([^\"]+)\"").matcher(body);
                    while (m.find()) models.add(m.group(1));
                    SwingUtilities.invokeLater(() -> {
                        modelCombo.removeAllItems();
                        for (String mod : models) modelCombo.addItem(mod);
                        if (settings.llmModel != null) modelCombo.setSelectedItem(settings.llmModel);
                    });
                } catch (Exception ex) {
                    SwingUtilities.invokeLater(() -> {
                        if (settings.llmModel != null) modelCombo.addItem(settings.llmModel);
                    });
                }
            }).start();
        };
        fetchModels.run();
        vendorCombo.addActionListener(e -> fetchModels.run());
        apiKeyField.getDocument().addDocumentListener(new javax.swing.event.DocumentListener() {
            private javax.swing.Timer debounce = new javax.swing.Timer(500, e -> fetchModels.run());
            { debounce.setRepeats(false); }
            public void insertUpdate(javax.swing.event.DocumentEvent e) { debounce.restart(); }
            public void removeUpdate(javax.swing.event.DocumentEvent e) { debounce.restart(); }
            public void changedUpdate(javax.swing.event.DocumentEvent e) { debounce.restart(); }
        });

        apiKeyLink.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                try { Desktop.getDesktop().browse(java.net.URI.create(vendorData[vendorCombo.getSelectedIndex()][1])); }
                catch (Exception ignored) {}
            }
        });

        gbc.gridy = 8; gbc.gridx = 0; gbc.gridwidth = 2;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(llmPanel, gbc);
        gbc.gridwidth = 1;

        // Chat Colors
        JPanel chatColorsPanel = new JPanel(new GridBagLayout());
        chatColorsPanel.setBorder(BorderFactory.createTitledBorder("Chat Colors"));
        GridBagConstraints ccgbc = new GridBagConstraints();
        ccgbc.insets = new Insets(4, 5, 4, 5);
        ccgbc.anchor = GridBagConstraints.WEST;

        Color[] userColor = {settings.userPromptColor};
        Color[] aiColor = {settings.aiResponseColor};

        ccgbc.gridx = 0; ccgbc.gridy = 0;
        chatColorsPanel.add(new JLabel("User Prompt:"), ccgbc);
        JPanel userSwatch = new JPanel();
        userSwatch.setBackground(userColor[0]);
        userSwatch.setPreferredSize(new Dimension(60, 24));
        userSwatch.setBorder(BorderFactory.createLineBorder(Color.DARK_GRAY));
        userSwatch.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        userSwatch.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                Color c = JColorChooser.showDialog(dialog, "User Prompt Color", userColor[0]);
                if (c != null) { userColor[0] = c; userSwatch.setBackground(c); }
            }
        });
        ccgbc.gridx = 1;
        chatColorsPanel.add(userSwatch, ccgbc);

        ccgbc.gridx = 2;
        chatColorsPanel.add(new JLabel("  AI Response:"), ccgbc);
        JPanel aiSwatch = new JPanel();
        aiSwatch.setBackground(aiColor[0]);
        aiSwatch.setPreferredSize(new Dimension(60, 24));
        aiSwatch.setBorder(BorderFactory.createLineBorder(Color.DARK_GRAY));
        aiSwatch.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        aiSwatch.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                Color c = JColorChooser.showDialog(dialog, "AI Response Color", aiColor[0]);
                if (c != null) { aiColor[0] = c; aiSwatch.setBackground(c); }
            }
        });
        ccgbc.gridx = 3;
        chatColorsPanel.add(aiSwatch, ccgbc);

        // Console Colors
        JPanel consoleColorsPanel = new JPanel(new GridBagLayout());
        consoleColorsPanel.setBorder(BorderFactory.createTitledBorder("Console Colors"));
        GridBagConstraints cngbc = new GridBagConstraints();
        cngbc.insets = new Insets(4, 5, 4, 5);
        cngbc.anchor = GridBagConstraints.WEST;

        Color[] conFg = {settings.consoleFg};
        Color[] conBg = {settings.consoleBg};

        cngbc.gridx = 0; cngbc.gridy = 0;
        consoleColorsPanel.add(new JLabel("Foreground:"), cngbc);
        JPanel conFgSwatch = new JPanel();
        conFgSwatch.setBackground(conFg[0]);
        conFgSwatch.setPreferredSize(new Dimension(60, 24));
        conFgSwatch.setBorder(BorderFactory.createLineBorder(Color.DARK_GRAY));
        conFgSwatch.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        conFgSwatch.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                Color c = JColorChooser.showDialog(dialog, "Console Foreground", conFg[0]);
                if (c != null) { conFg[0] = c; conFgSwatch.setBackground(c); }
            }
        });
        cngbc.gridx = 1;
        consoleColorsPanel.add(conFgSwatch, cngbc);

        cngbc.gridx = 2;
        consoleColorsPanel.add(new JLabel("  Background:"), cngbc);
        JPanel conBgSwatch = new JPanel();
        conBgSwatch.setBackground(conBg[0]);
        conBgSwatch.setPreferredSize(new Dimension(60, 24));
        conBgSwatch.setBorder(BorderFactory.createLineBorder(Color.DARK_GRAY));
        conBgSwatch.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        conBgSwatch.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                Color c = JColorChooser.showDialog(dialog, "Console Background", conBg[0]);
                if (c != null) { conBg[0] = c; conBgSwatch.setBackground(c); }
            }
        });
        cngbc.gridx = 3;
        consoleColorsPanel.add(conBgSwatch, cngbc);

        gbc.gridy = 6; gbc.gridx = 0; gbc.gridwidth = 2;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(consoleColorsPanel, gbc);
        gbc.gridwidth = 1;

        gbc.gridy = 7; gbc.gridx = 0; gbc.gridwidth = 2;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(chatColorsPanel, gbc);
        gbc.gridwidth = 1;

        // Buttons
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        JButton okBtn = new JButton("OK");
        JButton cancelBtn = new JButton("Cancel");
        buttonPanel.add(cancelBtn);
        buttonPanel.add(okBtn);

        cancelBtn.addActionListener(e -> dialog.dispose());
        okBtn.addActionListener(e -> {
            // Apply font
            String fontName = (String) fontCombo.getSelectedItem();
            int size = sizeValues[sizeCombo.getSelectedIndex()];
            Font font = new Font(fontName, Font.PLAIN, size);
            editor.setFont(font);
            console.setFont(fontName, size);

            // Apply colors
            for (int i = 0; i < colors.length; i++) {
                highlighter.setColor(i, colors[i]);
            }
            highlighter.highlight();

            // Save settings
            settings.fontName = fontName;
            settings.fontSize = size;
            settings.aiFontName = (String) aiFontCombo.getSelectedItem();
            settings.aiFontSize = sizeValues[aiSizeCombo.getSelectedIndex()];
            System.arraycopy(colors, 0, settings.colors, 0, colors.length);
            settings.llmVendor = (String) vendorCombo.getSelectedItem();
            settings.llmModel = (String) modelCombo.getSelectedItem();
            String key = new String(apiKeyField.getPassword()).trim();
            settings.llmApiKey = key.isEmpty() ? null : key;
            settings.userPromptColor = userColor[0];
            settings.aiResponseColor = aiColor[0];
            settings.consoleFg = conFg[0];
            settings.consoleBg = conBg[0];
            settings.save();

            dialog.dispose();
        });

        dialog.add(new JScrollPane(panel), BorderLayout.CENTER);
        dialog.add(buttonPanel, BorderLayout.SOUTH);
        dialog.pack();
        dialog.setLocationRelativeTo(parent);
        dialog.setVisible(true);
    }

    private static String[] getMonospacedFonts() {
        GraphicsEnvironment ge = GraphicsEnvironment.getLocalGraphicsEnvironment();
        String[] allFonts = ge.getAvailableFontFamilyNames();
        List<String> mono = new ArrayList<>();
        FontMetrics fm;
        for (String name : allFonts) {
            Font font = new Font(name, Font.PLAIN, 12);
            // Skip ornamental/symbol fonts
            if (!font.canDisplay('A') || !font.canDisplay('z') || !font.canDisplay('0')) continue;
            fm = new Canvas().getFontMetrics(font);
            if (fm.charWidth('m') == fm.charWidth('i') && fm.charWidth('m') > 0) {
                mono.add(name);
            }
        }
        if (mono.isEmpty()) mono.add(Font.MONOSPACED);
        return mono.toArray(new String[0]);
    }
}
