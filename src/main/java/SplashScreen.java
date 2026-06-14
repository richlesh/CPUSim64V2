// SPDX-License-Identifier: Apache-2.0
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class SplashScreen {

    public static void show() {
        JDialog dialog = new JDialog((Frame) null, true);
        dialog.setUndecorated(true);
        dialog.setBackground(new Color(30, 30, 30));

        JPanel panel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
        panel.setBackground(new Color(30, 30, 30));
        panel.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(new Color(60, 60, 60)),
            BorderFactory.createEmptyBorder(30, 40, 30, 40)
        ));

        // App icon
        var iconUrl = SplashScreen.class.getResource("/app_icon_256.png");
        if (iconUrl != null) {
            JLabel iconLabel = new JLabel(new ImageIcon(new ImageIcon(iconUrl)
                .getImage().getScaledInstance(128, 128, Image.SCALE_SMOOTH)));
            iconLabel.setAlignmentX(Component.CENTER_ALIGNMENT);
            panel.add(iconLabel);
        }
        panel.add(Box.createVerticalStrut(16));

        // App name
        JLabel name = new JLabel("CPUSim64");
        name.setFont(new Font(Font.SANS_SERIF, Font.BOLD, 22));
        name.setForeground(Color.WHITE);
        name.setAlignmentX(Component.CENTER_ALIGNMENT);
        panel.add(name);
        panel.add(Box.createVerticalStrut(6));

        // Version
        JLabel version = new JLabel("Version " + cloud.lesh.CPUSim64.BuildInfo.VERSION);
        version.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 14));
        version.setForeground(new Color(170, 170, 170));
        version.setAlignmentX(Component.CENTER_ALIGNMENT);
        panel.add(version);
        panel.add(Box.createVerticalStrut(20));

        // Message
        JLabel msg1 = new JLabel("If you enjoy using this product");
        JLabel msg2 = new JLabel("please consider donating to help");
        JLabel msg3 = new JLabel("fund this and other open source");
        for (JLabel l : new JLabel[]{msg1, msg2, msg3}) {
            l.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 14));
            l.setForeground(new Color(200, 200, 200));
            l.setAlignmentX(Component.CENTER_ALIGNMENT);
            panel.add(l);
        }

        // Link line
        JLabel msg4 = new JLabel("<html>projects at <a style='color:#4da3ff;'>Glowing Cat Software</a>.</html>");
        msg4.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 14));
        msg4.setForeground(new Color(200, 200, 200));
        msg4.setAlignmentX(Component.CENTER_ALIGNMENT);
        msg4.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        msg4.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                try { Desktop.getDesktop().browse(java.net.URI.create("https://glowingcatsoftware.com")); }
                catch (Exception ignored) {}
            }
        });
        panel.add(msg4);

        // Click anywhere (except link) to dismiss
        panel.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) { dialog.dispose(); }
        });
        for (Component c : panel.getComponents()) {
            if (c != msg4) {
                c.addMouseListener(new MouseAdapter() {
                    @Override public void mouseClicked(MouseEvent e) { dialog.dispose(); }
                });
            }
        }

        dialog.setContentPane(panel);
        dialog.pack();
        dialog.setLocationRelativeTo(null);

        // Auto-close after 20 seconds
        Timer timer = new Timer(20000, e -> dialog.dispose());
        timer.setRepeats(false);
        timer.start();

        dialog.setVisible(true);
    }
}
