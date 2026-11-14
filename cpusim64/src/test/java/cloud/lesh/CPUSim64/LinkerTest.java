// SPDX-License-Identifier: Apache-2.0
package cloud.lesh.CPUSim64;

import static org.junit.jupiter.api.Assertions.*;

import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.List;

import java.util.concurrent.TimeUnit;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;
import org.junit.jupiter.api.io.TempDir;

/** Relocatable assembly (assemble -r) and the Linker. */
@Timeout(value = 30, unit = TimeUnit.SECONDS)
class LinkerTest {

	private static Linker.Module module(Path dir, String name, String src) throws Exception {
		Path asm = dir.resolve(name + ".asm");
		Files.writeString(asm, src);
		Assembler.ModuleResult r = Assembler.assembleModuleFile(asm, dir.resolve("obj").resolve(name), List.of());
		assertNotNull(r, "assembly of " + name + " failed");
		return Linker.load(dir.resolve("obj").resolve(name + ".ro64"));
	}

	private static Linker linker(Path dir) {
		return new Linker(List.of(), null).cacheDir(dir.resolve("cache"));
	}

	/** Runs a linked program and returns its result (r0 at STOP, or the iEXIT code). */
	private static long run(List<Long> words) {
		// same sizes as Simulation's defaults (1M words of memory, 8K stack)
		Simulator sim = new Simulator(1048576, 0, 8192, new String[] {"test"});
		sim.loadProgram(words, 0L, new HashMap<>());
		try {
			return sim.run(words.get(0));
		} finally {
			sim.unregister();
		}
	}

	private static final String MAIN = """
			#extern add3, counter, table
			__MAIN__:
				move	r1, 10
				push	r1
				call	add3
				add		sp, 1
				load	r1, counter
				add		r0, r1
				load	r1, table[2]		// 30
				add		r0, r1
				move	r1, table+1
				load	r1, r1[0]			// 20
				add		r0, r1
				stop
			""";

	private static final String UTIL = """
			#export add3, counter, table
			add3:
				load	r0, sf[3]
				add		r0, 3
				load	r1, counter
				add		r1, 1
				store	r1, counter
				return
			table:	.dci 10
				.dci 20
				.dci 30
			#global counter: .dci 41
			""";

	@Test
	void linksTwoModules(@TempDir Path dir) throws Exception {
		var main = module(dir, "main", MAIN);
		var util = module(dir, "util", UTIL);
		assertTrue(main.rel().imports.containsAll(List.of("ADD3", "COUNTER", "TABLE")));
		assertEquals(1, util.rel().dataSize);
		Linker.Result r = linker(dir).link(List.of(main, util));
		// 13 + 42 (counter after add3) + 30 + 20
		assertEquals(105, run(r.words()));
		assertEquals(List.of("util", "main"), r.order().stream().map(Linker.Module::name).toList(),
				"the main module is linked last");
		assertTrue(r.labels().containsKey("ADD3"));
		assertEquals(r.labels().get("__CODE_END__"), r.labels().get("__DATA__"));
	}

	@Test
	void inputOrderDoesNotMatter(@TempDir Path dir) throws Exception {
		var main = module(dir, "main", MAIN);
		var util = module(dir, "util", UTIL);
		assertEquals(105, run(linker(dir).link(List.of(util, main)).words()));
	}

	@Test
	void singleModuleIsIdenticalToNormalAssembly(@TempDir Path dir) throws Exception {
		String src = """
				move	r1, msg
				move	r3, msg+1
				load	r2, count
				jump	done
				nop
				done:	stop
				msg:	.dcs "hi"
				#global count: .dci 7
				""";
		var m = module(dir, "one", src);
		Linker.Result r = linker(dir).link(List.of(m));
		Path n = Files.createDirectories(dir.resolve("n")).resolve("one.asm");
		Files.writeString(n, src);
		assertEquals(0, Assembler.run(new String[] {n.toString()}));
		assertEquals(AsmIO.readU64BE(dir.resolve("n/one.o64").toFile()), r.words());
	}

	@Test
	void privateLabelsDoNotClash(@TempDir Path dir) throws Exception {
		var a = module(dir, "a", """
				#extern f
				__MAIN__:
					call	f
					jump	done
					nop
				done:	stop
				""");
		var b = module(dir, "b", """
				#export f
				f:		jump	done
						nop
				done:	move	r0, 7
						return
				""");
		Linker.Result r = linker(dir).link(List.of(a, b));
		assertEquals(7, run(r.words()));
		assertTrue(r.labels().containsKey("A$DONE") && r.labels().containsKey("B$DONE"),
				"clashing private labels are qualified with the module name: " + r.labels().keySet());
	}

