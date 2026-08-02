// SPDX-License-Identifier: Apache-2.0
package com.glowingcat;

import javax.swing.*;
import javax.swing.table.*;
import java.awt.*;
import java.awt.event.*;
import java.nio.file.*;
import java.io.*;
import java.util.*;
import java.util.List;
import cloud.lesh.CPUSim64.*;

public class DebuggerWindow extends JFrame {
    private Simulator sim;
    private final EditorPanel editorPanel;
    private JTextArea disasmArea;
    private JTable regTable;
    private DefaultTableModel regModel;
    private JTable stackTable;
    private DefaultTableModel stackModel;
    private long startPC;
    private Map<Long, String> reverseSymbolMap;
    private final Set<Long> breakpointAddresses = new HashSet<>();
    private final List<Long> disasmAddresses = new ArrayList<>();
    // Display mode: false=decimal, true=hex per register row
    private final boolean[] intRegHex = new boolean[32];
    // Stack display: 0=dec, 1=hex, 2=float per row
    private final Map<Long, Integer> stackDisplayMode = new HashMap<>();
    private final List<Long> stackRowAddresses = new ArrayList<>();

    private final Map<Integer, Long> sourceLineToAddr = new HashMap<>();
    private final Map<Long, Integer> addrToSourceLine = new HashMap<>();
    private JSplitPane mainSplit;
    private JSplitPane regStackSplit;
    private final AppSettings settings;
    private final PrintStream origOut = System.out;
    private final PrintStream origErr = System.err;
    private final InputStream origIn = System.in;

