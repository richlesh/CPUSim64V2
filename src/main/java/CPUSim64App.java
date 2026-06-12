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
import java.awt.*;
import java.io.*;
import java.nio.file.*;

public class CPUSim64App {
    private static final boolean IS_WINDOWS = System.getProperty("os.name").toLowerCase().contains("win");

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            Icon icon = null;
            var iconUrl = CPUSim64App.class.getResource("/app_icon_256.png");
            if (iconUrl != null) {
                icon = new ImageIcon(new ImageIcon(iconUrl)
                    .getImage().getScaledInstance(64, 64, Image.SCALE_SMOOTH));
            }
            int result = JOptionPane.showOptionDialog(
                null,
                "CPUSim64 needs to install command line tools on your system?",
                "CPUSim64",
                JOptionPane.DEFAULT_OPTION,
                JOptionPane.QUESTION_MESSAGE,
                icon,
                new Object[]{"Install", "Cancel"},
                "Install"
            );
            if (result == 0) {
                install();
            }
            System.exit(0);
        });
    }

    private static void install() {
        try {
            String toolName = IS_WINDOWS ? "cpusim64.exe" : "cpusim64";

            InputStream in = CPUSim64App.class.getResourceAsStream("/" + toolName);
            if (in == null) {
                JOptionPane.showMessageDialog(null,
                    "Error: " + toolName + " not found in application resources.",
                    "CPUSim64", JOptionPane.ERROR_MESSAGE);
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

            String message = "Command line tools installed successfully to " + destToolPath;
            if (IS_WINDOWS) {
                message += "\n\nPlease add " + destDir + " to your PATH environment variable.";
            } else {
                message += "\n\nEnsure " + destDir + " is in your PATH.";
            }

            JOptionPane.showMessageDialog(null, message, "CPUSim64", JOptionPane.INFORMATION_MESSAGE);
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null,
                "Installation failed: " + e.getMessage() + "\nYou may need to run with administrator privileges.",
                "CPUSim64", JOptionPane.ERROR_MESSAGE);
        }
    }
}
