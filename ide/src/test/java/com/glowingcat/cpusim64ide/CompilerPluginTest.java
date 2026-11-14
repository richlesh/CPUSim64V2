// SPDX-License-Identifier: Apache-2.0
package com.glowingcat.cpusim64ide;

import static org.junit.jupiter.api.Assertions.*;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.attribute.FileTime;
import java.util.List;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

class CompilerPluginTest {

    private static Path plugin(Path compilers, String name, String config) throws Exception {
        Path d = Files.createDirectories(compilers.resolve(name));
        if (config != null) Files.writeString(d.resolve("config.properties"), config);
        return d;
    }

    @Test
    void discoversPluginsByFolderName(@TempDir Path tmp) throws Exception {
        Path compilers = Files.createDirectories(tmp.resolve("compilers"));
        Path c11 = plugin(compilers, "C11", "syntax_language: text/c\ncompiler_source: cc\nfile_extensions: c,.h\n");
        Files.writeString(c11.resolve("system_prompt.md"), "You help with C11.");
        plugin(compilers, "Alpha", "compiler_source: alpha.asm\n");
        plugin(compilers, "NoConfig", null);                       // not a plugin
        plugin(compilers, "NoSource", "syntax_language: text/c\n"); // missing compiler_source
        Files.writeString(compilers.resolve("README.txt"), "not a directory");

        List<CompilerPlugin> found = CompilerPlugin.discover(compilers);
        assertEquals(List.of("Alpha", "C11"), found.stream().map(CompilerPlugin::getName).toList());

        CompilerPlugin alpha = found.get(0);
        assertEquals("alpha", alpha.getCompilerSource(), ".asm is optional");
        assertEquals("text/plain", alpha.getSyntaxLanguage(), "default syntax");
        assertTrue(alpha.getFileExtensions().isEmpty());
        assertTrue(alpha.acceptsFile("anything.xyz"));
        assertNull(alpha.getSystemPrompt());
        assertEquals("16M", alpha.getCompilerMem());
        assertNull(alpha.getEntryLabel());
        assertEquals("256K", alpha.getCompilerStack());

        CompilerPlugin cc = found.get(1);
        assertEquals("cc", cc.getCompilerSource());
        assertEquals("text/c", cc.getSyntaxLanguage());
        assertEquals(List.of("c", "h"), cc.getFileExtensions());
        assertTrue(cc.acceptsFile("Prog.C"));
        assertFalse(cc.acceptsFile("prog.asm"));
        assertEquals("You help with C11.", cc.getSystemPrompt());
        assertEquals(c11.resolve("cc.asm"), cc.compilerAsm(c11));
        assertEquals(c11.resolve("obj").resolve("cc.o64"), cc.compilerObj(c11));
    }

    @Test
    void noCompilersDirectoryGivesNoPlugins(@TempDir Path tmp) {
        assertTrue(CompilerPlugin.discover(null).isEmpty());
        assertTrue(CompilerPlugin.discover(tmp.resolve("missing")).isEmpty());
    }

    @Test
    void findsCompilersNextToTheJar(@TempDir Path tmp) throws Exception {
        Path app = Files.createDirectories(tmp.resolve("app"));
        Path jar = Files.writeString(app.resolve("CPUSim64IDE.jar"), "x");
        assertNull(CompilerPlugin.findCompilersDir(jar, tmp.resolve("elsewhere")));
        Files.createDirectories(app.resolve("compilers"));
        assertEquals(app.resolve("compilers"), CompilerPlugin.findCompilersDir(jar, tmp.resolve("elsewhere")));
    }

    @Test
    void findsCompilersSiblingOfIdeFolderWhenRunningFromClasses(@TempDir Path tmp) throws Exception {
        Path classes = Files.createDirectories(tmp.resolve("repo/ide/target/classes"));
        Files.createDirectories(tmp.resolve("repo/compilers"));
        assertEquals(tmp.resolve("repo/compilers"), CompilerPlugin.findCompilersDir(classes, tmp));
    }

