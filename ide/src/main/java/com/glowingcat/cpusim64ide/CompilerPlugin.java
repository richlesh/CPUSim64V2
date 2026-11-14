// SPDX-License-Identifier: Apache-2.0
package com.glowingcat.cpusim64ide;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Properties;
import java.util.stream.Stream;

/**
 * A compiler written in CPUSim64 assembly language that the IDE can use to edit
 * and run another programming language (for example C11).
 *
 * <p>Each plugin is a folder inside the {@code compilers} directory. The folder
 * name is the plugin's name (it is shown in the File &gt; New Compiler menu and in
 * the window title). The folder contains:
 * <ul>
 *   <li>{@code config.properties} with
 *     <ul>
 *       <li>{@code compiler_source} - the compiler's main {@code .asm} file (the
 *           {@code .asm} extension is optional);</li>
 *       <li>{@code syntax_language} - the RSyntaxTextArea syntax style used for
 *           highlighting, e.g. {@code text/c} (default {@code text/plain});</li>
 *       <li>{@code file_extensions} - optional comma separated list of source file
 *           extensions, e.g. {@code c,h}; the first is the default for Save As;</li>
 *       <li>{@code compiler_mem}, {@code compiler_stack} - optional memory and
 *           stack sizes used when running the compiler (default 16M and 256K);</li>
 *       <li>{@code main_function} - name of the program's entry function (default
 *           {@code main}): Run and Debug are enabled for a source that defines it.</li>
 *     </ul></li>
 *   <li>{@code system_prompt.md} - optional AI Chat system prompt;</li>
 *   <li>{@code obj/} - the assembled compiler is kept here (created on demand).</li>
 * </ul>
 *
 * The compiler is run with the path of the source file as its argument and must
 * write {@code <source without extension>.asm} next to the source and exit with
 * 0 on success.
 */
public final class CompilerPlugin {

    public static final String COMPILERS_DIR_PROPERTY = "cpusim64.compilers";

    private final String name;
    private final Path dir;
    private final String compilerSource;
    private final String syntaxLanguage;
    private final List<String> fileExtensions;
    private final String compilerMem;
    private final String compilerStack;
    private final String mainFunction;
    private final String systemPrompt;

    private CompilerPlugin(String name, Path dir, Properties props) {
        this.name = name;
        this.dir = dir;
        String src = props.getProperty("compiler_source", "").trim();
        if (src.endsWith(".asm")) src = src.substring(0, src.length() - 4);
        this.compilerSource = src;
        String syn = props.getProperty("syntax_language", "").trim();
        this.syntaxLanguage = syn.isEmpty() ? "text/plain" : syn;
        List<String> exts = new ArrayList<>();
        for (String e : props.getProperty("file_extensions", "").split("[,\\s]+")) {
            if (e.startsWith(".")) e = e.substring(1);
            if (!e.isEmpty()) exts.add(e.toLowerCase());
        }
        this.fileExtensions = List.copyOf(exts);
        this.compilerMem = props.getProperty("compiler_mem", "16M").trim();
        this.compilerStack = props.getProperty("compiler_stack", "256K").trim();
        String mf = props.getProperty("main_function", "main").trim();
        this.mainFunction = mf.isEmpty() ? "main" : mf;
        String prompt = null;
        Path p = dir.resolve("system_prompt.md");
        if (Files.isRegularFile(p)) {
            try { prompt = Files.readString(p, StandardCharsets.UTF_8); } catch (IOException ignored) {}
        }
        this.systemPrompt = prompt;
    }

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------

    /** The plugin name: the name of its folder inside {@code compilers}. */
    public String getName() { return name; }
    public Path getDir() { return dir; }
    /** Compiler main source name without the {@code .asm} extension. */
    public String getCompilerSource() { return compilerSource; }
    /** RSyntaxTextArea syntax style, e.g. {@code text/c}. */
    public String getSyntaxLanguage() { return syntaxLanguage; }
    public List<String> getFileExtensions() { return fileExtensions; }
    public String getCompilerMem() { return compilerMem; }
    public String getCompilerStack() { return compilerStack; }
    /** Name of the program entry function (config {@code main_function}, default {@code main}). */
    public String getMainFunction() { return mainFunction; }
    /** Contents of {@code system_prompt.md}, or null if the plugin has none. */
    public String getSystemPrompt() { return systemPrompt; }

