// SPDX-License-Identifier: Apache-2.0
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.locks.ReentrantLock;

/**
 * A fast canvas-based terminal emulator component.
 * Supports ANSI colors, scrollback, carriage return, text selection, keyboard input.
 */
public class TerminalPanel extends JComponent implements Scrollable {
    private static final int DEFAULT_COLS = 500;
    private static final int SCROLLBACK_LINES = 10000;
    private Color termBg = new Color(30, 30, 30);
    private Color termFg = Color.WHITE;

    // Circular buffer: each line is an array of codepoints and colors
    private final int[] chars;       // [line * maxCols + col] = codepoint
    private final Color[] colors;    // [line * maxCols + col] = foreground color
    private final byte[] attrs;      // [line * maxCols + col] = style bitmask
    private final int maxCols;
    private final int totalLines;    // scrollback + visible

    private static final byte ATTR_BOLD = 1;
    private static final byte ATTR_DIM = 2;
    private static final byte ATTR_ITALIC = 4;
    private static final byte ATTR_UNDERLINE = 8;
    private static final byte ATTR_HIDDEN = 16;
    private static final byte ATTR_STRIKETHROUGH = 32;
    private static final byte ATTR_REVERSE = 64;

    private int cursorRow;           // row within the buffer (absolute)
    private int cursorCol;
    private int scrollOffset;        // lines scrolled back from bottom (0 = at bottom)
    private int firstLine;           // first line index in circular buffer
    private int lineCount;           // total lines written

    private Font termFont;
    private int charWidth;
    private int charHeight;
    private int charAscent;

    // ANSI state
    private Color currentFg = termFg;
    private byte currentAttrs = 0;
    private StringBuilder escBuf = null; // non-null when inside an escape sequence

    // Selection
    private int selStartRow = -1, selStartCol = -1;
    private int selEndRow = -1, selEndCol = -1;
    private boolean selecting = false;

    // Input
    private PipedOutputStream inputPipe;
    private boolean inputEnabled = false;
    private boolean userTyping = false;
    private int inputStartRow, inputStartCol;

    // Cursor blink
    private boolean cursorVisible = true;
    private Timer blinkTimer;

    private final ReentrantLock bufferLock = new ReentrantLock();
    private Runnable interruptHandler;
    private JScrollBar scrollBar;

    public void setInterruptHandler(Runnable handler) { this.interruptHandler = handler; }

    public void setColors(Color fg, Color bg) {
        this.termFg = fg;
        this.termBg = bg;
        this.currentFg = fg;
        setBackground(bg);
        repaint();
    }

    public void attachScrollBar(JScrollBar sb) {
        this.scrollBar = sb;
        sb.addAdjustmentListener(e -> {
            if (!e.getValueIsAdjusting()) return;
            int max = Math.max(0, lineCount - getVisibleRows());
            scrollOffset = max - e.getValue();
            repaint();
        });
    }

    private void updateScrollBar() {
        if (scrollBar != null) {
            int max = Math.max(0, lineCount - getVisibleRows());
            scrollBar.setMaximum(max);
            scrollBar.setVisibleAmount(1);
            scrollBar.setValue(max - scrollOffset);
        }
    }