    @Test
    void findsCompilersFromSourceCheckoutWhenRunningTheJarFromLib(@TempDir Path tmp) throws Exception {
        Path lib = Files.createDirectories(tmp.resolve("repo/lib"));
        Path jar = Files.writeString(lib.resolve("CPUSim64IDE.jar"), "x");
        Files.createDirectories(tmp.resolve("repo/ide"));
        Files.createDirectories(tmp.resolve("repo/compilers"));
        assertEquals(tmp.resolve("repo/compilers"), CompilerPlugin.findCompilersDir(jar, tmp.resolve("unrelated")));
    }

    @Test
    void rebuildsWhenObjectMissingOrOlderThanAnyCompilerSource(@TempDir Path tmp) throws Exception {
        Path compilers = Files.createDirectories(tmp.resolve("compilers"));
        Path dir = plugin(compilers, "T", "compiler_source: main\n");
        CompilerPlugin t = CompilerPlugin.discover(compilers).get(0);
        Path main = Files.writeString(dir.resolve("main.asm"), "nop");
        Path module = Files.writeString(dir.resolve("module.asm"), "nop");
        assertTrue(t.needsBuild(dir), "no object file yet");

        Files.createDirectories(dir.resolve("obj"));
        Path obj = Files.writeString(t.compilerObj(dir), "o64");
        long now = System.currentTimeMillis();
        Files.setLastModifiedTime(main, FileTime.fromMillis(now - 10_000));
        Files.setLastModifiedTime(module, FileTime.fromMillis(now - 10_000));
        Files.setLastModifiedTime(obj, FileTime.fromMillis(now - 5_000));
        assertFalse(t.needsBuild(dir), "object newer than every source");

        Files.setLastModifiedTime(module, FileTime.fromMillis(now));      // an included module changed
        assertTrue(t.needsBuild(dir));
        Files.setLastModifiedTime(module, FileTime.fromMillis(now - 10_000));
        Files.setLastModifiedTime(main, FileTime.fromMillis(now));        // the main source changed
        assertTrue(t.needsBuild(dir));
    }

    @Test
    void syncCopiesChangesButNotTestsOrObj(@TempDir Path tmp) throws Exception {
        Path from = Files.createDirectories(tmp.resolve("from"));
        Files.writeString(from.resolve("cc.asm"), "v1");
        Files.createDirectories(from.resolve("lib"));
        Files.writeString(from.resolve("lib/stdio.h"), "h");
        Files.createDirectories(from.resolve("tests"));
        Files.writeString(from.resolve("tests/t.c"), "t");
        Files.createDirectories(from.resolve("obj"));
        Files.writeString(from.resolve("obj/cc.o64"), "o");

        Path to = tmp.resolve("to");
        CompilerPlugin.syncTree(from, to);
        assertEquals("v1", Files.readString(to.resolve("cc.asm")));
        assertEquals("h", Files.readString(to.resolve("lib/stdio.h")));
        assertFalse(Files.exists(to.resolve("tests")));
        assertFalse(Files.exists(to.resolve("obj")));

        Files.writeString(from.resolve("cc.asm"), "v2");
        Files.setLastModifiedTime(from.resolve("cc.asm"), FileTime.fromMillis(System.currentTimeMillis() + 5_000));
        CompilerPlugin.syncTree(from, to);
        assertEquals("v2", Files.readString(to.resolve("cc.asm")));
    }

    @Test
    void detectsMainLabel(@TempDir Path tmp) throws Exception {
        Path a = Files.writeString(tmp.resolve("a.asm"), "foo:\n   nop\n  __main__:\n   stop\n");
        Path b = Files.writeString(tmp.resolve("b.asm"), "foo:\n   nop\n");
        assertTrue(CompilerPlugin.hasMainLabel(a));
        assertFalse(CompilerPlugin.hasMainLabel(b));
    }
}
