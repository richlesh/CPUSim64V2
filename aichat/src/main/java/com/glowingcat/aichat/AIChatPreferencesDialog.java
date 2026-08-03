/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import java.awt.*;
import java.net.URI;
import java.net.http.*;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.*;

/**
 * Standalone dialog for AI Chat preferences. Saves settings to
 * ~/.glowingcat-ai-settings.json via AIChatPreferences.
 */
public class AIChatPreferencesDialog extends JDialog {

    private final JComboBox<String> llmVendorCombo;
    private final JComboBox<String> llmModelCombo;
    private final JPasswordField llmApiKeyField;
    private final JTextField llmEndpointField;
    private final JComboBox<String> aiFontCombo;
    private final JComboBox<Integer> aiFontSizeCombo;
    private final JComboBox<String> aiCodeFontCombo;
    private final JComboBox<Integer> aiCodeFontSizeCombo;
    private final Color[] userPromptColor;
    private final Color[] userTextColor;
    private final Color[] aiResponseColor;
    private final Color[] aiTextColor;
    private boolean confirmed = false;
    private final Runnable[] fetchModelsHolder = new Runnable[1];

    private static final Integer[] FONT_SIZES = {8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 28, 32, 36};

    private static final List<VendorRegistry.VendorInfo> VENDORS = VendorRegistry.getVendors();

    public AIChatPreferencesDialog(Window owner, AIChatPreferences prefs) {
        this(owner, prefs, null);
    }

