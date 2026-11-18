# Code generation: Rust to C for the CPUSim64 C11 compiler

Stage 4 contract. `src/cg.h` declares the functions; this file fixes the
representation and the conventions that every part of the code generator and
`lib/rust_rt.c` must agree on. The input is the checked AST (sema.h: node->ty,
node->sym, node->adj, check_callee(), closure_captures()); the output is one C
file `<base>.gen.c` compiled by compilers/C11.

## Facts about the target (verified on the simulator)

- Every scalar is one 64-bit word and `sizeof` counts words (`sizeof(long)` is 1).
- Pointers are word addresses; `(long)p` gives the word address. The byte
  address of the first byte of word `w` is `w * 8`; byte 0 is the most
  significant one. `__ldb(byte_addr)` and `__stb(byte_addr, v)` are single
  LDB/STB instructions.
- Structs and unions can be passed, returned and assigned by value; arrays inside
  structs too. Function pointers can be stored in a `long` and called through a cast.
- `malloc`/`realloc`/`free` count words; `realloc` keeps the contents.
- **Arguments are evaluated right to left.** Rust evaluates left to right, so
  any argument (or operand) whose evaluation has side effects or can panic is
  first stored in a temporary. The generated C never relies on C evaluation
  order: operands of a call, of a binary operator and of a struct literal are
  temporaries unless they are side-effect free (locals, constants, field reads of
  locals).
- C string literals hold one codepoint per word; they are only used for text
  passed to the C library (file names). Rust string data never uses them.
- `fputc`/`fgetc` on stdout/stdin/stderr move one codepoint (the port encodes
  UTF-8). The runtime converts between UTF-8 bytes and codepoints.

## Representation of Rust types in C

| Rust | C | Notes |
|---|---|---|
| `bool`, `char`, every integer type, `usize`, `isize` | `long` | narrow types hold a normalized value: `u8` 0..255, `i8` -128..127, `u32` 0..2^32-1, ...; `u64`/`usize` hold the bit pattern |
| `f64`, `f32` | `double` | `f32` is computed as `f64` |
| `()`, `!`, zero-sized structs and tuples, fn items, closures without captures | `long` (always 0) | a function returning `()` returns `long` 0 |
| `(A, B, ..)` | `struct Tup_N { A f0; B f1; .. }` | |
| `struct S { x: A, .. }` / tuple struct | `struct S_N { A x; ..}` / `{ A f0; .. }` | field names are the Rust names prefixed `_` (`_x`) to avoid C keywords |
| `enum E` | `struct E_N { long tag; union { struct E_N_v0 v0; .. } u; }` | `tag` = variant index (0, 1, ..); the explicit discriminant is only used by `as` casts; an enum without payloads has only `tag`; variant structs `struct E_N_vK { A f0; .. }` (named fields `_x`); an empty variant struct has `long _;` |
| `[T; N]` | `struct Arr_N { T e[N]; }` | `N == 0`: `{ long _; }` |
| `[u8; N]` | `struct Arr_N { long w[(N + 7) / 8]; }` | **packed**: byte `i` is at byte address `(long)&a.w[0] * 8 + i` |
| `&T`, `&mut T`, `*const T`, `*mut T`, `Box<T>` for sized `T` | `long` | the address: a word address, except when `T` is `u8`, where it is a **byte address**. The byte address of a `u8` that lives in a word (a local, a field, a temporary) is `(long)&x * 8 + 7` (the least significant byte) |
| `&[T]`, `&mut [T]`, `&str`, `Box<[T]>`, `Box<str>` | `struct rs_slice { long base; long len; }` | `base` is a byte address for `[u8]` and `str` (packed), else a word address; `len` counts elements (bytes for `str`) |
| `&dyn Tr`, `&mut dyn Tr`, `Box<dyn Tr>` | `struct rs_dyn { long data; long vt; }` | `data` as the thin reference to the concrete type; `vt` the address of its vtable |
| fn pointer `fn(A) -> R` | `long` | the C function's address |
| closure with captures | `struct Clo_N { .. }` | one field per capture: by reference a `long` address (or `rs_slice`/`rs_dyn` for unsized), by move the value |
| `Vec<T>`, `String`, `Option<T>`, ... | as declared in `lib/core.rs` | no special cases except `Box` (lang item `owned_box`), which is represented as the reference to its pointee (the `ptr` field is ignored) |

Loads and stores through a reference: `*(T *)(r)` for word pointees; `__ldb(r)` /
`__stb(r, v)` for `u8`. Indexing a packed `[u8]` / `str`: `__ldb(base + i)`.

No niche optimization: `Option<&T>` is an ordinary enum (tag + payload).
`size_of::<T>()` is `sizeof(C type of T) * 8`.

Generic types and functions are monomorphized: every distinct instantiation
gets its own C struct and C function. A C type or function is emitted once,
the first time it is needed; types are emitted in dependency order (a struct
that contains another by value comes after it; references are `long`, so
cycles through references need no forward declarations).

## Names

