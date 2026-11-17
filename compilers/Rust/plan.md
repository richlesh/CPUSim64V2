# Implementation Plan: Rust compiler plug-in (compilers/Rust)

## Goal

A plug-in that runs Rust 2021 programs in the IDE and on both simulators.  Like
`compilers/C++14`, it is a translator written in portable C, compiled by our C11
compiler.  It converts `prog.rs` to `prog.gen.c` (with `#line` directives) and
the C11 compiler produces the assembly (`translate_to: C11`).

## Design decisions

- Output is C11 that the CPUSim64 C11 compiler accepts: no float, bit-fields,
  VLAs, `_Generic` or `_Atomic`; every scalar is one word.  Code that must also
  build with the C11 compiler fills non-zero values with loops, not `memset`.
- The translator is one translation unit (`src/rustc.c` includes the others)
  and builds natively with `build.sh --native` for development.
- Errors: `file:line: error: message` on stderr, exit status 1.
- Generics are monomorphized.  `panic!` exits with status 101 (no unwinding).
- The standard library is Rust source (`lib/core.rs`, `collections.rs`,
  `rc.rs`) over compiler intrinsics and a small C runtime (`lib/rust_rt.c`).
- Strings: `u8` is packed 8 per word (big-endian) and accessed with LDB/STB;
  `&str` is (byte address, byte length).  See `design-strings.md`.

## Status

0. **LDB/STB**: done.  Byte load/store instructions in the simulators and
   assembler, used by the C11 compiler for `char` data.
1. **Scaffold and string design**: done.  Plug-in layout, `build.sh`, tests,
   string and byte representation (`design-strings.md`).
2. **Lexer and parser**: done.  The full Rust 2021 token set and syntax, AST
   dump (`--tokens`, `--ast`), checked against the ANTLR grammars in `grammar/`.
3. **Macro expansion, name resolution and type check**: done.  Built-in macros,
   `macro_rules!`, derives (`design-expand.md`); modules and `use`; type
   inference with trait bounds, impl selection, method lookup with auto-ref and
   deref; move and mutability checks.  Dumps `--expand`, `--resolve`, `--types`.
4. **Lowering and C code generation**: done.  Monomorphization, enum layout and
   `match`, closures, trait objects (vtables), drop elaboration, `#line`
   (`design-codegen.md`).
5. **Standard library in Rust** (`lib/`): done.  Core traits, `Option` /
   `Result`, `Vec`, `String`, `Box`, iterators, formatting, `HashMap`,
   `HashSet`, `BTreeMap`, `BTreeSet`, `VecDeque`, `BinaryHeap`, `Rc`, `RefCell`,
   `Cell`, single-threaded `Arc` / `Mutex`.  No `std::thread`.
6. **Tests, projects and IDE** (in progress): regression tests are in place
   (`tests/run_tests.sh`: lex, parse, expand, resolve, check, errors and 78 run
   programs compared with rustc 1.97).  Under way: documentation (README,
   `Compilers.html`, system prompt), sample projects, the IDE entry and the
   installer.
7. **Verification** (pending): differential runs against a host `rustc` on
   larger programs, the full test suite on both simulators, measurements of
   translation time.

## Known gaps

- `impl Trait` (argument and return position): not supported yet, being
  fixed, so this limitation may be lifted.
- No `i128` / `u128`; `f32` is an `f64`.
- No `async` / `await`, procedural macros or inline `unsafe` asm.
- No lifetime or borrow checking (moves and mutability are checked).
- No threads; `env::args()` gives only the program name.
- `HashMap` / `HashSet` have no hasher parameter; `Rc` only for sized types.
- `f64` transcendental functions are a few ulp from libm.

## Registration (outside this folder)

The IDE discovers plug-ins by scanning the `compilers/` folder, so no code
change is needed.  The packaging workflows copy `compilers/` as is.  The top
level `README.md` and `Compilers.html` describe this plug-in.