    public TerminalPanel(String fontName, int fontSize) {
        maxCols = DEFAULT_COLS;
        totalLines = SCROLLBACK_LINES;
        chars = new int[totalLines * maxCols];
        colors = new Color[totalLines * maxCols];
        attrs = new byte[totalLines * maxCols];
        firstLine = 0;
        lineCount = 1;
        cursorRow = 0;
        cursorCol = 0;

        setFont(fontName, fontSize);
        setBackground(termBg);
        setFocusable(true);
        setOpaque(true);

        blinkTimer = new Timer(500, e -> { cursorVisible = !cursorVisible; repaintCursorLine(); });
        blinkTimer.start();

        addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent e) {
                requestFocusInWindow();
                if (SwingUtilities.isLeftMouseButton(e)) {
                    Point p = charPosAt(e.getPoint());
                    selStartRow = p.y; selStartCol = p.x;
                    selEndRow = p.y; selEndCol = p.x;
                    selecting = true;
                    repaint();
                }
            }
            public void mouseReleased(MouseEvent e) { selecting = false; }
        });
        addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent e) {
                if (selecting) {
                    int y = e.getY();
                    if (y < 0) {
                        scrollOffset = Math.min(lineCount - getVisibleRows(), scrollOffset + 1);
                    } else if (y > getHeight()) {
                        scrollOffset = Math.max(0, scrollOffset - 1);
                    }
                    Point p = charPosAt(e.getPoint());
                    selEndRow = p.y; selEndCol = p.x;
                    repaint();
                    updateScrollBar();
                }
            }
        });
        addMouseWheelListener(e -> {
            scrollOffset = Math.max(0, Math.min(lineCount - getVisibleRows(), scrollOffset + e.getWheelRotation() * 3));
            repaint();
            updateScrollBar();
        });
        addKeyListener(new KeyAdapter() {
            public void keyPressed(KeyEvent e) {
                if (e.getKeyCode() == KeyEvent.VK_C && (e.getModifiersEx() & Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()) != 0) {
                    copySelection();
                    return;
                }
                if (e.getKeyCode() == KeyEvent.VK_C && (e.getModifiersEx() & KeyEvent.CTRL_DOWN_MASK) != 0) {
                    e.consume();
                    if (interruptHandler != null) interruptHandler.run();
                    return;
                }
                if (!inputEnabled) return;
                if (e.getKeyCode() == KeyEvent.VK_ENTER) {
                    e.consume();
                    String input = getInputText() + "\n";
                    write("\n");
                    sendInput(input);
                } else if (e.getKeyCode() == KeyEvent.VK_BACK_SPACE) {
                    if (cursorCol > inputStartCol || cursorRow > inputStartRow) {
                        bufferLock.lock();
                        try {
                            if (cursorCol > 0) cursorCol--;
                            else if (cursorRow > inputStartRow) { cursorRow--; cursorCol = getVisibleCols() - 1; }
                            int idx = cursorRow * maxCols + cursorCol;
                            chars[idx] = 0;
                            colors[idx] = termFg;
                        } finally { bufferLock.unlock(); }
                        repaint();
                    }
                } else if (e.getKeyCode() == KeyEvent.VK_D && (e.getModifiersEx() & KeyEvent.CTRL_DOWN_MASK) != 0) {
                    e.consume();
                    if (inputPipe != null) try { inputPipe.close(); } catch (Exception ignored) {}
                    inputEnabled = false;
                }
            }
            public void keyTyped(KeyEvent e) {
                if (!inputEnabled) return;
                char c = e.getKeyChar();
                if (c == KeyEvent.CHAR_UNDEFINED || c == '\n' || c == '\r' || c == '\b') return;
                if ((e.getModifiersEx() & Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()) != 0) return;
                Color saved = currentFg;
                currentFg = Color.WHITE;
                userTyping = true;
                write(String.valueOf(c));
                userTyping = false;
                currentFg = saved;
            }
        });
    }

    public void setFont(String fontName, int fontSize) {
        termFont = new Font(fontName, Font.PLAIN, fontSize);
        FontMetrics fm = getFontMetrics(termFont);
        charWidth = fm.charWidth('M');
        charHeight = fm.getHeight();
        charAscent = fm.getAscent();
        revalidate();
        repaint();
    }

    public int getVisibleRows() {
        return Math.max(1, getHeight() / charHeight);
    }

    public int getVisibleCols() {
        int w = getWidth();
        if (w <= 0) return maxCols;
        return Math.min(maxCols, Math.max(1, w / charWidth));
    }

    // ===== OUTPUT =====

    public void write(String text) {
        bufferLock.lock();
        try {
            for (int i = 0; i < text.length(); i++) {
                char c = text.charAt(i);
                if (escBuf != null) {
                    escBuf.append(c);
                    if (Character.isLetter(c)) {
                        processEscape(escBuf.toString());
                        escBuf = null;
                    }
                    continue;
                }
                if (c == '\u001B') {
                    escBuf = new StringBuilder();
                    escBuf.append(c);
                } else if (c == '\r') {
                    cursorCol = 0;
                } else if (c == '\n') {
                    newLine();
                } else if (c == '\b') {
                    if (cursorCol > 0) cursorCol--;
                } else {
                    int idx = cursorRow * maxCols + cursorCol;
                    chars[idx] = c;
                    colors[idx] = currentFg;
                    attrs[idx] = currentAttrs;
                    cursorCol++;
                    if (cursorCol >= getVisibleCols()) {
                        cursorCol = 0;
                        newLine();
                    }
                }
            }
        } finally { bufferLock.unlock(); }
        if (!userTyping) { inputStartRow = cursorRow; inputStartCol = cursorCol; }
        scrollOffset = 0;
        SwingUtilities.invokeLater(() -> { repaint(); updateScrollBar(); });
    }

    private void newLine() {
        cursorRow++;
        cursorCol = 0;
        if (cursorRow >= totalLines) {
            cursorRow = 0; // wrap in circular buffer
        }
        if (lineCount < totalLines) lineCount++;
        else firstLine = (firstLine + 1) % totalLines;
        // Clear the new line
        int base = cursorRow * maxCols;
        for (int i = 0; i < maxCols; i++) {
            chars[base + i] = 0;
            colors[base + i] = termFg;
            attrs[base + i] = 0;
        }
    }

    private void processEscape(String seq) {
        if (seq.startsWith("\u001B[") && seq.endsWith("m")) {
            // SGR - color
            String nums = seq.substring(2, seq.length() - 1);
            if (nums.isEmpty() || nums.equals("0")) { currentFg = termFg; currentAttrs = 0; return; }
            for (String n : nums.split(";")) {
                switch (n) {
                    case "0" -> { currentFg = termFg; currentAttrs = 0; }
                    case "1" -> currentAttrs |= ATTR_BOLD;
                    case "2" -> currentAttrs |= ATTR_DIM;
                    case "3" -> currentAttrs |= ATTR_ITALIC;
                    case "4" -> currentAttrs |= ATTR_UNDERLINE;
                    case "7" -> currentAttrs |= ATTR_REVERSE;
                    case "8" -> currentAttrs |= ATTR_HIDDEN;
                    case "9" -> currentAttrs |= ATTR_STRIKETHROUGH;
                    case "22" -> currentAttrs &= ~(ATTR_BOLD | ATTR_DIM);
                    case "23" -> currentAttrs &= ~ATTR_ITALIC;
                    case "24" -> currentAttrs &= ~ATTR_UNDERLINE;
                    case "27" -> currentAttrs &= ~ATTR_REVERSE;
                    case "28" -> currentAttrs &= ~ATTR_HIDDEN;
                    case "29" -> currentAttrs &= ~ATTR_STRIKETHROUGH;
                    case "30" -> currentFg = Color.DARK_GRAY;
                    case "31" -> currentFg = new Color(200, 50, 50);
                    case "32" -> currentFg = new Color(50, 200, 50);
                    case "33" -> currentFg = new Color(200, 200, 50);
                    case "34" -> currentFg = new Color(80, 80, 255);
                    case "35" -> currentFg = new Color(200, 50, 200);
                    case "36" -> currentFg = new Color(50, 200, 200);
                    case "37" -> currentFg = Color.WHITE;
                    case "90" -> currentFg = Color.GRAY;
                    case "91" -> currentFg = new Color(255, 100, 100);
                    case "92" -> currentFg = new Color(100, 255, 100);
                    case "93" -> currentFg = new Color(255, 255, 100);
                    case "94" -> currentFg = new Color(130, 130, 255);
                    case "95" -> currentFg = new Color(255, 100, 255);
                    case "96" -> currentFg = new Color(100, 255, 255);
                    case "97" -> currentFg = Color.WHITE;
                }
            }
        } else if (seq.equals("\u001B[?25l")) {
            cursorVisible = false; blinkTimer.stop();
        } else if (seq.equals("\u001B[?25h")) {
            cursorVisible = true; blinkTimer.start();
        }
    }

    public void clear() {
        bufferLock.lock();
        try {
            java.util.Arrays.fill(chars, 0);
            java.util.Arrays.fill(colors, termFg);
            java.util.Arrays.fill(attrs, (byte) 0);
            cursorRow = 0; cursorCol = 0;
            firstLine = 0; lineCount = 1;
            scrollOffset = 0;
            currentFg = termFg;
            currentAttrs = 0;
            escBuf = null;
        } finally { bufferLock.unlock(); }
        repaint();
    }

    // ===== INPUT =====

    public void enableInput(PipedOutputStream pipe) {
        this.inputPipe = pipe;
        this.inputEnabled = true;
        this.inputStartRow = cursorRow;
        this.inputStartCol = cursorCol;
        requestFocusInWindow();
    }

    public void disableInput() {
        this.inputEnabled = false;
        this.inputPipe = null;
    }

    private String getInputText() {
        bufferLock.lock();
        try {
            StringBuilder sb = new StringBuilder();
            int r = inputStartRow, c = inputStartCol;
            while (r != cursorRow || c != cursorCol) {
                int idx = r * maxCols + c;
                if (chars[idx] != 0) sb.appendCodePoint(chars[idx]);
                c++;
                if (c >= maxCols) { c = 0; r = (r + 1) % totalLines; }
            }
            return sb.toString();
        } finally { bufferLock.unlock(); }
    }

    private void sendInput(String text) {
        if (inputPipe != null) {
            try {
                inputPipe.write(text.getBytes(StandardCharsets.UTF_8));
                inputPipe.flush();
            } catch (Exception ignored) {}
        }
        inputStartRow = cursorRow;
        inputStartCol = cursorCol;
    }

    // ===== SELECTION & COPY =====

    private void copySelection() {
        String sel = getSelectedText();
        if (sel != null && !sel.isEmpty()) {
            Toolkit.getDefaultToolkit().getSystemClipboard().setContents(new StringSelection(sel), null);
        }
    }

    public String getSelectedText() {
        if (selStartRow < 0) return null;
        int sr = selStartRow, sc = selStartCol, er = selEndRow, ec = selEndCol;
        // Normalize direction
        if (sr > er || (sr == er && sc > ec)) {
            int t = sr; sr = er; er = t;
            t = sc; sc = ec; ec = t;
        }
        bufferLock.lock();
        try {
            StringBuilder sb = new StringBuilder();
            int r = sr, c = sc;
            while (r < er || (r == er && c <= ec)) {
                int bufRow = (firstLine + r) % totalLines;
                int idx = bufRow * maxCols + c;
                int ch = chars[idx];
                if (ch != 0) sb.appendCodePoint(ch);
                c++;
                if (c >= maxCols) {
                    sb.append('\n');
                    c = 0; r++;
                }
            }
            return sb.toString();
        } finally { bufferLock.unlock(); }
    }

    // ===== PAINTING =====

    @Override
    protected void paintComponent(Graphics g) {
        Graphics2D g2 = (Graphics2D) g;
        g2.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);
        g2.setColor(termBg);
        g2.fillRect(0, 0, getWidth(), getHeight());
        g2.setFont(termFont);

        int visRows = getVisibleRows();
        int startLine = Math.max(0, lineCount - visRows - scrollOffset);

        // Normalize selection
        int sr = selStartRow, sc = selStartCol, er = selEndRow, ec = selEndCol;
        if (sr > er || (sr == er && sc > ec)) {
            int t = sr; sr = er; er = t; t = sc; sc = ec; ec = t;
        }

        bufferLock.lock();
        try {
            for (int row = 0; row < visRows; row++) {
                int bufLine = startLine + row;
                if (bufLine >= lineCount) break;
                int bufRow = (firstLine + bufLine) % totalLines;
                int y = row * charHeight + charAscent;
                for (int col = 0; col < maxCols; col++) {
                    int idx = bufRow * maxCols + col;
                    int ch = chars[idx];
                    // Selection highlight
                    boolean inSelection = (bufLine > sr && bufLine < er) ||
                        (bufLine == sr && bufLine == er && col >= sc && col <= ec) ||
                        (bufLine == sr && bufLine != er && col >= sc) ||
                        (bufLine == er && bufLine != sr && col <= ec);
                    if (inSelection) {
                        g2.setColor(new Color(70, 130, 200));
                        g2.fillRect(col * charWidth, row * charHeight, charWidth, charHeight);
                    }
                    if (ch != 0) {
                        byte attr = attrs[idx];
                        Color fg = inSelection ? Color.WHITE : (colors[idx] != null ? colors[idx] : termFg);
                        Color bg = termBg;
                        if ((attr & ATTR_REVERSE) != 0) { Color t = fg; fg = bg; bg = t; }
                        if ((attr & ATTR_DIM) != 0) fg = fg.darker();
                        if ((attr & ATTR_BOLD) != 0) fg = fg.brighter();
                        if ((attr & ATTR_REVERSE) != 0 && !inSelection) {
                            g2.setColor(bg);
                            g2.fillRect(col * charWidth, row * charHeight, charWidth, charHeight);
                        }
                        if ((attr & ATTR_HIDDEN) == 0) {
                            Font drawFont = termFont;
                            if ((attr & ATTR_BOLD) != 0 && (attr & ATTR_ITALIC) != 0)
                                drawFont = drawFont.deriveFont(Font.BOLD | Font.ITALIC);
                            else if ((attr & ATTR_BOLD) != 0) drawFont = drawFont.deriveFont(Font.BOLD);
                            else if ((attr & ATTR_ITALIC) != 0) drawFont = drawFont.deriveFont(Font.ITALIC);
                            g2.setFont(drawFont);
                            g2.setColor(fg);
                            g2.drawString(new String(Character.toChars(ch)), col * charWidth, y);
                            if ((attr & ATTR_UNDERLINE) != 0) {
                                g2.drawLine(col * charWidth, y + 1, (col + 1) * charWidth, y + 1);
                            }
                            if ((attr & ATTR_STRIKETHROUGH) != 0) {
                                int midY = row * charHeight + charHeight / 2;
                                g2.drawLine(col * charWidth, midY, (col + 1) * charWidth, midY);
                            }
                            g2.setFont(termFont);
                        }
                    }
                }
            }
            // Draw cursor
            if (inputEnabled && cursorVisible) {
                int curLine = (cursorRow - firstLine + totalLines) % totalLines;
                int screenRow = curLine - startLine;
                if (screenRow >= 0 && screenRow < visRows) {
                    g2.setColor(termFg);
                    g2.fillRect(cursorCol * charWidth, screenRow * charHeight, charWidth, charHeight);
                }
            }
        } finally { bufferLock.unlock(); }
    }

    private void repaintCursorLine() {
        if (inputEnabled) {
            int visRows = getVisibleRows();
            int startLine = Math.max(0, lineCount - visRows - scrollOffset);
            int curLine = (cursorRow - firstLine + totalLines) % totalLines;
            int screenRow = curLine - startLine;
            if (screenRow >= 0 && screenRow < visRows) {
                repaint(0, screenRow * charHeight, getWidth(), charHeight);
            }
        }
    }

    private Point charPosAt(Point pixel) {
        int visRows = getVisibleRows();
        int startLine = Math.max(0, lineCount - visRows - scrollOffset);
        int row = pixel.y / charHeight + startLine;
        int col = Math.min(maxCols - 1, Math.max(0, pixel.x / charWidth));
        return new Point(col, row);
    }

    // ===== SIZING =====

    @Override public Dimension getPreferredSize() {
        return new Dimension(maxCols * charWidth, getVisibleRows() * charHeight);
    }

    @Override public Dimension getPreferredScrollableViewportSize() { return getPreferredSize(); }
    @Override public int getScrollableUnitIncrement(Rectangle vis, int ori, int dir) { return charHeight; }
    @Override public int getScrollableBlockIncrement(Rectangle vis, int ori, int dir) { return vis.height; }
    @Override public boolean getScrollableTracksViewportWidth() { return true; }
    @Override public boolean getScrollableTracksViewportHeight() { return true; }

    // ===== UTILITY =====

    public String getText() {
        bufferLock.lock();
        try {
            StringBuilder sb = new StringBuilder();
            for (int line = 0; line < lineCount; line++) {
                int bufRow = (firstLine + line) % totalLines;
                int lastNonEmpty = -1;
                for (int col = maxCols - 1; col >= 0; col--) {
                    if (chars[bufRow * maxCols + col] != 0) { lastNonEmpty = col; break; }
                }
                for (int col = 0; col <= lastNonEmpty; col++) {
                    int ch = chars[bufRow * maxCols + col];
                    sb.appendCodePoint(ch != 0 ? ch : ' ');
                }
                if (line < lineCount - 1) sb.append('\n');
            }
            return sb.toString();
        } finally { bufferLock.unlock(); }
    }
}