    @Override public String toString() { return name; }

    /** True if {@code fileName} has one of the plugin's source extensions (or the plugin names none). */
    public boolean acceptsFile(String fileName) {
        if (fileExtensions.isEmpty()) return true;
        String lower = fileName.toLowerCase();
        for (String e : fileExtensions) if (lower.endsWith("." + e)) return true;
        return false;
    }

    // ------------------------------------------------------------------
    // Discovery
    // ------------------------------------------------------------------

    /**
     * Locates the {@code compilers} directory, or returns null if there is none.
     * Looked up in this order:
     * <ol>
     *   <li>the {@code -Dcpusim64.compilers=DIR} system property;</li>
     *   <li>a {@code compilers} folder in the same folder as the IDE jar (deployment);</li>
     *   <li>when running from classes in an IDE such as IntelliJ IDEA: a
     *       {@code compilers} folder that is a sibling of the {@code ide} folder;</li>
     *   <li>the same sibling test, walking up from the jar folder and from the
     *       working directory (running the jar from a source checkout).</li>
     * </ol>
     */
    public static Path findCompilersDir() {
        String override = System.getProperty(COMPILERS_DIR_PROPERTY);
        if (override != null && !override.isBlank()) {
            Path p = Path.of(override);
            return Files.isDirectory(p) ? p : null;
        }
        Path code = null;
        try {
            var loc = CompilerPlugin.class.getProtectionDomain().getCodeSource().getLocation();
            code = Path.of(loc.toURI());
        } catch (Exception ignored) {}
        return findCompilersDir(code, Path.of("").toAbsolutePath());
    }

    /** Testable form of {@link #findCompilersDir()}. */
    static Path findCompilersDir(Path codeLocation, Path workingDir) {
        if (codeLocation != null) {
            // Deployment: compilers/ next to the jar.
            Path jarDir = Files.isDirectory(codeLocation) ? null : codeLocation.toAbsolutePath().getParent();
            if (jarDir != null && Files.isDirectory(jarDir.resolve("compilers"))) {
                return jarDir.resolve("compilers");
            }
            // Development: a folder called "ide" with a sibling "compilers"
            // (class files are in ide/target/classes).
            Path found = siblingOfIde(codeLocation.toAbsolutePath());
            if (found != null) return found;
            if (jarDir != null) {
                found = repoCompilers(jarDir);
                if (found != null) return found;
            }
        }
        if (workingDir != null) return repoCompilers(workingDir);
        return null;
    }

    private static Path siblingOfIde(Path start) {
        for (Path p = start; p != null; p = p.getParent()) {
            Path fn = p.getFileName();
            if (fn != null && fn.toString().equals("ide") && p.getParent() != null) {
                Path c = p.getParent().resolve("compilers");
                if (Files.isDirectory(c)) return c;
            }
        }
        return null;
    }

    /** Walks up from {@code start} to a folder that holds both "ide" and "compilers". */
    private static Path repoCompilers(Path start) {
        int levels = 0;
        for (Path p = start; p != null && levels < 8; p = p.getParent(), levels++) {
            Path c = p.resolve("compilers");
            if (Files.isDirectory(c) && Files.isDirectory(p.resolve("ide"))) return c;
        }
        return null;
    }

    /** All plugins found in the {@code compilers} directory, sorted by name. */
    public static List<CompilerPlugin> discover() {
        return discover(findCompilersDir());
    }

