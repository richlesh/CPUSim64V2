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
import java.nio.file.*;

public class CPUSim64App {
    private static final boolean IS_MAC = System.getProperty("os.name").toLowerCase().contains("mac");
    private static final boolean IS_WINDOWS = System.getProperty("os.name").toLowerCase().contains("win");

    private JFrame frame;
    private JTextPane codeEditor;
    private AsmSyntaxHighlighter highlighter;
    private JTextArea console;
    private JTextField argsField;
    private Path currentFile;
    private javax.swing.Timer highlightTimer;
    private AppSettings settings;
    private UndoManager undoManager = new UndoManager();
    private boolean highlightingInProgress = false;
    private boolean modified = false;
    private JMenuItem saveItem;

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

    private void createAndShowGUI() {
        settings = AppSettings.load();
        frame = new JFrame("CPUSim64");
        frame.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        frame.addWindowListener(new WindowAdapter() {
            @Override public void windowClosing(WindowEvent e) {
                if (promptSaveIfNeeded()) frame.dispose();
            }
        });
        frame.setSize(1024, 768);

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
        for (int i = 0; i < settings.colors.length; i++) {
            highlighter.setColor(i, settings.colors[i]);
        }
        highlighter.highlight();
    }

    private JMenuBar createMenuBar() {
        JMenuBar menuBar = new JMenuBar();

        // Application menu
        JMenu appMenu = new JMenu("Application");
        JMenuItem installItem = new JMenuItem("CLI Tools...");
        installItem.addActionListener(e -> {
            Object[] options = {"Install", "Remove", "Cancel"};
            int result = JOptionPane.showOptionDialog(frame,
                "Install or remove CPUSim64 command line tools?",
                "CLI Tools", JOptionPane.DEFAULT_OPTION, JOptionPane.QUESTION_MESSAGE,
                null, options, "Install");
            if (result == 0) CLIInstaller.install();
            else if (result == 1) CLIInstaller.uninstall();
        });
        JMenuItem settingsItem = new JMenuItem("Settings");
        settingsItem.addActionListener(e -> SettingsDialog.show(frame, codeEditor, console, highlighter, settings));
        JMenuItem licenseItem = new JMenuItem("License Key");
        licenseItem.addActionListener(e -> LicenseDialog.show(frame, settings));
        JMenuItem aboutItem = new JMenuItem("About CPUSim64");
        aboutItem.addActionListener(e -> {
            Icon icon = null;
            var iconUrl = CPUSim64App.class.getResource("/app_icon_256.png");
            if (iconUrl != null) {
                icon = new ImageIcon(new ImageIcon(iconUrl)
                    .getImage().getScaledInstance(96, 96, Image.SCALE_SMOOTH));
            }
            boolean licensed = LicenseDialog.isLicensed(settings);
            String thankYou = licensed ? "<br><br><b>Thank you for purchasing a license<br>for CPUSim64!</b>" : "";
            JEditorPane msg = new JEditorPane("text/html",
                "<html><body style='text-align:center;font-family:sans-serif;'>" +
                "<b style='font-size:14pt;'>CPUSim64</b><br><br>" +
                "Version " + cloud.lesh.CPUSim64.BuildInfo.VERSION + "<br>" +
                "\u00a92026 Richard Lesh<br>" +
                "<a href='https://glowingcatsoftware.com'>Glowing Cat Software</a><br>" +
                "<a href='https://github.com/richlesh/CPUSim64V2/issues'>Report issues on GitHub</a>" +
                thankYou +
                "</body></html>");
            msg.setEditable(false);
            msg.setOpaque(false);
            msg.addHyperlinkListener(he -> {
                if (he.getEventType() == javax.swing.event.HyperlinkEvent.EventType.ACTIVATED) {
                    try { Desktop.getDesktop().browse(he.getURL().toURI()); }
                    catch (Exception ignored) {}
                }
            });
            JOptionPane.showMessageDialog(frame, msg,
                "About CPUSim64", JOptionPane.INFORMATION_MESSAGE, icon);
        });
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
        JMenuItem runItem = new JMenuItem("Run");
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

        // Edit menu
        JMenu editMenu = new JMenu("Edit");
        JMenuItem undoItem = new JMenuItem("Undo");
        undoItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Z, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        undoItem.addActionListener(e -> { if (undoManager.canUndo()) undoManager.undo(); });
        JMenuItem redoItem = new JMenuItem("Redo");
        redoItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_Z, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx() | KeyEvent.SHIFT_DOWN_MASK));
        redoItem.addActionListener(e -> { if (undoManager.canRedo()) undoManager.redo(); });
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

        menuBar.add(appMenu);
        menuBar.add(fileMenu);
        menuBar.add(editMenu);
        menuBar.add(Box.createHorizontalStrut(10));
        menuBar.add(new JLabel("Args:"));
        argsField = new JTextField(20);
        argsField.setMaximumSize(new Dimension(300, 24));
        menuBar.add(argsField);
        return menuBar;
    }

    private JSplitPane createMainPanel() {
        codeEditor = new JTextPane();
        codeEditor.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 14));
        highlighter = new AsmSyntaxHighlighter(codeEditor);
        highlightTimer = new javax.swing.Timer(300, e -> {
            highlightingInProgress = true;
            highlighter.highlight();
            highlightingInProgress = false;
        });
        highlightTimer.setRepeats(false);
        codeEditor.getDocument().addDocumentListener(new DocumentListener() {
            public void insertUpdate(DocumentEvent e) { highlightTimer.restart(); markModified(); }
            public void removeUpdate(DocumentEvent e) { highlightTimer.restart(); markModified(); }
            public void changedUpdate(DocumentEvent e) {}
        });
        codeEditor.getDocument().addUndoableEditListener(e -> {
            if (!highlightingInProgress) {
                String type = e.getEdit().getPresentationName();
                if ("addition".equals(type) || "deletion".equals(type)) {
                    undoManager.addEdit(e.getEdit());
                }
            }
        });
        JScrollPane editorScroll = new JScrollPane(codeEditor);

        console = new JTextArea();
        console.setFont(new Font(Font.MONOSPACED, Font.PLAIN, 13));
        console.setEditable(false);
        console.setBackground(new Color(30, 30, 30));
        console.setForeground(Color.WHITE);
        JScrollPane consoleScroll = new JScrollPane(console);

        JSplitPane splitPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT, editorScroll, consoleScroll);
        splitPane.setResizeWeight(0.7);
        return splitPane;
    }

    private void openFile() {
        if (!promptSaveIfNeeded()) return;
        JFileChooser chooser = new JFileChooser();
        chooser.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("Assembly Files", "asm"));
        if (chooser.showOpenDialog(frame) == JFileChooser.APPROVE_OPTION) {
            loadFile(chooser.getSelectedFile().toPath());
        }
    }

    private void loadFile(Path path) {
        try {
            highlightingInProgress = true;
            codeEditor.setText(Files.readString(path));
            codeEditor.setCaretPosition(0);
            highlightingInProgress = false;
            currentFile = path;
            frame.setTitle("CPUSim64 - " + path.getFileName());
            undoManager.discardAllEdits();
            modified = false;
            saveItem.setEnabled(false);
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
        JFileChooser chooser = new JFileChooser();
        chooser.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("Assembly Files", "asm"));
        if (currentFile != null) chooser.setSelectedFile(currentFile.toFile());
        if (chooser.showSaveDialog(frame) == JFileChooser.APPROVE_OPTION) {
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
    }

    /** Returns true if it's safe to proceed (saved or discarded), false if cancelled. */
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
            PrintStream origOut = System.out;
            PrintStream origErr = System.err;
            try {
                PrintStream consoleStream = new PrintStream(new OutputStream() {
                    private final ByteArrayOutputStream buf = new ByteArrayOutputStream();
                    @Override public void write(int b) {
                        if (b == '\n') {
                            final String line = buf.toString(java.nio.charset.StandardCharsets.UTF_8);
                            buf.reset();
                            SwingUtilities.invokeLater(() -> appendConsole(line + "\n"));
                        } else {
                            buf.write(b);
                        }
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

                SwingUtilities.invokeLater(() -> appendConsole("> Assembling " + currentFile.getFileName() + "...\n"));
                int asmResult = Assembler.run(new String[]{asmFile});
                if (asmResult != 0) {
                    SwingUtilities.invokeLater(() -> appendConsole("\nAssembly failed.\n"));
                    return;
                }

                SwingUtilities.invokeLater(() -> appendConsole("> Running...\n"));
                java.util.List<String> simArgs = new java.util.ArrayList<>();
                simArgs.add(objFile);
                simArgs.add("--verbose");
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
            }
        }).start();
    }

    private void appendConsole(String text) {
        console.append(text);
        console.setCaretPosition(console.getDocument().getLength());
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
