/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import javax.swing.*;
import java.awt.*;
import java.awt.datatransfer.StringSelection;
import java.awt.event.*;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.commonmark.Extension;
import org.commonmark.ext.autolink.AutolinkExtension;
import org.commonmark.ext.gfm.strikethrough.StrikethroughExtension;
import org.commonmark.ext.gfm.tables.TablesExtension;
import org.commonmark.ext.task.list.items.TaskListItemsExtension;
import org.commonmark.node.Node;
import org.commonmark.parser.Parser;
import org.commonmark.renderer.html.HtmlRenderer;

/**
 * AI Chat panel that provides LLM-powered markdown writing assistance.
 * Renders conversation as HTML in a JavaFX WebView with full markdown support
 * (tables, math, code highlighting). Falls back to a basic Swing renderer
 * when WebView is unavailable.
 *
 * Use {@link #builder()} to construct instances.
 */
public class AIChatPanel extends JPanel {

    private final JTextArea inputArea;
    private final JButton sendBtn;
    private final DocumentEditor editor;
    private final AIChatPreferences aiPreferences;
    private ChatColors chatColors;
    private final List<Map<String, String>> messages = new ArrayList<>();
    private final List<ChatMessage> chatMessages = new ArrayList<>();
    private final String systemPrompt;
    private final Parser mdParser;
    private final HtmlRenderer mdRenderer;
    private final String humanIconDataUri;
    private final String aiIconDataUri;
    private final String humanIconFallbackUrl;
    private final String aiIconFallbackUrl;

    // WebView rendering (loaded via WebViewHelper to avoid class-loading JavaFX when unavailable)
    private Object webViewHelper; // WebViewHelper instance
    private boolean useWebView = false;
    // Strong reference to prevent GC of the JavaScript bridge object
    private ChatBridge chatBridge;

    // Fallback rendering
    private JPanel fallbackChatPanel;
    private JScrollPane fallbackScroll;

    private volatile Thread currentThread;
    private boolean pulsing = false;
    private int promptCount = 0;
    private LLMClient llmClient;
    private Runnable promptNagCallback;
    private boolean darkMode = false;
    private final List<ContextProvider> contextProviders;
    private final DocumentRetriever retriever;

    /** Internal chat message record. */
    private static class ChatMessage {
        final String role; // "user" or "assistant"
        final String markdown;
        String copyContent; // if set, copy button copies this instead of markdown
        boolean accepted; // for code approval messages
        boolean rejected;
        boolean isApproval; // whether this contains a document replacement

        ChatMessage(String role, String markdown) {
            this.role = role;
            this.markdown = markdown;
        }
    }

    /** A labeled supplier of additional context text for the LLM. */
    private record ContextProvider(String label, java.util.function.Supplier<String> supplier) {}

    /**
     * Create a builder for constructing an AIChatPanel.
     */
    public static Builder builder() {
        return new Builder();
    }

    /**
     * Builder for AIChatPanel.
     */
    public static class Builder {
        private DocumentEditor editor;
        private AIChatPreferences preferences;
        private ChatColors chatColors;
        private LLMClient llmClient;
        private Runnable onPromptNag;
        private DocumentRetriever documentRetriever;
        private final List<ContextProvider> contextProviders = new ArrayList<>();

        private Builder() {}

        /** Set the document editor (required). */
        public Builder editor(DocumentEditor editor) {
            this.editor = editor;
            return this;
        }

        /** Set the AI chat preferences (required). */
        public Builder preferences(AIChatPreferences preferences) {
            this.preferences = preferences;
            return this;
        }

        /** Set the chat bubble colors (optional; falls back to AIChatPreferences defaults). */
        public Builder chatColors(ChatColors chatColors) {
            this.chatColors = chatColors;
            return this;
        }

        /** Set the LLM client (optional; will use LLMClientFactory if not set). */
        public Builder llmClient(LLMClient llmClient) {
            this.llmClient = llmClient;
            return this;
        }

        /** Set a callback invoked every N prompts for nag/licensing (optional). */
        public Builder onPromptNag(Runnable onPromptNag) {
            this.onPromptNag = onPromptNag;
            return this;
        }

        /**
         * Set the document retriever for RAG-based context (optional).
         * If not set, no RAG retrieval will be performed.
         *
         * @param retriever a configured DocumentRetriever instance
         */
        public Builder documentRetriever(DocumentRetriever retriever) {
            this.documentRetriever = retriever;
            return this;
        }

        /**
         * Add an additional context provider. Each provider supplies a labeled
         * block of text that is appended to every user message sent to the LLM.
         * Can be called multiple times to register multiple providers.
         *
         * @param label    the label shown to the LLM (e.g. "Console output")
         * @param supplier supplies the context text; may return null or empty to skip
         */
        public Builder contextProvider(String label, java.util.function.Supplier<String> supplier) {
            contextProviders.add(new ContextProvider(label, supplier));
            return this;
        }

        /** Build the AIChatPanel. */
        public AIChatPanel build() {
            if (editor == null) throw new IllegalStateException("editor is required");
            if (preferences == null) throw new IllegalStateException("preferences is required");
            return new AIChatPanel(this);
        }
    }