    public static List<CompilerPlugin> discover(Path compilersDir) {
        List<CompilerPlugin> result = new ArrayList<>();
        if (compilersDir == null || !Files.isDirectory(compilersDir)) return result;
        try (Stream<Path> children = Files.list(compilersDir)) {
            for (Path d : (Iterable<Path>) children.sorted(
                    Comparator.comparing((Path x) -> x.getFileName().toString().toLowerCase()))::iterator) {
                if (!Files.isDirectory(d) || d.getFileName().toString().startsWith(".")) continue;
                Path cfg = d.resolve("config.properties");
                if (!Files.isRegularFile(cfg)) continue;
                Properties props = new Properties();
                try (InputStream in = Files.newInputStream(cfg)) {
                    props.load(new java.io.InputStreamReader(in, StandardCharsets.UTF_8));
                } catch (IOException e) {
                    continue;
                }
                CompilerPlugin plugin = new CompilerPlugin(d.getFileName().toString(), d, props);
                if (!plugin.compilerSource.isEmpty()) result.add(plugin);
            }
        } catch (IOException ignored) {}
        return result;
    }

    // ------------------------------------------------------------------
    // Building the compiler
    // ------------------------------------------------------------------

    /**
     * The folder in which the compiler is assembled and run. Normally the plugin
     * folder itself; if that cannot be written to (for example an installed,
     * read-only application folder) a per-user copy of the plugin (without tests
     * and obj) is kept in {@code ~/.cpusim64/compilers/<name>} and used instead.
     */
    public Path workingRoot() throws IOException {
        if (isWritableDir(dir)) return dir;
        Path cache = Path.of(System.getProperty("user.home"), ".cpusim64", "compilers", name);
        syncTree(dir, cache);
        return cache;
    }

    private static boolean isWritableDir(Path d) {
        try {
            Files.createDirectories(d.resolve("obj"));
            Path probe = Files.createTempFile(d, ".cpusim64-write-test", ".tmp");
            Files.deleteIfExists(probe);
            return true;
        } catch (IOException | SecurityException e) {
            return false;
        }
    }

    /** Copies new or changed files from {@code from} to {@code to}, skipping tests/ and obj/. */
    static void syncTree(Path from, Path to) throws IOException {
        Files.createDirectories(to);
        try (Stream<Path> walk = Files.walk(from)) {
            for (Path src : (Iterable<Path>) walk::iterator) {
                Path rel = from.relativize(src);
                if (rel.getNameCount() > 0) {
                    String top = rel.getName(0).toString();
                    if (top.equals("tests") || top.equals("obj") || top.startsWith(".")) continue;
                }
                Path dst = to.resolve(rel.toString());
                if (Files.isDirectory(src)) {
                    Files.createDirectories(dst);
                } else if (!Files.exists(dst)
                        || Files.getLastModifiedTime(src).compareTo(Files.getLastModifiedTime(dst)) > 0) {
                    Files.copy(src, dst, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.COPY_ATTRIBUTES);
                }
            }
        }
    }

    public static Path compilerAsm(Path root, String source) { return root.resolve(source + ".asm"); }

    public Path compilerAsm(Path root) { return compilerAsm(root, compilerSource); }

    public Path compilerObj(Path root) { return root.resolve("obj").resolve(compilerSource + ".o64"); }

    /**
     * True if the compiler must be (re)assembled: the object file is missing or
     * older than the compiler's source. Every {@code .asm} and {@code .def} file in
     * the plugin folder counts as source, because the compiler is normally split
     * into several included modules.
     */
    public boolean needsBuild(Path root) throws IOException {
        Path obj = compilerObj(root);
        if (!Files.isRegularFile(obj)) return true;
        long objTime = Files.getLastModifiedTime(obj).toMillis();
        long newest = Files.getLastModifiedTime(compilerAsm(root)).toMillis();
        try (Stream<Path> files = Files.list(root)) {
            for (Path f : (Iterable<Path>) files::iterator) {
                String n = f.getFileName().toString();
                if (n.endsWith(".asm") || n.endsWith(".def")) {
                    newest = Math.max(newest, Files.getLastModifiedTime(f).toMillis());
                }
            }
        }
        return newest > objTime;
    }

    /** True if the assembly source has a {@code __MAIN__:} label (assembler needs --hasMain). */
    public static boolean hasMainLabel(Path asm) {
        try {
            for (String line : Files.readAllLines(asm, StandardCharsets.UTF_8)) {
                if (line.matches("(?i)^\\s*__MAIN__:.*")) return true;
            }
        } catch (IOException ignored) {}
        return false;
    }
}
