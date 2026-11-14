// SPDX-License-Identifier: Apache-2.0
package cloud.lesh.CPUSim64;

import static org.junit.jupiter.api.Assertions.*;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Map;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

class RelocInfoTest {

	@Test
	void jsonRoundTripOfScalarsAndStructures() {
		String text = "{\"a\": 1, \"b\": -42, \"c\": [true, false, null], \"d\": {\"e\": \"x\\ny\\\"z\\u00e9\"}, \"f\": 2.5, \"g\": []}";
		Object v = MiniJson.parse(text);
		Map<String, Object> m = MiniJson.obj(v, "root");
		assertEquals(1L, m.get("a"));
		assertEquals(-42L, m.get("b"));
		assertEquals(java.util.Arrays.asList(true, false, null), m.get("c"));
		assertEquals("x\ny\"z\u00e9", MiniJson.obj(m.get("d"), "d").get("e"));
		assertEquals(2.5, m.get("f"));
		assertEquals(List.of(), m.get("g"));
		assertEquals(v, MiniJson.parse(MiniJson.write(v)), "write then parse gives the same value");
		assertEquals(Long.MIN_VALUE, MiniJson.parse(Long.toString(Long.MIN_VALUE)));
	}

	@Test
	void malformedJsonIsReportedWithPosition() {
		var ex = assertThrows(MiniJson.JsonException.class, () -> MiniJson.parse("{\n  \"a\": [1, 2,\n}"));
		assertTrue(ex.getMessage().contains("line 3"), ex.getMessage());
		assertThrows(MiniJson.JsonException.class, () -> MiniJson.parse("{\"a\": 1} x"));
		assertThrows(MiniJson.JsonException.class, () -> MiniJson.parse("\"unterminated"));
	}

	private static RelocInfo sample() {
		RelocInfo r = new RelocInfo();
		r.module = "io";
		r.source = "resource:system/io.asm";
		r.defines = List.of("--DEBUG", "-DX=1");
		r.codeSize = 812;
		r.litSize = 3;
		r.dataSize = 4;
		r.requires.add(new RelocInfo.Require(true, "system/thread.asm"));
		r.requires.add(new RelocInfo.Require(false, "/abs/dir/util.asm"));
		r.inputs.add("resource:system/io.asm");
		r.fingerprint = "abc123";
		r.imports.add("PUTS");
		r.linkerSymbols.add("__CODE_END__");
		r.symbols.add(new RelocInfo.Symbol("PUTLINE", RelocInfo.Section.CODE, 120, true, null));
		r.symbols.add(new RelocInfo.Symbol("__PORT_MAP_HANDLE", RelocInfo.Section.DATA, 0, true, "INT"));
		r.symbols.add(new RelocInfo.Symbol("_PUTLINE$LOOP", RelocInfo.Section.CODE, 130, false, null));
		r.relocs.add(new RelocInfo.Reloc(17, RelocInfo.Field.IMM42, "PUTS", null, 0, 0));
		r.relocs.add(new RelocInfo.Reloc(40, RelocInfo.Field.IMM28, null, RelocInfo.Section.DATA, 0, 1));
		r.relocs.add(new RelocInfo.Reloc(41, RelocInfo.Field.IMM56, "__CODE_END__", null, 0, -3));
		r.reverse.put(120L, "PUTLINE");
		r.srcmap.put(17L, "\u00abio.asm\u00bb:34");
		return r;
	}

	@Test
	void relRoundTripThroughJsonAndFile(@TempDir Path tmp) throws Exception {
		RelocInfo r = sample();
		Path f = tmp.resolve("io.rel");
		r.write(f);
		String text = Files.readString(f);
		assertTrue(text.contains("\"format\": 1"));
		RelocInfo back = RelocInfo.read(f);
		assertEquals(MiniJson.write(r.toJson()), MiniJson.write(back.toJson()), "re-read .rel is identical");
		assertEquals(r.requires, back.requires);
		assertEquals(r.symbols, back.symbols);
		assertEquals(r.relocs, back.relocs);
		assertEquals(r.srcmap, back.srcmap);
		assertEquals("abc123", back.fingerprint);
		assertEquals(2, back.exports().size());
		assertEquals(819, back.imageSize());
		assertEquals(RelocInfo.Section.LIT, back.sectionOf(813));
		assertEquals(1, back.sectionOffset(813));
		assertEquals(RelocInfo.Section.DATA, back.sectionOf(815));
		assertEquals(0, back.sectionOffset(815));
	}

	@Test
	void rejectsUnknownFormat() {
		var json = sample().toJson();
		json.put("format", 99L);
		assertThrows(MiniJson.JsonException.class, () -> RelocInfo.fromJson(json));
	}

	@Test
	void fieldHelpers() {
		RelocInfo.Field f = RelocInfo.Field.IMM28;
		long word = 0xC123456789ABCDEFL;
		long w2 = f.put(word, -5);
		assertEquals(-5, f.get(w2));
		assertEquals(word & ~f.mask(), w2 & ~f.mask(), "other bits are kept");
		assertTrue(f.fits((1L << 27) - 1));
		assertFalse(f.fits(1L << 27));
		assertEquals(RelocInfo.Field.IMM56, RelocInfo.Field.forType(1));
		assertNull(RelocInfo.Field.forType(0));
	}
}
