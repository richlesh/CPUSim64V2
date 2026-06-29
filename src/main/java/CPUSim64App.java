// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2001-2026 Richard Lesh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.undo.UndoManager;
import javax.swing.undo.UndoableEdit;
import javax.swing.undo.AbstractUndoableEdit;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.nio.file.*;
import java.util.regex.*;

public class CPUSim64App {
    private static final boolean IS_MAC = System.getProperty("os.name").toLowerCase().contains("mac");
    private static final boolean IS_WINDOWS = System.getProperty("os.name").toLowerCase().contains("win");

    private JFrame frame;
    private JTextPane codeEditor;
    private AsmSyntaxHighlighter highlighter;
    private JTextPane console;
    private JTextField argsField;
    private Path currentFile;
    private javax.swing.Timer highlightTimer;
    private AppSettings settings;
    private UndoManager undoManager = new UndoManager();
    private boolean highlightingInProgress = false;
    private boolean undoInProgress = false;
    private final java.util.Deque<String> undoStack = new java.util.ArrayDeque<>();
    private final java.util.Deque<String> redoStack = new java.util.ArrayDeque<>();
    private static final int MAX_UNDO = 200;
    private String lastSavedText = "";
    private boolean modified = false;
    private JMenuItem saveItem;
    private JMenuItem undoItem, redoItem;
    private FindReplaceDialog findReplaceDialog;
    private AIChatPanel aiChatPanel;
    private File lastDirectory;
    private JSplitPane mainSplit;
    private LineNumberPanel lineNumberPanel;
    private JMenuBar menuBar;
    private JToolBar consoleToolBar;
    private JMenuItem runItem, debugItem;
    private JButton runBtn, debugBtn;
    private volatile Thread runThread;
    private volatile int consoleInputStart;

    public static void main(String[] args) {
        if (args.length > 0 && args[0].equals("--uninstall")) {
            CLIInstaller.uninstall();
            return;
        }
        if (!IS_WINDOWS && !IS_MAC) {
            checkXWayland();
        }
        SwingUtilities.invokeLater(() -> new CPUSim64App().createAndShowGUI());
    }

    void createAndShowGUI() {
        settings = AppSettings.load();
        if (!LicenseDialog.isLicensed(settings)) {
            SplashScreen.show();
        }
        frame = new JFrame("CPUSim64");
        frame.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        frame.addWindowListener(new WindowAdapter() {
            @Override public void windowClosing(WindowEvent e) {
                if (promptSaveIfNeeded()) frame.dispose();
            }
        });
        frame.setSize(1024, 768);
        var frameIconUrl = CPUSim64App.class.getResource("/app_icon_256.png");
        if (frameIconUrl != null) frame.setIconImage(new ImageIcon(frameIconUrl).getImage());

        frame.setJMenuBar(createMenuBar());
        frame.add(createMainPanel(), BorderLayout.CENTER);

        applySettings();

        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }

    private void applySettings() {
        Font font = new Font(settings.fontName, Font.PLAIN, settings.fontSize);
        codeEditor.setFont(font);
        console.setFont(font);
        // Update existing styled text in console
        javax.swing.text.StyledDocument consoleDoc = console.getStyledDocument();
        javax.swing.text.SimpleAttributeSet consoleAttr = new javax.swing.text.SimpleAttributeSet();
        javax.swing.text.StyleConstants.setFontFamily(consoleAttr, settings.fontName);
        javax.swing.text.StyleConstants.setFontSize(consoleAttr, settings.fontSize);
        consoleDoc.setCharacterAttributes(0, consoleDoc.getLength(), consoleAttr, false);
        // Update existing styled text in code editor
        javax.swing.text.StyledDocument editorDoc = codeEditor.getStyledDocument();
        javax.swing.text.SimpleAttributeSet editorAttr = new javax.swing.text.SimpleAttributeSet();
        javax.swing.text.StyleConstants.setFontFamily(editorAttr, settings.fontName);
        javax.swing.text.StyleConstants.setFontSize(editorAttr, settings.fontSize);
        editorDoc.setCharacterAttributes(0, editorDoc.getLength(), editorAttr, false);
        // Update AI chat panel font
        if (aiChatPanel != null) aiChatPanel.updateFont(settings.fontName, settings.fontSize);
        if (menuBar != null) {
            menuBar.setFont(font);
            for (int i = 0; i < menuBar.getMenuCount(); i++) {
                JMenu m = menuBar.getMenu(i);
                if (m != null) {
                    m.setFont(font);
                    for (int j = 0; j < m.getItemCount(); j++) {
                        JMenuItem item = m.getItem(j);
                        if (item != null) item.setFont(font);
                    }
                }
            }
            for (Component c : menuBar.getComponents()) c.setFont(font);
        }
        if (consoleToolBar != null) {
            for (Component c : consoleToolBar.getComponents()) c.setFont(font);
        }
        for (int i = 0; i < settings.colors.length; i++) {
            highlighter.setColor(i, settings.colors[i]);
        }
        highlighter.highlight();
    }

