// SPDX-License-Identifier: Apache-2.0
package cloud.lesh.CPUSim64;

import static org.junit.jupiter.api.Assertions.*;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.TimeUnit;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.io.TempDir;

/** The .source / .source_end directives: compiler source lines in the .srcmap file. */
@Timeout(value = 30, unit = TimeUnit.SECONDS)
class SourceDirectiveTest {

	/** Reads a .srcmap file: address -> «file»:line. */
	private static Map<Long, String> srcmap(Path file) throws Exception {
		Map<Long, String> m = new TreeMap<>();
		for (String line : Files.readAllLines(file)) {
			int c = line.indexOf(':');
			if (c > 0) m.put(Long.parseLong(line.substring(0, c).trim()), line.substring(c + 1).trim());
		}
		return m;
	}

	private static final String PROG = """
			\tmove\tr1, 1
			.source "x.c", 7
			\tmove\tr2, 2
			\tmove\tr3, 3
			.source "x.c", 9
			\tmove\tr4, 4
			.source_end
			\tmove\tr5, 5
			\tstop
			""";

	private static String abs(Path p) {
		return p.toAbsolutePath().normalize().toString();
	}

	@Test
	void sourceLinesReplaceAssemblyLines(@TempDir Path dir) throws Exception {
		Path asm = dir.resolve("p.asm");
		Files.writeString(asm, PROG);
		assertEquals(0, Assembler.run(new String[] {asm.toString()}));
		Map<Long, String> m = srcmap(dir.resolve("p.srcmap"));
		String x = "\u00ab" + abs(dir.resolve("x.c")) + "\u00bb:";
		// the five moves and the stop, in order
		List<Long> addrs = List.copyOf(m.keySet());
		assertEquals(6, addrs.size(), m.toString());
		assertEquals("\u00abp.asm\u00bb:1", m.get(addrs.get(0)));
		assertEquals(x + "7", m.get(addrs.get(1)));
		assertEquals(x + "7", m.get(addrs.get(2)));
		assertEquals(x + "9", m.get(addrs.get(3)));
		assertEquals("\u00abp.asm\u00bb:8", m.get(addrs.get(4)));
		assertEquals("\u00abp.asm\u00bb:9", m.get(addrs.get(5)));
	}

	@Test
	void relativePathIsRelativeToTheFileWithTheDirective(@TempDir Path dir) throws Exception {
		Path sub = Files.createDirectories(dir.resolve("sub"));
		Files.writeString(sub.resolve("inc.asm"), "f:\n.source \"../src/f.mms\", 3\n\tmove\tr0, 1\n.source_end\n\treturn\n");
		Path asm = dir.resolve("main.asm");
		Files.writeString(asm, "\tcall\tf\n\tstop\n#include \"sub/inc.asm\"\n");
		assertEquals(0, Assembler.run(new String[] {asm.toString()}));
		Map<Long, String> m = srcmap(dir.resolve("main.srcmap"));
		assertTrue(m.containsValue("\u00ab" + abs(dir.resolve("src/f.mms")) + "\u00bb:3"), m.toString());
	}

	@Test
	void absolutePathIsKept(@TempDir Path dir) throws Exception {
		Path asm = dir.resolve("p.asm");
		String file = abs(dir.resolve("elsewhere/y.c"));
		Files.writeString(asm, ".source " + Utils.sourceDirectiveLiteral(file) + ", 12\n\tmove\tr1, 1\n\tstop\n");
		assertEquals(0, Assembler.run(new String[] {asm.toString()}));
		assertTrue(srcmap(dir.resolve("p.srcmap")).containsValue("\u00ab" + file + "\u00bb:12"));
	}

	@Test
	void relocatableModuleAndLinkKeepTheSourceLines(@TempDir Path dir) throws Exception {
		Path a = dir.resolve("a.asm"), b = dir.resolve("b.asm");
		Files.writeString(a, "#extern f\n__MAIN__:\n.source \"a.c\", 4\n\tcall\tf\n\tstop\n");
		Files.writeString(b, "#export f\nf:\n.source \"b.c\", 10\n\tmove\tr0, 5\n\treturn\n");
		assertEquals(0, Assembler.run(new String[] {"-o", dir.resolve("prog").toString(), a.toString(), b.toString()}));
		Map<Long, String> m = srcmap(dir.resolve("prog.srcmap"));
		assertTrue(m.containsValue("\u00ab" + abs(dir.resolve("a.c")) + "\u00bb:4"), m.toString());
		assertTrue(m.containsValue("\u00ab" + abs(dir.resolve("b.c")) + "\u00bb:10"), m.toString());
	}

	@Test
	void errorsStillReportTheAssemblyLine(@TempDir Path dir) throws Exception {
		Path asm = dir.resolve("p.asm");
		Files.writeString(asm, ".source \"x.c\", 7\n\tmove\tr1, 1\n\tbogus\tr1\n\tstop\n");
		PrintStream old = System.err;
		ByteArrayOutputStream err = new ByteArrayOutputStream();
		System.setErr(new PrintStream(err, true));
		try {
			assertNotEquals(0, Assembler.run(new String[] {asm.toString()}));
		} finally {
			System.setErr(old);
		}
		assertTrue(err.toString().contains("p.asm"), err.toString());
		assertFalse(err.toString().contains("x.c"), err.toString());
	}

	@Test
	void pathLiteralRoundTrip() {
		String p = "C:\\dir\\a \"b\".c";
		assertEquals(p, Utils.sourceDirectivePath(Utils.sourceDirectiveLiteral(p)));
		assertEquals(".source \"/abs/x.c\", 3",
				Utils.absolutizeSourceDirective(".source \"x.c\", 3", s -> "/abs/" + s));
		assertEquals("\tmove\tr1, 1", Utils.absolutizeSourceDirective("\tmove\tr1, 1", s -> "/abs/" + s));
	}
}
