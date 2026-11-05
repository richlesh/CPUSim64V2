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

import java.io.*;
import java.nio.file.*;

/**
 * Locates (and, if necessary, extracts from the application JAR) the native
 * {@code cpusim64_native} simulator executable together with its optional
 * {@code instructions.data} timing table, so the IDE can launch it as a
 * subprocess when the "Use Native CPUSim64 Simulator" setting is enabled.
 *
 * <p>The native tool reads {@code instructions.data} from its own directory, so
 * both files are placed in the same extraction directory.
 */
public final class NativeSimulator {
    private static final boolean IS_WINDOWS =
        System.getProperty("os.name").toLowerCase().contains("win");

    private static final String EXE_NAME = IS_WINDOWS ? "cpusim64_native.exe" : "cpusim64_native";

    private static Path cachedExe = null;

    private NativeSimulator() {}

    /** True if the native tool is available (bundled in resources or already installed). */
    public static boolean isAvailable() {
        try {
            return resolveExecutable() != null;
        } catch (IOException e) {
            return false;
        }
    }

    /**
     * Returns the path to a runnable {@code cpusim64_native} executable,
     * extracting it (and {@code instructions.data}) from the JAR to a temp
     * directory on first use. Returns {@code null} if the tool cannot be found.
     */
    public static synchronized Path resolveExecutable() throws IOException {
        if (cachedExe != null && Files.isRegularFile(cachedExe)) {
            return cachedExe;
        }

        // 1) Prefer a copy sitting next to the application JAR (installer app/ dir).
        Path beside = besideJar();
        if (beside != null && Files.isRegularFile(beside)) {
            cachedExe = beside;
            return cachedExe;
        }

        // 2) Development fallback: the Cargo build output. Useful when running
        // the IDE from an IDE/Maven (IntelliJ) where the native tool has not been
        // copied into resources by the CI packaging workflow. Honors an optional
        // -Dcpusim64.native=/path/to/exe override first.
        Path dev = devBuildExecutable();
        if (dev != null && Files.isRegularFile(dev)) {
            if (!IS_WINDOWS) {
                try { dev.toFile().setExecutable(true, false); } catch (Exception ignored) {}
            }
            cachedExe = dev;
            return cachedExe;
        }

        // 3) Otherwise extract from classpath resources to a temp directory.
        InputStream exeIn = NativeSimulator.class.getResourceAsStream("/" + EXE_NAME);
        if (exeIn == null) {
            return null;
        }
        Path dir = Files.createTempDirectory("cpusim64_native");
        dir.toFile().deleteOnExit();
        Path exe = dir.resolve(EXE_NAME);
        try (exeIn) {
            Files.copy(exeIn, exe, StandardCopyOption.REPLACE_EXISTING);
        }
        if (!IS_WINDOWS) {
            exe.toFile().setExecutable(true, false);
        }
        exe.toFile().deleteOnExit();

        // Extract instructions.data alongside it (optional; tool works without it).
        try (InputStream dataIn = NativeSimulator.class.getResourceAsStream("/instructions.data")) {
            if (dataIn != null) {
                Path data = dir.resolve("instructions.data");
                Files.copy(dataIn, data, StandardCopyOption.REPLACE_EXISTING);
                data.toFile().deleteOnExit();
            }
        }

        cachedExe = exe;
        return cachedExe;
    }

    /** Locate the executable next to the running application JAR, if present. */
    private static Path besideJar() {
        try {
            var loc = NativeSimulator.class.getProtectionDomain().getCodeSource().getLocation();
            Path jar = Path.of(loc.toURI());
            Path dir = Files.isDirectory(jar) ? jar : jar.getParent();
            if (dir == null) return null;
            Path candidate = dir.resolve(EXE_NAME);
            return candidate;
        } catch (Exception e) {
            return null;
        }
    }

    /**
     * Locate a locally built native executable for development runs (e.g. from
     * IntelliJ/Maven), where the CI packaging step hasn't copied the tool into
     * resources. Resolution order:
     *   1. the {@code -Dcpusim64.native=<path>} system property, if set;
     *   2. {@code cpusim64_native/target/{release,debug}/<exe>} searched from the
     *      working directory and the module/class output directory upward.
     */
    private static Path devBuildExecutable() {
        // Explicit override wins.
        String override = System.getProperty("cpusim64.native");
        if (override != null && !override.isBlank()) {
            Path p = Path.of(override);
            if (Files.isRegularFile(p)) return p;
        }

        // Candidate roots to search upward from.
        java.util.List<Path> roots = new java.util.ArrayList<>();
        roots.add(Path.of("").toAbsolutePath()); // process working directory
        try {
            var loc = NativeSimulator.class.getProtectionDomain().getCodeSource().getLocation();
            roots.add(Path.of(loc.toURI()));
        } catch (Exception ignored) {}

        for (Path start : roots) {
            Path dir = Files.isDirectory(start) ? start : start.getParent();
            // Walk up a bounded number of levels looking for the repo's crate dir.
            for (int i = 0; i < 8 && dir != null; i++, dir = dir.getParent()) {
                for (String profile : new String[]{"release", "debug"}) {
                    Path candidate = dir
                        .resolve("cpusim64_native")
                        .resolve("target")
                        .resolve(profile)
                        .resolve(EXE_NAME);
                    if (Files.isRegularFile(candidate)) {
                        return candidate;
                    }
                }
            }
        }
        return null;
    }
}
