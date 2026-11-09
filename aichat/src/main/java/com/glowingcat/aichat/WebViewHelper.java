/*
 * (c) 2026 Glowing Cat Software
 */
package com.glowingcat.aichat;

import javafx.application.Platform;
import javafx.concurrent.Worker;
import javafx.embed.swing.JFXPanel;
import javafx.scene.Scene;
import javafx.scene.web.WebEngine;
import javafx.scene.web.WebView;
import javax.swing.*;

/**
 * Isolates all JavaFX class references so that AIChatPanel can be loaded
 * even when JavaFX is not on the classpath. This class should only be
 * instantiated inside a try-catch for NoClassDefFoundError/ClassNotFoundException.
 */
class WebViewHelper {

    public final JFXPanel fxPanel;
    private volatile WebEngine webEngine;
    private volatile boolean ready = false;
    private Object jsBridge; // strong reference to prevent GC
    private Runnable onReady; // callback when WebView is initialized

    WebViewHelper(Object bridge, Runnable onReady) {
        this.onReady = onReady;
        fxPanel = new JFXPanel();
        Platform.runLater(() -> {
            try {
                WebView webView = new WebView();
                webEngine = webView.getEngine();

                // Prevent the WebView from opening new windows (prevents lock-up on Linux)
                webEngine.setCreatePopupHandler(features -> null);

                webEngine.getLoadWorker().stateProperty().addListener((obs, oldState, newState) -> {
                    if (newState == Worker.State.SUCCEEDED) {
                        netscape.javascript.JSObject win =
                            (netscape.javascript.JSObject) webEngine.executeScript("window");
                        win.setMember("chatBridge", bridge);
                        jsBridge = bridge;
                        ready = true;
                    }
                });

                Scene scene = new Scene(webView);
                fxPanel.setScene(scene);
                webEngine.loadContent("<html><body></body></html>");

                // Signal that webEngine is ready to accept content
                SwingUtilities.invokeLater(() -> {
                    if (this.onReady != null) this.onReady.run();
                });
            } catch (Throwable t) {
                // JavaFX initialization failed — caller will fall back
                SwingUtilities.invokeLater(() -> fxPanel.setVisible(false));
            }
        });
    }

    public boolean isReady() {
        return webEngine != null;
    }

    private java.io.File tempHtmlFile;

    public void loadContent(String html) {
        if (webEngine == null) return;
        Platform.runLater(() -> {
            try {
                // Write to temp file and load via URL for proper UTF-8 encoding.
                // loadContent(String) has JavaFX bugs with non-BMP characters (emoji)
                // even when encoded as HTML entities — file-based loading avoids this.
                if (tempHtmlFile == null) {
                    tempHtmlFile = java.io.File.createTempFile("pp-aichat-", ".html");
                    tempHtmlFile.deleteOnExit();
                }
                java.nio.file.Files.writeString(tempHtmlFile.toPath(), html,
                        java.nio.charset.StandardCharsets.UTF_8);
                webEngine.load(tempHtmlFile.toURI().toString());
            } catch (java.io.IOException e) {
                // Fallback to loadContent if file write fails
                webEngine.loadContent(html, "text/html");
            }
        });
    }
}
