// SPDX-License-Identifier: Apache-2.0
import javax.swing.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.event.*;
import java.nio.charset.StandardCharsets;
import cloud.lesh.CPUSim64.Simulator;

/**
 * Memory display window showing memory contents starting at a given address.
 * Shift-click a register value in the debugger to open.
 */
public class MemoryWindow extends JFrame {
    private static final int DISPLAY_ROWS = 64;
    private final Simulator sim;
    private final long baseAddress;
    private final DefaultTableModel model;
    private final int[] displayMode; // 0=dec, 1=hex, 2=float, 3=char

    public MemoryWindow(Simulator sim, long address, Font font) {
        super(String.format("Memory @ 0x%08X", address));
        this.sim = sim;
        this.baseAddress = address;
        this.displayMode = new int[DISPLAY_ROWS];

        model = new DefaultTableModel(new String[]{"Address", "Value"}, DISPLAY_ROWS);
        JTable table = new JTable(model) {
            @Override public boolean isCellEditable(int r, int c) { return false; }
        };
        table.setFont(font);
        table.getTableHeader().setFont(font);
        table.setRowHeight(table.getRowHeight() + 4);

        // Cell renderer with padding
        DefaultTableCellRenderer renderer = new DefaultTableCellRenderer() {
            @Override public Component getTableCellRendererComponent(JTable t, Object v, boolean s, boolean f, int r, int c) {
                Component comp = super.getTableCellRendererComponent(t, v, s, f, r, c);
                setBorder(BorderFactory.createEmptyBorder(2, 4, 2, 4));
                return comp;
            }
        };
        renderer.setFont(font);
        for (int i = 0; i < 2; i++) table.getColumnModel().getColumn(i).setCellRenderer(renderer);

        table.getColumnModel().getColumn(0).setPreferredWidth(100);
        table.getColumnModel().getColumn(1).setPreferredWidth(200);

        // Left-click value column to cycle display format
        table.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                int row = table.rowAtPoint(e.getPoint());
                int col = table.columnAtPoint(e.getPoint());
                if (row < 0 || col != 1) return;

                if (SwingUtilities.isRightMouseButton(e)) {
                    showContextMenu(e, table, row);
                } else if (SwingUtilities.isLeftMouseButton(e)) {
                    displayMode[row] = (displayMode[row] + 1) % 4;
                    updateRow(row);
                }
            }
        });

        populateTable();

        JScrollPane scroll = new JScrollPane(table);
        add(scroll, BorderLayout.CENTER);

        setSize(350, 600);
        setLocationRelativeTo(null);
        setDefaultCloseOperation(DISPOSE_ON_CLOSE);
        setVisible(true);
    }

    private void populateTable() {
        for (int i = 0; i < DISPLAY_ROWS; i++) {
            updateRow(i);
        }
    }

    private void updateRow(int row) {
        long addr = baseAddress + row;
        long value;
        try {
            value = sim.memRead(addr);
        } catch (Exception ex) {
            model.setValueAt(String.format("%08X", addr), row, 0);
            model.setValueAt("???", row, 1);
            return;
        }
        model.setValueAt(String.format("%08X", addr), row, 0);
        model.setValueAt(formatValue(value, displayMode[row]), row, 1);
    }

    private String formatValue(long value, int mode) {
        return switch (mode) {
            case 0 -> Long.toString(value);
            case 1 -> String.format("0x%016X", value);
            case 2 -> String.format("%g", Double.longBitsToDouble(value));
            case 3 -> {
                if (value >= 0 && value <= 0x10FFFF) {
                    yield "'" + new String(Character.toChars((int) value)) + "' (U+" + String.format("%04X", value) + ")";
                }
                yield "(invalid codepoint)";
            }
            default -> Long.toString(value);
        };
    }

    private void showContextMenu(MouseEvent e, JTable table, int row) {
        JPopupMenu menu = new JPopupMenu();
        JMenuItem decItem = new JMenuItem("Decimal");
        JMenuItem hexItem = new JMenuItem("Hexadecimal");
        JMenuItem floatItem = new JMenuItem("Float");
        JMenuItem charItem = new JMenuItem("Character");
        JMenuItem strItem = new JMenuItem("String");

        decItem.addActionListener(ev -> { displayMode[row] = 0; updateRow(row); });
        hexItem.addActionListener(ev -> { displayMode[row] = 1; updateRow(row); });
        floatItem.addActionListener(ev -> { displayMode[row] = 2; updateRow(row); });
        charItem.addActionListener(ev -> { displayMode[row] = 3; updateRow(row); });
        strItem.addActionListener(ev -> showString(baseAddress + row));

        menu.add(decItem);
        menu.add(hexItem);
        menu.add(floatItem);
        menu.add(charItem);
        menu.addSeparator();
        menu.add(strItem);
        menu.show(table, e.getX(), e.getY());
    }

    private void showString(long addr) {
        try {
            long byteCount = sim.memRead(addr);
            if (byteCount < 0 || byteCount > 100000) {
                JOptionPane.showMessageDialog(this, "Invalid string length: " + byteCount, "Error", JOptionPane.ERROR_MESSAGE);
                return;
            }
            byte[] bytes = new byte[(int) byteCount];
            int outIndex = 0;
            long wordAddr = addr + 1;
            for (; outIndex < byteCount; wordAddr++) {
                long w = sim.memRead(wordAddr);
                for (int b = 7; b >= 0 && outIndex < byteCount; b--) {
                    bytes[outIndex++] = (byte) ((w >> (b * 8)) & 0xFF);
                }
            }
            String str = new String(bytes, StandardCharsets.UTF_8);

            JTextArea textArea = new JTextArea(str);
            textArea.setFont(getContentPane().getFont());
            textArea.setEditable(false);
            textArea.setLineWrap(true);
            textArea.setWrapStyleWord(true);
            JScrollPane scroll = new JScrollPane(textArea);
            scroll.setPreferredSize(new Dimension(400, 300));

            JDialog dialog = new JDialog(this, String.format("String @ 0x%08X (%d bytes)", addr, byteCount), false);
            dialog.add(scroll);
            dialog.pack();
            dialog.setLocationRelativeTo(this);
            dialog.setVisible(true);
        } catch (Exception ex) {
            JOptionPane.showMessageDialog(this, "Error reading string: " + ex.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
        }
    }
}
