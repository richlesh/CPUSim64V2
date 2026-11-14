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
package com.glowingcat.cpusim64ide;

import com.glowingcat.aichat.GenericVendorConfig;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.nio.file.*;
import java.util.regex.*;
import cloud.lesh.CPUSim64.StdInterruptHandler;
import cloud.lesh.CPUSim64.Assembler;
import cloud.lesh.CPUSim64.Simulation;
import cloud.lesh.CPUSim64.Simulator;
import com.glowingcat.aichat.AIChatPanel;
import com.glowingcat.aichat.AIChatPreferences;
import com.glowingcat.aichat.AIChatPreferencesDialog;
import com.glowingcat.aichat.DocumentEditor;
import org.fife.ui.rsyntaxtextarea.*;
import org.fife.ui.rtextarea.*;

public class IDEApp {
    private static final boolean IS_MAC = System.getProperty("os.name").toLowerCase().contains("mac");
    private static final boolean IS_WINDOWS = System.getProperty("os.name").toLowerCase().contains("win");

    private JFrame frame;
    private RSyntaxTextArea codeEditor;
    private TerminalPanel console;
    private JTextField argsField;
    private JTextField optionsField;            // compiler windows only
    /** The focused editor tab; codeEditor is its text area. */
    private SourceTab cur;
    private JTabbedPane editorTabs;
    private final java.util.List<SourceTab> tabs = new java.util.ArrayList<>();
    /** File being built/run (fixed while a run is in progress, even if another tab is focused). */
    private Path buildFile;
    private AppSettings settings;
    private static AppSettings sharedSettings;
    private boolean undoInProgress = false;
    private static final int MAX_UNDO = 200;
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
    private JMenuItem killAllItem;
    private JCheckBoxMenuItem aiMenuItem;
    private JMenu windowMenu;
    private JButton runBtn, debugBtn;
    private volatile Thread runThread;
    private volatile Process nativeProcess;
    private int runDebugCount = 0;
    /** The compiler plugin this window edits for, or null for a plain assembly window. */
    private final CompilerPlugin plugin;

    public IDEApp() { this(null); }

    public IDEApp(CompilerPlugin plugin) { this.plugin = plugin; }

    /** "CPUSim64", or "CPUSim64 C11" in a window for the C11 compiler plugin. */
    private String baseTitle() {
        return plugin == null ? "CPUSim64" : "CPUSim64 " + plugin.getName();
    }

