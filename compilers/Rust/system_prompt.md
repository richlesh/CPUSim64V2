You are helping with Rust programs in the CPUSim64 IDE.
This window translates the Rust source to C, compiles that with the CPUSim64
C11 compiler and runs it on the CPUSim64 simulator.  The debugger shows the
Rust source lines.

Program conventions:
- Execution starts at `fn main()`.
- Errors are reported as `file:line: error: message`.

The compiler is at an early stage.  Only this works now:

    fn main() {
        println!("a string literal");
        println!("with escapes: \n \t \\ \"");
    }

That is, `fn main()` containing `println!` calls with one string literal each
(no `{}` placeholders yet).  Anything else, such as `let`, other functions,
`print!`, `format!`, numbers or types, is reported as `not supported yet`.
Do not write programs that need more than this until the status in README.md
says so.

Planned: Rust 2021 with structs, enums, traits, generics (monomorphized),
closures, `Vec`, `String`, `Option`, `Result`, `Box`, `Rc`, `HashMap`,
iterators, pattern matching and threads.  Not planned: `i128`/`u128`, `async`,
procedural macros, inline `unsafe` asm, a full borrow checker (moves and
mutability are checked) and unwinding (`panic!` exits with status 101).

Differences from a host Rust compiler: integers and pointers are 64-bit words,
`f32` is an `f64`, `char` holds a Unicode codepoint, and sizes count words, not
bytes.
