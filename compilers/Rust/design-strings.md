# Rust strings and bytes on CPUSim64: design notes

Status: Stage 1 decision record. Facts below come from the documentation in
`cpusim64/src/main/resources/documentation/` and `compilers/C11/README.md`.
Anything not read in source is marked UNVERIFIED.

## 1. What the machine provides

- Memory is word addressed (8-byte words). `LDB` (opcode 39) and `STB`
  (opcode 40) address bytes: byte address = word address * 8 + index, index 0
  is the most significant byte (Instruction_Set_Reference.html, "Load Byte").
  `STB` is an atomic read-modify-write. Negative (shared memory) byte
  addresses are not supported.
- A CPUSim64 string is a length word (UTF-8 byte count) followed by the UTF-8
  bytes packed 8 per word, big-endian (`.dcs`; Programmer_Guide.html, strings).
- `OUT` and `IN` take a size from `<system/io.def>`: `CHAR` (one UTF-8 encoded
  codepoint, variable length) or `BYTE` and wider (1 to 8 bytes)
  (Instruction_Set_Reference.html, I/O sizes). A byte buffer can therefore be
  written with one `OUT` of size `BYTE` per byte, using `ldb` to fetch each one.
  Writing 8 bytes per `OUT` needs a word-aligned source (a slice is not
  aligned in general). UNVERIFIED: exact `OUT` operand forms for a register
  source with a byte size; read Instruction_Set_Reference.html lines 510 to 570.
- Interrupts that take a CPUSim64 string: `iPUTS` (205), `iPUT_LINE` (206),
  `iGETS` (211), `iGET_LINE` (212), file open calls (220 to 225),
  `iSUBSTRING`, `iPREFIX`. `iPUTS` needs a length word, so it prints a whole
  string only. A `&str` slice has no length word in front of it.
- Raw files (`iOPEN_RAW_FILE_*`, 223 to 225) exist for byte I/O. UNVERIFIED:
  whether `OUT`/`IN` of size `BYTE` on a text port transforms line endings.

## 2. What the C11 compiler provides

- C strings, `char *` and string literals are one codepoint per word (UTF-32),
  zero terminated. The C library works on these. Conversion to CPUSim64 strings
  happens in a few assembly routines at the OS boundary: file names, formatted
  output blocks and `argv` (C11 README, lines 236 to 250).
- Console and files are read and written one codepoint at a time (`CHAR`
  size), so text files are UTF-8 on disk and UTF-32 in memory.
- `__ldb(byte_addr)` and `__stb(byte_addr, v)` compile to one instruction. A C
  pointer is a word address, so the byte address of pointer `p` is
  `(long)p * 8`.

Consequence: generated Rust code cannot use C string literals as `&str`
(those are UTF-32). Rust needs its own literal representation.

## 3. Decisions

### 3.1 Representation

| Rust | Representation in generated C |
|---|---|
| `&str` | two words: byte address, byte length |
| `&[u8]` | same pair |
| `&[T]` (T not u8) | two words: word address, element count |
| `String` | three words: byte address of buffer, capacity, length |
| `Vec<u8>` | same three words as `String` |
| `Vec<T>` (T not u8) | three words: word address, capacity, length |
| `Box<T>` | word address of a heap block |
| `char` | one word holding the Unicode scalar value |
| `bool`, `u8`..`i64`, `u64`, `usize`, `isize` | one word each |
| `f64`, `f32` | `double`; `f32` is rounded to `f64` precision only |
| `i128`, `u128` | not supported |
| enums | tag word plus payload words (tagged union) |
| `Option<&T>`, `Option<Box<T>>` | niche: 0 is `None` |

`u8` stored alone (a local, a struct field) takes one word. Only arrays,
slices, `Vec<u8>` and strings are byte packed, because those are the only
places the byte instructions pay off. `[u8; N]` is packed.

### 3.2 Literals

A string literal becomes a packed `.dcs`-style block in the data segment and
the translator emits the pair (byte address, length) directly. The byte
address of the data is `(label + 1) * 8` because the length word comes first.
UNVERIFIED: how to get a `.dcs` block from C source. The C11 compiler has a
`__pstr` type that is "still accepted" (README line 252); the plan is to read
`compilers/C11` codegen for `__pstr` and, if needed, add a small intrinsic
`__strlit("...")` yielding the byte address (C11 change, a few lines).

### 3.3 Output and input

Runtime helpers written in C in `rt/` (UNVERIFIED names, to be decided in
Stage 4):

- `rs_write(port, byte_addr, len)`: loop of `__ldb` and `OUT` size `BYTE`.
- `rs_read_line(port, String *buf)`: loop of `IN` size `BYTE`.
- `rs_to_cstr` and `rs_from_cstr`: bridges to the UTF-32 C library, used only
  for file names and `printf`-style numeric formatting.
- Integer and float formatting is done in Rust or C into a byte buffer, not by
  the C library, so it never needs the bridge.

An efficient bulk write (8 bytes per `OUT`) needs aligned words, so the helper
copies the unaligned head and tail bytewise. This is an optimization for
later.

### 3.4 Heap

`malloc`, `realloc` and `free` come from the C11 library on top of the
simulator allocator. UNVERIFIED: the allocation interrupt numbers and whether
`realloc` preserves contents (the C library must provide that behavior, so the
Rust runtime can rely on `realloc`). Growth policy for `String` and `Vec` is
doubling, as in Rust.

### 3.5 Arithmetic

- `i64`, `u64`, `usize`: one word. Overflow checks on `+`, `-`, `*` are done
  with comparisons in generated C (no hardware trap), and panic with exit code
  101 and Rust's message text.
- `u8`, `u16`, `u32`, `i8`, `i16`, `i32`: one word, with a range check after
  each operation. `as` casts truncate and sign-extend explicitly.
- Unsigned compares use the sign-bit xor trick the C11 compiler already emits.

## 4. Risks and open questions

1. `&str` slices cross word boundaries, so every byte access is `__ldb` or
   `__stb` (about the cost of a `LOAD`). Code that only needs words is
   unaffected.
2. No negative byte addresses: shared memory (the `fork` shared region) cannot
   hold `String` data. Threads use the ordinary heap and are unaffected.
3. The UTF-32 C library is unused by the Rust runtime for text, so
   `{}` formatting and `{:?}` need their own implementation in `lib/`.
4. UNVERIFIED items above must be settled before Stage 4 (literals, `OUT`
   byte forms, raw file behavior).
5. Per-element `u8` as a struct field takes a whole word; `size_of` will not
   match real Rust. Documented as a limitation.
