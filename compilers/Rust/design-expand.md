# Built-in macro lowering (expand.c) and the core library it targets

expand.c replaces every built-in macro call by ordinary AST built from the
tokens below (it may construct the nodes directly or build tokens and use
reparse_expr / reparse_stmts). lib/core.rs must define every item named here
with exactly these paths and signatures. All paths are absolute (`::core::`)
so user names cannot capture them. Names starting with `__` are reserved for
expansion temporaries.

## Format strings

A format string is a string literal (raw strings allowed). Grammar, as in
std::fmt: `{{` and `}}` are literal braces; `{[arg][:spec]}` with arg = empty
(next positional), an integer (positional), or an identifier (a named argument
`name = expr`, or else a captured variable `name` in scope). spec =
`[[fill]align][sign]['#']['0'][width]['.' precision][type]`, align `<` `^` `>`,
sign `+` `-`, width/precision = integer, `name$`, `N$`, precision also `*`
(takes the next positional argument as usize, then the value). type = empty
(Display), `?` (Debug), `x` `X` `o` `b` `e` `E` (LowerHex, UpperHex, Octal,
Binary, LowerExp, UpperExp). `{:#?}` is pretty Debug (the alt flag). Errors
(unclosed `{`, unknown argument, unused argument, bad spec) are reported at
the macro's line with rustc's wording, e.g. `invalid format string: expected
`}`, found `x``, `argument never used`, `there is no argument named `x``.

Lowering of the formatting part, given a destination formatter variable
`__f`:

```rust
{
    let __a0 = &(EXPR0);          // every argument once, left to right
    let __a1 = &(EXPR1);
    __f.write_str("literal piece");                       // non-empty pieces only
    __f.set_spec(FILL, ALIGN, SIGN, ALT, ZERO, WIDTH, HAS_WIDTH, PREC, HAS_PREC);
    let _ = ::core::fmt::Display::fmt(__a0, &mut __f);    // trait by type letter
    __f.clear_spec();                                     // only if set_spec was used
}
```

FILL is a char literal (default `' '`), ALIGN is `0u8` unknown, `1u8` left,
`2u8` center, `3u8` right; SIGN `0u8` none, `1u8` plus, `2u8` minus; ALT and
ZERO are bool literals; WIDTH and PREC are usize expressions (`0usize` when
absent; `*__aN` when taken from an argument). set_spec is emitted only when
the spec has anything besides the trait letter.

## The macros

| Macro | Expansion |
|---|---|
| `print!(fmt, args..)` | `{ let mut __out = ::core::io::stdout_raw(); let mut __f = ::core::fmt::Formatter::new(&mut __out); FORMAT; }` |
| `println!(..)` | same with `"\n"` appended to the last piece; `println!()` writes `"\n"` |
| `eprint!` / `eprintln!` | same with `::core::io::stderr_raw()` |
| `format!(..)` | `{ let mut __s = ::core::string::String::new(); { let mut __f = ::core::fmt::Formatter::new(&mut __s); FORMAT; } __s }` |
| `write!(dst, ..)` | `{ let mut __f = ::core::fmt::Formatter::new(&mut *(dst)); FORMAT; ::core::result::Result::Ok(()) }` (type `::core::fmt::Result`) |
| `writeln!(dst, ..)` | same with `"\n"` appended; `writeln!(dst)` writes `"\n"` |
| `panic!()` | `::core::panicking::panic_str(FILE, LINE, COL, "explicit panic")` |
| `panic!("lit")` without args | `::core::panicking::panic_str(FILE, LINE, COL, "lit")` (braces still processed: `{{` -> `{`) |
| `panic!(fmt, args..)` | `{ let mut __out = ::core::panicking::begin(FILE, LINE, COL); let mut __f = ::core::fmt::Formatter::new(&mut __out); FORMAT; ::core::panicking::end() }` |
| `assert!(c)` | `if !(c) { ::core::panicking::panic_str(FILE, LINE, COL, "assertion failed: SRC") }` where SRC is the token text of c, rustc-style spacing |
| `assert!(c, fmt, args..)` | `if !(c) { panic!(fmt, args..) }` |
| `assert_eq!(a, b)` / `assert_ne!` | `match (&(a), &(b)) { (__l, __r) => if !(*__l == *__r) { ::core::panicking::assert_failed("==", &*__l, &*__r, FILE, LINE, COL) } }` (`!=` for assert_ne; with a message: `assert_failed_msg(op, l, r, FILE, LINE, COL, &format!(..))`) |
| `debug_assert*!` | same as the non-debug forms |
| `unreachable!()` / with fmt | panic with `"internal error: entered unreachable code"` / `"internal error: entered unreachable code: " + fmt` |
| `todo!()` / `unimplemented!()` | panic with `"not yet implemented"` / `"not implemented"` (with fmt: `"not yet implemented: .."`) |
| `vec![]` | `::core::vec::Vec::new()` |
| `vec![a, b, c]` | `::core::vec::Vec::from_array([a, b, c])` |
| `vec![x; n]` | `::core::vec::from_elem(x, n)` |
| `matches!(e, pat)` / `matches!(e, pat if g)` | `match e { pat => true, _ => false }` / with the guard |
| `line!()` `column!()` `file!()` | `LINE` (u32 literal), `COL` (u32), `FILE` (&str literal of the source file name as given to rustc) |
| `stringify!(tt..)` | a string literal of the tokens |
| `concat!(lit, ..)` | one string literal |
| `dbg!(e)` | not supported: error `dbg! is not supported` |