    public AIChatPreferencesDialog(Window owner, AIChatPreferences prefs, ChatColors colors) {
        super(owner, "AI Settings", ModalityType.APPLICATION_MODAL);

        String[] fontFamilies = GraphicsEnvironment.getLocalGraphicsEnvironment()
                .getAvailableFontFamilyNames();

        // Initialize LLM combos
        String[] vendorNames = VendorRegistry.getVendorNames();
        llmVendorCombo = new JComboBox<>(vendorNames);
        if (prefs.getLlmVendor() != null) llmVendorCombo.setSelectedItem(prefs.getLlmVendor());

        llmModelCombo = new JComboBox<>();
        llmModelCombo.setEditable(true);
        llmModelCombo.setMinimumSize(new Dimension(200, llmModelCombo.getPreferredSize().height));

        llmApiKeyField = new JPasswordField(prefs.getLlmApiKey() != null ? prefs.getLlmApiKey() : "", 20);

        llmEndpointField = new JTextField(prefs.getLlmEndpoint() != null ? prefs.getLlmEndpoint() : "", 20);

        aiFontCombo = new JComboBox<>(fontFamilies);
        aiFontCombo.setSelectedItem(prefs.getAiFontFamily());
        aiFontSizeCombo = new JComboBox<>(FONT_SIZES);
        aiFontSizeCombo.setSelectedItem(prefs.getAiFontSize());

        // Monospaced font combo for code
        String[] monoFonts = java.util.Arrays.stream(fontFamilies)
            .filter(name -> {
                Font f = new Font(name, Font.PLAIN, 13);
                FontMetrics fm = new JLabel().getFontMetrics(f);
                return fm.charWidth('i') == fm.charWidth('m');
            })
            .toArray(String[]::new);
        aiCodeFontCombo = new JComboBox<>(monoFonts);
        aiCodeFontCombo.setSelectedItem(prefs.getAiCodeFontFamily());
        aiCodeFontSizeCombo = new JComboBox<>(FONT_SIZES);
        aiCodeFontSizeCombo.setSelectedItem(prefs.getAiCodeFontSize());

        // Use ChatColors if provided, otherwise fall back to AIChatPreferences
        if (colors != null) {
            userPromptColor = new Color[]{Color.decode(colors.getUserPromptColor())};
            userTextColor = new Color[]{Color.decode(colors.getUserTextColor())};
            aiResponseColor = new Color[]{Color.decode(colors.getAiResponseColor())};
            aiTextColor = new Color[]{Color.decode(colors.getAiTextColor())};
        } else {
            userPromptColor = new Color[]{prefs.getUserPromptColorObj()};
            userTextColor = new Color[]{prefs.getUserTextColorObj()};
            aiResponseColor = new Color[]{prefs.getAiResponseColorObj()};
            aiTextColor = new Color[]{prefs.getAiTextColorObj()};
        }

        // Build the panel
        JPanel mainPanel = buildPanel(prefs);
        mainPanel.setBorder(BorderFactory.createEmptyBorder(12, 12, 12, 12));
        add(mainPanel, BorderLayout.CENTER);

        // Buttons
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT));
        JButton okButton = new JButton("OK");
        JButton cancelButton = new JButton("Cancel");
        okButton.addActionListener(e -> { confirmed = true; dispose(); });
        cancelButton.addActionListener(e -> dispose());
        buttonPanel.add(okButton);
        buttonPanel.add(cancelButton);
        add(buttonPanel, BorderLayout.SOUTH);

        // Wire up model fetching
        Runnable fetchModels = () -> {
            int vi = llmVendorCombo.getSelectedIndex();
            String apiKey = new String(llmApiKeyField.getPassword()).trim();

            // Handle Generic (YAML-configured) vendor separately
            if ("Generic".equals(VENDORS.get(vi).name())) {
                llmModelCombo.removeAllItems();
                new Thread(() -> {
                    try {
                        GenericVendorConfig config = new GenericVendorConfig();
                        List<String> models = config.fetchModels(apiKey);
                        SwingUtilities.invokeLater(() -> {
                            llmModelCombo.removeAllItems();
                            for (String mod : models) llmModelCombo.addItem(mod);
                            if (prefs.getLlmModel() != null) llmModelCombo.setSelectedItem(prefs.getLlmModel());
                        });
                    } catch (Exception ex) {
                        // leave model combo empty on failure
                    }
                }).start();
                return;
            }

            String resolvedUrl = VENDORS.get(vi).baseUrl();
            if ("Generic OpenAI API".equals(VENDORS.get(vi).name())) {
                resolvedUrl = llmEndpointField.getText().trim();
                if (resolvedUrl.isEmpty()) {
                    llmModelCombo.removeAllItems();
                    return;
                }
                if (resolvedUrl.endsWith("/")) resolvedUrl = resolvedUrl.substring(0, resolvedUrl.length() - 1);
            }
            final String baseUrl = resolvedUrl;
            llmModelCombo.removeAllItems();
            if (apiKey.isEmpty() && !"Ollama".equals(VENDORS.get(vi).name()) && !"Generic OpenAI API".equals(VENDORS.get(vi).name())) {
                return;
            }
            new Thread(() -> {
                try {
                    String modelsUrl = "Perplexity".equals(VENDORS.get(vi).name())
                        ? baseUrl + "/v1/models" : baseUrl + "/models";
                    HttpRequest.Builder reqBuilder = HttpRequest.newBuilder()
                        .uri(URI.create(modelsUrl))
                        .header("Content-Type", "application/json")
                        .GET();
                    if ("Anthropic".equals(VENDORS.get(vi).name())) {
                        reqBuilder.header("x-api-key", apiKey);
                        reqBuilder.header("anthropic-version", "2023-06-01");
                    } else if (!apiKey.isEmpty()) {
                        reqBuilder.header("Authorization", "Bearer " + apiKey);
                    }
                    HttpResponse<String> resp = HttpClient.newHttpClient()
                        .send(reqBuilder.build(), HttpResponse.BodyHandlers.ofString());
                    String body = resp.body();
                    List<String> models = new ArrayList<>();
                    Matcher m = Pattern.compile("\"id\"\\s*:\\s*\"([^\"]+)\"").matcher(body);
                    while (m.find()) {
                        String id = m.group(1);
                        if ("Perplexity".equals(VENDORS.get(vi).name()) && id.contains("/")) {
                            id = id.substring(id.indexOf('/') + 1);
                        }
                        models.add(id);
                    }
                    SwingUtilities.invokeLater(() -> {
                        llmModelCombo.removeAllItems();
                        for (String mod : models) llmModelCombo.addItem(mod);
                        if (prefs.getLlmModel() != null) llmModelCombo.setSelectedItem(prefs.getLlmModel());
                    });
                } catch (Exception ex) {
                    // leave model combo empty on failure
                }
            }).start();
        };
        fetchModelsHolder[0] = fetchModels;
        if (prefs.getLlmModel() != null) llmModelCombo.addItem(prefs.getLlmModel());
        fetchModels.run();
        llmVendorCombo.addActionListener(e -> {
            llmApiKeyField.setText("");
            llmModelCombo.removeAllItems();
            fetchModels.run();
        });
        llmApiKeyField.getDocument().addDocumentListener(new DocumentListener() {
            private final Timer debounce = new Timer(500, e -> {
                llmModelCombo.removeAllItems();
                fetchModels.run();
            });
            { debounce.setRepeats(false); }
            public void insertUpdate(DocumentEvent e) { debounce.restart(); }
            public void removeUpdate(DocumentEvent e) { debounce.restart(); }
            public void changedUpdate(DocumentEvent e) { debounce.restart(); }
        });
        llmEndpointField.getDocument().addDocumentListener(new DocumentListener() {
            private final Timer debounce = new Timer(500, e -> {
                llmModelCombo.removeAllItems();
                fetchModels.run();
            });
            { debounce.setRepeats(false); }
            public void insertUpdate(DocumentEvent e) { debounce.restart(); }
            public void removeUpdate(DocumentEvent e) { debounce.restart(); }
            public void changedUpdate(DocumentEvent e) { debounce.restart(); }
        });

        getRootPane().setDefaultButton(okButton);
        pack();
        setLocationRelativeTo(owner);
        setResizable(false);
    }

    private JPanel buildPanel(AIChatPreferences prefs) {
        JPanel panel = new JPanel(new GridBagLayout());
        panel.setBorder(BorderFactory.createTitledBorder("AI Chat"));
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(4, 6, 4, 6);
        gbc.anchor = GridBagConstraints.WEST;
        int row = 0;

        // LLM Connection
        gbc.gridx = 0; gbc.gridy = row; gbc.gridwidth = 2;
        JLabel llmHeader = new JLabel("LLM Connection");
        llmHeader.setFont(llmHeader.getFont().deriveFont(Font.BOLD));
        panel.add(llmHeader, gbc);

        gbc.gridwidth = 1;
        gbc.gridy = ++row; gbc.gridx = 0; gbc.fill = GridBagConstraints.NONE;
        panel.add(new JLabel("Vendor:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL; gbc.weightx = 1;
        panel.add(llmVendorCombo, gbc);
        gbc.weightx = 0;

        gbc.gridy = ++row; gbc.gridx = 0; gbc.fill = GridBagConstraints.NONE;
        panel.add(new JLabel("Model:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(llmModelCombo, gbc);

        gbc.gridy = ++row; gbc.gridx = 0; gbc.fill = GridBagConstraints.NONE;
        panel.add(new JLabel("API Key:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(llmApiKeyField, gbc);

        JLabel endpointLabel = new JLabel("Endpoint:");
        JLabel apiKeyLink = new JLabel("<html><nobr><a href=''>Get API key...</a></nobr></html>");
        JLabel configureLink = new JLabel("<html><nobr><a href=''>Configure...</a></nobr></html>");

        gbc.gridy = ++row; gbc.gridx = 0; gbc.fill = GridBagConstraints.NONE;
        panel.add(endpointLabel, gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(llmEndpointField, gbc);

        // Show/hide endpoint based on vendor selection
        boolean isGeneric = "Generic OpenAI API".equals(llmVendorCombo.getSelectedItem());
        boolean isGenericYaml = "Generic".equals(llmVendorCombo.getSelectedItem());
        endpointLabel.setVisible(isGeneric);
        llmEndpointField.setVisible(isGeneric);
        apiKeyLink.setVisible(!isGeneric && !isGenericYaml);
        configureLink.setVisible(isGenericYaml);

        gbc.gridy = ++row; gbc.gridx = 1; gbc.fill = GridBagConstraints.NONE;
        apiKeyLink.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        apiKeyLink.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                int vi = llmVendorCombo.getSelectedIndex();
                String url = VENDORS.get(vi).apiKeyUrl();
                if (!url.isEmpty()) {
                    try { Desktop.getDesktop().browse(URI.create(url)); }
                    catch (Exception ignored) {}
                }
            }
        });
        panel.add(apiKeyLink, gbc);

        configureLink.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        configureLink.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                GenericVendorDialog dlg = new GenericVendorDialog(AIChatPreferencesDialog.this);
                dlg.setVisible(true);
                if (dlg.isConfirmed() && fetchModelsHolder[0] != null) {
                    fetchModelsHolder[0].run();
                }
            }
        });
        panel.add(configureLink, gbc);

        // Update endpoint/link visibility when vendor changes
        llmVendorCombo.addActionListener(e -> {
            boolean generic = "Generic OpenAI API".equals(llmVendorCombo.getSelectedItem());
            boolean genericYaml = "Generic".equals(llmVendorCombo.getSelectedItem());
            endpointLabel.setVisible(generic);
            llmEndpointField.setVisible(generic);
            apiKeyLink.setVisible(!generic && !genericYaml);
            configureLink.setVisible(genericYaml);
        });

        // Separator
        gbc.gridy = ++row; gbc.gridx = 0; gbc.gridwidth = 2; gbc.fill = GridBagConstraints.HORIZONTAL;
        gbc.insets = new Insets(10, 6, 10, 6);
        panel.add(new JSeparator(), gbc);
        gbc.insets = new Insets(4, 6, 4, 6);

        // Appearance
        gbc.gridy = ++row; gbc.gridx = 0; gbc.gridwidth = 2;
        JLabel appearHeader = new JLabel("Appearance");
        appearHeader.setFont(appearHeader.getFont().deriveFont(Font.BOLD));
        panel.add(appearHeader, gbc);

        gbc.gridwidth = 1;
        gbc.gridy = ++row; gbc.gridx = 0; gbc.fill = GridBagConstraints.NONE;
        panel.add(new JLabel("Font:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(aiFontCombo, gbc);

        gbc.gridy = ++row; gbc.gridx = 0; gbc.fill = GridBagConstraints.NONE;
        panel.add(new JLabel("Size:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(aiFontSizeCombo, gbc);

        gbc.gridy = ++row; gbc.gridx = 0; gbc.fill = GridBagConstraints.NONE;
        panel.add(new JLabel("Code Font:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(aiCodeFontCombo, gbc);

        gbc.gridy = ++row; gbc.gridx = 0; gbc.fill = GridBagConstraints.NONE;
        panel.add(new JLabel("Code Size:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(aiCodeFontSizeCombo, gbc);

        gbc.gridy = ++row; gbc.gridx = 0; gbc.fill = GridBagConstraints.NONE;
        panel.add(new JLabel("User Color:"), gbc);
        JPanel userColorPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        userColorPanel.setOpaque(false);
        JPanel userSwatch = new JPanel();
        userSwatch.setBackground(userPromptColor[0]);
        userSwatch.setPreferredSize(new Dimension(60, 24));
        userSwatch.setMinimumSize(new Dimension(60, 24));
        userSwatch.setBorder(BorderFactory.createLineBorder(Color.DARK_GRAY));
        userSwatch.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        userSwatch.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                Color c = JColorChooser.showDialog(AIChatPreferencesDialog.this, "User Prompt Color", userPromptColor[0]);
                if (c != null) { userPromptColor[0] = c; userSwatch.setBackground(c); }
            }
        });
        userColorPanel.add(userSwatch);
        userColorPanel.add(new JLabel("Text:"));
        JPanel userTextSwatch = new JPanel();
        userTextSwatch.setBackground(userTextColor[0]);
        userTextSwatch.setPreferredSize(new Dimension(60, 24));
        userTextSwatch.setMinimumSize(new Dimension(60, 24));
        userTextSwatch.setBorder(BorderFactory.createLineBorder(Color.DARK_GRAY));
        userTextSwatch.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        userTextSwatch.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                Color c = JColorChooser.showDialog(AIChatPreferencesDialog.this, "User Text Color", userTextColor[0]);
                if (c != null) { userTextColor[0] = c; userTextSwatch.setBackground(c); }
            }
        });
        userColorPanel.add(userTextSwatch);
        Dimension userColorSize = userColorPanel.getPreferredSize();
        userColorPanel.setMinimumSize(userColorSize);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(userColorPanel, gbc);

        gbc.gridy = ++row; gbc.gridx = 0; gbc.fill = GridBagConstraints.NONE;
        panel.add(new JLabel("AI Color:"), gbc);
        JPanel aiColorPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        aiColorPanel.setOpaque(false);
        JPanel aiSwatch = new JPanel();
        aiSwatch.setBackground(aiResponseColor[0]);
        aiSwatch.setPreferredSize(new Dimension(60, 24));
        aiSwatch.setMinimumSize(new Dimension(60, 24));
        aiSwatch.setBorder(BorderFactory.createLineBorder(Color.DARK_GRAY));
        aiSwatch.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        aiSwatch.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                Color c = JColorChooser.showDialog(AIChatPreferencesDialog.this, "AI Response Color", aiResponseColor[0]);
                if (c != null) { aiResponseColor[0] = c; aiSwatch.setBackground(c); }
            }
        });
        aiColorPanel.add(aiSwatch);
        aiColorPanel.add(new JLabel("Text:"));
        JPanel aiTextSwatch = new JPanel();
        aiTextSwatch.setBackground(aiTextColor[0]);
        aiTextSwatch.setPreferredSize(new Dimension(60, 24));
        aiTextSwatch.setMinimumSize(new Dimension(60, 24));
        aiTextSwatch.setBorder(BorderFactory.createLineBorder(Color.DARK_GRAY));
        aiTextSwatch.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        aiTextSwatch.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                Color c = JColorChooser.showDialog(AIChatPreferencesDialog.this, "AI Text Color", aiTextColor[0]);
                if (c != null) { aiTextColor[0] = c; aiTextSwatch.setBackground(c); }
            }
        });
        aiColorPanel.add(aiTextSwatch);
        Dimension aiColorSize = aiColorPanel.getPreferredSize();
        aiColorPanel.setMinimumSize(aiColorSize);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(aiColorPanel, gbc);

        // Vertical glue to push content to top
        gbc.gridy = ++row; gbc.gridx = 0; gbc.gridwidth = 2; gbc.weighty = 1;
        gbc.fill = GridBagConstraints.VERTICAL;
        panel.add(Box.createVerticalGlue(), gbc);

        return panel;
    }

    public boolean isConfirmed() { return confirmed; }

    public void applyTo(AIChatPreferences prefs) {
        prefs.setLlmVendor((String) llmVendorCombo.getSelectedItem());
        Object modelItem = llmModelCombo.getSelectedItem();
        prefs.setLlmModel(modelItem != null ? modelItem.toString() : null);
        String key = new String(llmApiKeyField.getPassword()).trim();
        prefs.setLlmApiKey(key.isEmpty() ? null : key);
        String endpoint = llmEndpointField.getText().trim();
        prefs.setLlmEndpoint(endpoint.isEmpty() ? null : endpoint);
        prefs.setAiFontFamily((String) aiFontCombo.getSelectedItem());
        prefs.setAiFontSize((Integer) aiFontSizeCombo.getSelectedItem());
        prefs.setAiCodeFontFamily((String) aiCodeFontCombo.getSelectedItem());
        prefs.setAiCodeFontSize((Integer) aiCodeFontSizeCombo.getSelectedItem());
    }

    /** Get the selected user prompt bubble background color. */
    public Color getSelectedUserPromptColor() { return userPromptColor[0]; }

    /** Get the selected user prompt text color. */
    public Color getSelectedUserTextColor() { return userTextColor[0]; }

    /** Get the selected AI response bubble background color. */
    public Color getSelectedAiResponseColor() { return aiResponseColor[0]; }

    /** Get the selected AI response text color. */
    public Color getSelectedAiTextColor() { return aiTextColor[0]; }
}
