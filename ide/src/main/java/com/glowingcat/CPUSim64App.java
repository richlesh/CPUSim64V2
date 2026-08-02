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
package com.glowingcat;

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
import cloud.lesh.CPUSim64.StdInterruptHandler;
import com.glowingcat.aichat.AIChatPanel;
import com.glowingcat.aichat.AIChatPreferences;
import com.glowingcat.aichat.AIChatPreferencesDialog;
import com.glowingcat.aichat.DocumentEditor;
import org.fife.ui.rsyntaxtextarea.*;
import org.fife.ui.rtextarea.*;

public class CPUSim64App {
    private static final boolean IS_MAC = System.getProperty("os.name").toLowerCase().contains("mac");
    private static final boolean IS_WINDOWS = System.getProperty("os.name").toLowerCase().contains("win");

    private JFrame frame;
    private RSyntaxTextArea codeEditor;
    private EditorPanel editorPanel;
    private TerminalPanel console;
    private JTextField argsField;
    private Path currentFile;
    private AppSettings settings;
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
    private AIChatPreferences aiPreferences;
    private File lastDirectory;
    private JSplitPane mainSplit;
    private JMenuBar menuBar;
    private JToolBar consoleToolBar;
    private JMenuItem runItem, debugItem;
    private JCheckBoxMenuItem aiMenuItem;
    private JButton runBtn, debugBtn;
    private volatile Thread runThread;
    private int runDebugCount = 0;