    private JMenuBar createMenuBar() {
        menuBar = new JMenuBar();

        // Application menu
        JMenu appMenu = new JMenu("CPUSim64");
        JMenuItem installItem = new JMenuItem("CLI Tools...");
        installItem.addActionListener(e -> {
            Icon appIcon = null;
            var url = CPUSim64App.class.getResource("/app_icon_256.png");
            if (url != null) appIcon = new ImageIcon(new ImageIcon(url).getImage().getScaledInstance(64, 64, Image.SCALE_SMOOTH));
            Object[] options = {"Install", "Remove", "Cancel"};
            int result = JOptionPane.showOptionDialog(frame,
                "Install or remove CPUSim64 command line tools?",
                "CLI Tools", JOptionPane.DEFAULT_OPTION, JOptionPane.QUESTION_MESSAGE,
                appIcon, options, "Install");
            if (result == 0) CLIInstaller.install();
            else if (result == 1) CLIInstaller.uninstall();
        });
        JMenuItem settingsItem = new JMenuItem("Settings");
        settingsItem.addActionListener(e -> {
            SettingsDialog.show(frame, codeEditor, console, highlighter, settings);
            applySettings();
            aiChatPanel.updateFont();
        });
        JMenuItem licenseItem = new JMenuItem("License Key");
        licenseItem.addActionListener(e -> LicenseDialog.show(frame, settings));
        JMenuItem aboutItem = new JMenuItem("About CPUSim64");
        aboutItem.addActionListener(e -> showAboutDialog());
        appMenu.add(aboutItem);
        appMenu.addSeparator();
        appMenu.add(installItem);
        appMenu.add(settingsItem);
        appMenu.add(licenseItem);
        appMenu.addSeparator();
        JMenuItem quitItem = new JMenuItem("Quit");
        quitItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Q, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        quitItem.addActionListener(e -> {
            for (Window w : Window.getWindows()) {
                if (w instanceof JFrame && w.isVisible()) {
                    w.dispatchEvent(new WindowEvent(w, WindowEvent.WINDOW_CLOSING));
                    if (w.isVisible()) return; // user cancelled
                }
            }
            System.exit(0);
        });
        appMenu.add(quitItem);

        // File menu
        JMenu fileMenu = new JMenu("File");
        JMenuItem newItem = new JMenuItem("New");
        newItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_N, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        newItem.addActionListener(e -> SwingUtilities.invokeLater(() -> new CPUSim64App().createAndShowGUI()));
        JMenuItem openItem = new JMenuItem("Open");
        openItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_O, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        openItem.addActionListener(e -> openFile());
        JMenuItem closeItem = new JMenuItem("Close");
        closeItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_W, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        closeItem.addActionListener(e -> { if (promptSaveIfNeeded()) frame.dispose(); });
        saveItem = new JMenuItem("Save");
        saveItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_S, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        saveItem.setEnabled(false);
        saveItem.addActionListener(e -> saveFile());
        JMenuItem saveAsItem = new JMenuItem("Save As...");
        saveAsItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_S, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx() | KeyEvent.SHIFT_DOWN_MASK));
        saveAsItem.addActionListener(e -> saveFileAs());
        runItem = new JMenuItem("Run");
        runItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_R, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        runItem.addActionListener(e -> runFile());
        fileMenu.add(newItem);
        fileMenu.add(openItem);
        fileMenu.add(closeItem);
        fileMenu.addSeparator();
        fileMenu.add(saveItem);
        fileMenu.add(saveAsItem);
        fileMenu.addSeparator();
        fileMenu.add(runItem);
        debugItem = new JMenuItem("Debug");
        debugItem.addActionListener(e -> launchDebugger());
        fileMenu.add(debugItem);

        // Edit menu
        JMenu editMenu = new JMenu("Edit");
        undoItem = new JMenuItem("Undo");
        undoItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Z, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        undoItem.addActionListener(e -> performUndo());
        undoItem.setEnabled(false);
        redoItem = new JMenuItem("Redo");
        redoItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Z, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx() | KeyEvent.SHIFT_DOWN_MASK));
        redoItem.addActionListener(e -> performRedo());
        JMenuItem cutItem = new JMenuItem("Cut");
        cutItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_X, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        cutItem.addActionListener(e -> codeEditor.cut());
        JMenuItem copyItem = new JMenuItem("Copy");
        copyItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_C, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        copyItem.addActionListener(e -> {
            if (codeEditor.getSelectedText() != null) codeEditor.copy();
            else if (console.getSelectedText() != null) console.copy();
        });
        JMenuItem pasteItem = new JMenuItem("Paste");
        pasteItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_V, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        pasteItem.addActionListener(e -> codeEditor.paste());
        editMenu.add(undoItem);
        editMenu.add(redoItem);
        editMenu.addSeparator();
        editMenu.add(cutItem);
        editMenu.add(copyItem);
        editMenu.add(pasteItem);
        editMenu.addSeparator();
        JMenuItem findItem = new JMenuItem("Find...");
        findItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        findItem.addActionListener(e -> {
            if (findReplaceDialog == null) findReplaceDialog = new FindReplaceDialog(frame, codeEditor);
            findReplaceDialog.show();
        });
        editMenu.add(findItem);
        editMenu.addSeparator();
        JMenuItem shiftLeftItem = new JMenuItem("Shift Selection Left");
        shiftLeftItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_OPEN_BRACKET, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        shiftLeftItem.addActionListener(e -> shiftIndent(false));
        editMenu.add(shiftLeftItem);
        JMenuItem shiftRightItem = new JMenuItem("Shift Selection Right");
        shiftRightItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_CLOSE_BRACKET, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        shiftRightItem.addActionListener(e -> shiftIndent(true));
        editMenu.add(shiftRightItem);

        // Help menu
        JMenu helpMenu = new JMenu("Help");
        JMenuItem onlineDocsItem = new JMenuItem("Online Documentation");
        onlineDocsItem.addActionListener(e -> {
            try { Desktop.getDesktop().browse(new java.net.URI("http://cpusim64.lesh.cloud/")); }
            catch (Exception ex) { ex.printStackTrace(); }
        });
        helpMenu.add(onlineDocsItem);

        menuBar.add(appMenu);
        menuBar.add(fileMenu);
        menuBar.add(editMenu);
        menuBar.add(helpMenu);
        menuBar.add(Box.createHorizontalGlue());
        var aiIconUrl = CPUSim64App.class.getResource("/AI.png");
        JButton aiBtn = new JButton(aiIconUrl != null
            ? new ImageIcon(new ImageIcon(aiIconUrl).getImage().getScaledInstance(24, 24, Image.SCALE_SMOOTH))
            : null);
        if (aiIconUrl == null) aiBtn.setText("AI");
        aiBtn.setToolTipText("Toggle AI Assistant");
        aiBtn.setBorderPainted(false);
        aiBtn.setFocusPainted(false);
        aiBtn.addActionListener(e -> toggleAIPanel());
        menuBar.add(aiBtn);
        return menuBar;
    }

    private JSplitPane createMainPanel() {
        codeEditor = new JTextPane();
        codeEditor.setFocusTraversalKeysEnabled(false);
        codeEditor.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_TAB, 0), "insert-spaces");
        codeEditor.getActionMap().put("insert-spaces", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) {
                try {
                    if (codeEditor.getSelectedText() != null) {
                        codeEditor.replaceSelection("");
                    }
                    int pos = codeEditor.getCaretPosition();
                    int lineStart = javax.swing.text.Utilities.getRowStart(codeEditor, pos);
                    int col = pos - lineStart;
                    int spaces = 4 - (col % 4);
                    codeEditor.getDocument().insertString(pos, " ".repeat(spaces), null);
                } catch (javax.swing.text.BadLocationException ignored) {}
            }
        });
        codeEditor.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_BACK_SPACE, 0), "smart-backspace");
        codeEditor.getActionMap().put("smart-backspace", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) {
                try {
                    if (codeEditor.getSelectedText() != null) {
                        codeEditor.replaceSelection("");
                        return;
                    }
                    int pos = codeEditor.getCaretPosition();
                    if (pos == 0) return;
                    int lineStart = javax.swing.text.Utilities.getRowStart(codeEditor, pos);
                    int col = pos - lineStart;
                    String lineText = codeEditor.getText(lineStart, col);
                    if (col > 0 && lineText.trim().isEmpty()) {
                        int target = ((col - 1) / 4) * 4;
                        int del = col - target;
                        codeEditor.getDocument().remove(pos - del, del);
                    } else {
                        codeEditor.getDocument().remove(pos - 1, 1);
                    }
                } catch (javax.swing.text.BadLocationException ignored) {}
            }
        });
        int mod = Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx();
        codeEditor.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_Z, mod), "safe-undo");
        codeEditor.getActionMap().put("safe-undo", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) { performUndo(); }
        });
        codeEditor.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_Z, mod | KeyEvent.SHIFT_DOWN_MASK), "safe-redo");
        codeEditor.getActionMap().put("safe-redo", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) { performRedo(); }
        });
        codeEditor.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 14));
        FontMetrics fm = codeEditor.getFontMetrics(codeEditor.getFont());
        int tabWidth = fm.charWidth(' ') * 4;
        javax.swing.text.TabStop[] tabs = new javax.swing.text.TabStop[64];
        for (int i = 0; i < tabs.length; i++) tabs[i] = new javax.swing.text.TabStop((i + 1) * tabWidth);
        javax.swing.text.TabSet tabSet = new javax.swing.text.TabSet(tabs);
        javax.swing.text.SimpleAttributeSet attr = new javax.swing.text.SimpleAttributeSet();
        javax.swing.text.StyleConstants.setTabSet(attr, tabSet);
        codeEditor.getStyledDocument().setParagraphAttributes(0, 0, attr, false);
        codeEditor.addPropertyChangeListener("font", evt -> {
            FontMetrics fm2 = codeEditor.getFontMetrics(codeEditor.getFont());
            int tw = fm2.charWidth(' ') * 4;
            javax.swing.text.TabStop[] ts = new javax.swing.text.TabStop[64];
            for (int i = 0; i < ts.length; i++) ts[i] = new javax.swing.text.TabStop((i + 1) * tw);
            javax.swing.text.SimpleAttributeSet a = new javax.swing.text.SimpleAttributeSet();
            javax.swing.text.StyleConstants.setTabSet(a, new javax.swing.text.TabSet(ts));
            codeEditor.getStyledDocument().setParagraphAttributes(0, codeEditor.getDocument().getLength(), a, false);
        });
        highlighter = new AsmSyntaxHighlighter(codeEditor);
        codeEditor.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                boolean modClick = IS_MAC ? e.isMetaDown() : e.isControlDown();
                if (modClick && e.getButton() == MouseEvent.BUTTON1 && e.getClickCount() == 1 && currentFile != null) {
                    int pos = codeEditor.viewToModel2D(e.getPoint());
                    if (pos < 0) return;
                    try {
                        String text = codeEditor.getDocument().getText(0, codeEditor.getDocument().getLength());
                        Pattern p = Pattern.compile("#[iI][nN][cC][lL][uU][dD][eE]\\s+[<\"](.+?)[>\"]");
                        Matcher m = p.matcher(text);
                        while (m.find()) {
                            if (pos >= m.start(1) && pos <= m.end(1)) {
                                String file = m.group(1);
                                openIncludeFile(file);
                                break;
                            }
                        }
                    } catch (javax.swing.text.BadLocationException ignored) {}
                }
            }
        });
        highlightTimer = new javax.swing.Timer(300, e -> {
            highlightingInProgress = true;
            highlighter.highlight();
            applyTabStops();
            highlightingInProgress = false;
        });
        highlightTimer.setRepeats(false);
        codeEditor.getDocument().addDocumentListener(new DocumentListener() {
            public void insertUpdate(DocumentEvent e) { highlightTimer.restart(); markModified(); }
            public void removeUpdate(DocumentEvent e) { highlightTimer.restart(); markModified(); }
            public void changedUpdate(DocumentEvent e) {}
        });
        codeEditor.getDocument().addDocumentListener(new DocumentListener() {
            public void insertUpdate(DocumentEvent e) { recordUndo(); }
            public void removeUpdate(DocumentEvent e) { recordUndo(); }
            public void changedUpdate(DocumentEvent e) {}
            private void recordUndo() {
                if (highlightingInProgress || undoInProgress) return;
                String current = codeEditor.getText();
                if (current.equals(lastSavedText)) return;
                undoStack.push(lastSavedText);
                if (undoStack.size() > MAX_UNDO) ((java.util.ArrayDeque<String>)undoStack).removeLast();
                redoStack.clear();
                lastSavedText = current;
                updateUndoRedo();
            }
        });
        JScrollPane editorScroll = new JScrollPane(codeEditor);
        lineNumberPanel = new LineNumberPanel(codeEditor);
        editorScroll.setRowHeaderView(lineNumberPanel);

        console = new JTextPane();
        console.setFont(new Font(settings.fontName, Font.PLAIN, settings.fontSize));
        console.setEditable(false);
        console.setBackground(new Color(30, 30, 30));
        console.setForeground(Color.WHITE);
        JScrollPane consoleScroll = new JScrollPane(console);

        consoleToolBar = new JToolBar();
        consoleToolBar.setFloatable(false);
        runBtn = new JButton("Run");
        runBtn.addActionListener(e -> runFile());
        debugBtn = new JButton("Debug");
        debugBtn.addActionListener(e -> launchDebugger());
        consoleToolBar.add(runBtn);
        consoleToolBar.add(debugBtn);
        consoleToolBar.addSeparator();
        consoleToolBar.add(new JLabel("Args: "));
        argsField = new JTextField(20);
        argsField.setMaximumSize(new Dimension(300, 24));
        consoleToolBar.add(argsField);
        consoleToolBar.addSeparator();
        consoleToolBar.add(new JLabel("Heap: "));
        JTextField heapField = new JTextField(String.valueOf(settings.heapSizeMiB), 5);
        heapField.setMaximumSize(new Dimension(70, 24));
        heapField.addFocusListener(new java.awt.event.FocusAdapter() {
            public void focusLost(java.awt.event.FocusEvent e) {
                try { settings.heapSizeMiB = Double.parseDouble(heapField.getText().trim()); settings.save(); } catch (NumberFormatException ignored) {}
            }
        });
        consoleToolBar.add(heapField);
        consoleToolBar.add(new JLabel("MiB "));
        consoleToolBar.addSeparator();
        consoleToolBar.add(new JLabel("Stack: "));
        JTextField stackField = new JTextField(String.valueOf(settings.stackSizeKiB), 5);
        stackField.setMaximumSize(new Dimension(70, 24));
        stackField.addFocusListener(new java.awt.event.FocusAdapter() {
            public void focusLost(java.awt.event.FocusEvent e) {
                try { settings.stackSizeKiB = Double.parseDouble(stackField.getText().trim()); settings.save(); } catch (NumberFormatException ignored) {}
            }
        });
        consoleToolBar.add(stackField);
        consoleToolBar.add(new JLabel("kiB"));

        JPanel consolePanel = new JPanel(new BorderLayout());
        consolePanel.add(consoleToolBar, BorderLayout.NORTH);
        consolePanel.add(consoleScroll, BorderLayout.CENTER);

        JSplitPane splitPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT, editorScroll, consolePanel);
        splitPane.setResizeWeight(0.7);

        aiChatPanel = new AIChatPanel(codeEditor, console, settings);
        aiChatPanel.setOnCodeChanged(() -> { modified = true; saveItem.setEnabled(true); });
        mainSplit = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, splitPane, aiChatPanel);
        mainSplit.setResizeWeight(1.0);
        aiChatPanel.setVisible(false);
        mainSplit.setDividerSize(0);
        return mainSplit;
    }

    private void toggleAIPanel() {
        boolean show = !aiChatPanel.isVisible();
        aiChatPanel.setVisible(show);
        mainSplit.setDividerSize(show ? 6 : 0);
        if (show) mainSplit.setDividerLocation(mainSplit.getWidth() - 400);
    }

    private void openFile() {
        if (!promptSaveIfNeeded()) return;
        JFileChooser chooser = new JFileChooser(lastDirectory != null ? lastDirectory : new File(System.getProperty("user.dir")));
        chooser.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("Assembly Files", "asm"));
        if (chooser.showOpenDialog(frame) == JFileChooser.APPROVE_OPTION) {
            lastDirectory = chooser.getSelectedFile().getParentFile();
            loadFile(chooser.getSelectedFile().toPath());
        }
    }

    void loadFile(Path path) {
        try {
            highlightingInProgress = true;
            String content = Files.readString(path);
            if (content.indexOf('\t') >= 0) {
                StringBuilder sb = new StringBuilder();
                int col = 0;
                for (int i = 0; i < content.length(); i++) {
                    char c = content.charAt(i);
                    if (c == '\t') {
                        int spaces = 4 - (col % 4);
                        sb.append(" ".repeat(spaces));
                        col += spaces;
                    } else if (c == '\n') {
                        sb.append(c);
                        col = 0;
                    } else {
                        sb.append(c);
                        col++;
                    }
                }
                content = sb.toString();
            }
            codeEditor.setText(content);
            codeEditor.setCaretPosition(0);
            highlightingInProgress = false;
            currentFile = path;
            frame.setTitle("CPUSim64 - " + path.getFileName());
            undoManager.discardAllEdits();
            undoStack.clear();
            redoStack.clear();
            lastSavedText = codeEditor.getText();
            modified = false;
            saveItem.setEnabled(false);
            updateUndoRedo();
            highlighter.highlight();
        } catch (IOException e) {
            highlightingInProgress = false;
            appendConsole("Error opening file: " + e.getMessage());
        }
    }

    private void saveFile() {
        if (currentFile == null) {
            saveFileAs();
            return;
        }
        try {
            Files.writeString(currentFile, codeEditor.getText());
            modified = false;
            saveItem.setEnabled(false);
        } catch (IOException e) {
            appendConsole("Error saving file: " + e.getMessage() + "\n");
        }
    }

    private void saveFileAs() {
        JFileChooser chooser = new JFileChooser(lastDirectory != null ? lastDirectory : new File(System.getProperty("user.dir")));
        chooser.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("Assembly Files", "asm"));
        if (currentFile != null) chooser.setSelectedFile(currentFile.toFile());
        if (chooser.showSaveDialog(frame) == JFileChooser.APPROVE_OPTION) {
            lastDirectory = chooser.getSelectedFile().getParentFile();
            currentFile = chooser.getSelectedFile().toPath();
            if (!currentFile.toString().endsWith(".asm")) {
                currentFile = Path.of(currentFile.toString() + ".asm");
            }
            saveFile();
            frame.setTitle("CPUSim64 - " + currentFile.getFileName());
        }
    }

    private void markModified() {
        if (!highlightingInProgress && !modified) {
            modified = true;
            saveItem.setEnabled(true);
        }
        updateUndoRedo();
    }

    private void updateUndoRedo() {
        undoItem.setEnabled(!undoStack.isEmpty());
        redoItem.setEnabled(!redoStack.isEmpty());
    }

    private void performUndo() {
        if (undoStack.isEmpty()) return;
        undoInProgress = true;
        redoStack.push(lastSavedText);
        lastSavedText = undoStack.pop();
        int caret = codeEditor.getCaretPosition();
        codeEditor.setText(lastSavedText);
        codeEditor.setCaretPosition(Math.min(caret, lastSavedText.length()));
        undoInProgress = false;
        updateUndoRedo();
        highlighter.highlight();
    }

    private void performRedo() {
        if (redoStack.isEmpty()) return;
        undoInProgress = true;
        undoStack.push(lastSavedText);
        lastSavedText = redoStack.pop();
        int caret = codeEditor.getCaretPosition();
        codeEditor.setText(lastSavedText);
        codeEditor.setCaretPosition(Math.min(caret, lastSavedText.length()));
        undoInProgress = false;
        updateUndoRedo();
        highlighter.highlight();
    }

    private void shiftIndent(boolean right) {
        try {
            int selStart = codeEditor.getSelectionStart();
            int selEnd = codeEditor.getSelectionEnd();
            if (selStart == selEnd) {
                // No selection — use current line
                selStart = javax.swing.text.Utilities.getRowStart(codeEditor, selStart);
                selEnd = javax.swing.text.Utilities.getRowEnd(codeEditor, selEnd);
            } else {
                selStart = javax.swing.text.Utilities.getRowStart(codeEditor, selStart);
                int rowEnd = javax.swing.text.Utilities.getRowEnd(codeEditor, selEnd - 1);
                selEnd = rowEnd;
            }
            String text = codeEditor.getText(selStart, selEnd - selStart);
            String[] lines = text.split("\n", -1);
            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < lines.length; i++) {
                if (right) {
                    sb.append("    ").append(lines[i]);
                } else {
                    String line = lines[i];
                    int remove = 0;
                    while (remove < 4 && remove < line.length() && line.charAt(remove) == ' ') remove++;
                    sb.append(line.substring(remove));
                }
                if (i < lines.length - 1) sb.append("\n");
            }
            codeEditor.select(selStart, selEnd);
            codeEditor.replaceSelection(sb.toString());
            codeEditor.select(selStart, selStart + sb.length());
        } catch (javax.swing.text.BadLocationException ignored) {}
    }
    private boolean promptSaveIfNeeded() {
        if (!modified) return true;
        int result = JOptionPane.showConfirmDialog(frame,
            "Do you want to save changes?", "CPUSim64",
            JOptionPane.YES_NO_CANCEL_OPTION, JOptionPane.WARNING_MESSAGE);
        if (result == JOptionPane.YES_OPTION) {
            saveFile();
            return !modified; // false if save was cancelled
        }
        return result == JOptionPane.NO_OPTION;
    }

    private void runFile() {
        if (runThread != null) {
            runThread.interrupt();
            return;
        }
        runWithMode(null);
    }

    private void runWithMode(String mode) {
        if (currentFile == null) {
            appendConsole("No file open. Open an .asm file first.\n");
            return;
        }
        if (modified && !promptSaveIfNeeded()) return;
        console.setText("");

        String base = currentFile.toString();
        if (base.endsWith(".asm")) base = base.substring(0, base.length() - 4);
        final String asmFile = base + ".asm";
        final String objFile = base + ".o64";

        new Thread(() -> {
            runThread = Thread.currentThread();
            SwingUtilities.invokeLater(() -> { runBtn.setText("Stop"); runItem.setText("Stop"); });
            PrintStream origOut = System.out;
            PrintStream origErr = System.err;
            InputStream origIn = System.in;
            try {
                PrintStream consoleStream = new PrintStream(new OutputStream() {
                    private final ByteArrayOutputStream buf = new ByteArrayOutputStream();
                    @Override public void write(int b) {
                        buf.write(b);
                        if (b == '\n' || b == '\r') flush();
                    }
                    @Override public void flush() {
                        if (buf.size() > 0) {
                            final String text = buf.toString(java.nio.charset.StandardCharsets.UTF_8);
                            buf.reset();
                            SwingUtilities.invokeLater(() -> appendConsole(text));
                        }
                    }
                }, true, java.nio.charset.StandardCharsets.UTF_8);
                System.setOut(consoleStream);
                System.setErr(consoleStream);

                // Set up input pipe
                PipedOutputStream inputPipe = new PipedOutputStream();
                PipedInputStream pis = new PipedInputStream(inputPipe);
                System.setIn(pis);

                // Allow console to accept input
                SwingUtilities.invokeLater(() -> {
                    console.setEditable(true);
                    console.addKeyListener(consoleKeyListener(inputPipe));
                });

                SwingUtilities.invokeLater(() -> appendConsole("> Assembling " + currentFile.getFileName() + "...\n"));
                int asmResult = Assembler.run(mode != null ? new String[]{asmFile, "--DEBUG"} : new String[]{asmFile});
                if (asmResult != 0) {
                    SwingUtilities.invokeLater(() -> appendConsole("\nAssembly failed.\n"));
                    return;
                }

                SwingUtilities.invokeLater(() -> appendConsole("> Running...\n"));
                java.util.List<String> simArgs = new java.util.ArrayList<>();
                simArgs.add(objFile);
                simArgs.add("--verbose");
                simArgs.add("--mem=" + (int)(settings.heapSizeMiB * 1024 * 1024));
                simArgs.add("--stack=" + (int)(settings.stackSizeKiB * 1024));
                if (mode != null) simArgs.add(mode);
                String userArgs = argsField.getText().trim();
                if (!userArgs.isEmpty()) {
                    for (String arg : userArgs.split("\\s+")) simArgs.add(arg);
                }
                Simulation.run(simArgs.toArray(new String[0]));

            } catch (Exception e) {
                SwingUtilities.invokeLater(() -> appendConsole("Error: " + e.getMessage() + "\n"));
            } finally {
                System.setOut(origOut);
                System.setErr(origErr);
                System.setIn(origIn);
                runThread = null;
                SwingUtilities.invokeLater(() -> {
                    runBtn.setText("Run"); runItem.setText("Run");
                    for (var kl : console.getKeyListeners()) console.removeKeyListener(kl);
                    console.setEditable(false);
                });
            }
        }).start();
    }

    private void launchDebugger() {
        if (currentFile == null) {
            appendConsole("No file open. Open an .asm file first.\n");
            return;
        }
        if (modified && !promptSaveIfNeeded()) return;
        console.setText("");

        String base = currentFile.toString();
        if (base.endsWith(".asm")) base = base.substring(0, base.length() - 4);
        final String asmFile = base + ".asm";
        final String objFile = base + ".o64";

        new Thread(() -> {
            try {
                // Assemble with --DEBUG
                SwingUtilities.invokeLater(() -> appendConsole("> Assembling (debug) " + currentFile.getFileName() + "...\n"));
                int asmResult = Assembler.run(new String[]{asmFile, "--DEBUG"});
                if (asmResult != 0) {
                    SwingUtilities.invokeLater(() -> appendConsole("\nAssembly failed.\n"));
                    return;
                }
                SwingUtilities.invokeLater(() -> {
                    appendConsole("> Opening debugger...\n");
                    runItem.setEnabled(false);
                    debugItem.setEnabled(false);
                    runBtn.setEnabled(false);
                    debugBtn.setEnabled(false);
                    Runnable onClose = () -> { runItem.setEnabled(true); debugItem.setEnabled(true); runBtn.setEnabled(true); debugBtn.setEnabled(true); };
                    new DebuggerWindow(frame, objFile, asmFile, lineNumberPanel, argsField.getText().trim(), settings, console, onClose);
                });
            } catch (Exception e) {
                SwingUtilities.invokeLater(() -> appendConsole("Error: " + e.getMessage() + "\n"));
            }
        }).start();
    }

    private void openIncludeFile(String file) {
        // Try local file first
        if (currentFile != null) {
            Path dir = currentFile.getParent();
            Path target = dir.resolve(file);
            if (Files.exists(target)) {
                SwingUtilities.invokeLater(() -> {
                    CPUSim64App app = new CPUSim64App();
                    app.createAndShowGUI();
                    app.loadFile(target);
                });
                return;
            }
        }
        // Try JAR resource
        InputStream is = CPUSim64App.class.getResourceAsStream("/" + file);
        if (is != null) {
            try {
                String content = new String(is.readAllBytes(), java.nio.charset.StandardCharsets.UTF_8);
                is.close();
                openReadOnlyViewer(file, content);
            } catch (IOException ignored) {}
        }
    }

    private void openReadOnlyViewer(String title, String content) {
        JFrame viewer = new JFrame("CPUSim64 - " + title + " (read-only)");
        viewer.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
        viewer.setSize(800, 600);

        JTextPane pane = new JTextPane();
        pane.setFont(new Font(Font.MONOSPACED, Font.PLAIN, settings.fontSize));
        pane.setEditable(false);
        pane.setText(content);
        pane.setCaretPosition(0);

        AsmSyntaxHighlighter hl = new AsmSyntaxHighlighter(pane);
        hl.highlight();

        // Set tab stops
        FontMetrics fm = pane.getFontMetrics(pane.getFont());
        int tw = fm.charWidth(' ') * 4;
        javax.swing.text.TabStop[] ts = new javax.swing.text.TabStop[64];
        for (int i = 0; i < ts.length; i++) ts[i] = new javax.swing.text.TabStop((i + 1) * tw);
        javax.swing.text.SimpleAttributeSet a = new javax.swing.text.SimpleAttributeSet();
        javax.swing.text.StyleConstants.setTabSet(a, new javax.swing.text.TabSet(ts));
        pane.getStyledDocument().setParagraphAttributes(0, pane.getDocument().getLength(), a, false);

        JScrollPane scroll = new JScrollPane(pane);
        scroll.setRowHeaderView(new LineNumberPanel(pane));
        viewer.add(scroll);
        viewer.setLocationRelativeTo(frame);
        viewer.setVisible(true);
    }

    private void applyTabStops() {
        FontMetrics fm = codeEditor.getFontMetrics(codeEditor.getFont());
        int tw = fm.charWidth(' ') * 4;
        javax.swing.text.TabStop[] ts = new javax.swing.text.TabStop[64];
        for (int i = 0; i < ts.length; i++) ts[i] = new javax.swing.text.TabStop((i + 1) * tw);
        javax.swing.text.SimpleAttributeSet a = new javax.swing.text.SimpleAttributeSet();
        javax.swing.text.StyleConstants.setTabSet(a, new javax.swing.text.TabSet(ts));
        codeEditor.getStyledDocument().setParagraphAttributes(0, codeEditor.getDocument().getLength(), a, false);
    }

    private void showAboutDialog() {
        JDialog dialog = new JDialog(frame, "About CPUSim64", true);
        dialog.setUndecorated(true);
        dialog.getRootPane().setBorder(BorderFactory.createLineBorder(new Color(80, 80, 80), 1));

        JPanel panel = new JPanel();
        panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
        panel.setBackground(new Color(40, 40, 40));
        panel.setBorder(BorderFactory.createEmptyBorder(30, 50, 30, 50));

        // Icon
        var iconUrl = CPUSim64App.class.getResource("/app_icon_256.png");
        if (iconUrl != null) {
            JLabel iconLabel = new JLabel(new ImageIcon(new ImageIcon(iconUrl)
                .getImage().getScaledInstance(96, 96, Image.SCALE_SMOOTH)));
            iconLabel.setAlignmentX(Component.CENTER_ALIGNMENT);
            panel.add(iconLabel);
        }
        panel.add(Box.createVerticalStrut(14));

        JLabel name = new JLabel("CPUSim64");
        name.setFont(new Font(Font.SANS_SERIF, Font.BOLD, 20));
        name.setForeground(Color.WHITE);
        name.setAlignmentX(Component.CENTER_ALIGNMENT);
        panel.add(name);
        panel.add(Box.createVerticalStrut(10));

        JLabel ver = new JLabel("Version " + cloud.lesh.CPUSim64.BuildInfo.VERSION);
        ver.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 14));
        ver.setForeground(new Color(180, 180, 180));
        ver.setAlignmentX(Component.CENTER_ALIGNMENT);
        panel.add(ver);
        panel.add(Box.createVerticalStrut(4));

        JLabel copy = new JLabel("\u00a92026 Richard Lesh");
        copy.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 14));
        copy.setForeground(new Color(180, 180, 180));
        copy.setAlignmentX(Component.CENTER_ALIGNMENT);
        panel.add(copy);
        panel.add(Box.createVerticalStrut(12));

        JLabel link1 = new JLabel("<html><a style='color:#4da3ff;'>Glowing Cat Software</a></html>");
        link1.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 14));
        link1.setHorizontalAlignment(SwingConstants.CENTER);
        link1.setAlignmentX(Component.CENTER_ALIGNMENT);
        link1.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        link1.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                try { Desktop.getDesktop().browse(java.net.URI.create("https://glowingcatsoftware.com")); }
                catch (Exception ignored) {}
            }
        });
        panel.add(link1);
        panel.add(Box.createVerticalStrut(4));

        JLabel link2 = new JLabel("<html><a style='color:#4da3ff;'>Report issues on GitHub</a></html>");
        link2.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 14));
        link2.setHorizontalAlignment(SwingConstants.CENTER);
        link2.setAlignmentX(Component.CENTER_ALIGNMENT);
        link2.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        link2.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                try { Desktop.getDesktop().browse(java.net.URI.create("https://github.com/richlesh/CPUSim64V2/issues")); }
                catch (Exception ignored) {}
            }
        });
        panel.add(link2);
        panel.add(Box.createVerticalStrut(18));

        JButton okBtn = new JButton("OK");
        okBtn.setFont(new Font(Font.SANS_SERIF, Font.BOLD, 14));
        okBtn.setAlignmentX(Component.CENTER_ALIGNMENT);
        okBtn.addActionListener(ev -> dialog.dispose());
        panel.add(okBtn);

        if (LicenseDialog.isLicensed(settings)) {
            panel.add(Box.createVerticalStrut(14));
            JLabel thanks = new JLabel("<html><p style='text-align: center;'><b>Thank you for purchasing a<br>license for CPUSim64!</b></p></html>");
            thanks.setFont(new Font(Font.SANS_SERIF, Font.BOLD, 13));
            thanks.setForeground(new Color(100, 200, 100));
            thanks.setHorizontalAlignment(SwingConstants.CENTER);
            thanks.setAlignmentX(Component.CENTER_ALIGNMENT);
            panel.add(thanks);
        }

        dialog.setContentPane(panel);
        dialog.pack();
        dialog.setLocationRelativeTo(frame);
        dialog.setVisible(true);
    }

    private Color consoleCurrentColor = Color.WHITE;
    private String consolePending = "";

    private void appendConsole(String text) {
        text = consolePending + text;
        consolePending = "";
        // If text ends with an incomplete ANSI sequence, buffer it
        int escIdx = text.lastIndexOf('\u001B');
        if (escIdx >= 0 && !text.substring(escIdx).matches("\u001B\\[[0-9;]*m")) {
            consolePending = text.substring(escIdx);
            text = text.substring(0, escIdx);
        }
        if (text.isEmpty()) return;
        javax.swing.text.StyledDocument doc = console.getStyledDocument();
        java.util.regex.Matcher m = java.util.regex.Pattern.compile("(\r|\u001B\\[[0-9;]*m|[^\r\u001B]+|\u001B)").matcher(text);
        while (m.find()) {
            String seg = m.group();
            if (seg.equals("\r")) {
                // Carriage return: remove from current position back to last newline
                try {
                    String docText = doc.getText(0, doc.getLength());
                    int lastNl = docText.lastIndexOf('\n');
                    int lineStart = lastNl + 1;
                    if (lineStart < doc.getLength()) {
                        doc.remove(lineStart, doc.getLength() - lineStart);
                    }
                    consoleInputStart = doc.getLength();
                } catch (javax.swing.text.BadLocationException ignored) {}
            } else if (seg.startsWith("\u001B[") && seg.endsWith("m")) {
                // ANSI color code
                consoleCurrentColor = ansiToColor(seg);
            } else if (seg.equals("\u001B")) {
                // Lone escape — ignore
            } else {
                try {
                    javax.swing.text.SimpleAttributeSet attr = new javax.swing.text.SimpleAttributeSet();
                    javax.swing.text.StyleConstants.setForeground(attr, consoleCurrentColor);
                    javax.swing.text.StyleConstants.setFontFamily(attr, settings.fontName);
                    javax.swing.text.StyleConstants.setFontSize(attr, settings.fontSize);
                    doc.insertString(doc.getLength(), seg, attr);
                } catch (javax.swing.text.BadLocationException ignored) {}
            }
        }
        console.setCaretPosition(doc.getLength());
        consoleInputStart = doc.getLength();
    }

    private Color ansiToColor(String code) {
        String nums = code.substring(2, code.length() - 1);
        if (nums.isEmpty() || nums.equals("0")) return Color.WHITE;
        for (String n : nums.split(";")) {
            switch (n) {
                case "0": return Color.WHITE;
                case "30": return Color.DARK_GRAY;
                case "31": return new Color(200, 50, 50);
                case "32": return new Color(50, 200, 50);
                case "33": return new Color(200, 200, 50);
                case "34": return new Color(80, 80, 255);
                case "35": return new Color(200, 50, 200);
                case "36": return new Color(50, 200, 200);
                case "37": return Color.WHITE;
                case "90": return Color.GRAY;
                case "91": return new Color(255, 100, 100);
                case "92": return new Color(100, 255, 100);
                case "93": return new Color(255, 255, 100);
                case "94": return new Color(130, 130, 255);
                case "95": return new Color(255, 100, 255);
                case "96": return new Color(100, 255, 255);
                case "97": return Color.WHITE;
            }
        }
        return consoleCurrentColor;
    }

    private java.awt.event.KeyListener consoleKeyListener(PipedOutputStream pipe) {
        return new java.awt.event.KeyAdapter() {
            @Override public void keyPressed(java.awt.event.KeyEvent e) {
                // Prevent editing before the input start position
                if (console.getCaretPosition() < consoleInputStart && e.getKeyCode() != KeyEvent.VK_ENTER) {
                    console.setCaretPosition(console.getDocument().getLength());
                }
                if (e.getKeyCode() == KeyEvent.VK_D && (e.getModifiersEx() & KeyEvent.CTRL_DOWN_MASK) != 0) {
                    e.consume();
                    try { pipe.close(); } catch (Exception ignored) {}
                    return;
                }
                if (e.getKeyCode() == KeyEvent.VK_C && (e.getModifiersEx() & KeyEvent.CTRL_DOWN_MASK) != 0) {
                    e.consume();
                    if (runThread != null) runThread.interrupt();
                    return;
                }
                if (e.getKeyCode() == KeyEvent.VK_ENTER) {
                    e.consume();
                    try {
                        int len = console.getDocument().getLength();
                        String input = console.getDocument().getText(consoleInputStart, len - consoleInputStart) + "\n";
                        appendConsole("\n");
                        pipe.write(input.getBytes(java.nio.charset.StandardCharsets.UTF_8));
                        pipe.flush();
                        consoleInputStart = console.getDocument().getLength();
                    } catch (Exception ignored) {}
                }
            }
            @Override public void keyTyped(java.awt.event.KeyEvent e) {
                if (console.getCaretPosition() < consoleInputStart) {
                    console.setCaretPosition(console.getDocument().getLength());
                }
            }
        };
    }

    private static void checkXWayland() {
        String display = System.getenv("DISPLAY");
        if (display == null || display.isEmpty()) {
            System.err.println("Error: CPUSim64 requires an X11 display (DISPLAY not set).");
            System.err.println("If running Wayland, install XWayland: sudo apt install xwayland");
            System.exit(1);
        }
    }
}
