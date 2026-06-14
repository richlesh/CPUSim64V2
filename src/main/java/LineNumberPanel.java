// SPDX-License-Identifier: Apache-2.0
import javax.swing.*;
import javax.swing.event.*;
import javax.swing.text.*;
import java.awt.*;
import java.awt.geom.Rectangle2D;

public class LineNumberPanel extends JPanel implements DocumentListener {
    private final JTextPane editor;

    public LineNumberPanel(JTextPane editor) {
        this.editor = editor;
        setFont(editor.getFont());
        setBackground(new Color(240, 240, 240));
        setBorder(BorderFactory.createMatteBorder(0, 0, 0, 1, Color.LIGHT_GRAY));
        editor.getDocument().addDocumentListener(this);
        editor.addPropertyChangeListener("font", e -> { setFont(editor.getFont()); repaint(); });
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
        FontMetrics fm = g.getFontMetrics(getFont());
        g.setFont(getFont());
        g.setColor(Color.GRAY);

        int width = getWidth();
        Rectangle clip = g.getClipBounds();
        Document doc = editor.getDocument();
        Element root = doc.getDefaultRootElement();
        int startLine = 0;
        int endLine = root.getElementCount();

        for (int i = startLine; i < endLine; i++) {
            try {
                int startOffset = root.getElement(i).getStartOffset();
                Rectangle2D r = editor.modelToView2D(startOffset);
                if (r == null) continue;
                int y = (int) r.getY();
                if (y + fm.getHeight() < clip.y) continue;
                if (y > clip.y + clip.height) break;
                String num = String.valueOf(i + 1);
                int x = width - fm.stringWidth(num) - 4;
                g.drawString(num, x, y + fm.getAscent());
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