    private AIChatPanel(Builder builder) {
        super(new BorderLayout());
        this.editor = builder.editor;
        this.aiPreferences = builder.preferences;
        this.chatColors = builder.chatColors;
        this.llmClient = builder.llmClient;
        this.promptNagCallback = builder.onPromptNag;
        this.contextProviders = List.copyOf(builder.contextProviders);
        this.systemPrompt = buildSystemPrompt();
        this.humanIconDataUri = loadIconAsDataUri("/human.png");
        this.aiIconDataUri = loadIconAsDataUri("/AI.png");
        var humanUrl = AIChatPanel.class.getResource("/human.png");
        var aiUrl = AIChatPanel.class.getResource("/AI.png");
        this.humanIconFallbackUrl = humanUrl != null ? humanUrl.toString() : "";
        this.aiIconFallbackUrl = aiUrl != null ? aiUrl.toString() : "";

        // RAG retriever — lazily initialized on first query
        this.retriever = builder.documentRetriever;

        // Set up commonmark parser for rendering AI responses
        List<Extension> extensions = List.of(
            TablesExtension.create(),
            StrikethroughExtension.create(),
            TaskListItemsExtension.create(),
            AutolinkExtension.create()
        );
        mdParser = Parser.builder().extensions(extensions).build();
        mdRenderer = HtmlRenderer.builder().extensions(extensions).build();

        setPreferredSize(new Dimension(380, 0));
        setBorder(BorderFactory.createTitledBorder("AI Assistant"));

        // Initialize chat display (WebView or fallback)
        initChatDisplay();

        // Input area
        inputArea = new JTextArea(3, 20);
        inputArea.setFont(new Font(aiPreferences.getAiFontFamily(), Font.PLAIN, aiPreferences.getAiFontSize()));
        inputArea.setLineWrap(true);
        inputArea.setWrapStyleWord(true);
        inputArea.addKeyListener(new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent e) {
                if (e.getKeyCode() == KeyEvent.VK_ENTER && !e.isShiftDown()) {
                    e.consume();
                    sendMessage();
                } else if (e.getKeyCode() == KeyEvent.VK_ENTER && e.isShiftDown()) {
                    e.consume();
                    inputArea.insert("\n", inputArea.getCaretPosition());
                }
            }
        });
        JScrollPane inputScroll = new JScrollPane(inputArea);
        inputScroll.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);

        sendBtn = new JButton("Send");
        JButton clearBtn = new JButton("Clear");
        JPanel btnPanel = new JPanel(new GridLayout(2, 1, 0, 2));
        btnPanel.add(sendBtn);
        btnPanel.add(clearBtn);

        JPanel inputPanel = new JPanel(new BorderLayout(4, 0));
        inputPanel.setBorder(BorderFactory.createEmptyBorder(4, 4, 4, 4));
        inputPanel.add(inputScroll, BorderLayout.CENTER);
        inputPanel.add(btnPanel, BorderLayout.EAST);

        JLabel statusBar = new JLabel(" ");
        statusBar.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 10));
        statusBar.setBorder(BorderFactory.createEmptyBorder(2, 4, 2, 4));

        Runnable statusUpdater = () -> {
            int sp = systemPrompt.length();
            int doc = editor.getText().length();
            statusBar.setText(String.format("System: %,d chars    Document: %,d chars", sp, doc));
        };
        statusUpdater.run();

        JPanel southPanel = new JPanel(new BorderLayout());
        southPanel.add(inputPanel, BorderLayout.CENTER);
        southPanel.add(statusBar, BorderLayout.SOUTH);

        add(southPanel, BorderLayout.SOUTH);

        sendBtn.addActionListener(e -> sendMessage());
        clearBtn.addActionListener(e -> {
            messages.clear();
            chatMessages.clear();
            pulsing = false;
            if (llmClient instanceof GenericClient gc) {
                GenericVendorConfig cfg = gc.getConfig();
                if (cfg != null) cfg.resetGuid();
            }
            renderChat();
        });

        // Copy bundled AI vendor config files to Desktop on first run
        installConfigToDesktop();
    }

    private void initChatDisplay() {
        try {
            Class<?> helperClass = Class.forName("com.glowingcat.aichat.WebViewHelper");
            chatBridge = new ChatBridge();
            Object helper = helperClass.getDeclaredConstructor(Object.class, Runnable.class)
                .newInstance(chatBridge, (Runnable) this::renderChat);
            webViewHelper = helper;
            java.awt.Component fxPanel = (java.awt.Component) helperClass.getField("fxPanel").get(helper);
            add(fxPanel, BorderLayout.CENTER);
            useWebView = true;
        } catch (Throwable t) {
            webViewHelper = null;
            useWebView = false;
            initFallback();
        }
    }

    private void initFallback() {
        fallbackChatPanel = new JPanel();
        fallbackChatPanel.setLayout(new BoxLayout(fallbackChatPanel, BoxLayout.Y_AXIS));
        fallbackChatPanel.setBackground(Color.WHITE);
        fallbackScroll = new JScrollPane(fallbackChatPanel);
        fallbackScroll.getVerticalScrollBar().setUnitIncrement(16);
        add(fallbackScroll, BorderLayout.CENTER);
    }

    private javax.swing.Timer thinkingTimer;

    private void renderFallbackChat() {
        fallbackChatPanel.removeAll();
        Font chatFont = new Font(aiPreferences.getAiFontFamily(), Font.PLAIN, aiPreferences.getAiFontSize());
        Font codeFont = new Font(aiPreferences.getAiCodeFontFamily(), Font.PLAIN, aiPreferences.getAiCodeFontSize());
        ImageIcon humanIcon = loadScaledIcon("/human.png", 24);
        ImageIcon aiIcon = loadScaledIcon("/AI.png", 24);

        for (int i = 0; i < chatMessages.size(); i++) {
            ChatMessage msg = chatMessages.get(i);
            JPanel row = new JPanel(new BorderLayout(6, 0));
            row.setOpaque(false);
            row.setBorder(BorderFactory.createEmptyBorder(4, 4, 4, 4));
            row.setMaximumSize(new Dimension(Integer.MAX_VALUE, Integer.MAX_VALUE));

            JLabel icon = new JLabel("user".equals(msg.role) ? humanIcon : aiIcon);
            icon.setVerticalAlignment(SwingConstants.TOP);
            row.add(icon, BorderLayout.WEST);

            JTextArea textArea = new JTextArea(msg.markdown);
            textArea.setFont(chatFont);
            textArea.setLineWrap(true);
            textArea.setWrapStyleWord(true);
            textArea.setEditable(false);
            textArea.setOpaque(true);
            textArea.setBackground("user".equals(msg.role) ? new Color(0xE8F5E9) : new Color(0xE3F2FD));
            textArea.setBorder(BorderFactory.createEmptyBorder(6, 8, 6, 8));
            row.add(textArea, BorderLayout.CENTER);

            // Add Accept/Reject buttons for approval messages
            if (msg.isApproval && !msg.accepted && !msg.rejected) {
                final int idx = i;
                JPanel btnPanel = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 2));
                btnPanel.setOpaque(false);
                JButton acceptBtn = new JButton("Accept");
                JButton rejectBtn = new JButton("Reject");
                acceptBtn.addActionListener(e -> {
                    if (idx < chatMessages.size()) {
                        ChatMessage m = chatMessages.get(idx);
                        if (m instanceof ApprovalMessage am) {
                            if (am.diff != null) {
                                try {
                                    String patched = DiffApplier.apply(editor.getText(), am.diff);
                                    editor.setText(patched);
                                    m.accepted = true;
                                } catch (Exception ex) {
                                    JOptionPane.showMessageDialog(null,
                                        "Failed to apply diff: " + ex.getMessage(),
                                        "Diff Error", JOptionPane.ERROR_MESSAGE);
                                    return;
                                }
                            } else if (am.replacementMarkdown != null) {
                                editor.setText(am.replacementMarkdown);
                                m.accepted = true;
                            }
                            renderChat();
                        }
                    }
                });
                rejectBtn.addActionListener(e -> {
                    if (idx < chatMessages.size()) {
                        chatMessages.get(idx).rejected = true;
                        renderChat();
                    }
                });
                btnPanel.add(acceptBtn);
                btnPanel.add(rejectBtn);

                JPanel contentPanel = new JPanel(new BorderLayout());
                contentPanel.setOpaque(false);
                contentPanel.add(textArea, BorderLayout.CENTER);
                contentPanel.add(btnPanel, BorderLayout.SOUTH);
                row.add(contentPanel, BorderLayout.CENTER);
            } else if (msg.isApproval && msg.accepted) {
                JPanel contentPanel = new JPanel(new BorderLayout());
                contentPanel.setOpaque(false);
                contentPanel.add(textArea, BorderLayout.CENTER);
                JLabel status = new JLabel("✓ Changes accepted.");
                status.setForeground(new Color(0x2E7D32));
                status.setBorder(BorderFactory.createEmptyBorder(2, 8, 2, 0));
                contentPanel.add(status, BorderLayout.SOUTH);
                row.add(contentPanel, BorderLayout.CENTER);
            } else if (msg.isApproval && msg.rejected) {
                JPanel contentPanel = new JPanel(new BorderLayout());
                contentPanel.setOpaque(false);
                contentPanel.add(textArea, BorderLayout.CENTER);
                JLabel status = new JLabel("✗ Changes rejected.");
                status.setForeground(new Color(0xC62828));
                status.setBorder(BorderFactory.createEmptyBorder(2, 8, 2, 0));
                contentPanel.add(status, BorderLayout.SOUTH);
                row.add(contentPanel, BorderLayout.CENTER);
            }

            fallbackChatPanel.add(row);
        }

        // Pulsing "Thinking..." indicator
        if (pulsing) {
            JPanel thinkingRow = new JPanel(new BorderLayout(6, 0));
            thinkingRow.setOpaque(false);
            thinkingRow.setBorder(BorderFactory.createEmptyBorder(4, 4, 4, 4));
            thinkingRow.setMaximumSize(new Dimension(Integer.MAX_VALUE, 40));
            thinkingRow.add(new JLabel(aiIcon), BorderLayout.WEST);
            JLabel thinkingLabel = new JLabel("Thinking...");
            thinkingLabel.setFont(chatFont.deriveFont(Font.ITALIC));
            thinkingRow.add(thinkingLabel, BorderLayout.CENTER);
            fallbackChatPanel.add(thinkingRow);

            // Animate pulsing with a timer
            if (thinkingTimer != null) thinkingTimer.stop();
            thinkingTimer = new javax.swing.Timer(600, new ActionListener() {
                private int dots = 3;
                @Override public void actionPerformed(ActionEvent e) {
                    dots = (dots % 3) + 1;
                    thinkingLabel.setText("Thinking" + ".".repeat(dots));
                }
            });
            thinkingTimer.start();
        } else {
            if (thinkingTimer != null) { thinkingTimer.stop(); thinkingTimer = null; }
        }

        fallbackChatPanel.add(Box.createVerticalGlue());
        fallbackChatPanel.revalidate();
        fallbackChatPanel.repaint();
        SwingUtilities.invokeLater(() -> {
            JScrollBar v = fallbackScroll.getVerticalScrollBar();
            v.setValue(v.getMaximum());
        });
    }

    private static ImageIcon loadScaledIcon(String resource, int size) {
        var url = AIChatPanel.class.getResource(resource);
        if (url != null) {
            return new ImageIcon(new ImageIcon(url).getImage().getScaledInstance(size, size, java.awt.Image.SCALE_SMOOTH));
        }
        return null;
    }

    /** JavaScript bridge for WebView callbacks. */
    public class ChatBridge {
        /** Called when user clicks Accept on a code approval bubble. */
        public void acceptChanges(int index) {
            SwingUtilities.invokeLater(() -> {
                if (index >= 0 && index < chatMessages.size()) {
                    ChatMessage msg = chatMessages.get(index);
                    if (msg instanceof ApprovalMessage am) {
                        if (am.diff != null) {
                            // Apply unified diff to current document
                            try {
                                String patched = DiffApplier.apply(editor.getText(), am.diff);
                                msg.accepted = true;
                                editor.setText(patched);
                            } catch (DiffApplier.DiffException ex) {
                                // If diff fails, show error in chat
                                ChatMessage errMsg = new ChatMessage("assistant",
                                    "Failed to apply diff: " + ex.getMessage() + ". Try asking the AI to regenerate the changes.");
                                chatMessages.add(errMsg);
                            }
                        } else if (am.replacementMarkdown != null) {
                            // Full document replacement
                            msg.accepted = true;
                            editor.setText(am.replacementMarkdown);
                        }
                        renderChat();
                    }
                }
            });
        }

        /** Called when user clicks Reject on a code approval bubble. */
        public void rejectChanges(int index) {
            SwingUtilities.invokeLater(() -> {
                if (index >= 0 && index < chatMessages.size()) {
                    ChatMessage msg = chatMessages.get(index);
                    msg.rejected = true;
                    renderChat();
                }
            });
        }

        /** Called when user clicks Copy on an AI response bubble. */
        public void copyMarkdown(int index) {
            SwingUtilities.invokeLater(() -> {
                if (index >= 0 && index < chatMessages.size()) {
                    ChatMessage msg = chatMessages.get(index);
                    String md;
                    if (msg instanceof ApprovalMessage am) {
                        md = am.diff != null ? am.diff : am.replacementMarkdown;
                    } else if (msg.copyContent != null) {
                        md = msg.copyContent;
                    } else {
                        md = msg.markdown;
                    }
                    if (md != null) {
                        StringSelection sel = new StringSelection(md);
                        Toolkit.getDefaultToolkit().getSystemClipboard().setContents(sel, null);
                    }
                }
            });
        }

        /** Called when a link is clicked. */
        public void openLink(String url) {
            SwingUtilities.invokeLater(() -> {
                try {
                    java.awt.Desktop.getDesktop().browse(new java.net.URI(url));
                } catch (Exception ex) {
                    // Silently fail
                }
            });
        }
    }

    /** Set or replace the LLM client at runtime (e.g., after preferences change). */
    public void setLlmClient(LLMClient client) {
        this.llmClient = client;
    }

    /** Set or replace the prompt nag callback at runtime. */
    public void setPromptNagCallback(Runnable callback) {
        this.promptNagCallback = callback;
    }

    /** Set or replace the chat colors at runtime and re-render. */
    public void setChatColors(ChatColors colors) {
        this.chatColors = colors;
        renderChat();
    }

    /** Set dark mode and re-render the chat. */
    public void setDarkMode(boolean dark) {
        this.darkMode = dark;
        renderChat();
    }

    /** Update fonts after preferences change. */
    public void updateFont() {
        inputArea.setFont(new Font(aiPreferences.getAiFontFamily(), Font.PLAIN, aiPreferences.getAiFontSize()));
        renderChat();
    }

    /**
     * Lazily initialize the RAG retriever if needed for the current vendor.
     * Shows a progress dialog while indexing.
     */
    private void ensureRetrieverInitialized() {
        if (retriever == null) return;
        if (!retriever.needsInitialization(aiPreferences)) return;

        String vendor = aiPreferences.getLlmVendor();
        JDialog dialog = new JDialog(
            (java.awt.Frame) SwingUtilities.getWindowAncestor(this),
            "Indexing", true);
        JLabel label = new JLabel("Indexing context documents for use by " + vendor + "...");
        label.setBorder(BorderFactory.createEmptyBorder(20, 30, 20, 30));
        JProgressBar progress = new JProgressBar();
        progress.setIndeterminate(true);
        JPanel panel = new JPanel(new java.awt.BorderLayout(0, 10));
        panel.setBorder(BorderFactory.createEmptyBorder(10, 20, 10, 20));
        panel.add(label, java.awt.BorderLayout.NORTH);
        panel.add(progress, java.awt.BorderLayout.CENTER);
        dialog.setContentPane(panel);
        dialog.pack();
        dialog.setLocationRelativeTo(this);
        dialog.setDefaultCloseOperation(JDialog.DO_NOTHING_ON_CLOSE);

        // Run initialization on background thread, close dialog when done
        new Thread(() -> {
            try {
                retriever.initialize(aiPreferences);
            } catch (Exception e) {
                // Non-fatal
            } finally {
                SwingUtilities.invokeLater(dialog::dispose);
            }
        }, "RAG-Indexer").start();

        dialog.setVisible(true); // blocks until disposed
    }

    private void sendMessage() {
        String text = inputArea.getText().trim();
        if (text.isEmpty()) return;
        inputArea.setText("");

        // Add user bubble
        ChatMessage userMsg = new ChatMessage("user", text);
        chatMessages.add(userMsg);

        // Invoke nag callback every 10 prompts
        promptCount++;
        if (promptCount % 10 == 0 && promptNagCallback != null) {
            promptNagCallback.run();
        }

        String context = "Current document:\n```\n" + editor.getText() + "\n```";
        for (ContextProvider cp : contextProviders) {
            String extra = cp.supplier().get();
            if (extra != null && !extra.isEmpty()) {
                context += "\n\n" + cp.label() + ":\n```\n" + extra + "\n```";
            }
        }

        // Add RAG-retrieved relevant documentation chunks
        ensureRetrieverInitialized();
        if (retriever != null && retriever.isInitialized()) {
            // Combine user prompt with document sample for better retrieval relevance
            String retrievalQuery = text;
            String docText = editor.getText();
            if (docText != null && !docText.isEmpty()) {
                String docSample = docText.length() > 500 ? docText.substring(0, 500) : docText;
                retrievalQuery = text + "\n\n" + docSample;
            }
            List<String> relevantDocs = retriever.retrieve(retrievalQuery);
            if (!relevantDocs.isEmpty()) {
                context += "\n\nRelevant documentation:\n";
                for (String doc : relevantDocs) {
                    context += "\n---\n" + doc;
                }
            }
        }

        if (messages.isEmpty()) {
            messages.add(Map.of("role", "system", "content", systemPrompt));
        }
        String fullUserMessage = context + "\n\nUser request: " + text;
        messages.add(Map.of("role", "user", "content", fullUserMessage));

        // In developer mode, write the composed prompt to a file for inspection
        if (isDeveloperMode()) {
            writeDevFile(".glowingcat-ai-prompt.md", fullUserMessage);
        }

        // Ensure we have an LLM client
        if (llmClient == null) {
            llmClient = LLMClientFactory.create(aiPreferences);
        }

        sendBtn.setEnabled(false);
        pulsing = true;
        renderChat();

        final LLMClient client = llmClient;
        currentThread = new Thread(() -> {
            try {
                String response = client.chat(messages, systemPrompt);
                messages.add(Map.of("role", "assistant", "content", response));
                // In developer mode, write the AI response to a file for inspection
                if (isDeveloperMode()) {
                    writeDevFile(".glowingcat-ai-response.md", response);
                }
                SwingUtilities.invokeLater(() -> {
                    pulsing = false;
                    processResponse(response);
                    sendBtn.setEnabled(true);
                });
            } catch (Exception ex) {
                SwingUtilities.invokeLater(() -> {
                    pulsing = false;
                    if (!Thread.currentThread().isInterrupted()) {
                        ChatMessage errMsg = new ChatMessage("assistant",
                            "Error (" + ex.getClass().getSimpleName() + "): " + ex.getMessage());
                        chatMessages.add(errMsg);
                    }
                    renderChat();
                    sendBtn.setEnabled(true);
                });
            }
        });
        currentThread.start();
    }

    private void processResponse(String response) {
        // Normalize line endings
        String normalized = response.replace("\r\n", "\n").replace("\r", "\n");

        // Check for ```fulltext block — full source replacement applied immediately (no Accept/Reject)
        // Support 3 or more backticks as the fence (LLMs sometimes use 4+)
        int fulltextStart = -1;
        int fulltextFenceLen = 0;
        {
            int idx = 0;
            while (idx < normalized.length()) {
                idx = normalized.indexOf("```", idx);
                if (idx < 0) break;
                int fenceStart = idx;
                while (idx < normalized.length() && normalized.charAt(idx) == '`') idx++;
                int fenceLen = idx - fenceStart;
                if (normalized.startsWith("fulltext\n", idx)) {
                    fulltextStart = fenceStart;
                    fulltextFenceLen = fenceLen;
                    break;
                }
            }
        }
        if (fulltextStart >= 0) {
            int blockStart = normalized.indexOf("\n", fulltextStart) + 1;
            String closingFence = "`".repeat(fulltextFenceLen);
            int blockEnd = findClosingFenceExact(normalized, blockStart, closingFence);
            if (blockEnd > blockStart) {
                String newSource = normalized.substring(blockStart, blockEnd);
                String explanation = normalized.substring(0, fulltextStart).trim();
                int fenceEndPos = normalized.indexOf("\n", blockEnd + 1);
                if (fenceEndPos < 0) fenceEndPos = blockEnd + fulltextFenceLen + 1;
                if (fenceEndPos < normalized.length()) {
                    String after = normalized.substring(fenceEndPos).trim();
                    if (!after.isEmpty()) explanation += (explanation.isEmpty() ? "" : "\n") + after;
                }
                // Apply directly to editor
                editor.setText(newSource);
                // Show explanation as a normal message (or a default if none)
                String display = explanation.isEmpty() ? "Updated the source code." : explanation;
                ChatMessage msg = new ChatMessage("assistant", display);
                msg.copyContent = newSource;
                chatMessages.add(msg);
                renderChat();
                return;
            }
        }

        // Check for ```diff block (preferred format for changes)
        int diffStart = normalized.indexOf("```diff\n");
        if (diffStart >= 0) {
            int blockStart = normalized.indexOf("\n", diffStart) + 1;
            int blockEnd = findClosingFence(normalized, blockStart);
            if (blockEnd > blockStart) {
                String diffContent = normalized.substring(blockStart, blockEnd);
                String explanation = normalized.substring(0, diffStart).trim();
                int fenceEndPos = normalized.indexOf("\n", blockEnd + 1);
                if (fenceEndPos < 0) fenceEndPos = blockEnd + 4;
                if (fenceEndPos < normalized.length()) {
                    String after = normalized.substring(fenceEndPos).trim();
                    if (!after.isEmpty()) explanation += (explanation.isEmpty() ? "" : "\n") + after;
                }
                chatMessages.add(new ApprovalMessage(explanation, null, diffContent));
                renderChat();
                return;
            }
        }

        // No diff or fulltext block found — show as normal chat message
        ChatMessage aiMsg = new ChatMessage("assistant", response);
        chatMessages.add(aiMsg);
        renderChat();
    }

    /** Find the last closing ``` fence after a block start position. */
    private int findClosingFence(String text, int blockStart) {
        int blockEnd = -1;
        int searchFrom = blockStart;
        while (searchFrom < text.length()) {
            int candidate = text.indexOf("\n```", searchFrom);
            if (candidate < 0) break;
            int afterFence = candidate + 4;
            if (afterFence >= text.length()) {
                blockEnd = candidate;
            } else {
                char nextChar = text.charAt(afterFence);
                if (nextChar == '\n' || text.substring(afterFence).trim().isEmpty()) {
                    blockEnd = candidate;
                }
            }
            searchFrom = candidate + 4;
        }
        return blockEnd;
    }

    /** Find the closing fence matching an exact backtick sequence (e.g., ```` or `````) after a block start. */
    private int findClosingFenceExact(String text, int blockStart, String fence) {
        String target = "\n" + fence;
        int searchFrom = blockStart;
        while (searchFrom < text.length()) {
            int candidate = text.indexOf(target, searchFrom);
            if (candidate < 0) break;
            int afterFence = candidate + target.length();
            // Ensure fence is not followed by more backticks (which would mean a longer fence)
            if (afterFence < text.length() && text.charAt(afterFence) == '`') {
                searchFrom = afterFence;
                continue;
            }
            // Ensure fence is at end of line or followed by whitespace/newline
            if (afterFence >= text.length() || text.charAt(afterFence) == '\n'
                    || text.substring(afterFence).trim().isEmpty()) {
                return candidate;
            }
            searchFrom = afterFence;
        }
        return -1;
    }

    /** Special message type for document replacement approvals. */
    private static class ApprovalMessage extends ChatMessage {
        final String explanation;
        final String replacementMarkdown; // full replacement or null if using diff
        final String diff; // unified diff or null if using full replacement

        ApprovalMessage(String explanation, String replacementMarkdown, String diff) {
            super("assistant", explanation.isEmpty()
                ? (diff != null ? "Here are the proposed changes. Review and accept or reject."
                    : "Here's the updated document. Review and accept or reject the changes.")
                : explanation);
            this.explanation = explanation;
            this.replacementMarkdown = replacementMarkdown;
            this.diff = diff;
            this.isApproval = true;
        }
    }

    /** Render the full chat as HTML and load into WebView or fallback. */
    private void renderChat() {
        String html = buildChatHtml();
        if (useWebView && webViewHelper != null) {
            try {
                webViewHelper.getClass().getMethod("loadContent", String.class).invoke(webViewHelper, html);
            } catch (Exception ignored) {}
        } else if (fallbackChatPanel != null) {
            renderFallbackChat();
        }
    }

    private String buildChatHtml() {
        String fontFamily = aiPreferences.getAiFontFamily();
        int fontSize = aiPreferences.getAiFontSize();
        String codeFontFamily = aiPreferences.getAiCodeFontFamily();
        int codeFontSize = aiPreferences.getAiCodeFontSize();
        String userBg = chatColors != null ? chatColors.getUserPromptColor() : aiPreferences.getUserPromptColor();
        String userText = chatColors != null ? chatColors.getUserTextColor() : aiPreferences.getUserTextColor();
        String aiBg = chatColors != null ? chatColors.getAiResponseColor() : aiPreferences.getAiResponseColor();
        String aiText = chatColors != null ? chatColors.getAiTextColor() : aiPreferences.getAiTextColor();

        StringBuilder html = new StringBuilder();
        html.append("<html><head><meta charset=\"utf-8\"><style>");
        // Dynamic styles that depend on preferences
        html.append("body { font-family: '").append(fontFamily).append("', 'Apple Color Emoji', 'Segoe UI Emoji', 'Noto Color Emoji', sans-serif; ");
        html.append("font-size: ").append(fontSize).append("pt; }");
        html.append(".user-bubble { background: ").append(userBg).append("; color: ").append(userText).append("; }");
        html.append(".ai-bubble { background: ").append(aiBg).append("; color: ").append(aiText).append("; }");
        html.append(".approval-btns button { font-size: ").append(fontSize).append("pt; }");
        html.append("code, pre { font-family: '").append(codeFontFamily).append("', monospace; ");
        html.append("font-size: ").append(codeFontSize).append("pt; }");
        html.append("</style>");
        // Static styles from resource file
        html.append("<style>").append(loadCssResource(darkMode)).append("</style>");
        html.append("<script>");
        html.append("MathJax = {");
        html.append("  tex: { inlineMath: [['$','$'], ['\\\\(','\\\\)']], displayMath: [['$$','$$'], ['\\\\[','\\\\]']] },");
        html.append("  options: { skipHtmlTags: ['script','noscript','style','textarea','pre','code'] },");
        html.append("  svg: { fontCache: 'global' }");
        html.append("};");
        html.append("</script>");
        html.append("<script src=\"https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-svg.js\" async></script>");
        html.append("<script>");
        html.append("document.addEventListener('click', function(e) {");
        html.append("  var a = e.target.closest('a');");
        html.append("  if(a && a.href && (a.href.startsWith('http://') || a.href.startsWith('https://'))) {");
        html.append("    e.preventDefault();");
        html.append("    if(window.chatBridge) window.chatBridge.openLink(a.href);");
        html.append("  }");
        html.append("});");
        html.append("function acceptChanges(idx) { if(window.chatBridge) window.chatBridge.acceptChanges(idx); }");
        html.append("function rejectChanges(idx) { if(window.chatBridge) window.chatBridge.rejectChanges(idx); }");
        html.append("function copyMarkdown(idx) {");
        html.append("  if(window.chatBridge) window.chatBridge.copyMarkdown(idx);");
        html.append("  var btn = event.currentTarget;");
        html.append("  btn.innerHTML = '<svg width=\"14\" height=\"14\" viewBox=\"0 0 16 16\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\"><path d=\"M3 8.5L6.5 12L13 4\"/></svg>';");
        html.append("}");
        html.append("</script>");
        html.append("</head><body>");

        // Render all chat messages
        for (int i = 0; i < chatMessages.size(); i++) {
            ChatMessage msg = chatMessages.get(i);
            if ("user".equals(msg.role)) {
                html.append("<div class=\"bubble-row\">");
                html.append("<img class=\"bubble-icon\" src=\"").append(humanIconSrc()).append("\">");
                html.append("<div class=\"bubble user-bubble bubble-content\">");
                html.append(escapeHtml(msg.markdown));
                html.append("</div></div>");
            } else {
                // AI message — render markdown as HTML
                String renderedContent;
                if (msg instanceof ApprovalMessage am) {
                    renderedContent = renderMarkdownToHtml(am.explanation.isEmpty()
                        ? "Here's the updated document. Review and accept or reject the changes."
                        : am.explanation);
                } else {
                    renderedContent = renderMarkdownToHtml(msg.markdown);
                }

                html.append("<div class=\"bubble-row\">");
                html.append("<img class=\"bubble-icon\" src=\"").append(aiIconSrc()).append("\">");
                html.append("<div class=\"bubble ai-bubble bubble-content\">");
                html.append("<button class=\"copy-btn\" onclick=\"copyMarkdown(").append(i).append(")\" title=\"Copy markdown\"><svg width=\"14\" height=\"14\" viewBox=\"0 0 16 16\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"1.5\"><rect x=\"5\" y=\"5\" width=\"9\" height=\"9\" rx=\"1.5\"/><path d=\"M3 11V2.5A1.5 1.5 0 0 1 4.5 1H11\"/></svg></button>");
                html.append(renderedContent);

                // Approval buttons
                if (msg.isApproval) {
                    if (msg.accepted) {
                        html.append("<div class=\"status-label\">✓ Changes accepted.</div>");
                    } else if (msg.rejected) {
                        html.append("<div class=\"status-label\">✗ Changes rejected.</div>");
                    } else {
                        html.append("<div class=\"approval-btns\">");
                        html.append("<button class=\"accept-btn\" onclick=\"acceptChanges(").append(i).append(")\">Accept</button>");
                        html.append("<button class=\"reject-btn\" onclick=\"rejectChanges(").append(i).append(")\">Reject</button>");
                        html.append("</div>");
                    }
                }

                html.append("</div></div>");
            }
        }

        // Pulsing "thinking" indicator
        if (pulsing) {
            html.append("<div class=\"thinking\"><img class=\"bubble-icon\" src=\"").append(aiIconSrc()).append("\">Thinking...</div>");
        }

        // Auto-scroll to bottom
        html.append("<script>window.onload = function() { window.scrollTo(0, document.body.scrollHeight); ");
        html.append("if(window.MathJax && MathJax.typesetPromise) MathJax.typesetPromise(); };</script>");

        html.append("</body></html>");
        return html.toString();
    }

    private String renderMarkdownToHtml(String markdown) {
        if (markdown == null || markdown.isEmpty()) return "";
        // Convert LaTeX-style math delimiters to dollar-sign delimiters before parsing
        String converted = markdown.replaceAll("\\\\\\((.+?)\\\\\\)", "\\$$1\\$");
        converted = converted.replaceAll("(?s)\\\\\\[(.+?)\\\\\\]", "\\$\\$$1\\$\\$");
        Node document = mdParser.parse(converted);
        return mdRenderer.render(document);
    }

    private static String escapeHtml(String text) {
        return text.replace("&", "&amp;")
                   .replace("<", "&lt;")
                   .replace(">", "&gt;")
                   .replace("\"", "&quot;")
                   .replace("\n", "<br>");
    }

    private String buildSystemPrompt() {
        try (var is = AIChatPanel.class.getResourceAsStream("/system_prompt.md")) {
            if (is != null) {
                return new String(is.readAllBytes(), StandardCharsets.UTF_8).trim();
            }
        } catch (Exception e) {
            // Fall through to hardcoded fallback
        }
        return "You are an AI writing assistant. Help users write and improve markdown documents.";
    }

    private String humanIconSrc() {
        return useWebView ? humanIconDataUri : humanIconFallbackUrl;
    }

    private String aiIconSrc() {
        return useWebView ? aiIconDataUri : aiIconFallbackUrl;
    }

    private static String loadIconAsDataUri(String resourcePath) {
        try (var is = AIChatPanel.class.getResourceAsStream(resourcePath)) {
            if (is != null) {
                byte[] bytes = is.readAllBytes();
                String base64 = java.util.Base64.getEncoder().encodeToString(bytes);
                return "data:image/png;base64," + base64;
            }
        } catch (Exception e) {
            // Fall through
        }
        return "";
    }

    private static String loadCssResource(boolean dark) {
        String path = dark ? "/ai_chat_dark.css" : "/ai_chat.css";
        try (var is = AIChatPanel.class.getResourceAsStream(path)) {
            if (is != null) {
                return new String(is.readAllBytes(), StandardCharsets.UTF_8);
            }
        } catch (Exception e) {
            // Fall through
        }
        return "";
    }

    /**
     * Returns true if running from exploded class files (i.e. an IDE) rather than a packaged JAR.
     */
    private static boolean isDeveloperMode() {
        var url = AIChatPanel.class.getResource("/system_prompt.md");
        return url != null && "file".equals(url.getProtocol());
    }

    /**
     * Write content to a file in the user's home directory (developer mode only).
     */
    private static void writeDevFile(String filename, String content) {
        try {
            java.nio.file.Path path = java.nio.file.Path.of(System.getProperty("user.home"), filename);
            java.nio.file.Files.writeString(path, content, StandardCharsets.UTF_8);
        } catch (Exception e) {
            // Non-fatal — developer convenience only
        }
    }

    /**
     * Copy bundled config files from resources/config/ to the user's Desktop
     * as "Generic AI Configurations" folder. Only runs once — skips if the folder already exists.
     */
    private static void installConfigToDesktop() {
        try {
            Path desktopDir = Path.of(System.getProperty("user.home"), "Desktop");
            if (!Files.isDirectory(desktopDir)) return;

            Path destDir = desktopDir.resolve("Generic AI Configurations");
            if (Files.exists(destDir)) return; // already installed

            // Read the index of config files
            var indexStream = AIChatPanel.class.getResourceAsStream("/config/config-index.txt");
            if (indexStream == null) return;

            List<String> filenames;
            try (var reader = new java.io.BufferedReader(new java.io.InputStreamReader(indexStream, StandardCharsets.UTF_8))) {
                filenames = reader.lines()
                    .map(String::trim)
                    .filter(s -> !s.isEmpty())
                    .toList();
            }

            if (filenames.isEmpty()) return;

            Files.createDirectories(destDir);
            for (String filename : filenames) {
                var resStream = AIChatPanel.class.getResourceAsStream("/config/" + filename);
                if (resStream == null) continue;
                try (resStream) {
                    Files.copy(resStream, destDir.resolve(filename));
                }
            }
        } catch (Exception e) {
            // Non-fatal — best effort
        }
    }
}
