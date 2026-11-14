// SPDX-License-Identifier: Apache-2.0
package com.glowingcat.cpusim64ide;

import static org.junit.jupiter.api.Assertions.*;

import java.nio.file.Files;
import java.nio.file.Path;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

/** How the debugger finds the tab of a .srcmap file name (assembly files and .source files). */
class SourceMapTabTest {

    @Test
    void plainNamesMatchTheTabName() {
        Path f = Path.of("/work/prog/main.asm");
        assertTrue(IDEApp.tabMatchesSource(f, "main.asm", "main.asm"));
        assertFalse(IDEApp.tabMatchesSource(f, "main.asm", "io.asm"));
        assertTrue(IDEApp.tabMatchesSource(null, "io.asm", "io.asm"), "library tab");
    }

    @Test
    void absolutePathsMatchTheTabFile(@TempDir Path dir) {
        Path c = dir.resolve("prog.c");
        assertTrue(IDEApp.tabMatchesSource(c, "prog.c", c.toAbsolutePath().toString()));
        assertTrue(IDEApp.tabMatchesSource(c, "prog.c", dir.resolve("sub/../prog.c").toAbsolutePath().toString()));
        // same name in another directory is a different file
        assertFalse(IDEApp.tabMatchesSource(dir.resolve("other/prog.c"), "prog.c", c.toAbsolutePath().toString()));
        assertFalse(IDEApp.tabMatchesSource(null, "prog.c", c.toAbsolutePath().toString()));
    }

    @Test
    void fileOfASourceName(@TempDir Path dir) {
        assertEquals(dir.resolve("a.asm"), IDEApp.sourceFileFor(dir, "a.asm"));
        Path abs = dir.resolve("x/y.mms").toAbsolutePath();
        assertEquals(abs, IDEApp.sourceFileFor(Path.of("/elsewhere"), abs.toString()));
    }

    @Test
    void srcmapMentionsTheSourceFile(@TempDir Path dir) throws Exception {
        Path src = dir.resolve("hello.mms");
        Path map = dir.resolve("hello.srcmap");
        Files.writeString(map, "2: \u00abhello.asm\u00bb:3\n3: \u00ab" + src.toAbsolutePath() + "\u00bb:4\n");
        assertTrue(IDEApp.srcmapMentions(map, src));
        assertFalse(IDEApp.srcmapMentions(map, dir.resolve("other.mms")));
        assertFalse(IDEApp.srcmapMentions(dir.resolve("missing.srcmap"), src));
    }

    @Test
    void symbolicLinksNameTheSameFile(@TempDir Path dir) throws Exception {
        Path real = Files.createDirectories(dir.resolve("real"));
        Path c = Files.writeString(real.resolve("p.c"), "int main(void) { return 0; }\n");
        Path link = Files.createSymbolicLink(dir.resolve("link"), real);
        assertTrue(IDEApp.tabMatchesSource(link.resolve("p.c"), "p.c", c.toRealPath().toString()));
        Path map = Files.writeString(dir.resolve("p.srcmap"), "2: \u00ab" + c.toRealPath() + "\u00bb:1\n");
        assertTrue(IDEApp.srcmapMentions(map, link.resolve("p.c")));
    }
}