Every emitted C name is unique and starts with a short kind prefix plus a
number; a comment above each definition gives the Rust name:

- types: `S12` (struct/enum/tuple/array/closure), variant structs `S12_v0`
- functions: `F34`, drop glue `D12` (for type S12), vtables `VT5`, statics `G7`
- string literals: `L3` (`static const long L3[] = { packed words };`), used as
  the slice `{ (long)L3 * 8, len }`
- locals: `l_<rustname>_<n>` (the Rust name with hygiene suffixes removed),
  temporaries `t<n>`
- runtime: `rs_*` (lib/rust_rt.c)

## Functions

Every Rust fn instance becomes `static R F<n>(P0 a0, P1 a1, ..)`. Methods take
`self` first as declared (`&self` is a `long`). Unit results return `long`
0. `main` is called by the C `main`, which first runs the static initializers;
`fn main() -> Result<(), E>` prints `Error: {:?}` of the error to stderr and
exits with 1.

Trait method calls are resolved at monomorphization time: with the
instantiated Self type, `find_impl()` gives the impl and `assoc_item()` the
method (or the trait's default method). Calls through `dyn Tr` load the method
from the vtable. A vtable is `static long VT<n>[] = { (long)D<k> or 0,
size_in_words, (long)method0, (long)method1, .. }` with the methods in the
trait's declaration order (supertrait methods after them, in order); a method
in a vtable takes the receiver as the thin `long` data pointer.

Closures: the closure expression builds a `struct Clo_N`; its body is
`static R F<n>(long env, A0 a0, ..)` where `env` is the address of the closure
value. `Fn::call`, `FnMut::call_mut`, `FnOnce::call_once` on a closure type, a
fn item or a fn pointer become direct calls; through `dyn Fn` they go through
the vtable like any other trait.

## Statements and expressions

Each Rust expression is lowered to C statements in the current function body
plus a C expression for its value (usually a temporary or a local). Blocks with
a value assign to a temporary; `if`/`match` become C `if`/`else` chains with
`goto`-free structured code where possible (labels and `goto` are allowed for
`break`/`continue` with labels and values: `brk<n>:`). `match` tests the
patterns in order (tag compares, literal compares, range compares, guards) and
binds by value or by reference as the pattern's adj says.

Arithmetic follows rustc's debug build: `+ - *` on integers panic with
`attempt to add with overflow` (`subtract`, `multiply`, `negate`, `shift left`,
`shift right` with overflow); `/` and `%` panic with `attempt to divide by zero`
/ `attempt to calculate the remainder with a divisor of zero` and on
`MIN / -1` overflow. Unsigned compares, division and right shifts are done on
`unsigned long`. `as` casts truncate / sign-extend / saturate (float to int)
exactly as Rust. Indexing arrays, slices and `str` checks the bounds:
`index out of bounds: the len is 3 but the index is 5`; `str` slicing also
checks char boundaries (`byte index 1 is not a char boundary; it is inside 'é'
(bytes 0..2) of `héllo``).

Panics print rustc's message and exit with status 101:

```
thread 'main' panicked at src/main.rs:4:5:
attempt to add with overflow
note: run with `RUST_BACKTRACE=1` environment variable to display a backtrace
```

The file name is the path given to rustc (the tests use the relative name).
There is no unwinding: a panic does not run destructors.

## Drops

A value whose type needs dropping (it, or a field, has a `Drop` impl, or it is
a `Box`, `Vec`, `String`, closure owning such values, ...) is dropped at the end
of its scope in reverse declaration order, unless it was moved. Locals that
are moved on some paths only get a drop flag (`long df_<n>`). Temporaries are
dropped at the end of the enclosing statement. Assignment drops the old value
first. Drop glue `static long D<n>(long p)` (the address of the value) calls
`Drop::drop(&mut *p)` if the type implements Drop, then drops the fields in
declaration order (enum: the active variant's fields).

## Runtime: lib/rust_rt.c

Plain C copied at the top of every generated file (all functions `static`;
unused ones are compiled but harmless). It provides `struct rs_slice`,
`struct rs_dyn`, the intrinsics of lib/README.md that are functions rather than
inline expressions, panics with the messages above, checked arithmetic helpers,
UTF-8 output (`rs_write_bytes(port, base, len)` decodes UTF-8 and writes
codepoints with `fputc`; invalid sequences become U+FFFD), byte input
(`rs_read_byte(port)` encodes the codepoints `fgetc` returns as UTF-8 and returns
them a byte at a time), `fmt_f64` / `parse_f64` (exact output as Rust's
`Display` / `{:.N}` / `{:e}` for f64), allocation in words, and `exit`.

## Tests

`tests/run/NAME.rs` programs with `NAME.expected` (stdout) generated by the
real rustc on the host in a debug build, so overflow checks match
(`tests/gen_expected.sh NAME`: `rustc --edition 2021 -o /tmp/x run/NAME.rs`, run
it from `tests/`, keep stdout, and the exit status in `NAME.exit` and the panic
line in `NAME.stderr` when the program panics or exits with a status).
