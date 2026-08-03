// SPDX-License-Identifier: Apache-2.0
package com.glowingcat.cpusim64ide;

import org.fife.ui.rsyntaxtextarea.*;
import org.fife.ui.rtextarea.*;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;

/**
 * Wraps RSyntaxTextArea in an RTextScrollPane with breakpoint gutter icons
 * and execution line highlighting. This replaces both the old JTextPane + LineNumberPanel
 * combination and the AsmSyntaxHighlighter.
 */
public class EditorPanel extends JPanel {

    private final RSyntaxTextArea textArea;
    private final RTextScrollPane scrollPane;
    private final Gutter gutter;

    // Breakpoint tracking: line number (1-based) -> GutterIconInfo
    private final Map<Integer, GutterIconInfo> breakpointIcons = new HashMap<>();
    private int executionLine = -1; // 1-based, -1 = none
    private Object executionLineHighlight;
    private java.util.function.IntConsumer breakpointChangeListener;

    private static final Color EXECUTION_LINE_COLOR = new Color(200, 255, 200, 100);
    private static final Color BREAKPOINT_COLOR = Color.RED;

    private static ImageIcon breakpointIcon;

    public EditorPanel() {
        super(new BorderLayout());

        // Register custom syntax style for CPUSim64 assembly
        AbstractTokenMakerFactory atmf = (AbstractTokenMakerFactory) TokenMakerFactory.getDefaultInstance();
        atmf.putMapping("text/cpusim64", "com.glowingcat.cpusim64ide.CpuSim64TokenMaker");

        textArea = new RSyntaxTextArea();
        textArea.setSyntaxEditingStyle("text/cpusim64");
        textArea.setCodeFoldingEnabled(false);
        textArea.setAntiAliasingEnabled(true);
        textArea.setTabSize(4);
        textArea.setTabsEmulated(true);
        textArea.setAutoIndentEnabled(false);
        textArea.setMarkOccurrences(false);
        textArea.setHighlightCurrentLine(false);
        textArea.setPaintTabLines(false);
        textArea.setAnimateBracketMatching(false);
        textArea.setBracketMatchingEnabled(false);

        scrollPane = new RTextScrollPane(textArea);
        scrollPane.setLineNumbersEnabled(true);
        scrollPane.setIconRowHeaderEnabled(true);
        scrollPane.setFoldIndicatorEnabled(false);

        gutter = scrollPane.getGutter();
        gutter.setBookmarkingEnabled(false); // we manage our own icons

        // Click on icon row header (gutter) to toggle breakpoints.
        // The Gutter is a JPanel container; we must attach the listener to its
        // child components (icon row header and line number list) so clicks register.
        MouseAdapter breakpointClickListener = new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                try {
                    int y = e.getY();
                    int line = getLineAtY(y);
                    if (line >= 1) {
                        toggleBreakpoint(line);
                    }
                } catch (Exception ignored) {}
            }
        };
        for (Component child : gutter.getComponents()) {
            child.addMouseListener(breakpointClickListener);
        }

        add(scrollPane, BorderLayout.CENTER);
    }

    /**
     * Convert a Y coordinate in a gutter child component to a 1-based line number.
     * The gutter scrolls in sync with the text area, so Y is already in the
     * same coordinate space as the text area's visible region.
     */
    private int getLineAtY(int y) {
        try {
            int offset = textArea.viewToModel2D(new java.awt.geom.Point2D.Double(0, y));
            if (offset < 0) return -1;
            return textArea.getLineOfOffset(offset) + 1;
        } catch (Exception e) {
            return -1;
        }
    }

    public RSyntaxTextArea getTextArea() {
        return textArea;
    }

    public RTextScrollPane getScrollPane() {
        return scrollPane;
    }

    public Gutter getGutter() {
        return gutter;
    }

    // --- Breakpoint API (compatible with DebuggerWindow expectations) ---

    public void setBreakpointChangeListener(java.util.function.IntConsumer listener) {
        this.breakpointChangeListener = listener;
    }

    private void fireBreakpointChange(int line) {
        if (breakpointChangeListener != null) {
            breakpointChangeListener.accept(line);
        }
    }

    public void toggleBreakpoint(int line) {
        if (breakpointIcons.containsKey(line)) {
            removeBreakpoint(line);
        } else {
            setBreakpoint(line);
        }
        fireBreakpointChange(line);
    }

    public void setBreakpoint(int line) {
        if (breakpointIcons.containsKey(line)) return;
        try {
            GutterIconInfo info = gutter.addLineTrackingIcon(line - 1, getBreakpointIcon(), "Breakpoint");
            breakpointIcons.put(line, info);
        } catch (Exception ignored) {}
    }

    public void removeBreakpoint(int line) {
        GutterIconInfo info = breakpointIcons.remove(line);
        if (info != null) {
            gutter.removeTrackingIcon(info);
        }
    }

    public Set<Integer> getEnabledBreakpoints() {
        return new HashSet<>(breakpointIcons.keySet());
    }

    // --- Execution Line API ---

    public void setExecutionLine(int line) {
        clearExecutionLine();
        this.executionLine = line;
        try {
            executionLineHighlight = textArea.addLineHighlight(line - 1, EXECUTION_LINE_COLOR);
        } catch (Exception ignored) {}
        textArea.repaint();
    }

    public void clearExecutionLine() {
        if (executionLineHighlight != null) {
            textArea.removeLineHighlight(executionLineHighlight);
            executionLineHighlight = null;
        }
        this.executionLine = -1;
        textArea.repaint();
    }

    public void scrollToLine(int line) {
        try {
            int offset = textArea.getLineStartOffset(line - 1);
            textArea.setCaretPosition(offset);
            // Center the line in view
            Rectangle visible = textArea.getVisibleRect();
            Rectangle lineRect = textArea.modelToView(offset);
            if (lineRect != null) {
                int y = lineRect.y - visible.height / 2;
                if (y < 0) y = 0;
                textArea.scrollRectToVisible(new Rectangle(0, y, 1, visible.height));
            }
        } catch (Exception ignored) {}
    }

    // --- Utility ---

    private static ImageIcon getBreakpointIcon() {
        if (breakpointIcon == null) {
            // Create a small red circle icon for breakpoints
            int size = 12;
            java.awt.image.BufferedImage img = new java.awt.image.BufferedImage(size, size, java.awt.image.BufferedImage.TYPE_INT_ARGB);
            Graphics2D g2 = img.createGraphics();
            g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
            g2.setColor(BREAKPOINT_COLOR);
            g2.fillOval(1, 1, size - 2, size - 2);
            g2.dispose();
            breakpointIcon = new ImageIcon(img);
        }
        return breakpointIcon;
    }

    /**
     * Apply font settings to the text area and gutter.
     */
    public void setEditorFont(Font font) {
        textArea.setFont(font);
        gutter.setLineNumberFont(font);
    }
}
