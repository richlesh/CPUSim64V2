/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import javax.swing.*;
import java.awt.*;

/**
 * A dialog that presents a large text area for editing the Generic vendor
 * YAML configuration. Opened via the "Configure..." link in Preferences
 * when the "Generic" vendor is selected.
 */
public class GenericVendorDialog extends JDialog {

    private final JTextArea yamlArea;
    private boolean confirmed = false;

    public GenericVendorDialog(Window owner) {
        super(owner, "Generic Vendor Configuration", ModalityType.APPLICATION_MODAL);

        setLayout(new BorderLayout(8, 8));
        ((JComponent) getContentPane()).setBorder(BorderFactory.createEmptyBorder(12, 12, 12, 12));

        // Header
        JLabel header = new JLabel(
                "<html><b>YAML Configuration</b><br>" +
                "<small>Variables: ${AUTH_TOKEN}, ${MODEL}, ${PROMPT}, ${MESSAGES}, ${MESSAGES_NO_SYSTEM}, ${SYSTEM_PROMPT}, ${GUID}</small></html>");
        add(header, BorderLayout.NORTH);

        // YAML text area
        yamlArea = new JTextArea(30, 70);
        yamlArea.setFont(new Font("Monospaced", Font.PLAIN, 13));
        yamlArea.setTabSize(2);
        yamlArea.setText(GenericVendorConfig.loadYamlString());

        JScrollPane scroll = new JScrollPane(yamlArea);
        scroll.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
        add(scroll, BorderLayout.CENTER);

        // Buttons
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.RIGHT, 8, 0));
        JButton defaultsBtn = new JButton("Reset to Defaults");
        JButton okButton = new JButton("OK");
        JButton cancelButton = new JButton("Cancel");

        defaultsBtn.addActionListener(e -> {
            int choice = JOptionPane.showConfirmDialog(this,
                    "Reset YAML to the default template?",
                    "Reset Configuration", JOptionPane.YES_NO_OPTION);
            if (choice == JOptionPane.YES_OPTION) {
                yamlArea.setText(GenericVendorConfig.DEFAULT_YAML);
            }
        });
        okButton.addActionListener(e -> {
            confirmed = true;
            GenericVendorConfig.saveYamlString(yamlArea.getText());
            dispose();
        });
        cancelButton.addActionListener(e -> dispose());

        buttonPanel.add(defaultsBtn);
        buttonPanel.add(Box.createHorizontalStrut(20));
        buttonPanel.add(okButton);
        buttonPanel.add(cancelButton);
        add(buttonPanel, BorderLayout.SOUTH);

        getRootPane().setDefaultButton(okButton);
        pack();
        setLocationRelativeTo(owner);
        setMinimumSize(new Dimension(500, 400));
    }

    public boolean isConfirmed() {
        return confirmed;
    }

    public String getYamlText() {
        return yamlArea.getText();
    }
}
