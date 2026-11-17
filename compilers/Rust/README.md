# Rust translator for CPUSim64 (`compilers/Rust`)

Runs Rust programs (Rust 2021 subset) in the CPUSim64 IDE (File → New Compiler → Rust).
The translator `rustc` is written in portable C and compiled by the CPUSim64 C11
compiler (`rustc.asm`). It translates `prog.rs` to `prog.gen.c` with `#line`
directives that name `prog.rs`; the IDE then runs the C11 compiler on
`prog.gen.c` (`translate_to: C11` in `config.properties`) and uses the result as
`prog.asm`, so the debugger shows the Rust lines. The design follows `compilers/C++14`.
Errors are reported as `file:LINE: error: message` on stderr, exit status 1.

Pipeline (all in `src/`, one translation unit, `rustc.c` includes the others):
lexer, parser (`parse_*.c`), macro expansion (`expand.c`, also `macro_rules!`),
name resolution (`resolve.c`), item collection (`collect.c`), type check with trait
resolution and inference (`check.c`, `types.c`), move and mutability check
(`moves.c`), C generation (`cg_types.c`, `cg_expr.c`), with the runtime
`lib/rust_rt.c`. The standard library is Rust source in `lib/` (`core.rs`,
`collections.rs`, `rc.rs`), parsed as the crate `core` before every program;
`std` and `alloc` are aliases of it. See `plan.md`, `design-expand.md`,
`design-codegen.md`, `design-strings.md`, `lib/README.md`.

## Language

- Items: functions, structs (named, tuple, unit), enums with data, `impl`
  blocks (inherent and trait), traits (default methods, associated types and
  constants, supertraits, static dispatch and `dyn Trait` through vtables),
  generics by monomorphization with trait bounds and `where` clauses, modules in
  one file with `use`, `const`, `static`, type aliases, operator overloading
  through the `ops` traits, `Drop` with drop order.
- Expressions and statements: `let` with patterns and `else`, `if` / `if let` /
  `while let`, `match` (guards, ranges, bindings, nested patterns, slice patterns),
  `loop` / `while` / `for` with labels, ranges, closures (`Fn`, `FnMut`,
  `FnOnce`, boxed and returned), references and slices, `?` on `Result` and
  `Option` (with `From` conversion), casts with `as`, tuples, arrays.
- Derives: `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`, `PartialOrd`, `Ord`,
  `Default`, `Hash`.
- Macros: `println!`, `print!`, `eprintln!`, `eprint!`, `format!`, `write!`,
  `writeln!`, `panic!`, `assert!`, `assert_eq!`, `assert_ne!`, `debug_assert*!`,
  `unreachable!`, `todo!`, `unimplemented!`, `vec!`, `matches!`, `dbg!`,
  `line!`, `column!`, `file!`, `stringify!`, `concat!`, and user `macro_rules!`
  (fragments, repetitions, recursion, macros in item position).
- Format strings: positional, named and inline (`{x}`) arguments, width,
  precision, fill and alignment, sign, `#`, `0`, `{:?}`, `{:#?}`, `{:x}` `{:X}`
  `{:o}` `{:b}` `{:e}` `{:E}`, `{:width$}` and `{:.*}`.

## Library (Rust source in `lib/`)

- `lib/core.rs`: `Option`, `Result`, `Box`, `String` and `&str` (UTF-8, chars,
  split, find, trim, parse, replace, ...), `Vec`, slices (sort, binary search,
  windows, chunks, ...), iterators and all the usual adapters and consumers,
  `Ordering`, `Reverse`, `Default`, `From` / `Into`, `ToString`, `fmt`
  (`Display`, `Debug`, `Formatter`, `debug_struct` ...), `io` (`stdin`,
  `read_line`, `lines`, `BufRead`, `Write`, `BufWriter`), `process::exit`,
  `mem::{swap, replace, take}`, integer and `f64` methods (`checked_*`,
  `wrapping_*`, `saturating_*`, `pow`, `sqrt`, `sin` ...), `char` methods,
  `Error` and `Box<dyn Error>`.
- `lib/collections.rs`: `HashMap`, `HashSet`, `BTreeMap`, `BTreeSet`, `VecDeque`,
  `BinaryHeap`, `Entry` API, `Hash` and `Hasher`.
- `lib/rc.rs`: `Rc`, `Weak`, `Cell`, `RefCell` (runtime borrow checks), `Arc` and
  `Mutex` (single-threaded stand-ins).

## Representation

Every scalar is one 64-bit word (`i32`, `bool`, `char` as a codepoint, ...).
`u8` is packed (8 per word, big-endian) in strings, arrays of `u8` and
`Vec<u8>`, accessed with the `LDB` / `STB` byte instructions. `&str` is a pair
(byte address, byte length) and `&[T]` a pair (base, length). `size_of` counts
8 bytes per word.

## Panics and checks

Panics follow rustc's debug build: integer overflow (add, sub, mul, neg, casts
excepted), division by zero, index out of bounds, `unwrap` / `expect` failures,
`assert*!` and `panic!` print `thread 'main' panicked at file:line:col:` and the
message to stderr and exit with status 101 (no unwinding, no destructors).
The checker verifies types, trait bounds, moves (use after move, partial moves)
and mutability. There is no lifetime or borrow checker.

## Debug dumps

`rustc --tokens prog.rs`, `--ast`, `--expand` (after macro expansion), `--resolve`
(with resolved paths), `--types` (with inferred types).

## Known gaps

- `impl Trait` only in argument and return position (not in `let` bindings, struct fields, aliases or trait method return types), as in rustc.
- No `i128` / `u128`; `f32` is an `f64`.
- No `async` / `await`, no procedural macros, no inline `unsafe` asm.
- Lifetimes are parsed but not checked: some programs `rustc` rejects are accepted.
- No threads: `Arc` is `Rc` and `Mutex` has no real locking; `std::thread` is missing.
- `env::args()` yields only the program name.
- `HashMap` / `HashSet` have no hasher parameter; iteration order is deterministic but unspecified.
- `Rc` works only for sized types (no `Rc<dyn Trait>`, `Rc<str>`).
- `f64` transcendental functions are a few ulp away from libm.
- `to_uppercase` / `to_lowercase` are ASCII only.

## Build and test

```
./build.sh            # rustc.asm, compiled by the C11 compiler
./build.sh --native   # obj/rustc-native, built with the host cc (development)
tests/run_tests.sh [filter]   # translator on the simulator, C11 compiler, simulator
HOST=1 tests/run_tests.sh     # native translator, generated C run by the host cc
```

`tests/run_tests.sh` has the groups `lex`, `parse`, `expand`, `resolve`, `check`
(golden dumps), `errors` (programs that must be rejected, with the message),
and `run`: 78 programs in `tests/run/` (tiers `t1_` to `t5_`, `s5_` collections and `Rc`, `p_` panics,
`x_` exit and stdin) whose stdout, exit status and panic message equal the output of
rustc 1.97 (`tests/gen_expected.sh` regenerates the `.expected` files with a host
rustc). See `tests/run/README.md`.

`CPUSIM64_HOME=/path/to/install` makes `build.sh` and the tests use that install
(`run.sh`, `assemble.sh`, `bin/cpusim64_native`) instead of the repository root.
Use it when the checked-in `lib/cpusim64-*.jar` is older than the sources.

Layout: `src/` translator, `lib/` library and runtime, `grammar/` ANTLR grammars
the parser follows, `tests/`, `obj/` build output, `rustc.asm` the translator
compiled by the C11 compiler (committed because the IDE assembles it).
