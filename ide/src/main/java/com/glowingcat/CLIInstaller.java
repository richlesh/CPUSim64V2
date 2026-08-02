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
import java.awt.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class CLIInstaller {
    private static final boolean IS_WINDOWS = System.getProperty("os.name").toLowerCase().contains("win");
    private static final boolean IS_MAC = System.getProperty("os.name").toLowerCase().contains("mac");

    private static Icon appIcon() {
        var url = CLIInstaller.class.getResource("/app_icon_256.png");
        if (url == null) return null;
        return new ImageIcon(new ImageIcon(url).getImage().getScaledInstance(64, 64, Image.SCALE_SMOOTH));
    }

    public static void install() {
        try {
            String toolName = IS_WINDOWS ? "cpusim64.exe" : "cpusim64";

            InputStream in = CLIInstaller.class.getResourceAsStream("/" + toolName);
            if (in == null) {
                JOptionPane.showMessageDialog(null,
                    "Error: " + toolName + " not found in application resources.",
                    "CPUSim64", JOptionPane.ERROR_MESSAGE, appIcon());
                return;
            }

            Path destDir;
            if (IS_WINDOWS) {
                destDir = Path.of(System.getenv("LOCALAPPDATA"), "CPUSim64", "bin");
            } else {
                destDir = Path.of(System.getProperty("user.home"), ".local", "bin");
            }
            Path destToolPath = destDir.resolve(toolName);

            Files.createDirectories(destDir);
            Files.copy(in, destToolPath, StandardCopyOption.REPLACE_EXISTING);
            in.close();
            if (!IS_WINDOWS) {
                destToolPath.toFile().setExecutable(true, false);
            }

            String pathMessage = addToPath(destDir.toString());
            String message = "Command line tools installed successfully to " + destToolPath;
            if (pathMessage != null) {
                message += "\n\n" + pathMessage;
            }

            JOptionPane.showMessageDialog(null, message, "CPUSim64", JOptionPane.INFORMATION_MESSAGE, appIcon());
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null,
                "Installation failed: " + e.getMessage() + "\nYou may need to run with administrator privileges.",
                "CPUSim64", JOptionPane.ERROR_MESSAGE, appIcon());
        }
    }

    public static void uninstall() {
        if (IS_WINDOWS) {
            uninstallWindows();
            JOptionPane.showMessageDialog(null,
                "Command line tools removed successfully.",
                "CPUSim64", JOptionPane.INFORMATION_MESSAGE, appIcon());
        } else {
            uninstallUnix();
        }
    }

    private static void uninstallWindows() {
        try {
            Path destDir = Path.of(System.getenv("LOCALAPPDATA"), "CPUSim64", "bin");
            Path exe = destDir.resolve("cpusim64.exe");

            Files.deleteIfExists(exe);
            try { Files.deleteIfExists(destDir); } catch (Exception ignored) {}

            removeFromPathWindows(destDir.toString());
        } catch (Exception ignored) {}
    }

    private static void uninstallUnix() {
        try {
            Path destDir = Path.of(System.getProperty("user.home"), ".local", "bin");
            Path exe = destDir.resolve("cpusim64");

            Files.deleteIfExists(exe);

            Path profilePath = IS_MAC
                ? Path.of(System.getProperty("user.home"), ".zprofile")
                : Path.of(System.getProperty("user.home"), ".bashrc");
            if (Files.exists(profilePath)) {
                String content = Files.readString(profilePath);
                String snippet = "\n# Added by CPUSim64 installer\n"
                    + "case \":$PATH:\" in\n"
                    + "  *\":" + destDir + ":\"*) ;;\n"
                    + "  *) export PATH=\"" + destDir + ":$PATH\" ;;\n"
                    + "esac\n";
                String cleaned = content.replace(snippet, "");
                if (!cleaned.equals(content)) {
                    Files.writeString(profilePath, cleaned);
                }
            }

            JOptionPane.showMessageDialog(null,
                "Command line tools removed successfully.",
                "CPUSim64", JOptionPane.INFORMATION_MESSAGE, appIcon());
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null,
                "Uninstall failed: " + e.getMessage(),
                "CPUSim64", JOptionPane.ERROR_MESSAGE, appIcon());
        }
    }

    private static void removeFromPathWindows(String dir) throws Exception {
        Process p = Runtime.getRuntime().exec(new String[]{
            "reg", "query", "HKCU\\Environment", "/v", "Path"
        });
        String output = new String(p.getInputStream().readAllBytes());
        p.waitFor();
        if (p.exitValue() != 0) return;

        String currentPath = "";
        for (String line : output.split("\\r?\\n")) {
            line = line.trim();
            if (line.contains("REG_") && line.contains("Path")) {
                int idx = line.indexOf("REG_");
                idx = line.indexOf("    ", idx);
                if (idx >= 0) currentPath = line.substring(idx).trim();
                break;
            }
        }

        StringJoiner joiner = new StringJoiner(";");
        boolean found = false;
        for (String entry : currentPath.split(";")) {
            if (entry.trim().equalsIgnoreCase(dir)) {
                found = true;
            } else if (!entry.trim().isEmpty()) {
                joiner.add(entry.trim());
            }
        }
        if (!found) return;

        String newPath = joiner.toString();
        Runtime.getRuntime().exec(new String[]{
            "reg", "add", "HKCU\\Environment", "/v", "Path", "/t", "REG_EXPAND_SZ", "/d", newPath, "/f"
        }).waitFor();

        Runtime.getRuntime().exec(new String[]{
            "cmd", "/c", "setx", "CPUSIM64_PATH_SET", "1"
        }).waitFor();
    }

    private static String addToPath(String dir) {
        try {
            if (IS_WINDOWS) {
                return addToPathWindows(dir);
            } else {
                return addToPathUnix(dir);
            }
        } catch (Exception e) {
            return "Could not update PATH automatically: " + e.getMessage() +
                   "\nPlease add " + dir + " to your PATH manually.";
        }
    }

    private static String addToPathWindows(String dir) throws Exception {
        Process p = Runtime.getRuntime().exec(new String[]{
            "reg", "query", "HKCU\\Environment", "/v", "Path"
        });
        String output = new String(p.getInputStream().readAllBytes());
        p.waitFor();

        String currentPath = "";
        if (p.exitValue() == 0) {
            for (String line : output.split("\\r?\\n")) {
                line = line.trim();
                if (line.contains("REG_") && line.contains("Path")) {
                    int idx = line.indexOf("REG_");
                    idx = line.indexOf("    ", idx);
                    if (idx >= 0) currentPath = line.substring(idx).trim();
                    break;
                }
            }
        }

        for (String entry : currentPath.split(";")) {
            if (entry.trim().equalsIgnoreCase(dir)) {
                return null;
            }
        }

        String newPath = currentPath.isEmpty() ? dir : currentPath + ";" + dir;
        Process p2 = Runtime.getRuntime().exec(new String[]{
            "reg", "add", "HKCU\\Environment", "/v", "Path", "/t", "REG_EXPAND_SZ", "/d", newPath, "/f"
        });
        p2.waitFor();

        Runtime.getRuntime().exec(new String[]{
            "cmd", "/c", "setx", "CPUSIM64_PATH_SET", "1"
        }).waitFor();

        if (p2.exitValue() == 0) {
            return "PATH has been updated. Restart your terminal for it to take effect.";
        }
        return "Could not update PATH automatically.\nPlease add " + dir + " to your PATH manually.";
    }

    private static String addToPathUnix(String dir) throws Exception {
        Path profilePath;
        if (IS_MAC) {
            profilePath = Path.of(System.getProperty("user.home"), ".zprofile");
        } else {
            profilePath = Path.of(System.getProperty("user.home"), ".bashrc");
        }

        if (Files.exists(profilePath)) {
            String content = Files.readString(profilePath);
            if (content.contains(dir)) return null;
        }

        String snippet = "\n# Added by CPUSim64 installer\n"
            + "case \":$PATH:\" in\n"
            + "  *\":" + dir + ":\"*) ;;\n"
            + "  *) export PATH=\"" + dir + ":$PATH\" ;;\n"
            + "esac\n";

        Files.writeString(profilePath, snippet,
            StandardOpenOption.CREATE, StandardOpenOption.APPEND);

        return "PATH has been updated in " + profilePath.getFileName() +
               ". Restart your terminal for it to take effect.";
    }
}