	@Test
	void undefinedSymbol(@TempDir Path dir) throws Exception {
		var a = module(dir, "a", "#extern nowhere\n\tcall nowhere\n\tstop\n");
		var ex = assertThrows(Linker.LinkException.class, () -> linker(dir).link(List.of(a)));
		assertTrue(ex.getMessage().contains("Undefined symbol NOWHERE"), ex.getMessage());
	}

	@Test
	void unexportedSymbolIsReported(@TempDir Path dir) throws Exception {
		var a = module(dir, "a", "#extern f\n\tcall f\n\tstop\n");
		var b = module(dir, "b", "f:\treturn\n");
		var ex = assertThrows(Linker.LinkException.class, () -> linker(dir).link(List.of(a, b)));
		assertTrue(ex.getMessage().contains("does not #export"), ex.getMessage());
	}

	@Test
	void duplicateExport(@TempDir Path dir) throws Exception {
		var a = module(dir, "a", "#export f\nf:\treturn\n");
		var b = module(dir, "b", "#export f\nf:\treturn\n");
		var ex = assertThrows(Linker.LinkException.class, () -> linker(dir).link(List.of(a, b)));
		assertTrue(ex.getMessage().contains("Duplicate symbol F"), ex.getMessage());
	}

	@Test
	void duplicateMain(@TempDir Path dir) throws Exception {
		var a = module(dir, "a", "__MAIN__:\tstop\n");
		var b = module(dir, "b", "__MAIN__:\tstop\n");
		var ex = assertThrows(Linker.LinkException.class, () -> linker(dir).link(List.of(a, b)));
		assertTrue(ex.getMessage().contains("Duplicate __MAIN__") && ex.getMessage().contains(" a ")
				&& ex.getMessage().contains(" b "), ex.getMessage());
	}

	@Test
	void undeclaredExternalIsAnAssemblyError(@TempDir Path dir) throws Exception {
		Path asm = dir.resolve("a.asm");
		Files.writeString(asm, "\tcall nowhere\n\tstop\n");
		assertNull(Assembler.assembleModuleFile(asm, dir.resolve("a"), List.of()));
	}

	@Test
	void labelInTwelveBitFieldIsRejected(@TempDir Path dir) throws Exception {
		Path asm = dir.resolve("a.asm");
		Files.writeString(asm, "\tstore\tlab, r1[0]\n\tstop\nlab:\t.dci 1\n");
		PrintStream err = System.err;
		ByteArrayOutputStream buf = new ByteArrayOutputStream();
		System.setErr(new PrintStream(buf, true));
		try {
			assertNull(Assembler.assembleModuleFile(asm, dir.resolve("a"), List.of()));
		} finally {
			System.setErr(err);
		}
		assertTrue(buf.toString().contains("12-bit field"), buf.toString());
	}

	@Test
	void outputNameIsRequiredWithSeveralInputs(@TempDir Path dir) throws Exception {
		Files.writeString(dir.resolve("a.asm"), "#extern f\n__MAIN__:\tcall f\n\tstop\n");
		Files.writeString(dir.resolve("b.asm"), "#export f\nf:\tmove r0, 5\n\treturn\n");
		String a = dir.resolve("a.asm").toString(), b = dir.resolve("b.asm").toString();
		assertEquals(2, Assembler.run(new String[] {a, b}));
		assertEquals(2, Linker.run(new String[] {a, b}));
		String out = dir.resolve("prog").toString();
		assertEquals(0, Assembler.run(new String[] {"-o", out, a, b}));
		assertTrue(Files.isRegularFile(dir.resolve("obj/a.ro64")) && Files.isRegularFile(dir.resolve("obj/b.rel")));
		for (String ext : List.of(".o64", ".sym", ".sym1", ".sym2", ".srcmap"))
			assertTrue(Files.isRegularFile(dir.resolve("prog" + ext)), ext);
		assertEquals(5, run(AsmIO.readU64BE(dir.resolve("prog.o64").toFile())));
		// relink the modules with the linker command
		assertEquals(0, Linker.run(new String[] {dir.resolve("obj/a.ro64").toString(),
				dir.resolve("obj/b.ro64").toString(), "-o", dir.resolve("prog2").toString()}));
		assertEquals(AsmIO.readU64BE(dir.resolve("prog.o64").toFile()), AsmIO.readU64BE(dir.resolve("prog2.o64").toFile()));
	}

	@Test
	void directivesAreIgnoredInNormalAssembly(@TempDir Path dir) throws Exception {
		Path asm = dir.resolve("p.asm");
		Files.writeString(asm, "#export f\n#extern g\n#export *\n\tcall f\n\tstop\nf:\treturn\n");
		assertEquals(0, Assembler.run(new String[] {asm.toString()}));
		assertTrue(Files.isRegularFile(dir.resolve("p.o64")));
	}

