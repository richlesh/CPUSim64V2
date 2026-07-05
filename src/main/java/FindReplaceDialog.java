// SPDX-License-Identifier: Apache-2.0
import javax.swing.*;
import javax.swing.text.*;
import java.awt.*;
import java.util.regex.*;
import org.fife.ui.rsyntaxtextarea.RSyntaxTextArea;

public class FindReplaceDialog {
    private JDialog dialog;
    private JTextField searchField;
    private JTextField replaceField;
    private JCheckBox caseSensitive;
    private JCheckBox entireWord;
    private JCheckBox grepMode;
    private JCheckBox selectedOnly;
    private JCheckBox wrapAround;
    private RSyntaxTextArea editor;
    private int lastFoundPos = -1;

    public FindReplaceDialog(JFrame parent, RSyntaxTextArea editor) {
        this.editor = editor;
        dialog = new JDialog(parent, "Find / Replace", false);
        dialog.setLayout(new BorderLayout(8, 8));

        // Left panel: fields and options
        JPanel leftPanel = new JPanel(new GridBagLayout());
        leftPanel.setBorder(BorderFactory.createEmptyBorder(8, 8, 8, 0));
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(3, 3, 3, 3);
        gbc.anchor = GridBagConstraints.WEST;

        gbc.gridx = 0; gbc.gridy = 0;
        leftPanel.add(new JLabel("Search:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL; gbc.weightx = 1;
        searchField = new JTextField(24);
        leftPanel.add(searchField, gbc);

        gbc.gridx = 0; gbc.gridy = 1; gbc.fill = GridBagConstraints.NONE; gbc.weightx = 0;
        leftPanel.add(new JLabel("Replace:"), gbc);
        gbc.gridx = 1; gbc.fill = GridBagConstraints.HORIZONTAL; gbc.weightx = 1;
        replaceField = new JTextField(24);
        leftPanel.add(replaceField, gbc);

        // Matching Options
        gbc.gridx = 0; gbc.gridy = 2; gbc.gridwidth = 2; gbc.fill = GridBagConstraints.HORIZONTAL;
        JPanel matchPanel = new JPanel(new GridLayout(3, 1));
        matchPanel.setBorder(BorderFactory.createTitledBorder("Matching Options"));
        caseSensitive = new JCheckBox("Case Sensitive");
        entireWord = new JCheckBox("Entire Word");
        grepMode = new JCheckBox("Grep");
        matchPanel.add(caseSensitive);
        matchPanel.add(entireWord);
        matchPanel.add(grepMode);
        leftPanel.add(matchPanel, gbc);

        // Search In
        gbc.gridy = 3;
        JPanel searchInPanel = new JPanel(new GridLayout(2, 1));
        searchInPanel.setBorder(BorderFactory.createTitledBorder("Search In"));
        selectedOnly = new JCheckBox("Selected Text Only");
        wrapAround = new JCheckBox("Wrap Around", true);
        searchInPanel.add(selectedOnly);
        searchInPanel.add(wrapAround);
        leftPanel.add(searchInPanel, gbc);

        // Right panel: buttons
        JPanel buttonPanel = new JPanel(new GridLayout(6, 1, 0, 4));
        buttonPanel.setBorder(BorderFactory.createEmptyBorder(8, 4, 8, 8));
        JButton nextBtn = new JButton("Next");
        JButton prevBtn = new JButton("Previous");
        JButton firstBtn = new JButton("First");
        JButton replaceBtn = new JButton("Replace");
        JButton replaceAllBtn = new JButton("Replace All");
        JButton replaceFindBtn = new JButton("Replace and Find");
        buttonPanel.add(nextBtn);
        buttonPanel.add(prevBtn);
        buttonPanel.add(firstBtn);
        buttonPanel.add(replaceBtn);
        buttonPanel.add(replaceAllBtn);
        buttonPanel.add(replaceFindBtn);

        nextBtn.addActionListener(e -> findNext());
        prevBtn.addActionListener(e -> findPrevious());
        firstBtn.addActionListener(e -> findFirst());
        replaceBtn.addActionListener(e -> replaceCurrent());
        replaceAllBtn.addActionListener(e -> replaceAll());
        replaceFindBtn.addActionListener(e -> { replaceCurrent(); findNext(); });

        dialog.add(leftPanel, BorderLayout.CENTER);
        dialog.add(buttonPanel, BorderLayout.EAST);
        dialog.pack();
        dialog.getRootPane().setDefaultButton(nextBtn);
        dialog.setLocationRelativeTo(parent);
    }

    public void show() {
        dialog.setVisible(true);
        searchField.requestFocus();
        String sel = editor.getSelectedText();
        if (sel != null && !sel.contains("\n")) {
            searchField.setText(sel);
            searchField.selectAll();
        }
    }

    private Pattern buildPattern(String search) {
        int flags = caseSensitive.isSelected() ? 0 : Pattern.CASE_INSENSITIVE;
        String regex;
        if (grepMode.isSelected()) {
            regex = search;
        } else {
            regex = Pattern.quote(search);
        }
        if (entireWord.isSelected()) {
            regex = "\\b" + regex + "\\b";
        }
        return Pattern.compile(regex, flags);
    }

    private int[] getSearchRange() {
        if (selectedOnly.isSelected() && editor.getSelectionStart() != editor.getSelectionEnd()) {
            return new int[]{editor.getSelectionStart(), editor.getSelectionEnd()};
        }
        return new int[]{0, editor.getDocument().getLength()};
    }

    private void findNext() {
        String search = searchField.getText();
        if (search.isEmpty()) return;
        try {
            String text = editor.getDocument().getText(0, editor.getDocument().getLength());
            Pattern p = buildPattern(search);
            int startFrom = editor.getCaretPosition();
            Matcher m = p.matcher(text);
            if (m.find(startFrom)) {
                select(m.start(), m.end());
            } else if (wrapAround.isSelected() && m.find(0)) {
                select(m.start(), m.end());
            }
        } catch (BadLocationException ignored) {}
    }

    private void findPrevious() {
        String search = searchField.getText();
        if (search.isEmpty()) return;
        try {
            String text = editor.getDocument().getText(0, editor.getDocument().getLength());
            Pattern p = buildPattern(search);
            int endBefore = editor.getSelectionStart();
            Matcher m = p.matcher(text);
            int lastStart = -1, lastEnd = -1;
            while (m.find() && m.start() < endBefore) {
                lastStart = m.start();
                lastEnd = m.end();
            }
            if (lastStart >= 0) {
                select(lastStart, lastEnd);
            } else if (wrapAround.isSelected()) {
                // Search from end
                m = p.matcher(text);
                while (m.find()) { lastStart = m.start(); lastEnd = m.end(); }
                if (lastStart >= 0) select(lastStart, lastEnd);
            }
        } catch (BadLocationException ignored) {}
    }

    private void findFirst() {
        String search = searchField.getText();
        if (search.isEmpty()) return;
        try {
            String text = editor.getDocument().getText(0, editor.getDocument().getLength());
            Pattern p = buildPattern(search);
            Matcher m = p.matcher(text);
            if (m.find()) select(m.start(), m.end());
        } catch (BadLocationException ignored) {}
    }

    private void replaceCurrent() {
        if (editor.getSelectedText() == null) return;
        String replacement = replaceField.getText();
        if (grepMode.isSelected()) {
            try {
                Pattern p = buildPattern(searchField.getText());
                Matcher m = p.matcher(editor.getSelectedText());
                if (m.matches()) {
                    replacement = m.replaceFirst(replaceField.getText());
                }
            } catch (Exception ignored) {}
        }
        editor.replaceSelection(replacement);
    }

    private void replaceAll() {
        String search = searchField.getText();
        if (search.isEmpty()) return;
        try {
            String text = editor.getDocument().getText(0, editor.getDocument().getLength());
            Pattern p = buildPattern(search);
            String replacement = replaceField.getText();
            String result = p.matcher(text).replaceAll(grepMode.isSelected() ? replacement : Matcher.quoteReplacement(replacement));
            editor.setText(result);
        } catch (BadLocationException ignored) {}
    }

    private void select(int start, int end) {
        editor.setCaretPosition(start);
        editor.moveCaretPosition(end);
        editor.requestFocus();
    }
}
