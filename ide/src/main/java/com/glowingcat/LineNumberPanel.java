// SPDX-License-Identifier: Apache-2.0
package com.glowingcat;

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.text.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.Rectangle2D;
import java.util.*;

public class LineNumberPanel extends JPanel implements DocumentListener {
    private final JTextPane editor;
    // Breakpoint states: 0=none, 1=enabled (solid red), 2=disabled (hollow red)
    private final Map<Integer, Integer> breakpoints = new HashMap<>();
    private int executionLine = -1; // 1-based line with green arrow, -1 = none
    private java.util.function.IntConsumer breakpointChangeListener;

    public void setBreakpointChangeListener(java.util.function.IntConsumer listener) {
        this.breakpointChangeListener = listener;
    }

    private void fireBreakpointChange(int line) {
        if (breakpointChangeListener != null) breakpointChangeListener.accept(line);
    }

    public LineNumberPanel(JTextPane editor) {
        this.editor = editor;
        setFont(editor.getFont());
        setBackground(new Color(240, 240, 240));
        setBorder(BorderFactory.createMatteBorder(0, 0, 0, 1, Color.LIGHT_GRAY));
        editor.getDocument().addDocumentListener(this);
        editor.addPropertyChangeListener("font", e -> { setFont(editor.getFont()); repaint(); });

        addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                int line = getLineAtPoint(e.getY());
                if (line < 0) return;
                if (breakpoints.containsKey(line)) {
                    breakpoints.remove(line);
                } else {
                    breakpoints.put(line, 1);
                }
                repaint();
                fireBreakpointChange(line);
            }
        });
    }

    private int getLineAtPoint(int y) {
        Document doc = editor.getDocument();
        Element root = doc.getDefaultRootElement();
        for (int i = 0; i < root.getElementCount(); i++) {
            try {
                int offset = root.getElement(i).getStartOffset();
                Rectangle2D r = editor.modelToView2D(offset);
                if (r == null) continue;
                FontMetrics fm = getFontMetrics(getFont());
                if (y >= r.getY() && y < r.getY() + fm.getHeight()) return i + 1;
            } catch (BadLocationException ignored) {}
        }
        return -1;
    }

    public Set<Integer> getEnabledBreakpoints() {
        Set<Integer> result = new HashSet<>();
        for (var entry : breakpoints.entrySet()) {
            if (entry.getValue() == 1) result.add(entry.getKey());
        }
        return result;
    }

    public void setBreakpoint(int line) {
        breakpoints.put(line, 1);
        repaint();
    }

    public void removeBreakpoint(int line) {
        breakpoints.remove(line);
        repaint();
    }

    public void setExecutionLine(int line) {
        this.executionLine = line;
        repaint();
    }

    public void clearExecutionLine() {
        this.executionLine = -1;
        repaint();
    }

    public void scrollToLine(int line) {
        try {
            var root = editor.getDocument().getDefaultRootElement();
            int idx = Math.min(line - 1, root.getElementCount() - 1);
            if (idx < 0) return;
            int offset = root.getElement(idx).getStartOffset();
            var rect = editor.modelToView2D(offset).getBounds();
            var visible = editor.getVisibleRect();
            rect.y -= visible.height / 2;
            rect.height = visible.height;
            editor.scrollRectToVisible(rect);
        } catch (Exception ignored) {}
    }

    @Override
    public Dimension getPreferredSize() {
        int lines = getLineCount();
        int digits = Math.max(3, String.valueOf(lines).length());
        FontMetrics fm = getFontMetrics(getFont());
        int width = fm.charWidth('0') * (digits + 1) + 8;
        return new Dimension(width, editor.getPreferredSize().height);
    }

    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        Graphics2D g2 = (Graphics2D) g;
        g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        FontMetrics fm = g.getFontMetrics(getFont());
        g.setFont(getFont());

        int width = getWidth();
        Rectangle clip = g.getClipBounds();
        Document doc = editor.getDocument();
        Element root = doc.getDefaultRootElement();

        for (int i = 0; i < root.getElementCount(); i++) {
            try {
                int startOffset = root.getElement(i).getStartOffset();
                Rectangle2D r = editor.modelToView2D(startOffset);
                if (r == null) continue;
                int y = (int) r.getY();
                if (y + fm.getHeight() < clip.y) continue;
                if (y > clip.y + clip.height) break;

                int lineNum = i + 1;
                int cy = y + fm.getAscent() / 2 + fm.getHeight() / 4;
                int circleSize = fm.getHeight() - 4;

                // Green arrow for execution line (drawn alongside other indicators)
                if (lineNum == executionLine) {
                    g2.setColor(new Color(0, 180, 0));
                    int ax = 2, ay = cy - 5;
                    int[] xp = {ax, ax + 10, ax};
                    int[] yp = {ay, ay + 5, ay + 10};
                    g2.fillPolygon(xp, yp, 3);
                }
                // Breakpoint
                if (breakpoints.containsKey(lineNum)) {
                    int cx = width - circleSize - 4;
                    g2.setColor(Color.RED);
                    g2.fillOval(cx, y + 2, circleSize, circleSize);
                }
                // Line number (when no breakpoint)
                else {
                    g.setColor(Color.GRAY);
                    String num = String.valueOf(lineNum);
                    int x = width - fm.stringWidth(num) - 4;
                    g.drawString(num, x, y + fm.getAscent());
                }
            } catch (BadLocationException ignored) {}
        }
    }

    private int getLineCount() {
        return editor.getDocument().getDefaultRootElement().getElementCount();
    }

    @Override public void insertUpdate(DocumentEvent e) { repaint(); }
    @Override public void removeUpdate(DocumentEvent e) { repaint(); }
    @Override public void changedUpdate(DocumentEvent e) {}
}
