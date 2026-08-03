// SPDX-License-Identifier: Apache-2.0
package com.glowingcat.cpusim64ide;

import javax.swing.*;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;
import org.fife.ui.rsyntaxtextarea.RSyntaxTextArea;

public class SettingsDialog {

    /** Category names for syntax colors (same order as AppSettings.colors array) */
    public static final String[] CATEGORY_NAMES = {
        "Normal", "Keywords", "Directives", "Comments",
        "Strings", "Numbers", "Registers", "Labels", "Conditions"
    };

    public static void show(JFrame parent, RSyntaxTextArea editor, TerminalPanel console,
                            AppSettings settings) {
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

        int[] sizeValues = {9, 10, 11, 12, 13, 14, 16, 18, 20, 24, 28};
        String[] sizes = new String[sizeValues.length];
        for (int i = 0; i < sizeValues.length; i++) sizes[i] = sizeValues[i] + "pt";
        JComboBox<String> sizeCombo = new JComboBox<>(sizes);
        int currentSize = editor.getFont().getSize();
        for (int i = 0; i < sizeValues.length; i++) { if (sizeValues[i] == currentSize) { sizeCombo.setSelectedIndex(i); break; } }
        gbc.gridx = 1;
        panel.add(sizeCombo, gbc);

        // Separator
        gbc.gridx = 0; gbc.gridy = 2;
        gbc.gridwidth = 2;
        panel.add(new JSeparator(), gbc);
        gbc.gridwidth = 1;

        // Syntax Colors group
        JPanel colorsPanel = new JPanel(new GridBagLayout());
        colorsPanel.setBorder(BorderFactory.createTitledBorder("Syntax Colors"));
        GridBagConstraints cgbc = new GridBagConstraints();
        cgbc.insets = new Insets(3, 5, 3, 5);
        cgbc.anchor = GridBagConstraints.WEST;

        JButton[] colorButtons = new JButton[CATEGORY_NAMES.length];
        JTextField[] colorFields = new JTextField[CATEGORY_NAMES.length];
        Color[] colors = new Color[CATEGORY_NAMES.length];

        int colCount = 2;
        for (int i = 0; i < CATEGORY_NAMES.length; i++) {
            int col = i % colCount;
            int row = i / colCount;
            cgbc.gridy = row;
            cgbc.gridx = col * 2;
            cgbc.fill = GridBagConstraints.NONE;
            cgbc.weightx = 0;
            colorsPanel.add(new JLabel(CATEGORY_NAMES[i] + ":"), cgbc);

            colors[i] = settings.colors[i];
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

        gbc.gridy = 3; gbc.gridx = 0; gbc.gridwidth = 2;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(colorsPanel, gbc);
        gbc.gridwidth = 1;

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

        gbc.gridy = 4; gbc.gridx = 0; gbc.gridwidth = 2;
        gbc.fill = GridBagConstraints.HORIZONTAL;
        panel.add(consoleColorsPanel, gbc);
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

            // Save settings
            settings.fontName = fontName;
            settings.fontSize = size;
            System.arraycopy(colors, 0, settings.colors, 0, colors.length);
            settings.consoleFg = conFg[0];
            settings.consoleBg = conBg[0];
            settings.save();

            dialog.dispose();
        });

        dialog.add(new JScrollPane(panel), BorderLayout.CENTER);
        dialog.add(buttonPanel, BorderLayout.SOUTH);
        dialog.pack();
        dialog.setSize(dialog.getWidth(), dialog.getHeight() + 20);
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
