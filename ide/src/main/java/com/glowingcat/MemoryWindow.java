// SPDX-License-Identifier: Apache-2.0
package com.glowingcat;

import javax.swing.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.event.*;
import java.nio.charset.StandardCharsets;
import cloud.lesh.CPUSim64.Simulator;

/**
 * Memory display window showing memory contents starting at a given address.
 * Singleton: only one instance at a time. Shift-click a register value in the debugger to open/refresh.
 */
public class MemoryWindow extends JFrame {
    private static final int DISPLAY_ROWS = 74; // 10 before + 64 after
    private static final int PRE_ROWS = 10;
    private static MemoryWindow instance;

    private Simulator sim;
    private long baseAddress;
    private final DefaultTableModel model;
    private final JTable table;
    private final int[] displayMode;
    private final int[] rowType; // 0=normal, 1=allocated header, 2=free header
    private final AppSettings settings;

    private static final Color ALLOC_HEADER_COLOR = new Color(220, 220, 220);
    private static final Color FREE_HEADER_COLOR = new Color(200, 240, 200);

    public static void showMemory(Simulator sim, long address, Font font, AppSettings settings) {
        if (instance == null || !instance.isDisplayable()) {
            instance = new MemoryWindow(sim, address, font, settings);
        } else {
            instance.sim = sim;
            instance.baseAddress = address;
            instance.setTitle(String.format("Memory @ 0x%08X", address));
            instance.refresh();
            instance.toFront();
            instance.requestFocus();
        }
    }