FILE, LINE, COL are the call site (`&'static str`, `u32`, `u32`). Any other
`name!(..)` is looked up as a macro_rules! macro (by textual order and
`#[macro_export]`/module scoping, simplified: macros are visible after their
definition in the same file and in nested modules); unknown macros give
`cannot find macro `name` in this scope`.

## macro_rules!

Matchers support literal tokens, `$name:frag` with fragments `expr ident ty
pat pat_param path stmt block item literal tt lifetime vis meta`, and
repetitions `$( ... ) sep? *|+|?`. Rules are tried in order; the first match
wins; no match gives `no rules expected the token `x``. Transcription
substitutes fragments (an `expr` fragment is inserted as one parenthesized
unit so precedence is kept) and repetitions. Expansion is recursive with a
depth limit of 128 (`recursion limit reached while expanding `name!``).
The expansion of a call is re-parsed in the call's position (expression,
statement list, item list, pattern, type) with reparse_*.

## Items lib/core.rs provides for the expansions

```rust
pub mod fmt {
    pub type Result = ::core::result::Result<(), Error>;
    pub struct Error;
    pub trait Write { fn write_str(&mut self, s: &str) -> Result; fn write_char(&mut self, c: char) -> Result { .. } }
    pub struct Formatter<'a> { out: &'a mut dyn Write, fill: char, align: u8, sign: u8, alt: bool, zero: bool, width: usize, has_width: bool, prec: usize, has_prec: bool }
    impl<'a> Formatter<'a> {
        pub fn new(out: &'a mut dyn Write) -> Formatter<'a>;
        pub fn write_str(&mut self, s: &str) -> Result;
        pub fn set_spec(&mut self, fill: char, align: u8, sign: u8, alt: bool, zero: bool, width: usize, has_width: bool, prec: usize, has_prec: bool);
        pub fn clear_spec(&mut self);
        pub fn pad(&mut self, s: &str) -> Result;                       // applies width/align/prec to a string
        pub fn pad_integral(&mut self, nonneg: bool, prefix: &str, digits: &str) -> Result;
        pub fn alternate(&self) -> bool; pub fn width(&self) -> Option<usize>; pub fn precision(&self) -> Option<usize>;
        pub fn debug_struct(..), debug_tuple(..), debug_list(..)          // Stage 5
    }
    impl<'a> Write for Formatter<'a> { .. }
    pub trait Display { fn fmt(&self, f: &mut Formatter<'_>) -> Result; }
    pub trait Debug   { fn fmt(&self, f: &mut Formatter<'_>) -> Result; }
    pub trait LowerHex, UpperHex, Octal, Binary, LowerExp, UpperExp     // same shape
}
pub mod io { pub struct RawOut { port: i64 } impl fmt::Write for RawOut { .. }
             pub fn stdout_raw() -> RawOut; pub fn stderr_raw() -> RawOut; }
pub mod panicking {
    pub fn panic_str(file: &str, line: u32, col: u32, msg: &str) -> !;
    pub fn begin(file: &str, line: u32, col: u32) -> io::RawOut;       // prints "thread 'main' panicked at FILE:LINE:COL:\n" to stderr
    pub fn end() -> !;                                                // prints "\nnote: run with `RUST_BACKTRACE=1` environment variable to display a backtrace\n", exits 101
    pub fn assert_failed<T: Debug + ?Sized, U: Debug + ?Sized>(op: &str, l: &T, r: &U, file: &str, line: u32, col: u32) -> !;
}
pub mod string { pub struct String { .. } impl fmt::Write for String { .. } impl String { pub fn new() -> String; .. } }
pub mod vec { pub struct Vec<T> { .. } impl<T> Vec<T> { pub fn new() -> Vec<T>; pub fn from_array<const N: usize>(a: [T; N]) -> Vec<T>; .. }
              pub fn from_elem<T: Clone>(x: T, n: usize) -> Vec<T>; }
```

Panic output matches rustc 1.7x:

```
thread 'main' panicked at src/main.rs:4:5:
explicit panic
note: run with `RUST_BACKTRACE=1` environment variable to display a backtrace
```

(FILE is the path as passed to rustc; the tests use the relative name.)
