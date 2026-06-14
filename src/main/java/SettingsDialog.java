// SPDX-License-Identifier: Apache-2.0
import javax.swing.*;
import java.awt.*;
import java.util.ArrayList;
import java.util.List;

public class SettingsDialog {

    public static void show(JFrame parent, JTextPane editor, JTextArea console,
                            AsmSyntaxHighlighter highlighter, AppSettings settings) {
        JDialog dialog = new JDialog(parent, "Settings", true);
        dialog.setLayout(new BorderLayout(10, 10));

        JPanel panel = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.anchor = GridBagConstraints.WEST;

        // Font selector (monospaced only)
        gbc.gridx = 0; gbc.gridy = 0;
        panel.add(new JLabel("Font:"), gbc);

        String[] monoFonts = getMonospacedFonts();
        JComboBox<String> fontCombo = new JComboBox<>(monoFonts);
        fontCombo.setSelectedItem(editor.getFont().getFamily());
        gbc.gridx = 1;
        panel.add(fontCombo, gbc);

        // Size selector
        gbc.gridx = 0; gbc.gridy = 1;
        panel.add(new JLabel("Size:"), gbc);

        String[] sizes = {"Small (12pt)", "Medium (16pt)", "Large (20pt)"};
        JComboBox<String> sizeCombo = new JComboBox<>(sizes);
        int currentSize = editor.getFont().getSize();
        if (currentSize <= 12) sizeCombo.setSelectedIndex(0);
        else if (currentSize <= 16) sizeCombo.setSelectedIndex(1);
        else sizeCombo.setSelectedIndex(2);
        gbc.gridx = 1;
        panel.add(sizeCombo, gbc);

        // Color settings
        gbc.gridx = 0; gbc.gridy = 2;
        gbc.gridwidth = 2;
        panel.add(new JSeparator(), gbc);
        gbc.gridwidth = 1;

        gbc.gridy = 3;
        gbc.gridx = 0; gbc.gridwidth = 2;
        panel.add(new JLabel("Syntax Colors:"), gbc);
        gbc.gridwidth = 1;

        JButton[] colorButtons = new JButton[AsmSyntaxHighlighter.CATEGORY_NAMES.length];
        Color[] colors = new Color[AsmSyntaxHighlighter.CATEGORY_NAMES.length];

        for (int i = 0; i < AsmSyntaxHighlighter.CATEGORY_NAMES.length; i++) {
            gbc.gridy = 4 + i;
            gbc.gridx = 0;
            panel.add(new JLabel(AsmSyntaxHighlighter.CATEGORY_NAMES[i] + ":"), gbc);

            colors[i] = highlighter.getColor(i);
            JButton btn = new JButton("    ");
            btn.setBackground(colors[i]);
            btn.setOpaque(true);
            btn.setPreferredSize(new Dimension(60, 24));
            final int idx = i;
            btn.addActionListener(e -> {
                Color c = JColorChooser.showDialog(dialog, "Choose Color", colors[idx]);
                if (c != null) {
                    colors[idx] = c;
                    btn.setBackground(c);
                }
            });
            colorButtons[i] = btn;
            gbc.gridx = 1;
            panel.add(btn, gbc);
        }

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
            int size = switch (sizeCombo.getSelectedIndex()) {
                case 0 -> 12;
                case 1 -> 16;
                case 2 -> 20;
                default -> 14;
            };
            Font font = new Font(fontName, Font.PLAIN, size);
            editor.setFont(font);
            console.setFont(font);

            // Apply colors
            for (int i = 0; i < colors.length; i++) {
                highlighter.setColor(i, colors[i]);
            }
            highlighter.highlight();

            // Save settings
            settings.fontName = fontName;
            settings.fontSize = size;
            System.arraycopy(colors, 0, settings.colors, 0, colors.length);
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
            fm = new Canvas().getFontMetrics(font);
            if (fm.charWidth('m') == fm.charWidth('i')) {
                mono.add(name);
            }
        }
        if (mono.isEmpty()) mono.add(Font.MONOSPACED);
        return mono.toArray(new String[0]);
    }
}