    public DebuggerWindow(JFrame parent, String objFilePath, String sourceFilePath, EditorPanel editorPanel, String userArgs, AppSettings settings, TerminalPanel console, Runnable onClose) {
        super("CPUSim64 Debugger");
        this.editorPanel = editorPanel;
        this.settings = settings;
        Font monoFont = new Font(settings.fontName, Font.PLAIN, settings.fontSize);

        // Redirect System.out/err to IDE console
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
                    SwingUtilities.invokeLater(() -> console.write(text));
                }
            }
        }, true, java.nio.charset.StandardCharsets.UTF_8);
        System.setOut(consoleStream);
        System.setErr(consoleStream);

        // Redirect System.in from IDE console
        try {
            PipedOutputStream inputPipe = new PipedOutputStream();
            PipedInputStream pis = new PipedInputStream(inputPipe);
            System.setIn(pis);
            console.enableInput(inputPipe);
        } catch (Exception ignored) {}

        // Load program
        Path objPath = Path.of(objFilePath).toAbsolutePath();
        String baseName = objPath.getFileName().toString().replace(".o64", "");
        Path symPath = objPath.resolveSibling(baseName + ".sym1");
        Map<Long, String> rsm = null;
        try {
            if (Files.exists(symPath)) {
                rsm = Simulator.readReverseLabelMapFromFile(symPath.toFile());
                if (rsm.get(0L) == null) rsm.put(0L, "__START__");
            }
        } catch (Exception ignored) {}
        reverseSymbolMap = rsm;

        List<Long> program;
        try {
            program = AsmIO.readU64BE(objPath.toFile());
        } catch (Exception e) {
            JOptionPane.showMessageDialog(parent, "Failed to load: " + e.getMessage());
            dispose();
            return;
        }

        // Create simulator
        List<String> simArgs = new ArrayList<>();
        simArgs.add(objFilePath);
        if (userArgs != null && !userArgs.isEmpty()) {
            for (String a : userArgs.split("\\s+")) simArgs.add(a);
        }
        sim = new Simulator((int)(settings.heapSizeMiB * 1024 * 1024), 0, (int)(settings.stackSizeKiB * 1024), simArgs.toArray(new String[0]));
        sim.setDebug(true);
        sim.loadProgram(program, 0L, reverseSymbolMap);
        startPC = program.get(0);
        sim.initForDebug(startPC);

        // Build source-line-to-address map using .sym file
        buildLineMap(objPath, sourceFilePath);
        syncBreakpointsFromSource();

        // Listen for source breakpoint changes during debugging
        editorPanel.setBreakpointChangeListener((srcLine) -> {
            syncBreakpointsFromSource();
            Long addr = sourceLineToAddr.get(srcLine);
            if (addr != null) {
                scrollDisassemblyToAddress(addr);
            } else {
                updateDisassembly(false);
            }
        });

        // Build UI
        setSize(settings.debugWindowWidth, settings.debugWindowHeight);
        setLocationRelativeTo(parent);
        setDefaultCloseOperation(DISPOSE_ON_CLOSE);
        addWindowListener(new WindowAdapter() {
            @Override public void windowClosed(WindowEvent e) {
                settings.debugMainDivider = mainSplit.getDividerLocation();
                settings.debugRegStackDivider = regStackSplit.getDividerLocation();
                settings.debugWindowWidth = getWidth();
                settings.debugWindowHeight = getHeight();
                settings.debugRegColWidths = new int[4];
                for (int i = 0; i < 4; i++) settings.debugRegColWidths[i] = regTable.getColumnModel().getColumn(i).getWidth();
                settings.debugStackColWidths = new int[3];
                for (int i = 0; i < 3; i++) settings.debugStackColWidths[i] = stackTable.getColumnModel().getColumn(i).getWidth();
                settings.save();
                sim.stop();
                if (runningThread != null) {
                    runningThread.interrupt();
                    runningThread = null;
                }
                editorPanel.clearExecutionLine();
                editorPanel.setBreakpointChangeListener(null);
                System.setOut(origOut);
                System.setErr(origErr);
                System.setIn(origIn);
                if (onClose != null) onClose.run();
            }
        });

        // Left pane: buttons + disassembly
        JPanel leftPanel = new JPanel(new BorderLayout());
        JToolBar toolbar = new JToolBar();
        toolbar.setFloatable(false);
        Font btnFont = monoFont.deriveFont(Font.BOLD);
        JButton stepOverBtn = new JButton("Step Over");
        JButton stepIntoBtn = new JButton("Step Into");
        JButton stepOutBtn = new JButton("Step Out");
        JButton resumeBtn = new JButton("Resume");
        JButton stopBtn = new JButton("Stop");
        stepOverBtn.setFont(btnFont);
        stepIntoBtn.setFont(btnFont);
        stepOutBtn.setFont(btnFont);
        resumeBtn.setFont(btnFont);
        stopBtn.setFont(btnFont);
        toolbar.add(stepOverBtn);
        toolbar.add(stepIntoBtn);
        toolbar.add(stepOutBtn);
        toolbar.addSeparator();
        toolbar.add(resumeBtn);
        toolbar.add(stopBtn);
        leftPanel.add(toolbar, BorderLayout.NORTH);

        disasmArea = new JTextArea();
        disasmArea.setEditable(false);
        disasmArea.setFont(monoFont);
        disasmArea.setBackground(new Color(30, 30, 30));
        disasmArea.setForeground(Color.WHITE);
        disasmArea.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                try {
                    int offset = disasmArea.viewToModel2D(e.getPoint());
                    int line = disasmArea.getLineOfOffset(offset);
                    if (line >= 0 && line < disasmAddresses.size()) {
                        long addr = disasmAddresses.get(line);
                        if (addr >= 0) {
                            if (breakpointAddresses.contains(addr)) {
                                breakpointAddresses.remove(addr);
                                syncBreakpointToSource(addr, false);
                            } else {
                                breakpointAddresses.add(addr);
                                syncBreakpointToSource(addr, true);
                            }
                            int caretPos = disasmArea.getCaretPosition();
                            JScrollPane sp = (JScrollPane) SwingUtilities.getAncestorOfClass(JScrollPane.class, disasmArea);
                            int scrollVal = sp != null ? sp.getVerticalScrollBar().getValue() : 0;
                            updateDisassembly(false);
                            disasmArea.setCaretPosition(Math.min(caretPos, disasmArea.getDocument().getLength()));
                            if (sp != null) {
                                SwingUtilities.invokeLater(() -> sp.getVerticalScrollBar().setValue(scrollVal));
                            }
                        }
                    }
                } catch (Exception ignored) {}
            }
        });
        JScrollPane disasmScroll = new JScrollPane(disasmArea);
        leftPanel.add(disasmScroll, BorderLayout.CENTER);

        // Right pane: registers + stack
        JPanel rightPanel = new JPanel(new BorderLayout());

        // Register table: 4 columns (IntReg, Value, FPReg, Value)
        regModel = new DefaultTableModel(new String[]{"Reg", "Value", "FP Reg", "Value"}, 33);
        regTable = new JTable(regModel) {
            @Override public boolean isCellEditable(int r, int c) { return false; }
        };
        regTable.setFont(monoFont);
        regTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        regTable.getTableHeader().setFont(monoFont);
        regTable.setRowHeight(regTable.getRowHeight() + 4);
        javax.swing.table.DefaultTableCellRenderer padRenderer = new javax.swing.table.DefaultTableCellRenderer() {
            @Override public Component getTableCellRendererComponent(JTable t, Object v, boolean s, boolean f, int r, int c) {
                Component comp = super.getTableCellRendererComponent(t, v, s, f, r, c);
                setBorder(BorderFactory.createEmptyBorder(2, 4, 2, 4));
                return comp;
            }
        };
        padRenderer.setFont(monoFont);
        for (int i = 0; i < regTable.getColumnCount(); i++) regTable.getColumnModel().getColumn(i).setCellRenderer(padRenderer);
        regTable.getColumnModel().getColumn(0).setPreferredWidth(40);
        regTable.getColumnModel().getColumn(1).setPreferredWidth(140);
        regTable.getColumnModel().getColumn(2).setPreferredWidth(40);
        regTable.getColumnModel().getColumn(3).setPreferredWidth(160);
        if (settings.debugRegColWidths != null && settings.debugRegColWidths.length == 4) {
            for (int i = 0; i < 4; i++) regTable.getColumnModel().getColumn(i).setPreferredWidth(settings.debugRegColWidths[i]);
        }
        regTable.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                int row = regTable.rowAtPoint(e.getPoint());
                int col = regTable.columnAtPoint(e.getPoint());
                if (row >= 0 && row < 32 && col == 1) {
                    if (SwingUtilities.isRightMouseButton(e)) {
                        long addr = sim.getRegisters()[row];
                        MemoryWindow.showMemory(sim, addr, regTable.getFont(), settings);
                    } else if (SwingUtilities.isLeftMouseButton(e)) {
                        intRegHex[row] = !intRegHex[row];
                        updateRegisters();
                    }
                }
            }
        });
        JScrollPane regScroll = new JScrollPane(regTable);
        regScroll.setPreferredSize(new Dimension(420, 0));

        // Stack table
        stackModel = new DefaultTableModel(new String[]{"", "Address", "Value"}, 0);
        stackTable = new JTable(stackModel) {
            @Override public boolean isCellEditable(int r, int c) { return false; }
        };
        stackTable.setFont(monoFont);
        stackTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        stackTable.getTableHeader().setFont(monoFont);
        stackTable.setRowHeight(stackTable.getRowHeight() + 4);
        for (int i = 0; i < stackTable.getColumnCount(); i++) stackTable.getColumnModel().getColumn(i).setCellRenderer(padRenderer);
        stackTable.getColumnModel().getColumn(0).setPreferredWidth(30);
        stackTable.getColumnModel().getColumn(1).setPreferredWidth(80);
        stackTable.getColumnModel().getColumn(2).setPreferredWidth(140);
        if (settings.debugStackColWidths != null && settings.debugStackColWidths.length == 3) {
            for (int i = 0; i < 3; i++) stackTable.getColumnModel().getColumn(i).setPreferredWidth(settings.debugStackColWidths[i]);
        }
        stackTable.addMouseListener(new MouseAdapter() {
            @Override public void mouseClicked(MouseEvent e) {
                int row = stackTable.rowAtPoint(e.getPoint());
                int col = stackTable.columnAtPoint(e.getPoint());
                if (row >= 0 && col == 2 && row < stackRowAddresses.size()) {
                    if (SwingUtilities.isLeftMouseButton(e) && !e.isPopupTrigger()) {
                        long addr = stackRowAddresses.get(row);
                        int mode = stackDisplayMode.getOrDefault(addr, 0);
                        stackDisplayMode.put(addr, (mode + 1) % 4);
                        updateStack();
                    }
                }
            }
            @Override public void mousePressed(MouseEvent e) { maybeShowPopup(e); }
            @Override public void mouseReleased(MouseEvent e) { maybeShowPopup(e); }
            private void maybeShowPopup(MouseEvent e) {
                if (e.isPopupTrigger() || SwingUtilities.isRightMouseButton(e)) {
                    int row = stackTable.rowAtPoint(e.getPoint());
                    int col = stackTable.columnAtPoint(e.getPoint());
                    if (row >= 0 && col == 2 && row < stackRowAddresses.size()) {
                        long addr = stackRowAddresses.get(row);
                        showStackContextMenu(e, addr);
                    }
                }
            }
        });
        JScrollPane stackScroll = new JScrollPane(stackTable);
        stackScroll.setPreferredSize(new Dimension(280, 0));

        regStackSplit = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, regScroll, stackScroll);
        regStackSplit.setResizeWeight(0.6);
        rightPanel.add(regStackSplit, BorderLayout.CENTER);

        mainSplit = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, leftPanel, rightPanel);
        mainSplit.setResizeWeight(0.5);
        add(mainSplit);

        // Button actions
        stepOverBtn.addActionListener(e -> stepOver());
        stepIntoBtn.addActionListener(e -> stepInto());
        stepOutBtn.addActionListener(e -> stepOut());
        resumeBtn.addActionListener(e -> resume());
        stopBtn.addActionListener(e -> stopDebugger());

        // Initial state
        updateAll();
        setVisible(true);
        if (settings.debugMainDivider > 0) {
            mainSplit.setDividerLocation(settings.debugMainDivider);
        } else {
            mainSplit.setDividerLocation(0.5);
        }
        if (settings.debugRegStackDivider > 0) {
            regStackSplit.setDividerLocation(settings.debugRegStackDivider);
        }
    }

    private volatile Thread runningThread; // background execution thread (stepOver/stepOut/resume)

    private void setButtonsEnabled(boolean enabled) {
        for (Component c : ((JToolBar) ((JPanel) mainSplit.getLeftComponent()).getComponent(0)).getComponents()) {
            if (c instanceof JButton) c.setEnabled(enabled);
        }
    }

    private void stepInto() {
        if (!sim.isRunning()) return;
        sim.stepOne();
        updateAll();
    }

    private void stepOver() {
        if (!sim.isRunning()) return;
        long pc = sim.getPC();
        long instr = sim.memRead(pc);
        var d = Simulator.Decoded.decode(instr);
        if (d.getOpCode() == Opcode.CALL.getCode()) {
            // Run on background thread until return or breakpoint
            long returnAddr = pc + 1;
            setButtonsEnabled(false);
            runningThread = new Thread(() -> {
                while (sim.isRunning() && !Thread.currentThread().isInterrupted()) {
                    sim.stepOne();
                    if (sim.getPC() == returnAddr || isBreakpoint(sim.getPC())) break;
                }
                runningThread = null;
                SwingUtilities.invokeLater(() -> { setButtonsEnabled(true); updateAll(); });
            });
            runningThread.start();
        } else {
            sim.stepOne();
            updateAll();
        }
    }

    private void stepOut() {
        if (!sim.isRunning()) return;
        setButtonsEnabled(false);
        runningThread = new Thread(() -> {
            int depth = 0;
            while (sim.isRunning() && !Thread.currentThread().isInterrupted()) {
                int op = sim.stepOne();
                if (op == Opcode.CALL.getCode()) depth++;
                else if (op == Opcode.RETURN.getCode()) {
                    if (depth == 0) break;
                    depth--;
                }
                if (isBreakpoint(sim.getPC())) break;
            }
            runningThread = null;
            SwingUtilities.invokeLater(() -> { setButtonsEnabled(true); updateAll(); });
        });
        runningThread.start();
    }

    private void resume() {
        if (!sim.isRunning()) return;
        sim.stepOne(); // step past current breakpoint
        setButtonsEnabled(false);
        runningThread = new Thread(() -> {
            while (sim.isRunning() && !Thread.currentThread().isInterrupted()) {
                if (isBreakpoint(sim.getPC())) break;
                sim.stepOne();
            }
            runningThread = null;
            SwingUtilities.invokeLater(() -> { setButtonsEnabled(true); updateAll(); });
        });
        runningThread.start();
    }

    private void stopDebugger() {
        sim.stop();
        if (runningThread != null) {
            runningThread.interrupt();
            runningThread = null;
        }
        editorPanel.clearExecutionLine();
        dispose();
    }

    private boolean isBreakpoint(long pc) {
        return breakpointAddresses.contains(pc);
    }

    /** Sync source line breakpoints to debugger address breakpoints */
    private void syncBreakpointsFromSource() {
        Set<Integer> srcBps = editorPanel.getEnabledBreakpoints();
        // Build the expected set of addresses from source breakpoints
        Set<Long> expected = new HashSet<>();
        for (int line : srcBps) {
            Long addr = sourceLineToAddr.get(line);
            if (addr != null) expected.add(addr);
        }
        // Add missing, remove stale (only for addresses that have a source mapping)
        for (var entry : addrToSourceLine.entrySet()) {
            long addr = entry.getKey();
            if (expected.contains(addr)) {
                breakpointAddresses.add(addr);
            } else {
                breakpointAddresses.remove(addr);
            }
        }
    }

    /** When a breakpoint is set/removed in disasm, update the source line panel */
    private void syncBreakpointToSource(long addr, boolean set) {
        Integer srcLine = addrToSourceLine.get(addr);
        if (srcLine != null) {
            if (set) editorPanel.setBreakpoint(srcLine);
            else editorPanel.removeBreakpoint(srcLine);
        }
    }

    private void buildLineMap(Path objPath, String sourceFilePath) {
        // Read the .srcmap file written by the assembler (address → «filename»:lineNum)
        String baseName = objPath.getFileName().toString().replace(".o64", "");
        Path srcmapPath = objPath.resolveSibling(baseName + ".srcmap");
        if (!Files.exists(srcmapPath)) return;

        String sourceFileName = Path.of(sourceFilePath).getFileName().toString();
        try (BufferedReader reader = new BufferedReader(new FileReader(srcmapPath.toFile()))) {
            String line;
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty()) continue;
                int colon = line.indexOf(':');
                if (colon == -1) continue;
                long addr;
                try { addr = Long.parseLong(line.substring(0, colon).trim()); }
                catch (NumberFormatException e) { continue; }
                // Value is «filename»:lineNum - parse it
                String value = line.substring(colon + 1).trim();
                // Format: «filename»:lineNum
                int lastColon = value.lastIndexOf(':');
                if (lastColon == -1) continue;
                String file = value.substring(0, lastColon).trim();
                int srcLine;
                try { srcLine = Integer.parseInt(value.substring(lastColon + 1).trim()); }
                catch (NumberFormatException e) { continue; }
                // Strip «» from filename
                if (file.startsWith("\u00ab")) file = file.substring(1);
                if (file.endsWith("\u00bb")) file = file.substring(0, file.length() - 1);
                // Only map lines from the main source file
                if (!file.equals(sourceFileName)) continue;
                // First address for a given source line wins (most relevant for breakpoints)
                sourceLineToAddr.putIfAbsent(srcLine, addr);
                addrToSourceLine.putIfAbsent(addr, srcLine);
            }
        } catch (Exception ignored) {}
    }

    private void updateAll() {
        syncBreakpointsFromSource();
        updateDisassembly();
        updateRegisters();
        updateStack();
        if (sim.isRunning()) {
            Integer srcLine = addrToSourceLine.get(sim.getPC());
            if (srcLine != null) {
                editorPanel.setExecutionLine(srcLine);
                editorPanel.scrollToLine(srcLine);
            }
            else editorPanel.clearExecutionLine();
        } else {
            // Program has terminated — close the debugger
            editorPanel.clearExecutionLine();
            dispose();
        }
    }

    private void updateDisassembly() {
        updateDisassembly(true);
    }

    private void updateDisassembly(boolean scrollToPC) {
        long pc = sim.getPC();
        long heapStart = sim.getHeapStart();
        int windowSize = 80;
        long start = Math.max(1, pc - windowSize / 2);
        long end = Math.min(heapStart, pc + windowSize / 2);

        disasmAddresses.clear();
        StringBuilder sb = new StringBuilder();
        int pcLineIndex = 0;
        int lineCount = 0;
        for (long addr = start; addr < end; addr++) {
            String label = "";
            if (reverseSymbolMap != null) {
                String l = reverseSymbolMap.get(addr);
                if (l != null) label = l + ":\n";
            }
            String marker;
            if (addr == pc) marker = " \u25B6 ";
            else if (breakpointAddresses.contains(addr)) marker = " \u25CF ";
            else marker = "   ";
            String disasm;
            try { disasm = sim.disassembleOne(addr); } catch (Exception e) { break; }
            if (!label.isEmpty()) {
                sb.append(label);
                disasmAddresses.add(-1L); // label lines have no address
                lineCount++;
            }
            sb.append(String.format("%s%08x: %s\n", marker, addr, disasm));
            disasmAddresses.add(addr);
            if (addr == pc) pcLineIndex = lineCount;
            lineCount++;
        }
        disasmArea.setText(sb.toString());

        // Center the PC line
        if (scrollToPC) {
            final int pcLine = pcLineIndex;
            SwingUtilities.invokeLater(() -> {
                try {
                    int startOffset = disasmArea.getLineStartOffset(pcLine);
                    disasmArea.setCaretPosition(startOffset);
                    Rectangle r = disasmArea.modelToView(startOffset);
                    if (r != null) {
                        int viewH = disasmArea.getVisibleRect().height;
                        r.y -= viewH / 2;
                        r.height = viewH;
                        disasmArea.scrollRectToVisible(r);
                    }
                } catch (Exception ignored) {}
            });
        }
    }

    private void scrollDisassemblyToAddress(long targetAddr) {
        updateDisassembly(false);
        int targetLine = -1;
        for (int i = 0; i < disasmAddresses.size(); i++) {
            if (disasmAddresses.get(i) == targetAddr) { targetLine = i; break; }
        }
        if (targetLine >= 0) {
            final int line = targetLine;
            SwingUtilities.invokeLater(() -> {
                try {
                    int startOffset = disasmArea.getLineStartOffset(line);
                    disasmArea.setCaretPosition(startOffset);
                    Rectangle r = disasmArea.modelToView(startOffset);
                    if (r != null) {
                        int viewH = disasmArea.getVisibleRect().height;
                        r.y -= viewH / 2;
                        r.height = viewH;
                        disasmArea.scrollRectToVisible(r);
                    }
                } catch (Exception ignored) {}
            });
        }
    }

    private int getDisasmLineAtOffset(int offset) {
        try { return disasmArea.getLineOfOffset(offset); }
        catch (Exception e) { return -1; }
    }

    private void updateRegisters() {
        long[] R = sim.getRegisters();
        double[] F = sim.getFPRegisters();
        String[] regNames = {"R0","R1","R2","R3","R4","R5","R6","R7","R8","R9",
            "R10","R11","R12","R13","R14","R15","R16","R17","R18","R19",
            "R20","R21","R22","R23","R24","R25","R26","R27","R28","SF","SP","PC"};
        for (int i = 0; i < 32; i++) {
            String intName = regNames[i];
            String intVal = intRegHex[i] ? String.format("0x%016X", R[i]) : Long.toString(R[i]);
            String fpName = "F" + i;
            String fpVal = String.format("%.16g", F[i]);
            regModel.setValueAt(intName, i, 0);
            regModel.setValueAt(intVal, i, 1);
            regModel.setValueAt(fpName, i, 2);
            regModel.setValueAt(fpVal, i, 3);
        }
        // Status Register row
        long sr = sim.getSR();
        String srStr = ((sr & Simulator.SR_P) != 0 ? "P" : "p") +
                       ((sr & Simulator.SR_Z) != 0 ? "Z" : "z") +
                       ((sr & Simulator.SR_S) != 0 ? "S" : "s") +
                       ((sr & Simulator.SR_O) != 0 ? "O" : "o");
        regModel.setValueAt("SR", 32, 0);
        regModel.setValueAt(srStr, 32, 1);
        regModel.setValueAt("", 32, 2);
        regModel.setValueAt("", 32, 3);
    }

    private void showStackContextMenu(MouseEvent e, long addr) {
        long value;
        try { value = sim.memRead(addr); } catch (Exception ex) { return; }
        JPopupMenu menu = new JPopupMenu();
        JMenuItem decItem = new JMenuItem("Decimal");
        JMenuItem hexItem = new JMenuItem("Hexadecimal");
        JMenuItem floatItem = new JMenuItem("Float");
        JMenuItem charItem = new JMenuItem("Character");
        JMenuItem strItem = new JMenuItem("String");
        JMenuItem memItem = new JMenuItem("Memory");

        decItem.addActionListener(ev -> { stackDisplayMode.put(addr, 0); updateStack(); });
        hexItem.addActionListener(ev -> { stackDisplayMode.put(addr, 1); updateStack(); });
        floatItem.addActionListener(ev -> { stackDisplayMode.put(addr, 2); updateStack(); });
        charItem.addActionListener(ev -> { stackDisplayMode.put(addr, 3); updateStack(); });
        strItem.addActionListener(ev -> showStackString(value));
        memItem.addActionListener(ev -> {
            if (value < 0 || value >= sim.getHeapLimit()) {
                JOptionPane.showMessageDialog(this, String.format("Illegal address: 0x%016X", value), "Error", JOptionPane.ERROR_MESSAGE);
            } else {
                MemoryWindow.showMemory(sim, value, stackTable.getFont(), settings);
            }
        });

        menu.add(decItem);
        menu.add(hexItem);
        menu.add(floatItem);
        menu.add(charItem);
        menu.addSeparator();
        menu.add(strItem);
        menu.add(memItem);
        menu.show(stackTable, e.getX(), e.getY());
    }

    private void showStackString(long addr) {
        try {
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
            String str = new String(bytes, java.nio.charset.StandardCharsets.UTF_8);
            JTextArea textArea = new JTextArea(str);
            textArea.setFont(stackTable.getFont());
            textArea.setEditable(false);
            textArea.setLineWrap(true);
            textArea.setWrapStyleWord(true);
            JScrollPane scroll = new JScrollPane(textArea);
            scroll.setPreferredSize(new java.awt.Dimension(400, 300));
            JDialog dialog = new JDialog(this, String.format("String @ 0x%08X (%d bytes)", addr, byteCount), false);
            dialog.add(scroll);
            dialog.pack();
            dialog.setLocationRelativeTo(this);
            dialog.setVisible(true);
        } catch (Exception ex) {
            JOptionPane.showMessageDialog(this, "Error reading string: " + ex.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
        }
    }

    private void updateStack() {
        stackModel.setRowCount(0);
        stackRowAddresses.clear();
        long sp = sim.getSP();
        long sf = sim.getSF();
        // Show stack with high addresses at top, descending, including SP line
        for (long addr = sp + 32; addr >= sp; addr--) {
            String prefix = "";
            if (addr == sp) prefix = "SP:";
            else if (addr == sf) prefix = "SF:";
            String addrStr = String.format("%08X", addr);
            String valStr = "";
            if (addr != sp) {
                long val;
                try { val = sim.memRead(addr); } catch (Exception e) { continue; }
                int mode = stackDisplayMode.getOrDefault(addr, 0);
                valStr = switch (mode) {
                    case 1 -> String.format("0x%016X", val);
                    case 2 -> String.format("%.16g", Double.longBitsToDouble(val));
                    case 3 -> (val >= 0 && val <= 0x10FFFF) ? "'" + new String(Character.toChars((int) val)) + "' (U+" + String.format("%04X", val) + ")" : "(invalid)";
                    default -> Long.toString(val);
                };
            }
            stackModel.addRow(new Object[]{prefix, addrStr, valStr});
            stackRowAddresses.add(addr);
        }
    }
}