    private MemoryWindow(Simulator sim, long address, Font font, AppSettings settings) {
        super(String.format("Memory @ 0x%08X", address));
        this.sim = sim;
        this.baseAddress = address;
        this.settings = settings;
        this.displayMode = new int[DISPLAY_ROWS];
        this.rowType = new int[DISPLAY_ROWS];

        model = new DefaultTableModel(new String[]{"Address", "Value"}, DISPLAY_ROWS);
        table = new JTable(model) {
            @Override public boolean isCellEditable(int r, int c) { return false; }
        };
        table.setFont(font);
        table.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        table.getTableHeader().setFont(font);
        table.setRowHeight(table.getRowHeight() + 4);

        // Cell renderer with padding and heap block coloring
        DefaultTableCellRenderer renderer = new DefaultTableCellRenderer() {
            @Override public Component getTableCellRendererComponent(JTable t, Object v, boolean s, boolean f, int r, int c) {
                Component comp = super.getTableCellRendererComponent(t, v, s, f, r, c);
                setBorder(BorderFactory.createEmptyBorder(2, 4, 2, 4));
                if (!s) {
                    switch (rowType[r]) {
                        case 1 -> setBackground(ALLOC_HEADER_COLOR);
                        case 2 -> setBackground(FREE_HEADER_COLOR);
                        default -> setBackground(Color.WHITE);
                    }
                }
                return comp;
            }
        };
        renderer.setFont(font);
        for (int i = 0; i < 2; i++) table.getColumnModel().getColumn(i).setCellRenderer(renderer);

        // Restore column widths
        if (settings.memColWidths != null && settings.memColWidths.length == 2) {
            table.getColumnModel().getColumn(0).setPreferredWidth(settings.memColWidths[0]);
            table.getColumnModel().getColumn(1).setPreferredWidth(settings.memColWidths[1]);
        } else {
            table.getColumnModel().getColumn(0).setPreferredWidth(100);
            table.getColumnModel().getColumn(1).setPreferredWidth(200);
        }

        // Left-click value column to cycle display format, ctrl-click for context menu
        table.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                int row = table.rowAtPoint(e.getPoint());
                int col = table.columnAtPoint(e.getPoint());
                if (row < 0 || col != 1) return;

                if (SwingUtilities.isRightMouseButton(e) || e.isControlDown()) {
                    showContextMenu(e, row);
                } else if (SwingUtilities.isLeftMouseButton(e)) {
                    displayMode[row] = (displayMode[row] + 1) % 4;
                    updateRow(row);
                }
            }
        });

        populateTable();

        JScrollPane scroll = new JScrollPane(table);
        add(scroll, BorderLayout.CENTER);

        // Restore window size
        setSize(settings.memWindowWidth, settings.memWindowHeight);
        setLocationRelativeTo(null);
        setDefaultCloseOperation(DISPOSE_ON_CLOSE);

        addWindowListener(new WindowAdapter() {
            @Override public void windowClosed(WindowEvent e) {
                saveSettings();
                instance = null;
            }
        });

        setVisible(true);

        // Scroll to show the base address row (PRE_ROWS down)
        SwingUtilities.invokeLater(() -> {
            table.setRowSelectionInterval(PRE_ROWS, PRE_ROWS);
            Rectangle rect = table.getCellRect(PRE_ROWS, 0, true);
            table.scrollRectToVisible(rect);
        });
    }

    private void saveSettings() {
        settings.memWindowWidth = getWidth();
        settings.memWindowHeight = getHeight();
        settings.memColWidths = new int[]{
            table.getColumnModel().getColumn(0).getWidth(),
            table.getColumnModel().getColumn(1).getWidth()
        };
        settings.save();
    }

    public void refresh() {
        java.util.Arrays.fill(displayMode, 0);
        populateTable();
        SwingUtilities.invokeLater(() -> {
            table.setRowSelectionInterval(PRE_ROWS, PRE_ROWS);
            Rectangle rect = table.getCellRect(PRE_ROWS, 0, true);
            table.scrollRectToVisible(rect);
        });
    }

    private void populateTable() {
        classifyHeapRows();
        for (int i = 0; i < DISPLAY_ROWS; i++) {
            updateRow(i);
        }
    }

    private void classifyHeapRows() {
        java.util.Arrays.fill(rowType, 0);
        long heapStart = sim.getHeapStart();
        // Walk heap blocks from heapStart
        try {
            long p = heapStart;
            int safety = 10000;
            while (p > 0 && safety-- > 0) {
                long size = sim.memRead(p + 2);
                long blockSize = Math.abs(size);
                if (blockSize <= 0 || blockSize > sim.getHeapLimit()) break;
                boolean isFree = size < 0;
                // Mark the 3 header words (p, p+1, p+2)
                for (int offset = 0; offset < 3; offset++) {
                    int row = (int) (p + offset - addressForRow(0));
                    if (row >= 0 && row < DISPLAY_ROWS) {
                        rowType[row] = isFree ? 2 : 1;
                    }
                }
                // Move to next block
                long next = sim.memRead(p + 1);
                if (next <= p && next != -1) break; // prevent infinite loop
                if (next == -1) break;
                p = next;
            }
        } catch (Exception ignored) {}
    }

    private long addressForRow(int row) {
        return baseAddress - PRE_ROWS + row;
    }

    private void updateRow(int row) {
        long addr = addressForRow(row);
        if (addr < 0) {
            model.setValueAt("--------", row, 0);
            model.setValueAt("", row, 1);
            return;
        }
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

    private void showContextMenu(MouseEvent e, int row) {
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
        strItem.addActionListener(ev -> showString(addressForRow(row)));

        menu.add(decItem);
        menu.add(hexItem);
        menu.add(floatItem);
        menu.add(charItem);
        menu.addSeparator();
        menu.add(strItem);
        JMenuItem memItem = new JMenuItem("Memory");
        memItem.addActionListener(ev -> {
            try {
                long addr = sim.memRead(addressForRow(row));
                if (addr < 0 || addr >= sim.getHeapLimit()) {
                    JOptionPane.showMessageDialog(this, String.format("Illegal address: 0x%016X", addr), "Error", JOptionPane.ERROR_MESSAGE);
                    return;
                }
                baseAddress = addr;
                setTitle(String.format("Memory @ 0x%08X", addr));
                refresh();
            } catch (Exception ex) {
                JOptionPane.showMessageDialog(this, "Error reading memory", "Error", JOptionPane.ERROR_MESSAGE);
            }
        });
        menu.add(memItem);
        menu.show(table, e.getX(), e.getY());
    }

    private void showString(long addr) {
        try {
            if (addr < 0) return;
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
            textArea.setFont(table.getFont());
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
