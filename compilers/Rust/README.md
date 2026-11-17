# Rust translator for CPUSim64 (`compilers/Rust`)

Runs Rust programs in the CPUSim64 IDE (File → New Compiler → Rust).
The translator `rustc` is written in portable C and compiled by the CPUSim64 C11
compiler; it translates `prog.rs` to `prog.gen.c`, with `#line` directives that
name `prog.rs`.  The IDE then runs the C11 compiler on `prog.gen.c`
(`translate_to: C11` in `config.properties`) and uses the result as `prog.asm`,
so the debugger shows the Rust lines.  The design follows `compilers/C++14`.

See `plan.md` for the stages.

## Status

Stages 1 (scaffold) and 2 (lexer and parser) are done.  The whole Rust
syntax is lexed and parsed into an AST (`rustc --tokens` and `rustc --ast`
print them), and syntax errors are reported as `file:LINE: error: message`.
Code generation still accepts only

```rust
fn main() {
    println!("a string literal");
}
```

with any number of `println!` statements.  Each literal may use the escapes
`\n \r \t \\ \" \'` and may contain any Unicode text but no `{` or `}` (format
placeholders are not implemented).  Line and nested block comments are skipped.
Everything else gives `file:LINE: error: not supported yet`.

Layout: `src/rustc.c` (main, the println! code generator), `src/lexer.c`
(`token.h`), `src/ast.c` (`ast.h`, `nodes.inc`), the parser `src/parse_*.c`
(`parser.h`), `src/util.c`, `src/rustc.h`.  `rustc.asm` is the translator compiled by the C11 compiler, and
is committed because the IDE assembles it.

## Planned subset (Rust 2021)

- Items: functions, structs (named, tuple, unit), enums with data, `impl`
  blocks, traits (static dispatch and `dyn Trait`), generics by
  monomorphization, modules in one file, `const`, `static`, type aliases.
- Expressions: `let` (with patterns), `if` / `if let` / `while let`, `match`,
  `loop` / `while` / `for`, ranges, closures, references and slices, `?`,
  operators with overloading through traits.
- Library (written in Rust, in `lib/`): `Vec`, `String` and `&str`, `Option`,
  `Result`, `Box`, `Rc`, `HashMap`, iterators and adapters, `println!` /
  `format!` / `write!` with the usual format specs, `std::thread`.
- Checks: types, moves, and mutability.

Not supported: `i128` / `u128`, `f32` as a separate type (it is an `f64`),
`async`, procedural macros, inline `unsafe` asm, full borrow checking (moves and
mutability are checked, lifetimes are not), and unwinding (`panic!` prints a
message and exits with status 101).

## Build and test

```
./build.sh            # rustc.asm, compiled by the C11 compiler
./build.sh --native   # obj/rustc-native, built with the host cc (development)
tests/run_tests.sh    # translator on the simulator, C11 compiler, simulator
HOST=1 tests/run_tests.sh   # native translator, generated C run by the host cc
```

`CPUSIM64_HOME=/path/to/install` makes `build.sh` and the tests use that install
(`run.sh`, `assemble.sh`, `bin/cpusim64_native`) instead of the repository root.
Use it when the checked-in `lib/cpusim64-*.jar` is older than the sources.
