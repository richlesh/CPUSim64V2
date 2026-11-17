# Implementation Plan: Rust compiler plug-in (compilers/Rust)

## Goal

A plug-in that runs Rust 2021 programs in the IDE and on both simulators.  Like
`compilers/C++14`, it is a translator written in portable C, compiled by our C11
compiler.  It converts `prog.rs` to `prog.gen.c` (with `#line` directives) and
the C11 compiler produces the assembly (`translate_to: C11`).

## Design decisions so far

- Output is C11 that the CPUSim64 C11 compiler accepts: no float, bit-fields,
  VLAs, `_Generic` or `_Atomic`; every char is a word.
- The translator is one translation unit (`src/rustc.c` includes the others)
  and builds natively with `build.sh --native` for development.
- Errors: `file:line: error: message` on stderr, exit status 1.
- Generics are monomorphized.  `panic!` exits with status 101 (no unwinding).
- Strings: to be designed in stage 1 (see below).

## Stages

0. **LDB/STB** (done): byte load/store instructions in the simulators and
   assembler, used by the C11 compiler for `char` data.
1. **Scaffold and string design** (scaffold done): plug-in layout, `build.sh`,
   tests, `println!("literal")` end to end.  Still open: the representation of
   `&str` / `String` (one word per char like C, or packed bytes using LDB/STB;
   UTF-8 semantics for `len()` and slicing) and the format of generated names.
2. **Lexer and parser**: the full Rust 2021 token set (raw strings, char
   literals, lifetimes, number suffixes), items, expressions, patterns, types,
   attributes, and `macro_rules!` for the library macros.  AST dump option.
3. **Name resolution and type check**: modules and `use`, type inference
   (Hindley-Milner style with trait bounds), trait resolution, method lookup
   with auto-ref/deref, move and mutability checks.
4. **Lowering and C codegen**: monomorphization, enum layout and `match`
   decision trees, closures, trait objects (vtables), drop elaboration,
   `#line` for the debugger.
5. **Standard library in Rust** (`lib/`): core traits, `Option` / `Result`,
   `Vec`, `String`, `Box`, `Rc`, `HashMap`, iterators, formatting, threads.
6. **Tests, projects and IDE**: regression tests, sample projects, the
   registration in the IDE, installer and documentation.
7. **Verification**: differential runs against a host `rustc`, the full test
   suite on both simulators, measurements of translation time.

## Registration (outside this folder)

The IDE discovers plug-ins by scanning the `compilers/` folder, so no code
change is needed.  The packaging workflows copy `compilers/` as is.  The top
level `README.md` lists the plug-ins and should mention this one once it does
more than the first slice.