    public static void main(String[] args) {
        if (args.length > 0 && args[0].equals("--uninstall")) {
            CLIInstaller.uninstall();
            return;
        }
        // Installed builds have no console, so make any uncaught failure visible
        // instead of letting the app silently exit.
        // (Log only; the dialog is reserved for startup failures.)
        Thread.setDefaultUncaughtExceptionHandler((t, e) ->
                reportError("Uncaught exception in thread " + t.getName(), e, false));
        if (!IS_WINDOWS && !IS_MAC) {
            checkXWayland();
        }

        // Apply custom trust store settings before any HTTPS connections
        GenericVendorConfig.applyTrustStore();

        // Load the full gemoji shortcode set (:tada: -> emoji) from the bundled dataset.
        EmojiShortcodes.load();

        // Augment the bundled set from https://api.github.com/emojis in the background so
        // GitHub-only shortcodes (custom images with no Unicode character, e.g. :shipit:)
        // resolve to their hosted image. Best-effort and app-wide: it populates the shared
        // shortcode map and never blocks startup or requires network access.
        Thread emojiRefresh = new Thread(EmojiShortcodeValidator::refreshFromGitHub,
                "github-emoji-refresh");
        emojiRefresh.setDaemon(true);
        emojiRefresh.start();


        // On macOS, use the system menu bar at the top of the screen
        if (IS_MAC) {
            System.setProperty("apple.laf.useScreenMenuBar", "true");
            System.setProperty("apple.awt.application.name", "CPUSim64");
        }
        // Use native look and feel for each platform
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception ignored) {}
        SwingUtilities.invokeLater(() -> {
            try {
                new IDEApp().createAndShowGUI();
            } catch (Throwable e) {
                // Without this, a failure before the main window is shown leaves only
                // the splash screen, which closes and the app quits with no message.
                reportError("CPUSim64 failed to start", e, true);
                System.exit(1);
            }
        });
    }

    /**
     * Writes the error to stderr and ~/.cpusim64/startup-error.log, optionally showing a dialog.
     */
    static void reportError(String title, Throwable e, boolean showDialog) {
        StringWriter sw = new StringWriter();
        e.printStackTrace(new PrintWriter(sw));
        String trace = sw.toString();
        System.err.println(title);
        System.err.println(trace);
        Path log = Path.of(System.getProperty("user.home"), ".cpusim64", "startup-error.log");
        try {
            Files.createDirectories(log.getParent());
            Files.writeString(log, java.time.LocalDateTime.now() + "  " + title + System.lineSeparator()
                    + "java " + System.getProperty("java.version") + ", " + System.getProperty("os.name")
                    + " " + System.getProperty("os.arch") + System.lineSeparator() + trace + System.lineSeparator(),
                    StandardOpenOption.CREATE, StandardOpenOption.APPEND);
        } catch (Exception ignored) {}
        if (!showDialog) return;
        try {
            JTextArea text = new JTextArea(trace, 15, 80);
            text.setEditable(false);
            JOptionPane.showMessageDialog(null, new Object[] {
                    title + ": " + e, "Details were saved to " + log, new JScrollPane(text) },
                    "CPUSim64", JOptionPane.ERROR_MESSAGE);
        } catch (Throwable ignored) {
            // Headless or display unavailable: stderr and the log file are all we can do.
        }
    }

    void createAndShowGUI() {
        // All windows share one settings object: a change made in one window (for example
        // "Use Native CPUSim64 Emulator") applies to every window, and a window closing later
        // cannot write back its stale copy over the saved settings.
        synchronized (IDEApp.class) {
            if (sharedSettings == null) sharedSettings = AppSettings.load();
            settings = sharedSettings;
        }
        aiPreferences = AIChatPreferences.load();
        if (!LicenseDialog.isLicensed(settings)) {
            SplashScreen.show();
        }
        frame = new JFrame(baseTitle());
        frame.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        frame.addWindowListener(new WindowAdapter() {
            @Override public void windowClosing(WindowEvent e) {
                if (promptSaveAll()) {
                    saveWindowState();
                    frame.dispose();
                }
            }
        });
        frame.setSize(settings.mainWindowWidth, settings.mainWindowHeight);
        var frameIconUrl = IDEApp.class.getResource("/app_icon_256.png");
        if (frameIconUrl != null) frame.setIconImage(new ImageIcon(frameIconUrl).getImage());

        frame.setJMenuBar(createMenuBar());
        frame.add(createMainPanel(), BorderLayout.CENTER);
        applySettings();
        updateRunnable();

        // Center the window on the monitor (default position each session)
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
        mainWindowShown = true;

        // Restore AI panel visibility from saved settings (after window is shown so
        // the split pane has a valid width for divider positioning)
        if (settings.aiPanelVisible) {
            SwingUtilities.invokeLater(this::restoreAIPanel);
        }
    }

    /** Persist the main window size and AI panel state to settings. */
    private void saveWindowState() {
        settings.mainWindowWidth = frame.getWidth();
        settings.mainWindowHeight = frame.getHeight();
        settings.aiPanelVisible = aiChatPanel != null && aiChatPanel.isVisible();
        if (settings.aiPanelVisible && mainSplit != null) {
            settings.aiPanelWidth = mainSplit.getWidth() - mainSplit.getDividerLocation();
        }
        settings.save();
    }

    /** Show the AI panel at the saved width (used to restore state at startup). */
    private void restoreAIPanel() {
        aiChatPanel.setVisible(true);
        mainSplit.setDividerSize(6);
        int w = settings.aiPanelWidth > 0 ? settings.aiPanelWidth : 400;
        mainSplit.setDividerLocation(mainSplit.getWidth() - w);
        if (aiMenuItem != null) aiMenuItem.setSelected(true);
    }

    private void applySettings() {
        Font font = new Font(settings.fontName, Font.PLAIN, settings.fontSize);
        console.setFont(settings.fontName, settings.fontSize);
        console.setColors(settings.consoleFg, settings.consoleBg);
        if (consoleToolBar != null) {
            for (Component c : consoleToolBar.getComponents()) c.setFont(font);
        }
        for (SourceTab t : tabs) applyEditorSettings(t);
        // Update AI chat panel font
        if (aiChatPanel != null) aiChatPanel.updateFont();
    }

    /** Font and syntax colors of one tab's editor. */
    private void applyEditorSettings(SourceTab t) {
        Font font = new Font(settings.fontName, Font.PLAIN, settings.fontSize);
        t.panel.setEditorFont(font);
        // Apply syntax colors to RSyntaxTextArea scheme
        SyntaxScheme scheme = t.area.getSyntaxScheme();
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
        t.area.setSyntaxScheme(scheme);
        t.area.setForeground(settings.colors[0]);
        t.area.revalidate();
        t.area.repaint();
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
            var url = IDEApp.class.getResource("/app_icon_256.png");
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
        newItem.addActionListener(e -> SwingUtilities.invokeLater(() -> new IDEApp().createAndShowGUI()));
        JMenu newCompilerMenu = new JMenu("New Compiler");
        java.util.List<CompilerPlugin> compilers = CompilerPlugin.discover();
        for (CompilerPlugin cp : compilers) {
            JMenuItem cpItem = new JMenuItem(cp.getName());
            cpItem.addActionListener(e -> SwingUtilities.invokeLater(() -> new IDEApp(cp).createAndShowGUI()));
            newCompilerMenu.add(cpItem);
        }
        newCompilerMenu.setEnabled(!compilers.isEmpty());
        JMenuItem openItem = new JMenuItem("Open");
        openItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_O, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        openItem.addActionListener(e -> openFile());
        JMenuItem closeItem = new JMenuItem("Close");
        closeItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_W, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        closeItem.addActionListener(e -> closeTab(cur));
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
        fileMenu.add(newCompilerMenu);
        fileMenu.add(openItem);
        fileMenu.add(closeItem);
        fileMenu.addSeparator();
        fileMenu.add(saveItem);
        fileMenu.add(saveAsItem);
        fileMenu.addSeparator();
        fileMenu.add(runItem);
        killAllItem = new JMenuItem("Kill All");
        killAllItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_K, Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx()));
        killAllItem.addActionListener(e -> killAll());
        fileMenu.add(killAllItem);
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

        // Window menu
        windowMenu = new JMenu("Window");
        int shortcutMask = Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx();

        JMenuItem minimizeItem = new JMenuItem("Minimize");
        minimizeItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_M, shortcutMask));
        minimizeItem.addActionListener(e -> {
            Frame f = activeFrame();
            if (f != null) f.setExtendedState(f.getExtendedState() | Frame.ICONIFIED);
        });
        windowMenu.add(minimizeItem);

        JMenuItem zoomItem = new JMenuItem("Zoom");
        zoomItem.addActionListener(e -> {
            Frame f = activeFrame();
            if (f != null) {
                // Toggle maximized-both state.
                if ((f.getExtendedState() & Frame.MAXIMIZED_BOTH) == Frame.MAXIMIZED_BOTH)
                    f.setExtendedState(f.getExtendedState() & ~Frame.MAXIMIZED_BOTH);
                else
                    f.setExtendedState(f.getExtendedState() | Frame.MAXIMIZED_BOTH);
            }
        });
        windowMenu.add(zoomItem);

        windowMenu.addSeparator();

        JMenuItem prevItem = new JMenuItem("Previous");
        // Cmd/Ctrl + Shift + ','
        prevItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_COMMA, shortcutMask | InputEvent.SHIFT_DOWN_MASK));
        prevItem.addActionListener(e -> cycleWindow(-1));
        windowMenu.add(prevItem);

        JMenuItem nextItem = new JMenuItem("Next");
        // Cmd/Ctrl + Shift + '.'
        nextItem.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_PERIOD, shortcutMask | InputEvent.SHIFT_DOWN_MASK));
        nextItem.addActionListener(e -> cycleWindow(1));
        windowMenu.add(nextItem);

        windowMenu.addSeparator();

        JMenuItem cascadeItem = new JMenuItem("Cascade All");
        cascadeItem.addActionListener(e -> cascadeAllWindows());
        windowMenu.add(cascadeItem);

        JMenuItem tileItem = new JMenuItem("Tile All");
        tileItem.addActionListener(e -> tileAllWindows());
        windowMenu.add(tileItem);

        // Separator marking the boundary between the static items above and the
        // dynamic, alphabetized window list below. rebuildWindowList() preserves
        // everything up to and including this separator.
        windowMenu.addSeparator();

        // The alphabetized list of open windows is rebuilt each time the menu
        // opens so it always reflects the current set of visible windows.
        windowMenu.addMenuListener(new javax.swing.event.MenuListener() {
            @Override public void menuSelected(javax.swing.event.MenuEvent ev) { rebuildWindowList(windowMenu); }
            @Override public void menuDeselected(javax.swing.event.MenuEvent ev) {}
            @Override public void menuCanceled(javax.swing.event.MenuEvent ev) {}
        });

        // Also rebuild the list proactively whenever any window is opened or
        // closed so a closed window is dropped from the menu immediately, even
        // if the menu was already open or is opened via its keyboard mnemonic.
        // Additionally, when the last application window closes, shut the app
        // down. On macOS in particular, disposing the final window does not quit
        // the JVM, so we must exit explicitly.
        Toolkit.getDefaultToolkit().addAWTEventListener(ev -> {
            int id = ev.getID();
            if (id == WindowEvent.WINDOW_CLOSED || id == WindowEvent.WINDOW_OPENED) {
                // Defer so the closing window has finished disposing before we
                // enumerate Frame.getFrames().
                SwingUtilities.invokeLater(() -> {
                    rebuildWindowList(windowMenu);
                    // Only once the main window has been shown: during startup the
                    // splash can close while no app frame is visible yet (e.g. while
                    // JFXPanel waits for JavaFX in a nested event loop), which must
                    // not be mistaken for "last window closed".
                    if (id == WindowEvent.WINDOW_CLOSED && mainWindowShown && listAppWindows().isEmpty()) {
                        shutdown();
                    }
                });
            }
        }, AWTEvent.WINDOW_EVENT_MASK);

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
        menuBar.add(windowMenu);
        menuBar.add(helpMenu);
        menuBar.add(Box.createHorizontalGlue());
        var aiIconUrl = IDEApp.class.getResource("/AI.png");
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

    // ===== Window menu support =====

    /** Guard so shutdown runs exactly once. */
    private boolean shuttingDown = false;

    /** Set once the main IDE frame has been made visible; gates auto-shutdown. */
    private boolean mainWindowShown = false;

    /**
     * Terminate the application. Invoked when the last application window has
     * closed. Stops any running simulation (so no simulator/child threads keep
     * the JVM alive) and then exits. On macOS the JVM does not quit on its own
     * when the final window is disposed, so an explicit exit is required.
     */
    private void shutdown() {
        if (shuttingDown) return;
        shuttingDown = true;
        try { killAll(); } catch (Throwable ignore) {}
        System.exit(0);
    }

    /**
     * Returns the list of application windows that should appear in the Window
     * menu: visible, non-disposed top-level Frames that have a title. This
     * automatically includes the main IDE frame plus any Debugger/Memory
     * windows without requiring manual registration.
     */
    private java.util.List<Frame> listAppWindows() {
        java.util.List<Frame> result = new java.util.ArrayList<>();
        for (Frame f : Frame.getFrames()) {
            if (f.isDisplayable() && f.isVisible() && f.getTitle() != null && !f.getTitle().isEmpty()) {
                result.add(f);
            }
        }
        return result;
    }

    /** The currently active/focused application frame, or the main frame as a fallback. */
    private Frame activeFrame() {
        for (Frame f : listAppWindows()) {
            if (f.isActive() || f.isFocused()) return f;
        }
        return frame;
    }

    /** Bring the given window to the front, de-iconifying it if necessary. */
    private void bringToFront(Frame f) {
        if (f == null) return;
        if ((f.getExtendedState() & Frame.ICONIFIED) == Frame.ICONIFIED)
            f.setExtendedState(f.getExtendedState() & ~Frame.ICONIFIED);
        f.setVisible(true);
        f.toFront();
        f.requestFocus();
    }

    /** Cycle focus to the previous (-1) or next (+1) window in alphabetical order. */
    private void cycleWindow(int direction) {
        java.util.List<Frame> windows = listAppWindows();
        if (windows.size() < 2) return;
        windows.sort(java.util.Comparator.comparing(Frame::getTitle, String.CASE_INSENSITIVE_ORDER));
        Frame current = activeFrame();
        int idx = windows.indexOf(current);
        if (idx < 0) idx = 0;
        int next = (idx + direction + windows.size()) % windows.size();
        bringToFront(windows.get(next));
    }

    /** Cascade all application windows from the top-left with a fixed offset. */
    private void cascadeAllWindows() {
        java.util.List<Frame> windows = listAppWindows();
        windows.sort(java.util.Comparator.comparing(Frame::getTitle, String.CASE_INSENSITIVE_ORDER));
        GraphicsConfiguration gc = frame.getGraphicsConfiguration();
        Rectangle bounds = gc.getBounds();
        Insets insets = Toolkit.getDefaultToolkit().getScreenInsets(gc);
        int x0 = bounds.x + insets.left;
        int y0 = bounds.y + insets.top;
        int offset = 32;
        int i = 0;
        for (Frame f : windows) {
            if ((f.getExtendedState() & Frame.ICONIFIED) == Frame.ICONIFIED)
                f.setExtendedState(f.getExtendedState() & ~Frame.ICONIFIED);
            f.setExtendedState(f.getExtendedState() & ~Frame.MAXIMIZED_BOTH);
            int x = x0 + (i * offset);
            int y = y0 + (i * offset);
            f.setBounds(x, y, Math.max(f.getWidth(), 400), Math.max(f.getHeight(), 300));
            bringToFront(f);
            i++;
        }
    }

    /** Tile all application windows in a grid that fills the screen work area. */
    private void tileAllWindows() {
        java.util.List<Frame> windows = listAppWindows();
        if (windows.isEmpty()) return;
        windows.sort(java.util.Comparator.comparing(Frame::getTitle, String.CASE_INSENSITIVE_ORDER));
        GraphicsConfiguration gc = frame.getGraphicsConfiguration();
        Rectangle bounds = gc.getBounds();
        Insets insets = Toolkit.getDefaultToolkit().getScreenInsets(gc);
        int workX = bounds.x + insets.left;
        int workY = bounds.y + insets.top;
        int workW = bounds.width - insets.left - insets.right;
        int workH = bounds.height - insets.top - insets.bottom;
        int n = windows.size();
        int cols = (int) Math.ceil(Math.sqrt(n));
        int rows = (int) Math.ceil((double) n / cols);
        int cellW = workW / cols;
        int cellH = workH / rows;
        for (int i = 0; i < n; i++) {
            Frame f = windows.get(i);
            int r = i / cols;
            int c = i % cols;
            if ((f.getExtendedState() & Frame.ICONIFIED) == Frame.ICONIFIED)
                f.setExtendedState(f.getExtendedState() & ~Frame.ICONIFIED);
            f.setExtendedState(f.getExtendedState() & ~Frame.MAXIMIZED_BOTH);
            f.setBounds(workX + c * cellW, workY + r * cellH, cellW, cellH);
            bringToFront(f);
        }
    }

    /**
     * Rebuild the dynamic, alphabetized window list at the bottom of the Window
     * menu. Everything up to and including the last separator (after "Tile All")
     * is preserved; items below it are the current windows and are regenerated.
     */
    private void rebuildWindowList(JMenu windowMenu) {
        // Find the index of the last separator; the static items end there.
        int lastSep = -1;
        for (int i = 0; i < windowMenu.getMenuComponentCount(); i++) {
            if (windowMenu.getMenuComponent(i) instanceof JPopupMenu.Separator) lastSep = i;
        }
        // Remove any previously-added dynamic window items.
        while (windowMenu.getMenuComponentCount() > lastSep + 1) {
            windowMenu.remove(windowMenu.getMenuComponentCount() - 1);
        }
        java.util.List<Frame> windows = listAppWindows();
        windows.sort(java.util.Comparator.comparing(Frame::getTitle, String.CASE_INSENSITIVE_ORDER));
        Frame current = activeFrame();
        for (Frame f : windows) {
            JCheckBoxMenuItem item = new JCheckBoxMenuItem(f.getTitle());
            item.setSelected(f == current);   // check-mark the frontmost window
            item.addActionListener(e -> bringToFront(f));
            windowMenu.add(item);
        }
    }

    private JSplitPane createMainPanel() {
        editorTabs = new JTabbedPane(JTabbedPane.TOP, JTabbedPane.SCROLL_TAB_LAYOUT);
        editorTabs.addChangeListener(e -> tabSelected());
        newTab();
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
        if (plugin != null) {
            // extra compiler arguments, e.g. more source files and -lstd for the C11 compiler
            consoleToolBar.addSeparator();
            consoleToolBar.add(new JLabel("Options: "));
            optionsField = new JTextField(16);
            optionsField.setMaximumSize(new Dimension(260, 24));
            optionsField.setToolTipText("Extra compiler arguments (e.g. other source files, -lstd); "
                    + "relative file names are relative to the source file");
            consoleToolBar.add(optionsField);
        }
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

        JSplitPane splitPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT, editorTabs, consolePanel);
        splitPane.setResizeWeight(0.7);

        var retriever = new com.glowingcat.aichat.DocumentRetriever(
            java.nio.file.Path.of(System.getProperty("user.home"), ".cpusim64"),
            java.util.List.of(
                "/documentation/examples-index.txt",
                "/documentation/projects-index.txt"
            ),
            10
        );

        var aiBuilder = AIChatPanel.builder()
            .editor(new DocumentEditor() {
                @Override public String getText() { return codeEditor.getText(); }
                @Override public void setText(String text) {
                    codeEditor.setText(text);
                    cur.modified = true;
                    saveItem.setEnabled(true);
                }
                @Override public String getFilepath() {
                    return cur.file != null ? cur.file.toString() : null;
                }
            })
            .preferences(aiPreferences)
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
            });
        if (plugin != null) {
            // Compiler window: the plugin's own system_prompt.md replaces the assembly
            // prompt, and the assembly reference documents / examples are left out.
            String syn = plugin.getSyntaxLanguage();
            aiBuilder.contentType(syn.startsWith("text/") ? syn.substring(5) : syn);
            aiBuilder.systemPrompt(plugin.getSystemPrompt());
        } else {
            aiBuilder.contentType("cpusim64")
                .documentRetriever(retriever)
                .systemPromptDocs("/documentation/doc-index.txt");
        }
        aiChatPanel = aiBuilder.build();
        mainSplit = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, splitPane, aiChatPanel);
        mainSplit.setResizeWeight(1.0);
        aiChatPanel.setVisible(false);
        mainSplit.setDividerSize(0);
        return mainSplit;
    }

    // ===== Editor tabs =====

    /** A source file open in an editor tab. */
    final class SourceTab {
        final EditorPanel panel = new EditorPanel();
        final RSyntaxTextArea area = panel.getTextArea();
        final JLabel titleLabel = new JLabel();
        Path file;                  // null for an untitled tab (or a library resource)
        String resource;            // classpath resource of a read-only library tab
        boolean modified;
        String lastSavedText = "";
        final java.util.Deque<String> undoStack = new java.util.ArrayDeque<>();
        final java.util.Deque<String> redoStack = new java.util.ArrayDeque<>();

        boolean readOnly() { return resource != null; }
        String name() {
            if (file != null) return file.getFileName().toString();
            if (resource != null) return Path.of(resource).getFileName().toString();
            return "untitled";
        }
        boolean isEmptyUntitled() {
            return file == null && resource == null && !modified && area.getText().isEmpty();
        }
    }

    /** Opens a new empty tab and focuses it. */
    private SourceTab newTab() {
        SourceTab t = new SourceTab();
        setupEditor(t);
        tabs.add(t);
        editorTabs.addTab(t.name(), t.panel);
        int idx = editorTabs.indexOfComponent(t.panel);
        editorTabs.setTabComponentAt(idx, tabHeader(t));
        if (settings != null) applyEditorSettings(t);
        editorTabs.setSelectedComponent(t.panel);
        tabSelected();
        updateTabTitle(t);
        return t;
    }

    /** Tab header: the file name and a close button. */
    private JComponent tabHeader(SourceTab t) {
        JPanel p = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        p.setOpaque(false);
        p.add(t.titleLabel);
        JButton close = new JButton("\u00d7");
        close.setBorder(BorderFactory.createEmptyBorder(0, 2, 0, 2));
        close.setContentAreaFilled(false);
        close.setFocusable(false);
        close.setToolTipText("Close " + t.name());
        close.addActionListener(e -> closeTab(t));
        p.add(close);
        // clicking the label selects the tab (the custom header would otherwise swallow the click)
        t.titleLabel.addMouseListener(new MouseAdapter() {
            @Override public void mousePressed(MouseEvent e) { editorTabs.setSelectedComponent(t.panel); }
        });
        return p;
    }

    private void updateTabTitle(SourceTab t) {
        String title = t.name() + (t.modified ? " *" : "") + (t.readOnly() ? " (read-only)" : "");
        t.titleLabel.setText(title);
        int idx = editorTabs.indexOfComponent(t.panel);
        if (idx >= 0) {
            editorTabs.setTitleAt(idx, title);
            editorTabs.setToolTipTextAt(idx, t.file != null ? t.file.toString()
                    : t.resource != null ? "<" + t.resource + ">" : "untitled");
        }
        if (t == cur) updateTitle();
    }

    private void updateTitle() {
        if (frame == null || cur == null) return;
        boolean named = cur.file != null || cur.resource != null;
        frame.setTitle(baseTitle() + (named ? " - " + cur.name() : ""));
    }

    /** Called when another tab is selected: makes it the focused tab. */
    private void tabSelected() {
        Component c = editorTabs.getSelectedComponent();
        for (SourceTab t : tabs) {
            if (t.panel == c) {
                cur = t;
                codeEditor = t.area;
            }
        }
        if (cur == null) return;
        updateTitle();
        if (saveItem != null) saveItem.setEnabled(cur.modified && !cur.readOnly());
        if (undoItem != null) updateUndoRedo();
        updateRunnable();
    }

    /** Key bindings, change tracking, include links and syntax style of a tab's editor. */
    private void setupEditor(SourceTab t) {
        RSyntaxTextArea a = t.area;
        if (plugin != null) {
            // Highlight the compiler's language (an RSyntaxTextArea style such as text/c)
            a.setSyntaxEditingStyle(plugin.getSyntaxLanguage());
            a.setAutoIndentEnabled(true);
        }
        // Undo/redo key bindings
        int mod = Toolkit.getDefaultToolkit().getMenuShortcutKeyMaskEx();
        a.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_Z, mod), "safe-undo");
        a.getActionMap().put("safe-undo", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) { performUndo(); }
        });
        a.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_Z, mod | KeyEvent.SHIFT_DOWN_MASK), "safe-redo");
        a.getActionMap().put("safe-redo", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) { performRedo(); }
        });
        // Shift indent key bindings
        a.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_OPEN_BRACKET, mod), "shift-left");
        a.getActionMap().put("shift-left", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) { shiftIndent(false); }
        });
        a.getInputMap().put(KeyStroke.getKeyStroke(KeyEvent.VK_CLOSE_BRACKET, mod), "shift-right");
        a.getActionMap().put("shift-right", new javax.swing.AbstractAction() {
            public void actionPerformed(java.awt.event.ActionEvent e) { shiftIndent(true); }
        });
        a.getDocument().addDocumentListener(new javax.swing.event.DocumentListener() {
            public void insertUpdate(javax.swing.event.DocumentEvent e) { changed(); }
            public void removeUpdate(javax.swing.event.DocumentEvent e) { changed(); }
            public void changedUpdate(javax.swing.event.DocumentEvent e) {}
            private void changed() {
                runnableTimer.restart();
                if (undoInProgress) return;
                if (!t.modified) {
                    t.modified = true;
                    updateTabTitle(t);
                    if (t == cur && saveItem != null) saveItem.setEnabled(!t.readOnly());
                }
                String current = a.getText();
                if (current.equals(t.lastSavedText)) return;
                t.undoStack.push(t.lastSavedText);
                if (t.undoStack.size() > MAX_UNDO) ((java.util.ArrayDeque<String>) t.undoStack).removeLast();
                t.redoStack.clear();
                t.lastSavedText = current;
                if (t == cur) updateUndoRedo();
            }
        });
        // Cmd+click (macOS) / Ctrl+click on a file name in an #include opens it in a tab
        a.setHyperlinksEnabled(true);
        a.setLinkScanningMask(IS_MAC ? InputEvent.META_DOWN_MASK : InputEvent.CTRL_DOWN_MASK);
        a.setLinkGenerator((ta, offs) -> includeLinkAt(t, offs));
    }

    // ----- #include links

    private static final Pattern INCLUDE_LINE = Pattern.compile("^[ \t]*#[ \t]*include[ \t]*([<\"])([^>\"\r\n]+)[>\"]",
            Pattern.CASE_INSENSITIVE | Pattern.MULTILINE);

    /** The #include link at offset {@code offs} (a token start on an #include line), or null. */
    private org.fife.ui.rsyntaxtextarea.LinkGeneratorResult includeLinkAt(SourceTab t, int offs) {
        String text = t.area.getText();
        if (offs < 0 || offs > text.length()) return null;
        int lineStart = text.lastIndexOf('\n', Math.max(0, offs - 1)) + 1;
        if (offs > 0 && offs <= text.length() && text.charAt(offs - 1) == '\n') lineStart = offs;
        int lineEnd = text.indexOf('\n', lineStart);
        if (lineEnd < 0) lineEnd = text.length();
        Matcher m = INCLUDE_LINE.matcher(text.substring(lineStart, lineEnd));
        if (!m.find()) return null;
        if (offs > lineStart + m.end(2)) return null;
        final String name = m.group(2).trim();
        final boolean system = m.group(1).equals("<");
        final int src = offs;
        return new org.fife.ui.rsyntaxtextarea.LinkGeneratorResult() {
            @Override public javax.swing.event.HyperlinkEvent execute() {
                SwingUtilities.invokeLater(() -> openInclude(t, name, system));
                return null;
            }
            @Override public int getSourceOffset() { return src; }
        };
    }

    /** True if this window may open a file with this name (.asm/.def, or the compiler's extensions). */
    private boolean canOpen(String fileName) {
        String n = fileName.toLowerCase(java.util.Locale.ROOT);
        if (plugin == null) return n.endsWith(".asm") || n.endsWith(".def");
        return !plugin.getFileExtensions().isEmpty() && plugin.acceptsFile(fileName);
    }

    private String allowedDescription() {
        if (plugin == null) return ".asm and .def files";
        java.util.List<String> e = plugin.getFileExtensions();
        return e.isEmpty() ? plugin.getName() + " source files"
                : String.join(", ", e.stream().map(x -> "." + x).toList()) + " files";
    }

    /** Opens the file named in an #include of {@code from} in a tab. */
    private void openInclude(SourceTab from, String name, boolean system) {
        String file = name;
        if (plugin != null && system && !name.contains(".")) file = name + ".h";	// <stdio> = <stdio.h>
        if (!canOpen(file)) {
            appendConsole("This " + baseTitle() + " window opens only " + allowedDescription() + ": " + name + "\n");
            return;
        }
        if (!system) {
            Path dir = from.file != null ? from.file.toAbsolutePath().getParent() : null;
            if (dir == null && from.resource != null) {
                // a local include in a library resource: another resource next to it
                String parent = Path.of(from.resource).getParent() == null ? "" : Path.of(from.resource).getParent() + "/";
                openResource(parent + file);
                return;
            }
            if (dir == null) dir = lastDirectory != null ? lastDirectory.toPath() : Path.of("").toAbsolutePath();
            Path target = dir.resolve(file).normalize();
            if (Files.isRegularFile(target)) loadFile(target);
            else appendConsole("Include file not found: " + target + "\n");
            return;
        }
        if (plugin != null) {
            // <name.h>: the compiler's library headers (lib/ in the plug-in folder)
            Path target = plugin.getDir().resolve("lib").resolve(file).normalize();
            if (Files.isRegularFile(target)) loadFile(target);
            else appendConsole("Library header not found: " + target + "\n");
            return;
        }
        openResource(file.startsWith("/") ? file.substring(1) : file);
    }

    /** Opens a system/ADT library (classpath resource) in a read-only tab. */
    private SourceTab openResource(String resource) {
        for (SourceTab t : tabs) {
            if (resource.equals(t.resource)) {
                editorTabs.setSelectedComponent(t.panel);
                return t;
            }
        }
        try (InputStream is = IDEApp.class.getResourceAsStream("/" + resource)) {
            if (is == null) {
                appendConsole("Library file not found: <" + resource + ">\n");
                return null;
            }
            String content = new String(is.readAllBytes(), java.nio.charset.StandardCharsets.UTF_8);
            SourceTab t = cur != null && cur.isEmptyUntitled() ? cur : newTab();
            t.resource = resource;
            setTabText(t, expandTabs(content));
            t.area.setEditable(false);
            updateTabTitle(t);
            tabSelected();
            return t;
        } catch (IOException e) {
            appendConsole("Error opening <" + resource + ">: " + e.getMessage() + "\n");
            return null;
        }
    }

    /** Replaces a tab's text without recording an undo step or marking it modified. */
    private void setTabText(SourceTab t, String content) {
        undoInProgress = true;
        t.area.setText(content);
        t.area.setCaretPosition(0);
        undoInProgress = false;
        t.undoStack.clear();
        t.redoStack.clear();
        t.lastSavedText = t.area.getText();
        t.modified = false;
        if (t == cur) {
            if (saveItem != null) saveItem.setEnabled(false);
            updateUndoRedo();
        }
        updateRunnable();
    }

    private static String expandTabs(String content) {
        if (content.indexOf('\t') < 0) return content;
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
        return sb.toString();
    }

    /** Closes a tab (asking to save it); closing the last tab closes the window. */
    private void closeTab(SourceTab t) {
        if (t.modified) {
            editorTabs.setSelectedComponent(t.panel);
            if (!promptSaveIfNeeded()) return;
        }
        if (tabs.size() == 1) {
            saveWindowState();
            frame.dispose();
            return;
        }
        tabs.remove(t);
        editorTabs.remove(t.panel);
        tabSelected();
    }

    /** Asks to save every modified tab; false if the user cancelled. */
    private boolean promptSaveAll() {
        for (SourceTab t : new java.util.ArrayList<>(tabs)) {
            if (!t.modified) continue;
            editorTabs.setSelectedComponent(t.panel);
            if (!promptSaveIfNeeded()) return false;
        }
        return true;
    }

    // ----- Run / Debug enablement

    private final javax.swing.Timer runnableTimer = new javax.swing.Timer(300, e -> updateRunnable());
    { runnableTimer.setRepeats(false); }
    private boolean debuggerOpen = false;

    private static final Pattern MAIN_LABEL = Pattern.compile("(?im)^[ \t]*__MAIN__[ \t]*:");
    private static final Pattern EXPORT_DIRECTIVE = Pattern.compile("(?im)^[ \t]*#export\\b");
    private static final Pattern INCLUDE_GUARD = Pattern.compile("(?is)^#ifndef[ \t]+(\\w+)\\s+#define[ \t]+\\1\\b");

    /** Removes // and /* *\/ comments, and (if {@code literals}) string and character literals. */
    static String stripComments(String text, boolean literals) {
        StringBuilder sb = new StringBuilder(text.length());
        int n = text.length();
        for (int i = 0; i < n; i++) {
            char c = text.charAt(i);
            if (c == '/' && i + 1 < n && text.charAt(i + 1) == '/') {
                while (i < n && text.charAt(i) != '\n') i++;
                if (i < n) sb.append('\n');
            } else if (c == '/' && i + 1 < n && text.charAt(i + 1) == '*') {
                int end = text.indexOf("*/", i + 2);
                if (end < 0) end = n - 2;
                for (int k = i; k < end + 2 && k < n; k++) if (text.charAt(k) == '\n') sb.append('\n');
                sb.append(' ');
                i = end + 1;
            } else if (literals && (c == '"' || c == '\'')) {
                int k = i + 1;
                while (k < n && text.charAt(k) != c && text.charAt(k) != '\n') {
                    if (text.charAt(k) == '\\') k++;
                    k++;
                }
                sb.append(c).append(c);
                i = k;
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }

    /**
     * True if the tab holds a program that can be run: an assembly program with
     * a {@code __MAIN__:} label, or one that is not a library or module (not a
     * .def file, no include guard, no #export); a C program that defines the
     * compiler's main function.
     */
    boolean isRunnable(SourceTab t) {
        String text = t.area.getText();
        String n = t.name().toLowerCase(java.util.Locale.ROOT);
        if (plugin == null) {
            if (n.endsWith(".def")) return false;
            if (MAIN_LABEL.matcher(text).find()) return true;
            if (t.readOnly() || text.isBlank()) return false;
            String code = stripComments(text, false).stripLeading();
            if (INCLUDE_GUARD.matcher(code).find()) return false;
            return !EXPORT_DIRECTIVE.matcher(code).find();
        }
        if (n.endsWith(".h")) return false;
        return definesFunction(text, plugin.getMainFunction());
    }

    /** True if C source {@code text} defines (not just declares) the function {@code name}. */
    static boolean definesFunction(String text, String name) {
        String code = stripComments(text, true);
        return Pattern.compile("(?<![\\w.>])" + Pattern.quote(name) + "\\s*\\(([^()]|\\([^()]*\\))*\\)\\s*\\{")
                .matcher(code).find();
    }

    /** Enables Run and Debug when the focused tab is a runnable program. */
    private void updateRunnable() {
        if (runBtn == null || cur == null || runThread != null) return;
        boolean ok = !debuggerOpen && isRunnable(cur);
        runBtn.setEnabled(ok);
        debugBtn.setEnabled(ok);
        if (runItem != null) runItem.setEnabled(ok);
        if (debugItem != null) debugItem.setEnabled(ok);
    }

    // ----- Debugger: editors of the tabs

    /** Gives the debugger the tabs of this window, opening the source files it steps into. */
    private DebuggerWindow.SourceProvider sourceProvider(Path programSource) {
        final Path dir = programSource.toAbsolutePath().getParent();
        return new DebuggerWindow.SourceProvider() {
            @Override public EditorPanel editorFor(String fileName, boolean open) {
                SourceTab found = null;
                for (SourceTab t : tabs) {
                    if (!t.name().equals(fileName)) continue;
                    if (found == null || (t.file != null && dir.equals(t.file.toAbsolutePath().getParent()))) found = t;
                }
                if (found == null && open && canOpen(fileName)) {
                    Path p = dir.resolve(fileName);
                    if (Files.isRegularFile(p)) {
                        loadFile(p);
                        found = cur;
                    } else if (plugin == null) {
                        for (String lib : new String[] {"system/", "adt/"}) {
                            if (IDEApp.class.getResource("/" + lib + fileName) != null) {
                                found = openResource(lib + fileName);
                                break;
                            }
                        }
                    }
                }
                if (found != null && open && found != cur) editorTabs.setSelectedComponent(found.panel);
                return found == null ? null : found.panel;
            }
            @Override public java.util.Map<String, EditorPanel> openEditors() {
                java.util.Map<String, EditorPanel> m = new java.util.LinkedHashMap<>();
                for (SourceTab t : tabs) m.putIfAbsent(t.name(), t.panel);
                return m;
            }
        };
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
            if (aiChatPanel != null) {
                aiChatPanel.updateFont();
                aiChatPanel.refreshPreferences();
            }
        }
    }

    private void toggleAIPanel() {
        boolean show = !aiChatPanel.isVisible();
        aiChatPanel.setVisible(show);
        mainSplit.setDividerSize(show ? 6 : 0);
        if (show) {
            int w = settings.aiPanelWidth > 0 ? settings.aiPanelWidth : 400;
            mainSplit.setDividerLocation(mainSplit.getWidth() - w);
        } else {
            // Remember the width when hiding so it can be restored later
            settings.aiPanelWidth = mainSplit.getWidth() - mainSplit.getDividerLocation();
        }
        if (aiMenuItem != null) aiMenuItem.setSelected(show);
    }

    private void openFile() {
        FileDialog fd = new FileDialog(frame, plugin == null ? "Open Assembly File"
                : "Open " + plugin.getName() + " Source File", FileDialog.LOAD);
        if (lastDirectory != null) fd.setDirectory(lastDirectory.getAbsolutePath());
        else fd.setDirectory(System.getProperty("user.dir"));
        fd.setFilenameFilter((dir, name) -> canOpen(name));
        fd.setVisible(true);
        if (fd.getFile() != null) {
            lastDirectory = new File(fd.getDirectory());
            Path p = Path.of(fd.getDirectory(), fd.getFile());
            if (!canOpen(p.getFileName().toString())) {
                JOptionPane.showMessageDialog(frame, "This " + baseTitle() + " window opens only "
                        + allowedDescription() + ".", baseTitle(), JOptionPane.WARNING_MESSAGE);
                return;
            }
            loadFile(p);
        }
    }

    /**
     * Opens {@code path} in a tab: selects its tab if it is already open, else loads it
     * into the editor if that is an empty untitled tab, else into a new tab.
     */
    void loadFile(Path path) {
        Path abs = path.toAbsolutePath().normalize();
        for (SourceTab t : tabs) {
            if (t.file != null && t.file.toAbsolutePath().normalize().equals(abs)) {
                editorTabs.setSelectedComponent(t.panel);
                return;
            }
        }
        try {
            String content = expandTabs(Files.readString(path));
            SourceTab t = cur != null && cur.isEmptyUntitled() ? cur : newTab();
            t.file = path;
            setTabText(t, content);
            updateTabTitle(t);
            if (t.file.getParent() != null) lastDirectory = t.file.toAbsolutePath().getParent().toFile();
            tabSelected();
        } catch (IOException e) {
            appendConsole("Error opening file: " + e.getMessage() + "\n");
        }
    }

    private void saveFile() {
        if (cur.readOnly()) {
            appendConsole(cur.name() + " is a read-only library file.\n");
            return;
        }
        if (cur.file == null) {
            saveFileAs();
            return;
        }
        try {
            Files.writeString(cur.file, codeEditor.getText());
            cur.modified = false;
            saveItem.setEnabled(false);
            updateTabTitle(cur);
        } catch (IOException e) {
            appendConsole("Error saving file: " + e.getMessage() + "\n");
        }
    }

    private void saveFileAs() {
        FileDialog fd = new FileDialog(frame, plugin == null ? "Save Assembly File"
                : "Save " + plugin.getName() + " Source File", FileDialog.SAVE);
        // Extension appended to a name typed without one: ".asm", or the plugin's first
        // source extension (none if the plugin names no extensions).
        final String defaultExt = plugin == null ? ".asm"
                : plugin.getFileExtensions().isEmpty() ? "" : "." + plugin.getFileExtensions().get(0);
        if (lastDirectory != null) fd.setDirectory(lastDirectory.getAbsolutePath());
        else fd.setDirectory(System.getProperty("user.dir"));
        if (cur.file != null) fd.setFile(cur.file.getFileName().toString());
        else fd.setFile("untitled" + defaultExt);
        fd.setFilenameFilter((dir, name) -> canOpen(name));
        fd.setVisible(true);
        if (fd.getFile() != null) {
            lastDirectory = new File(fd.getDirectory());
            Path target = Path.of(fd.getDirectory(), fd.getFile());
            if (!target.getFileName().toString().contains(".") || (plugin == null && !canOpen(target.getFileName().toString())))
                target = Path.of(target.toString() + defaultExt);
            if (!canOpen(target.getFileName().toString())) {
                JOptionPane.showMessageDialog(frame, "This " + baseTitle() + " window saves only "
                        + allowedDescription() + ".", baseTitle(), JOptionPane.WARNING_MESSAGE);
                return;
            }
            cur.resource = null;
            cur.area.setEditable(true);
            cur.file = target;
            saveFile();
            updateTabTitle(cur);
            updateRunnable();
        }
    }

    private void markModified() {
        if (!undoInProgress && !cur.modified) {
            cur.modified = true;
            saveItem.setEnabled(true);
        }
        updateUndoRedo();
    }

    private void updateUndoRedo() {
        undoItem.setEnabled(!cur.undoStack.isEmpty());
        redoItem.setEnabled(!cur.redoStack.isEmpty());
    }

    private void performUndo() {
        if (cur.undoStack.isEmpty()) return;
        undoInProgress = true;
        cur.redoStack.push(cur.lastSavedText);
        cur.lastSavedText = cur.undoStack.pop();
        int caret = codeEditor.getCaretPosition();
        codeEditor.setText(cur.lastSavedText);
        codeEditor.setCaretPosition(Math.min(caret, cur.lastSavedText.length()));
        undoInProgress = false;
        updateUndoRedo();
    }

    private void performRedo() {
        if (cur.redoStack.isEmpty()) return;
        undoInProgress = true;
        cur.undoStack.push(cur.lastSavedText);
        cur.lastSavedText = cur.redoStack.pop();
        int caret = codeEditor.getCaretPosition();
        codeEditor.setText(cur.lastSavedText);
        codeEditor.setCaretPosition(Math.min(caret, cur.lastSavedText.length()));
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
        if (!cur.modified) return true;
        int result = JOptionPane.showConfirmDialog(frame,
            "Do you want to save changes?", "CPUSim64",
            JOptionPane.YES_NO_CANCEL_OPTION, JOptionPane.WARNING_MESSAGE);
        if (result == JOptionPane.YES_OPTION) {
            saveFile();
            return !cur.modified; // false if save was cancelled
        }
        return result == JOptionPane.NO_OPTION;
    }

    private void runFile() {
        if (runThread != null) {
            Process np = nativeProcess;
            if (np != null) np.destroyForcibly();
            runThread.interrupt();
            return;
        }
        runWithMode(null);
    }

    /**
     * Master "Kill All": forcibly terminate a runaway simulation, including all
     * child CPUs, processes, and threads. First requests a cooperative stop on
     * every active simulator ({@link Simulator#killAll()}), then interrupts the
     * IDE's run thread so it unwinds. Safe to invoke even when nothing is running.
     */
    private void killAll() {
        // Forcibly stop every active simulator tree and interrupt all child threads.
        try {
            Simulator.killAll();
        } catch (Throwable t) {
            // Best-effort: never let Kill All throw.
        }
        // Interrupt the IDE's run thread so its execution loop exits promptly.
        Thread t = runThread;
        if (t != null) {
            t.interrupt();
        }
        // Forcibly terminate a native simulator subprocess, if one is running.
        Process np = nativeProcess;
        if (np != null) {
            np.destroyForcibly();
        }
        appendConsole("\n*** Kill All: forcibly stopping emulator and all threads/processes. ***\n");
    }

    private void runWithMode(String mode) {
        if (plugin != null && cur.file == null) saveFileAs();   // the compiler needs a file on disk
        if (cur.file == null) {
            appendConsole(plugin == null ? "No file open. Open an .asm file first.\n"
                    : "No file open. Open or save a " + plugin.getName() + " source file first.\n");
            return;
        }
        if (cur.modified && !promptSaveIfNeeded()) return;
        if (plugin != null && cur.file.toString().endsWith(".asm")) {
            appendConsole("A " + plugin.getName() + " source file must not have the .asm extension.\n");
            return;
        }
        buildFile = cur.file;

        // Show splash screen every 10 Run/Debug actions when not licensed
        runDebugCount++;
        if (runDebugCount % 10 == 0 && !LicenseDialog.isLicensed(settings)) {
            SplashScreen.show();
        }

        console.clear();

        String base = generatedBase();
        final String asmFile = base + ".asm";
        final String baseName = Path.of(base).getFileName().toString();
        final Path asmDir = Path.of(base).getParent();
        final Path objDir = asmDir.resolve("obj");

        new Thread(() -> {
            runThread = Thread.currentThread();
            console.setInterruptHandler(() -> {
                Process np = nativeProcess;
                if (np != null) np.destroyForcibly();
                if (runThread != null) runThread.interrupt();
            });
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

                if (plugin != null && compileSource() == null) return;   // errors are already in the console

                // Say() writes through the buffered console stream so messages stay in order with
                // the output of the assembler and the program.
                System.out.println("> Assembling " + Path.of(asmFile).getFileName() + "...");
                int asmResult = Assembler.run(assemblerArgs(Path.of(asmFile), mode != null));
                if (asmResult != 0) {
                    System.out.println("\nAssembly failed.");
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
                    System.out.println("\nNo .o64 file found after assembly.");
                    return;
                }

                System.out.println("> Running...");
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

                if (settings.useNativeSimulator) {
                    runNative(simArgs, pis, consoleStream);
                } else {
                    Simulation.run(simArgs.toArray(new String[0]));
                }

            } catch (Exception e) {
                System.out.println("Error: " + e.getMessage());
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
                Process np = nativeProcess;
                if (np != null) {
                    np.destroyForcibly();
                    nativeProcess = null;
                }
                runThread = null;
                SwingUtilities.invokeLater(() -> {
                    runBtn.setText("Run"); runItem.setText("Run");
                    updateRunnable();
                    console.disableInput();
                    console.setInterruptHandler(null);
                });
            }
        }).start();
    }

    /**
     * Run a program with the bundled native cpusim64_native executable instead
     * of the in-process Java simulator. Wires the subprocess stdout/stderr to the
     * console stream and the console input pipe ({@code pis}) to the subprocess
     * stdin. Honors Stop/Kill via {@link #nativeProcess} destruction.
     */
    private void runNative(java.util.List<String> simArgs, InputStream consoleIn, PrintStream consoleStream)
            throws IOException, InterruptedException {
        Path exe = NativeSimulator.resolveExecutable();
        if (exe == null) {
            SwingUtilities.invokeLater(() -> appendConsole(
                "\nNative emulator (cpusim64_native) not found. "
                + "Disable 'Use Native CPUSim64 Emulator' in Settings or reinstall.\n"));
            return;
        }

        java.util.List<String> cmd = new java.util.ArrayList<>();
        cmd.add(exe.toString());
        cmd.addAll(simArgs);

        ProcessBuilder pb = new ProcessBuilder(cmd);
        pb.redirectErrorStream(true); // merge stderr into stdout → single console stream
        // Run in the object file's directory so relative file I/O resolves as expected.
        Path objPath = Path.of(simArgs.get(0)).toAbsolutePath();
        if (objPath.getParent() != null) pb.directory(objPath.getParent().toFile());

        Process proc = pb.start();
        nativeProcess = proc;

        // Pump console input → subprocess stdin on a daemon thread.
        Thread stdinPump = new Thread(() -> {
            try (OutputStream pout = proc.getOutputStream()) {
                byte[] buf = new byte[1024];
                int n;
                while ((n = consoleIn.read(buf)) != -1) {
                    pout.write(buf, 0, n);
                    pout.flush();
                }
            } catch (IOException ignored) {
                // pipe closed / process ended
            }
        }, "native-stdin-pump");
        stdinPump.setDaemon(true);
        stdinPump.start();

        // Pump subprocess stdout → console stream on this thread.
        try (InputStream pin = proc.getInputStream()) {
            byte[] buf = new byte[1024];
            int n;
            while ((n = pin.read(buf)) != -1) {
                consoleStream.write(buf, 0, n);
            }
        } catch (IOException ignored) {
            // process ended or stream closed
        }

        try {
            proc.waitFor();
        } catch (InterruptedException ie) {
            // Stop/Kill requested: terminate the subprocess.
            proc.destroyForcibly();
            Thread.currentThread().interrupt();
            throw ie;
        } finally {
            if (nativeProcess == proc) nativeProcess = null;
        }
    }

    // ------------------------------------------------------------------
    // Compiler plugin support
    // ------------------------------------------------------------------

    /** Path of the current file without its (last) extension; the compiler writes {@code <base>.asm}. */
    private String generatedBase() {
        String base = buildFile.toString();
        if (plugin == null) {
            return base.endsWith(".asm") ? base.substring(0, base.length() - 4) : base;
        }
        int slash = Math.max(base.lastIndexOf('/'), base.lastIndexOf('\\'));
        int dot = base.lastIndexOf('.');
        return dot > slash ? base.substring(0, dot) : base;
    }

    /** {@code #export} / {@code #extern} lines mark a relocatable module that must be linked. */
    private static final Pattern MODULE_DIRECTIVE = Pattern.compile("(?im)^\\s*#(export|extern)\\b");

    /** True if {@code asm} is a module (it has #export or #extern) and is built with -r and linked. */
    static boolean needsLink(Path asm) {
        try {
            return MODULE_DIRECTIVE.matcher(Files.readString(asm)).find();
        } catch (IOException e) {
            return false;
        }
    }

    /**
     * Command line for {@link Assembler#run}. A module ({@link #needsLink}) is
     * assembled with -r and linked with the modules it #includes straight into
     * obj/name.o64; other files are assembled as one program.
     */
    private String[] assemblerArgs(Path asm, boolean debug) {
        java.util.List<String> a = new java.util.ArrayList<>();
        a.add(asm.toString());
        if (debug) a.add("--DEBUG");
        if (needsLink(asm)) {
            String base = asm.getFileName().toString().replaceAll("(?i)\\.asm$", "");
            a.add(0, "-r");
            a.add("-o");
            a.add(asm.toAbsolutePath().resolveSibling("obj").resolve(base + ".o64").toString());
            return a.toArray(new String[0]);
        }
        // Compilers and generated programs may start at a __MAIN__ label (as assemble.sh detects).
        if (plugin != null && CompilerPlugin.hasMainLabel(asm)) a.add("--hasMain");
        return a.toArray(new String[0]);
    }

    /** Moves the assembler's output files from the source directory into its obj/ subdirectory. */
    private static void moveAssemblerOutputs(Path dir, String baseName) throws IOException {
        Path obj = dir.resolve("obj");
        Files.createDirectories(obj);
        for (String ext : new String[] {".o64", ".sym", ".sym1", ".sym2", ".srcmap"}) {
            Path src = dir.resolve(baseName + ext);
            if (Files.exists(src)) {
                Files.move(src, obj.resolve(baseName + ext), StandardCopyOption.REPLACE_EXISTING);
            }
        }
    }

    /**
     * Compiler windows: (re)assembles the compiler when its object file is missing or older
     * than its source, then compiles the current source file to {@code <source>.asm}.
     * Runs on a worker thread with System.out/err pointing at the console. Returns the
     * generated assembly file, or null after reporting the problem in the console.
     */
    private Path compileSource() throws Exception {
        final CompilerPlugin pl = plugin;
        Path root = pl.workingRoot();
        Path compilerAsm = pl.compilerAsm(root);
        if (!Files.isRegularFile(compilerAsm)) {
            System.out.println("Compiler source not found: " + compilerAsm);
            return null;
        }
        Path compilerObj = pl.compilerObj(root);
        if (pl.needsBuild(root)) {
            System.out.println("> Assembling the " + pl.getName() + " compiler (" + compilerAsm.getFileName() + ")...");
            Files.deleteIfExists(compilerObj);
            int rc = Assembler.run(assemblerArgs(compilerAsm, false));
            moveAssemblerOutputs(root, pl.getCompilerSource());
            if (rc != 0 || !Files.isRegularFile(compilerObj)) {
                System.out.println("\nAssembly of the " + pl.getName() + " compiler failed.");
                return null;
            }
        }

        Path source = buildFile.toAbsolutePath();
        Path asm = Path.of(generatedBase() + ".asm");
        Files.deleteIfExists(asm);           // never run a stale result of an earlier compile
        System.out.println("> Compiling " + source.getFileName() + "...");
        int result = runCompiler(pl, compilerObj, source);
        if (result != 0) {
            System.out.println("\nCompilation failed" + (result > 0 ? " (exit code " + result + ")" : "") + ".");
            return null;
        }
        if (!Files.isRegularFile(asm)) {
            System.out.println("\nThe compiler did not produce " + asm.getFileName() + ".");
            return null;
        }
        return asm;
    }

    /** Runs the compiler program on {@code source}; returns its exit code, or -1 if it did not finish. */
    /** The words of the Options field; file names are made absolute (relative to the source's directory). */
    private java.util.List<String> compilerOptions(Path source) {
        java.util.List<String> out = new java.util.ArrayList<>();
        if (optionsField == null) return out;
        String text = optionsField.getText().trim();
        if (text.isEmpty()) return out;
        for (String w : text.split("\\s+")) {
            if (w.startsWith("-")) out.add(w);
            else out.add(source.toAbsolutePath().getParent().resolve(w).normalize().toString());
        }
        return out;
    }

    private int runCompiler(CompilerPlugin pl, Path compilerObj, Path source) throws Exception {
        java.util.List<String> simArgs = new java.util.ArrayList<>();
        simArgs.add(compilerObj.toString());
        simArgs.add("--mem=" + pl.getCompilerMem());
        simArgs.add("--stack=" + pl.getCompilerStack());
        simArgs.add(source.toString());                  // the compiler's argument
        for (String opt : compilerOptions(source)) simArgs.add(opt);
        if (settings.useNativeSimulator) {
            return runCompilerNative(simArgs, source.getParent());
        }
        StdInterruptHandler.setGlobalTerminalSizeProvider(new StdInterruptHandler.TerminalSizeProvider() {
            @Override public int getColumns() { return console.getVisibleCols(); }
            @Override public int getRows() { return console.getVisibleRows(); }
        });
        long[] result = new long[1];
        int rc = Simulation.run(simArgs.toArray(new String[0]), result);
        return rc != 0 ? rc : (int) result[0];
    }

    /**
     * Runs the compiler with the native simulator. Its exit code is only reported by --verbose
     * ("Result: N"), so the banner and statistics that --verbose adds are filtered out again.
     */
    private int runCompilerNative(java.util.List<String> simArgs, Path workDir) throws IOException, InterruptedException {
        Path exe = NativeSimulator.resolveExecutable();
        if (exe == null) {
            System.out.println("\nNative emulator (cpusim64_native) not found. "
                + "Disable 'Use Native CPUSim64 Emulator' in Settings or reinstall.");
            return -1;
        }
        java.util.List<String> cmd = new java.util.ArrayList<>();
        cmd.add(exe.toString());
        cmd.add(simArgs.get(0));
        cmd.add("--verbose");
        cmd.addAll(simArgs.subList(1, simArgs.size()));
        ProcessBuilder pb = new ProcessBuilder(cmd);
        pb.redirectErrorStream(true);
        if (workDir != null) pb.directory(workDir.toFile());
        Process proc = pb.start();
        nativeProcess = proc;
        proc.getOutputStream().close();                  // compilers do not read stdin
        int result = -1;
        try (java.io.BufferedReader in = new java.io.BufferedReader(
                new java.io.InputStreamReader(proc.getInputStream(), java.nio.charset.StandardCharsets.UTF_8))) {
            int phase = 0;                               // 0 banner, 1 program output, 2 statistics
            String line;
            while ((line = in.readLine()) != null) {
                if (phase == 0 && line.matches("^(=+|CPUSim64 .*Emulator.*|\u00A9.*|Implements a CPUSim64.*|Debug: .*"
                        + "|Memory size: .*|Stack size: .*|Read \\d+ words from .*)$")) {
                    // "Read N words" is the last banner line; everything after it is program output
                    if (line.startsWith("Read ")) phase = 1;
                    continue;
                }
                if (phase < 2) {
                    phase = 1;
                    Matcher m = Pattern.compile("^Result: (-?\\d+)$").matcher(line);
                    if (m.matches()) {
                        result = (int) Long.parseLong(m.group(1));
                        phase = 2;
                        continue;
                    }
                } else if (line.matches("^(User CPU Cycles|User Time|System Time|Total Time|Heap Allocated \\w+): .*")) {
                    continue;
                }
                System.out.println(line);
            }
            proc.waitFor();
        } catch (InterruptedException ie) {
            proc.destroyForcibly();
            Thread.currentThread().interrupt();
            throw ie;
        } finally {
            if (nativeProcess == proc) nativeProcess = null;
        }
        return result;
    }

    /** Console output stream used when no run is in progress; every flush appends to the console. */
    private final class ConsoleOut extends OutputStream {
        private final ByteArrayOutputStream buf = new ByteArrayOutputStream();
        @Override public synchronized void write(int b) { buf.write(b); }
        @Override public synchronized void write(byte[] b, int off, int len) { buf.write(b, off, len); }
        @Override public void flush() {
            String text;
            synchronized (this) {
                text = buf.toString(java.nio.charset.StandardCharsets.UTF_8);
                buf.reset();
            }
            if (!text.isEmpty()) SwingUtilities.invokeLater(() -> appendConsole(text));
        }
    }

    /**
     * Debug in a compiler window: compile the source, then open the generated assembly
     * program in a regular CPUSim64 window and debug it there, so the debugger shows the
     * assembly source.
     */
    private void launchCompilerDebugger() {
        if (cur.file == null) saveFileAs();
        if (cur.file == null) {
            appendConsole("No file open. Open or save a " + plugin.getName() + " source file first.\n");
            return;
        }
        if (cur.modified && !promptSaveIfNeeded()) return;
        if (cur.file.toString().endsWith(".asm")) {
            appendConsole("A " + plugin.getName() + " source file must not have the .asm extension.\n");
            return;
        }
        buildFile = cur.file;
        runDebugCount++;
        if (runDebugCount % 10 == 0 && !LicenseDialog.isLicensed(settings)) {
            SplashScreen.show();
        }
        console.clear();
        final String userArgs = argsField.getText().trim();
        new Thread(() -> {
            PrintStream origOut = System.out;
            PrintStream origErr = System.err;
            PrintStream ps = new PrintStream(new ConsoleOut(), true, java.nio.charset.StandardCharsets.UTF_8);
            Path generated = null;
            try {
                System.setOut(ps);
                System.setErr(ps);
                generated = compileSource();
            } catch (Exception e) {
                System.out.println("Error: " + e.getMessage());
            } finally {
                ps.flush();
                System.setOut(origOut);
                System.setErr(origErr);
            }
            if (generated == null) return;
            final Path asm = generated;
            SwingUtilities.invokeLater(() -> {
                appendConsole("> Opening " + asm.getFileName() + " in the debugger...\n");
                IDEApp app = new IDEApp();
                app.createAndShowGUI();
                app.loadFile(asm);
                app.argsField.setText(userArgs);
                app.launchDebugger();
            });
        }, "compile-for-debug").start();
    }

    private void launchDebugger() {
        if (plugin != null) {
            launchCompilerDebugger();
            return;
        }
        if (cur.file == null) {
            appendConsole("No file open. Open an .asm file first.\n");
            return;
        }
        if (cur.modified && !promptSaveIfNeeded()) return;

        // Show splash screen every 10 Run/Debug actions when not licensed
        runDebugCount++;
        if (runDebugCount % 10 == 0 && !LicenseDialog.isLicensed(settings)) {
            SplashScreen.show();
        }

        console.clear();

        buildFile = cur.file;
        final SourceTab debugTab = cur;
        final Path debugFile = cur.file;
        String base = cur.file.toString();
        if (base.endsWith(".asm")) base = base.substring(0, base.length() - 4);
        final String asmFile = base + ".asm";
        final String baseName = Path.of(base).getFileName().toString();
        final Path asmDir = Path.of(base).getParent();
        final Path objDir = asmDir.resolve("obj");

        new Thread(() -> {
            try {
                // Assemble with --DEBUG
                SwingUtilities.invokeLater(() -> appendConsole("> Assembling (debug) " + debugFile.getFileName() + "...\n"));
                int asmResult = Assembler.run(assemblerArgs(Path.of(asmFile), true));
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
                    debuggerOpen = true;
                    updateRunnable();
                    Runnable onClose = () -> { debuggerOpen = false; updateRunnable(); };
                    editorTabs.setSelectedComponent(debugTab.panel);
                    new DebuggerWindow(frame, objFile, asmFile, debugTab.panel, sourceProvider(debugFile),
                            argsField.getText().trim(), settings, console, onClose);
                });
            } catch (Exception e) {
                SwingUtilities.invokeLater(() -> appendConsole("Error: " + e.getMessage() + "\n"));
            }
        }).start();
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
        var iconUrl = IDEApp.class.getResource("/app_icon_256.png");
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
