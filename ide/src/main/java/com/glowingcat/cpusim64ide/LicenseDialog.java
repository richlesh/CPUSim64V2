// SPDX-License-Identifier: Apache-2.0
package com.glowingcat.cpusim64ide;

import javax.swing.*;
import javax.swing.event.*;
import java.awt.*;
import java.nio.charset.StandardCharsets;
import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

public class LicenseDialog {

    public static void show(JFrame parent, AppSettings settings) {
        JDialog dialog = new JDialog(parent, "License Key", true);
        dialog.setLayout(new BorderLayout(10, 10));

        JPanel panel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
        panel.setBorder(BorderFactory.createEmptyBorder(20, 20, 20, 20));

        JLabel title = new JLabel("License Key");
        title.setFont(title.getFont().deriveFont(Font.BOLD, 16f));
        title.setAlignmentX(Component.CENTER_ALIGNMENT);
        panel.add(title);
        panel.add(Box.createVerticalStrut(8));

        JLabel desc = new JLabel("Enter your email address and license key");
        desc.setAlignmentX(Component.CENTER_ALIGNMENT);
        panel.add(desc);
        panel.add(Box.createVerticalStrut(12));

        JTextField emailField = new JTextField(settings.licenseEmail != null ? settings.licenseEmail : "", 24);
        emailField.setMaximumSize(new Dimension(Integer.MAX_VALUE, 30));
        emailField.setHorizontalAlignment(JTextField.CENTER);
        emailField.putClientProperty("JTextField.placeholderText", "Your email address");
        panel.add(emailField);
        panel.add(Box.createVerticalStrut(8));

        JTextField keyField = new JTextField(formatKey(settings.licenseKey != null ? settings.licenseKey : ""), 24);
        keyField.setMaximumSize(new Dimension(Integer.MAX_VALUE, 30));
        keyField.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 14));
        keyField.setHorizontalAlignment(JTextField.CENTER);
        keyField.putClientProperty("JTextField.placeholderText", "XXXX-XXXX-XXXX-XXXX");
        panel.add(keyField);
        panel.add(Box.createVerticalStrut(8));

        JLabel link = new JLabel("<html><a href=''>Donate at Glowing Cat Software to get a license key.</a></html>");
        link.setAlignmentX(Component.CENTER_ALIGNMENT);
        link.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        link.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseClicked(java.awt.event.MouseEvent e) {
                try { Desktop.getDesktop().browse(java.net.URI.create("https://glowingcat.com/CPUSim64.html")); }
                catch (Exception ignored) {}
            }
        });
        panel.add(link);
        panel.add(Box.createVerticalStrut(12));

        JPanel buttons = new JPanel(new FlowLayout(FlowLayout.CENTER, 10, 0));
        JButton cancelBtn = new JButton("Cancel");
        JButton saveBtn = new JButton("Save");
        saveBtn.setEnabled(false);
        buttons.add(cancelBtn);
        buttons.add(saveBtn);
        panel.add(buttons);

        // Auto-format key input and validate
        DocumentListener validator = new DocumentListener() {
            public void insertUpdate(DocumentEvent e) { validate(); }
            public void removeUpdate(DocumentEvent e) { validate(); }
            public void changedUpdate(DocumentEvent e) { validate(); }
            private void validate() {
                String email = emailField.getText().trim();
                String raw = keyField.getText().replace("-", "");
                boolean valid = raw.length() == 16 && email.length() > 0 &&
                    raw.equalsIgnoreCase(expectedLicenseKey(email));
                saveBtn.setEnabled(valid);
            }
        };
        emailField.getDocument().addDocumentListener(validator);
        keyField.getDocument().addDocumentListener(validator);

        keyField.getDocument().addDocumentListener(new DocumentListener() {
            private boolean updating = false;
            public void insertUpdate(DocumentEvent e) { reformat(); }
            public void removeUpdate(DocumentEvent e) { reformat(); }
            public void changedUpdate(DocumentEvent e) {}
            private void reformat() {
                if (updating) return;
                updating = true;
                SwingUtilities.invokeLater(() -> {
                    int caret = keyField.getCaretPosition();
                    String formatted = formatKey(keyField.getText());
                    if (!formatted.equals(keyField.getText())) {
                        keyField.setText(formatted);
                        keyField.setCaretPosition(Math.min(caret, formatted.length()));
                    }
                    updating = false;
                });
            }
        });

        cancelBtn.addActionListener(e -> dialog.dispose());
        saveBtn.addActionListener(e -> {
            settings.licenseEmail = emailField.getText().trim();
            settings.licenseKey = keyField.getText().replace("-", "").toUpperCase();
            settings.save();
            Icon appIcon = null;
            var url = LicenseDialog.class.getResource("/app_icon_256.png");
            if (url != null) appIcon = new ImageIcon(new ImageIcon(url).getImage().getScaledInstance(64, 64, Image.SCALE_SMOOTH));
            JOptionPane.showMessageDialog(dialog, "License saved. Thank you!",
                "CPUSim64", JOptionPane.INFORMATION_MESSAGE, appIcon);
            dialog.dispose();
        });

        dialog.add(panel);
        dialog.pack();
        dialog.setResizable(false);
        dialog.setLocationRelativeTo(parent);
        dialog.setVisible(true);
    }

    private static String formatKey(String raw) {
        String clean = raw.replaceAll("[^a-zA-Z0-9]", "").toUpperCase();
        if (clean.length() > 16) clean = clean.substring(0, 16);
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < clean.length(); i++) {
            if (i > 0 && i % 4 == 0) sb.append('-');
            sb.append(clean.charAt(i));
        }
        return sb.toString();
    }

    public static String expectedLicenseKey(String email) {
        try {
            Mac mac = Mac.getInstance("HmacSHA256");
            mac.init(new SecretKeySpec(License.LICENSE_SALT.getBytes(StandardCharsets.UTF_8), "HmacSHA256"));
            byte[] hash = mac.doFinal(email.toLowerCase().trim().getBytes(StandardCharsets.UTF_8));
            StringBuilder hex = new StringBuilder();
            for (byte b : hash) hex.append(String.format("%02X", b));
            return hex.substring(0, 16);
        } catch (Exception e) {
            return "";
        }
    }

    public static boolean isLicensed(AppSettings settings) {
        if (settings.licenseKey == null || settings.licenseEmail == null) return false;
        return settings.licenseKey.equalsIgnoreCase(expectedLicenseKey(settings.licenseEmail));
    }
}