    public static void main(String[] args) {
        if (args.length > 0 && args[0].equals("--uninstall")) {
            CLIInstaller.uninstall();
            return;
        }
        if (!IS_WINDOWS && !IS_MAC) {
            checkXWayland();
        }
        // On macOS, use the system menu bar at the top of the screen
        if (IS_MAC) {
            System.setProperty("apple.laf.useScreenMenuBar", "true");
            System.setProperty("apple.awt.application.name", "CPUSim64");
        }
        // Use native look and feel for each platform
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception ignored) {}
        SwingUtilities.invokeLater(() -> new CPUSim64App().createAndShowGUI());
    }

    void createAndShowGUI() {
        settings = AppSettings.load();
        aiPreferences = AIChatPreferences.load();
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
        editorPanel.setEditorFont(font);
        console.setFont(settings.fontName, settings.fontSize);
        console.setColors(settings.consoleFg, settings.consoleBg);
        if (consoleToolBar != null) {
            for (Component c : consoleToolBar.getComponents()) c.setFont(font);
        }
        // Apply syntax colors to RSyntaxTextArea scheme
        SyntaxScheme scheme = codeEditor.getSyntaxScheme();
        scheme.getStyle(Token.IDENTIFIER).foreground = settings.colors[0]; // Normal
        scheme.getStyle(Token.RESERVED_WORD).foreground = settings.colors[1]; // Keywords
        scheme.getStyle(Token.RESERVED_WORD).font = font.deriveFont(Font.BOLD);
        scheme.getStyle(Token.FUNCTION).foreground = settings.colors[2]; // Directives
        scheme.getStyle(Token.FUNCTION).font = font.deriveFont(Font.BOLD);
        scheme.getStyle(Token.COMMENT_EOL).foreground = settings.colors[3]; // Comments
        scheme.getStyle(Token.COMMENT_MULTILINE).foreground = settings.colors[3];
        scheme.getStyle(Token.LITERAL_STRING_DOUBLE_QUOTE).foreground = settings.colors[4]; // Strings
        scheme.getStyle(Token.LITERAL_CHAR).foreground = settings.colors[4];
        scheme.getStyle(Token.LITERAL_NUMBER_DECIMAL_INT).foreground = settings.colors[5]; // Numbers
        scheme.getStyle(Token.LITERAL_NUMBER_HEXADECIMAL).foreground = settings.colors[5];
        scheme.getStyle(Token.RESERVED_WORD_2).foreground = settings.colors[6]; // Registers
        scheme.getStyle(Token.VARIABLE).foreground = settings.colors[7]; // Labels
        scheme.getStyle(Token.VARIABLE).font = font.deriveFont(Font.BOLD);
        scheme.getStyle(Token.DATA_TYPE).foreground = settings.colors[8]; // Conditions
        scheme.getStyle(Token.PREPROCESSOR).foreground = settings.colors[2]; // PP directives same as directives
        scheme.getStyle(Token.PREPROCESSOR).font = font.deriveFont(Font.BOLD);
        codeEditor.setSyntaxScheme(scheme);
        codeEditor.setForeground(settings.colors[0]);
        codeEditor.revalidate();
        codeEditor.repaint();
        // Update AI chat panel font
        if (aiChatPanel != null) aiChatPanel.updateFont();
    }

    private JMenuBar createMenuBar() {
        menuBar = new JMenuBar();

        // Shared action handlers
        Runnable aboutAction = this::showAboutDialog;
        Runnable settingsAction = () -> {
            SettingsDialog.show(frame, codeEditor, console, settings);
            applySettings();
        };
        Runnable installAction = () -> {
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
        };
        Runnable licenseAction = () -> LicenseDialog.show(frame, settings);
        Runnable quitAction = () -> {
            for (Window w : Window.getWindows()) {
                if (w instanceof JFrame && w.isVisible()) {
                    w.dispatchEvent(new WindowEvent(w, WindowEvent.WINDOW_CLOSING));
                    if (w.isVisible()) return; // user cancelled
                }
            }
            System.exit(0);
        };

        if (IS_MAC) {
            // Register About and Settings with the macOS Apple menu
            Desktop desktop = Desktop.getDesktop();
            desktop.setAboutHandler(e -> aboutAction.run());
            desktop.setPreferencesHandler(e -> settingsAction.run());
            desktop.setQuitHandler((e, response) -> {
                quitAction.run();
                response.cancelQuit();
            });
        } else {
            // Windows/Linux: CPUSim64 application menu
            JMenu appMenu = new JMenu("CPUSim64");
            JMenuItem aboutItem = new JMenuItem("About CPUSim64");
            aboutItem.addActionListener(e -> aboutAction.run());
            appMenu.add(aboutItem);
            appMenu.addSeparator();
            JMenuItem installItem = new JMenuItem("CLI Tools...");
            installItem.addActionListener(e -> installAction.run());
            appMenu.add(installItem);
            JMenuItem settingsItem = new JMenuItem("Settings");
            settingsItem.addActionListener(e -> settingsAction.run());
            appMenu.add(settingsItem);
            JMenuItem licenseItem = new JMenuItem("License Key");
            licenseItem.addActionListener(e -> licenseAction.run());
            appMenu.add(licenseItem);
            appMenu.addSeparator();
            JMenuItem quitItem = new JMenuItem("Quit CPUSim64");
            quitItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Q, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
            quitItem.addActionListener(e -> quitAction.run());
            appMenu.add(quitItem);
            menuBar.add(appMenu);
        }

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

        // On macOS, put CLI Tools and License Key in the File menu
        if (IS_MAC) {
            fileMenu.addSeparator();
            JMenuItem installItem = new JMenuItem("CLI Tools...");
            installItem.addActionListener(e -> installAction.run());
            fileMenu.add(installItem);
            JMenuItem licenseItem = new JMenuItem("License Key");
            licenseItem.addActionListener(e -> licenseAction.run());
            fileMenu.add(licenseItem);
        }

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
            else {
                String sel = console.getSelectedText();
                if (sel != null && !sel.isEmpty()) {
                    Toolkit.getDefaultToolkit().getSystemClipboard().setContents(new java.awt.datatransfer.StringSelection(sel), null);
                }
            }
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
        helpMenu.addSeparator();
        JMenuItem aiSettingsItem = new JMenuItem("AI Chat Settings...");
        aiSettingsItem.addActionListener(e -> showAIChatSettings());
        helpMenu.add(aiSettingsItem);
        aiMenuItem = new JCheckBoxMenuItem("AI Assistant");
        aiMenuItem.addActionListener(e -> toggleAIPanel());
        helpMenu.add(aiMenuItem);

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
        editorPanel = new EditorPanel();
        codeEditor = editorPanel.getTextArea();
        codeEditor.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 14));

        // Cmd+click (macOS) / Ctrl+click to navigate #include files
        codeEditor.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                boolean modClick = IS_MAC ? e.isMetaDown() : e.isControlDown();
                if (modClick && e.getButton() == MouseEvent.BUTTON1 && e.getClickCount() == 1 && currentFile != null) {
                    int pos = codeEditor.viewToModel2D(e.getPoint());
                    if (pos < 0) return;
                    String text = codeEditor.getText();
                    Pattern p = Pattern.compile("#[iI][nN][cC][lL][uU][dD][eE]\\s+[<\"](.+?)[>\"]");
                    Matcher m = p.matcher(text);
                    while (m.find()) {
                        if (pos >= m.start(1) && pos <= m.end(1)) {
                            String file = m.group(1);
                            openIncludeFile(file);
                            break;
                        }
                    }
                }
            }
        });

        // Undo/redo key bindings
        int mod = Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx();
        codeEditor.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_Z, mod), "safe-undo");
        codeEditor.getActionMap().put("safe-undo", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) { performUndo(); }
        });
        codeEditor.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_Z, mod | KeyEvent.SHIFT_DOWN_MASK), "safe-redo");
        codeEditor.getActionMap().put("safe-redo", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) { performRedo(); }
        });

        // Shift indent key bindings
        codeEditor.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_OPEN_BRACKET, mod), "shift-left");
        codeEditor.getActionMap().put("shift-left", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) { shiftIndent(false); }
        });
        codeEditor.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_CLOSE_BRACKET, mod), "shift-right");
        codeEditor.getActionMap().put("shift-right", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) { shiftIndent(true); }
        });

        codeEditor.getDocument().addDocumentListener(new javax.swing.event.DocumentListener() {
            public void insertUpdate(javax.swing.event.DocumentEvent e) { markModified(); recordUndo(); }
            public void removeUpdate(javax.swing.event.DocumentEvent e) { markModified(); recordUndo(); }
            public void changedUpdate(javax.swing.event.DocumentEvent e) {}
            private void recordUndo() {
                if (undoInProgress) return;
                String current = codeEditor.getText();
                if (current.equals(lastSavedText)) return;
                undoStack.push(lastSavedText);
                if (undoStack.size() > MAX_UNDO) ((java.util.ArrayDeque<String>)undoStack).removeLast();
                redoStack.clear();
                lastSavedText = current;
                updateUndoRedo();
            }
        });
        console = new TerminalPanel(settings.fontName, settings.fontSize);
        console.setColors(settings.consoleFg, settings.consoleBg);
        JScrollBar consoleScrollBar = new JScrollBar(JScrollBar.VERTICAL);
        console.attachScrollBar(consoleScrollBar);
        JPanel consoleScrollPanel = new JPanel(new BorderLayout());
        consoleScrollPanel.add(console, BorderLayout.CENTER);
        consoleScrollPanel.add(consoleScrollBar, BorderLayout.EAST);

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
        Font toolbarFont = new Font(settings.fontName, Font.PLAIN, settings.fontSize);
        for (Component c : consoleToolBar.getComponents()) c.setFont(toolbarFont);

        JPanel consolePanel = new JPanel(new BorderLayout());
        consolePanel.add(consoleToolBar, BorderLayout.NORTH);
        consolePanel.add(consoleScrollPanel, BorderLayout.CENTER);

        JSplitPane splitPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT, editorPanel, consolePanel);
        splitPane.setResizeWeight(0.7);

        var retriever = new com.glowingcat.aichat.DocumentRetriever(
            java.nio.file.Path.of(System.getProperty("user.home"), ".cpusim64"),
            java.util.List.of(
                "/documentation/doc-index.txt",
                "/documentation/examples-index.txt",
                "/documentation/projects-index.txt"
            ),
            30
        );

        aiChatPanel = AIChatPanel.builder()
            .editor(new DocumentEditor() {
                @Override public String getText() { return codeEditor.getText(); }
                @Override public void setText(String text) {
                    codeEditor.setText(text);
                    modified = true;
                    saveItem.setEnabled(true);
                }
            })
            .preferences(aiPreferences)
            .documentRetriever(retriever)
            .contextProvider("Console output (truncated)", () -> {
                String text = console.getText();
                if (text.isEmpty()) return null;
                // Limit to last 100 lines
                String[] lines = text.split("\n", -1);
                if (lines.length > 100) {
                    text = String.join("\n", java.util.Arrays.copyOfRange(lines, lines.length - 100, lines.length));
                }
                // Limit to last 2000 characters
                if (text.length() > 2000) {
                    text = text.substring(text.length() - 2000);
                }
                return text;
            })
            .build();
        mainSplit = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, splitPane, aiChatPanel);
        mainSplit.setResizeWeight(1.0);
        aiChatPanel.setVisible(false);
        mainSplit.setDividerSize(0);
        return mainSplit;
    }

    private void showAIChatSettings() {
        AIChatPreferencesDialog dlg = new AIChatPreferencesDialog(frame, aiPreferences);
        dlg.setVisible(true);
        if (dlg.isConfirmed()) {
            dlg.applyTo(aiPreferences);
            aiPreferences.setUserPromptColor(dlg.getSelectedUserPromptColor());
            aiPreferences.setUserTextColor(dlg.getSelectedUserTextColor());
            aiPreferences.setAiResponseColor(dlg.getSelectedAiResponseColor());
            aiPreferences.setAiTextColor(dlg.getSelectedAiTextColor());
            aiPreferences.save();
            if (aiChatPanel != null) aiChatPanel.updateFont();
        }
    }

    private void toggleAIPanel() {
        boolean show = !aiChatPanel.isVisible();
        aiChatPanel.setVisible(show);
        mainSplit.setDividerSize(show ? 6 : 0);
        if (show) mainSplit.setDividerLocation(mainSplit.getWidth() - 400);
        if (aiMenuItem != null) aiMenuItem.setSelected(show);
    }

    private void openFile() {
        if (!promptSaveIfNeeded()) return;
        FileDialog fd = new FileDialog(frame, "Open Assembly File", FileDialog.LOAD);
        if (lastDirectory != null) fd.setDirectory(lastDirectory.getAbsolutePath());
        else fd.setDirectory(System.getProperty("user.dir"));
        fd.setFilenameFilter((dir, name) -> name.endsWith(".asm"));
        fd.setVisible(true);
        if (fd.getFile() != null) {
            lastDirectory = new File(fd.getDirectory());
            loadFile(Path.of(fd.getDirectory(), fd.getFile()));
        }
    }

    void loadFile(Path path) {
        console.clear();
        try {
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
            undoInProgress = true;
            codeEditor.setText(content);
            codeEditor.setCaretPosition(0);
            undoInProgress = false;
            currentFile = path;
            frame.setTitle("CPUSim64 - " + path.getFileName());
            undoStack.clear();
            redoStack.clear();
            lastSavedText = codeEditor.getText();
            modified = false;
            saveItem.setEnabled(false);
            updateUndoRedo();
        } catch (IOException e) {
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
        FileDialog fd = new FileDialog(frame, "Save Assembly File", FileDialog.SAVE);
        if (lastDirectory != null) fd.setDirectory(lastDirectory.getAbsolutePath());
        else fd.setDirectory(System.getProperty("user.dir"));
        if (currentFile != null) fd.setFile(currentFile.getFileName().toString());
        else fd.setFile("untitled.asm");
        fd.setFilenameFilter((dir, name) -> name.endsWith(".asm"));
        fd.setVisible(true);
        if (fd.getFile() != null) {
            lastDirectory = new File(fd.getDirectory());
            currentFile = Path.of(fd.getDirectory(), fd.getFile());
            if (!currentFile.toString().endsWith(".asm")) {
                currentFile = Path.of(currentFile.toString() + ".asm");
            }
            saveFile();
            frame.setTitle("CPUSim64 - " + currentFile.getFileName());
        }
    }

    private void markModified() {
        if (!undoInProgress && !modified) {
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
    }

    private void shiftIndent(boolean right) {
        try {
            int selStart = codeEditor.getSelectionStart();
            int selEnd = codeEditor.getSelectionEnd();
            if (selStart == selEnd) {
                // No selection — use current line
                int line = codeEditor.getLineOfOffset(selStart);
                selStart = codeEditor.getLineStartOffset(line);
                selEnd = codeEditor.getLineEndOffset(line);
                if (selEnd > selStart && codeEditor.getText().charAt(selEnd - 1) == '\n') selEnd--;
            } else {
                int startLine = codeEditor.getLineOfOffset(selStart);
                int endLine = codeEditor.getLineOfOffset(selEnd - 1);
                selStart = codeEditor.getLineStartOffset(startLine);
                selEnd = codeEditor.getLineEndOffset(endLine);
                if (selEnd > selStart && codeEditor.getText().charAt(selEnd - 1) == '\n') selEnd--;
            }
            String text = codeEditor.getText().substring(selStart, selEnd);
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

        // Show splash screen every 10 Run/Debug actions when not licensed
        runDebugCount++;
        if (runDebugCount % 10 == 0 && !LicenseDialog.isLicensed(settings)) {
            SplashScreen.show();
        }

        console.clear();

        String base = currentFile.toString();
        if (base.endsWith(".asm")) base = base.substring(0, base.length() - 4);
        final String asmFile = base + ".asm";
        final String baseName = Path.of(base).getFileName().toString();
        final Path asmDir = Path.of(base).getParent();
        final Path objDir = asmDir.resolve("obj");

        new Thread(() -> {
            runThread = Thread.currentThread();
            console.setInterruptHandler(() -> { if (runThread != null) runThread.interrupt(); });
            SwingUtilities.invokeLater(() -> { runBtn.setText("Stop"); runItem.setText("Stop"); });
            PrintStream origOut = System.out;
            PrintStream origErr = System.err;
            InputStream origIn = System.in;
            final StringBuilder consoleBuf = new StringBuilder();
            final Object consoleBufLock = new Object();
            javax.swing.Timer consoleFlushTimer = new javax.swing.Timer(50, evt -> {
                String chunk;
                synchronized (consoleBufLock) {
                    if (consoleBuf.length() == 0) return;
                    chunk = consoleBuf.toString();
                    consoleBuf.setLength(0);
                }
                appendConsole(chunk);
            });
            consoleFlushTimer.setRepeats(true);
            consoleFlushTimer.start();
            try {

                PrintStream consoleStream = new PrintStream(new OutputStream() {
                    @Override public void write(int b) {
                        synchronized (consoleBufLock) { consoleBuf.append((char) b); }
                    }
                    @Override public void write(byte[] buf, int off, int len) {
                        synchronized (consoleBufLock) { consoleBuf.append(new String(buf, off, len, java.nio.charset.StandardCharsets.UTF_8)); }
                    }
                    @Override public void flush() {}
                }, true, java.nio.charset.StandardCharsets.UTF_8);
                System.setOut(consoleStream);
                System.setErr(consoleStream);

                // Set up input pipe
                PipedOutputStream inputPipe = new PipedOutputStream();
                PipedInputStream pis = new PipedInputStream(inputPipe);
                System.setIn(pis);

                // Allow console to accept input
                SwingUtilities.invokeLater(() -> console.enableInput(inputPipe));

                SwingUtilities.invokeLater(() -> appendConsole("> Assembling " + currentFile.getFileName() + "...\n"));
                int asmResult = Assembler.run(mode != null ? new String[]{asmFile, "--DEBUG"} : new String[]{asmFile});
                if (asmResult != 0) {
                    SwingUtilities.invokeLater(() -> appendConsole("\nAssembly failed.\n"));
                    return;
                }

                // Create obj directory and move output files there
                Files.createDirectories(objDir);
                String[] extensions = {".o64", ".sym", ".sym1", ".sym2", ".srcmap"};
                for (String ext : extensions) {
                    Path src = asmDir.resolve(baseName + ext);
                    if (Files.exists(src)) {
                        Files.move(src, objDir.resolve(baseName + ext), java.nio.file.StandardCopyOption.REPLACE_EXISTING);
                    }
                }

                // Resolve the .o64 file: check obj directory first, fall back to source directory
                final String objFile;
                Path objInDir = objDir.resolve(baseName + ".o64");
                if (Files.exists(objInDir)) {
                    objFile = objInDir.toString();
                } else if (Files.exists(asmDir.resolve(baseName + ".o64"))) {
                    objFile = asmDir.resolve(baseName + ".o64").toString();
                } else {
                    SwingUtilities.invokeLater(() -> appendConsole("\nNo .o64 file found after assembly.\n"));
                    return;
                }

                SwingUtilities.invokeLater(() -> appendConsole("> Running...\n"));
                // Provide terminal dimensions to running programs
                StdInterruptHandler.setGlobalTerminalSizeProvider(new StdInterruptHandler.TerminalSizeProvider() {
                    @Override public int getColumns() { return console.getVisibleCols(); }
                    @Override public int getRows() { return console.getVisibleRows(); }
                });
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
                consoleFlushTimer.stop();
                // Final flush
                synchronized (consoleBufLock) {
                    if (consoleBuf.length() > 0) {
                        String remaining = consoleBuf.toString();
                        consoleBuf.setLength(0);
                        SwingUtilities.invokeLater(() -> appendConsole(remaining));
                    }
                }
                System.setOut(origOut);
                System.setErr(origErr);
                System.setIn(origIn);
                runThread = null;
                SwingUtilities.invokeLater(() -> {
                    runBtn.setText("Run"); runItem.setText("Run");
                    console.disableInput();
                    console.setInterruptHandler(null);
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

        // Show splash screen every 10 Run/Debug actions when not licensed
        runDebugCount++;
        if (runDebugCount % 10 == 0 && !LicenseDialog.isLicensed(settings)) {
            SplashScreen.show();
        }

        console.clear();

        String base = currentFile.toString();
        if (base.endsWith(".asm")) base = base.substring(0, base.length() - 4);
        final String asmFile = base + ".asm";
        final String baseName = Path.of(base).getFileName().toString();
        final Path asmDir = Path.of(base).getParent();
        final Path objDir = asmDir.resolve("obj");

        new Thread(() -> {
            try {
                // Assemble with --DEBUG
                SwingUtilities.invokeLater(() -> appendConsole("> Assembling (debug) " + currentFile.getFileName() + "...\n"));
                int asmResult = Assembler.run(new String[]{asmFile, "--DEBUG"});
                if (asmResult != 0) {
                    SwingUtilities.invokeLater(() -> appendConsole("\nAssembly failed.\n"));
                    return;
                }

                // Create obj directory and move output files there
                Files.createDirectories(objDir);
                String[] extensions = {".o64", ".sym", ".sym1", ".sym2", ".srcmap"};
                for (String ext : extensions) {
                    Path src = asmDir.resolve(baseName + ext);
                    if (Files.exists(src)) {
                        Files.move(src, objDir.resolve(baseName + ext), java.nio.file.StandardCopyOption.REPLACE_EXISTING);
                    }
                }

                // Resolve the .o64 file: check obj directory first, fall back to source directory
                final String objFile;
                Path objInDir = objDir.resolve(baseName + ".o64");
                if (Files.exists(objInDir)) {
                    objFile = objInDir.toString();
                } else if (Files.exists(asmDir.resolve(baseName + ".o64"))) {
                    objFile = asmDir.resolve(baseName + ".o64").toString();
                } else {
                    SwingUtilities.invokeLater(() -> appendConsole("\nNo .o64 file found after assembly.\n"));
                    return;
                }

                SwingUtilities.invokeLater(() -> {
                    appendConsole("> Opening debugger...\n");
                    runItem.setEnabled(false);
                    debugItem.setEnabled(false);
                    runBtn.setEnabled(false);
                    debugBtn.setEnabled(false);
                    Runnable onClose = () -> { runItem.setEnabled(true); debugItem.setEnabled(true); runBtn.setEnabled(true); debugBtn.setEnabled(true); };
                    new DebuggerWindow(frame, objFile, asmFile, editorPanel, argsField.getText().trim(), settings, console, onClose);
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

        RSyntaxTextArea pane = new RSyntaxTextArea();
        pane.setSyntaxEditingStyle("text/cpusim64");
        pane.setFont(new Font(Font.MONOSPACED, Font.PLAIN, settings.fontSize));
        pane.setEditable(false);
        pane.setCodeFoldingEnabled(false);
        pane.setTabSize(4);
        pane.setTabsEmulated(true);
        pane.setText(content);
        pane.setCaretPosition(0);

        RTextScrollPane scroll = new RTextScrollPane(pane);
        scroll.setLineNumbersEnabled(true);
        scroll.setFoldIndicatorEnabled(false);
        viewer.add(scroll);
        viewer.setLocationRelativeTo(frame);
        viewer.setVisible(true);
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

    private void appendConsole(String text) {
        console.write(text);
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