	@Test
	void systemLibraryIsLinkedFromCache(@TempDir Path dir) throws Exception {
		var m = module(dir, "hello", """
				#include <system/io.asm>
				__MAIN__:
					#call	putline("hi")
					move	r0, 3
					stop
				""");
		assertEquals("system/io.asm", m.rel().requires.get(0).path());
		Linker.Result r = linker(dir).link(List.of(m));
		assertEquals(3, run(r.words()));
		assertTrue(Files.isRegularFile(dir.resolve("cache/" + BuildInfo.VERSION + "/default/system/io.ro64")));
		// second link reuses the cached modules
		long t = Files.getLastModifiedTime(dir.resolve("cache/" + BuildInfo.VERSION + "/default/system/io.ro64")).toMillis();
		linker(dir).link(List.of(m));
		assertEquals(t, Files.getLastModifiedTime(dir.resolve("cache/" + BuildInfo.VERSION + "/default/system/io.ro64")).toMillis());
	}

	@Test
	void initFunctionsRunInDependencyOrderBeforeMain(@TempDir Path dir) throws Exception {
		// lib1 and lib2 each have a file-local __INIT__; main's result shows the order they ran in
		Files.writeString(dir.resolve("lib1.asm"), """
				#export trace
				jump	LIB1_END
				#def_func __INIT__()
					load	r0, trace
					mult	r0, 10
					add		r0, 1
					store	r0, trace
				#end_func
				LIB1_END: nop
				#global trace: .dci 0
				""");
		Files.writeString(dir.resolve("lib2.asm"), """
				#include "lib1.asm"
				jump	LIB2_END
				__INIT__:
					load	r0, trace
					mult	r0, 10
					add		r0, 2
					store	r0, trace
					return
				LIB2_END: nop
				""");
		String main = """
				#include "lib2.asm"
				#extern trace
				__MAIN__:
					load	r0, trace
					return
				""";
		// one piece: lib1 inlined first, so 1 then 2
		Path one = dir.resolve("one.asm");
		Files.writeString(one, main);
		assertEquals(0, Assembler.run(new String[] {one.toString()}));
		List<Long> single = AsmIO.readU64BE(dir.resolve("one.o64").toFile());
		assertEquals(12, run(single));
		// linked: lib1 and lib2 are required modules, assembled automatically
		var m = module(dir, "main", main);
		Linker.Result r = linker(dir).link(List.of(m));
		assertEquals(12, run(r.words()));
		assertEquals(List.of("lib1", "lib2", "main"), r.order().stream().map(Linker.Module::name).toList());
		assertEquals(single, r.words(), "linked program is identical to the one assembled in one piece");
	}

	@Test
	void mainExitsWithItsResult(@TempDir Path dir) throws Exception {
		var m = module(dir, "m", "__MAIN__:\tmove r0, 42\n\treturn\n");
		assertEquals(42, run(linker(dir).link(List.of(m)).words()));
	}

	@Test
	void programWithoutMainStartsWithTheMainModulesCode(@TempDir Path dir) throws Exception {
		// util's function is at the top level of util (no jump over it), and util is linked first
		var util = module(dir, "util", "#export add3\nadd3:\tload r0, sf[3]\n\tadd r0, 3\n\treturn\n");
		var main = module(dir, "main", "#extern add3\n\tmove r1, 4\n\tpush r1\n\tcall add3\n\tadd sp, 1\n\tstop\n");
		// without __MAIN__ the first input is the program; util is linked before it
		Linker.Result r = linker(dir).link(List.of(main, util));
		assertEquals(List.of("util", "main"), r.order().stream().map(Linker.Module::name).toList());
		assertEquals(7, run(r.words()));
	}

	@Test
	void relocatableWithProgramOutputLinksTheIncludedModules(@TempDir Path dir) throws Exception {
		Files.writeString(dir.resolve("util.asm"), "#export add3\njump UTIL_END\nadd3:\tload r0, sf[3]\n\tadd r0, 3\n\treturn\nUTIL_END: nop\n");
		Files.writeString(dir.resolve("main.asm"), "#include \"util.asm\"\n__MAIN__:\n\tmove r1, 4\n\tpush r1\n\tcall add3\n\tadd sp, 1\n\treturn\n");
		assertEquals(0, Assembler.run(new String[] {"-r", dir.resolve("main.asm").toString(),
				"-o", dir.resolve("obj/main.o64").toString()}));
		assertEquals(7, run(AsmIO.readU64BE(dir.resolve("obj/main.o64").toFile())));
		assertTrue(Files.isRegularFile(dir.resolve("obj/util.ro64")), "the included module was assembled");
	}
}
