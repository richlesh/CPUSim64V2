// SPDX-License-Identifier: Apache-2.0
package com.glowingcat.cpusim64ide;

import static org.junit.jupiter.api.Assertions.*;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

/** Files opened from the desktop (Finder, file associations): which window takes them. */
class OpenFromSystemTest {

    @Test
    void cpuSim64SourcesAreAsmAndDef() {
        assertTrue(IDEApp.isCpuSim64Source("prog.asm"));
        assertTrue(IDEApp.isCpuSim64Source("LIB.DEF"));
        assertFalse(IDEApp.isCpuSim64Source("prog.o64"));
        assertFalse(IDEApp.isCpuSim64Source("prog.c"));
    }

    @Test
    void otherSourcesGoToThePluginThatTakesTheExtension(@TempDir Path tmp) throws Exception {
        Path compilers = Files.createDirectories(tmp.resolve("compilers"));
        Path c = Files.createDirectories(compilers.resolve("C11"));
        Files.writeString(c.resolve("config.properties"), "compiler_source: cc\nfile_extensions: c,h\n");
        Path m = Files.createDirectories(compilers.resolve("MMIX"));
        Files.writeString(m.resolve("config.properties"), "compiler_source: mmixc\nfile_extensions: mms\n");
        Path any = Files.createDirectories(compilers.resolve("Any"));
        Files.writeString(any.resolve("config.properties"), "compiler_source: any\n");   // no extensions
        List<CompilerPlugin> plugins = CompilerPlugin.discover(compilers);
        assertEquals("C11", IDEApp.pluginFor("x.h", plugins).getName());
        assertEquals("MMIX", IDEApp.pluginFor("hello.mms", plugins).getName());
        assertNull(IDEApp.pluginFor("prog.o64", plugins), "a plug-in without extensions takes nothing");
    }

    @Test
    void argumentsThatNameFilesAreOpened(@TempDir Path tmp) throws Exception {
        Path f = Files.writeString(tmp.resolve("a.asm"), "\tstop\n");
        List<Path> files = IDEApp.filesFromArgs(new String[] {"--uninstall", f.toString(), tmp.resolve("missing.asm").toString()});
        assertEquals(List.of(f.toAbsolutePath().normalize()), files);
    }
}
