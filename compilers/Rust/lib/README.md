# lib/core.rs: the Rust prelude

`core.rs` is parsed as the crate `core` before every program. `std` and `alloc`
are aliases of it (resolve.c maps `std::x` and `alloc::x` to `core::x`). Check
it with `obj/rustc-native --ast lib/core.rs`.

Inside the file every path is absolute (`::core::...`); `unsafe` is not
written (the checker must not require it around intrinsic calls). Integer
types are `i8..i64`, `isize`, `u8..u64`, `usize` (no 128 bit types).

## What the other stages must support

- `macro_rules!` with `ident`, `expr`, `tt` fragments, repetitions
  (`$(...)*`, `$(...)+`), and macro invocations in item position and inside
  macro bodies (macros call other macros). The primitive impls are instantiated
  in `mod prim_impls`.
- Trait generic parameter defaults (`trait Add<Rhs = Self>`, `Sum<A = Self>`) and
  `?Sized` bounds.
- Inherent impls on primitives and slices: `impl i32 {..}`, `impl f64 {..}`,
  `impl char {..}`, `#[lang = "str"] impl str {..}`, `impl<T> [T] {..}`,
  `impl<S: AsRef<str>> [S] {..}`. They are legal only in `core`.
- Modules named like primitives (`core::char`, `core::str`, `core::f64`). A type
  path `char`, `str` or `f64` must resolve to the primitive unless the user
  imported the module; no item at the crate root of `core` uses these names.
- `for` loops, `?`, ranges and closures as described below.
- Impl selection by unification with bounds: the identity `impl<T> From<T> for T`
  coexists with `From<u8> for u32` and `impl<E: Error> From<E> for Box<dyn Error>`.
- Constants such as `f64::INFINITY = 1.0e308 * 10.0` and `f64::NAN = 0.0 / 0.0`
  are float expressions the code generator emits as C constants.
- Built-in operators on primitives and indexing of arrays and slices by `usize`
  are compiler built-ins (the impls in core.rs exist for generic code and use
  the built-in operator in their bodies, so they never recurse).
  `Index<usize>` for `[T]` / `[T; N]` is deliberately absent.
- Copy: `implements(t, Copy)` is built in for primitives, `&T`, raw pointers,
  tuples and arrays of Copy; core.rs declares `Copy` impls only for `Option`,
  `Result`, `Ordering`, `Error`, `PhantomData`.
- `derive` is not used anywhere in core.rs.

## Lang items

| `#[lang]` | Item | Used by |
|---|---|---|
| `sized`, `copy`, `send`, `sync` | marker traits | bounds |
| `phantom_data` | `marker::PhantomData<T>` | |
| `clone` | `Clone` | |
| `add sub mul div rem neg not bitand bitor bitxor shl shr` | operator traits | binary and unary operators on non-primitives and generics |
| `add_assign sub_assign mul_assign div_assign rem_assign bitand_assign bitor_assign bitxor_assign shl_assign shr_assign` | compound assignment traits | `a += b` |
| `index`, `index_mut` | `Index`, `IndexMut` | `a[i]` on non-array, non-slice types |
| `deref`, `deref_mut` | `Deref`, `DerefMut` | `*x`, auto-deref |
| `drop` | `Drop` | drop elaboration |
| `fn`, `fn_mut`, `fn_once` | `Fn<Args>`, `FnMut<Args>`, `FnOnce<Args>` with `type Output` | closure calls, bounds |
| `range`, `range_inclusive`, `range_from`, `range_to`, `range_to_inclusive`, `range_full` | the range structs (public fields `start`, `end`; `RangeInclusive` also `exhausted`) | `a..b`, `a..=b`, `a..`, `..b`, `..=b`, `..` |
| `range_inclusive_new` | `RangeInclusive::new(start, end)` | `a..=b` lowers to a call |
| `eq`, `eq_total`, `partial_ord`, `ord` | `PartialEq<Rhs>`, `Eq`, `PartialOrd<Rhs>`, `Ord` | `==`, `!=`, `<`, `<=`, `>`, `>=` |
| `ordering` | `cmp::Ordering` (`Less = -1`, `Equal = 0`, `Greater = 1`) | |
| `option` | `Option<T>` (`None` is variant 0, `Some` variant 1) | `?`, niche layout |
| `result` | `Result<T, E>` (`Ok` 0, `Err` 1) | `?` |
| `from`, `into` | `From<T>`, `Into<T>` | `?` error conversion |
| `default` | `Default` | |
| `iterator`, `into_iterator`, `double_ended_iterator`, `from_iterator` | iterator traits | `for`, `collect` |
| `owned_box` | `boxed::Box<T>` | `Box<dyn Trait>`, unsizing, `*b` move-out |
| `str`, `string`, `vec` | the inherent `impl str`, `string::String`, `vec::Vec<T>` | literals, `vec!`, `format!` |
| `formatter` | `fmt::Formatter<'a>` | |
| `panic_str` | `panicking::panic_str(file, line, col, msg) -> !` | assertion and overflow panics |
| `panic_bounds_check` | `panicking::panic_bounds_check(file, line, col, index, len) -> !` | built-in indexing |
| `panic_end` | `panicking::end() -> !` | |

Messages for built-in checks go through `panic_str`: `attempt to add with overflow`
(also `subtract`, `multiply`, `negate`), `attempt to divide by zero`,
`attempt to calculate the remainder with a divisor of zero`.

### Desugarings check.c should use

```
e?   (e: Result<T, E>, fn returns Result<U, F>)
        match e { Ok(v) => v, Err(err) => return Err(From::from(err)) }
e?   (e: Option<T>, fn returns Option<U>)
        match e { Some(v) => v, None => return None }
for p in e { body }
        { let mut __it = IntoIterator::into_iter(e);
          loop { match Iterator::next(&mut __it) { Some(p) => body, None => break } } }
a..=b   RangeInclusive::new(a, b)          a..b   Range { start: a, end: b }
f(x, y) (f: F where F: Fn(A, B) -> R)   Fn::call(&f, (x, y))
        (FnMut::call_mut, FnOnce::call_once likewise)
F: Fn(A, B) -> R     means  F: Fn<(A, B), Output = R>
a == b  PartialEq::eq(&a, &b)              a < b  PartialOrd::lt(&a, &b)
```

Closures and fn items get built-in `Fn*`, `Clone` and `Copy` impls from the
compiler; core.rs has no impls for them. `Box<dyn Fn(..)>` is called through the box.
Field names of `Vec` (`ptr`, `cap`, `len`) and `String::vec` are `pub` because other
core modules use them; user code is not expected to.

## Intrinsics (`core::intrinsics`, `extern "rust-intrinsic"`)

Generic intrinsics are instantiated per type argument. "word address" is a
CPUSim64 word address; "byte address" is `word address * 8 + index` (see
design-strings.md). `usize` addresses are plain integers.

### Element storage

For an element type `T`, a storage base is a **byte address when `T = u8`**
(packed bytes, 8 per word) and a **word address** for every other `T`. Element
`i` of a non-`u8` `T` is at `base + i * words(T)`. All the `elem_*` functions
below honour that rule, so core.rs never computes sizes.

| Intrinsic | Semantics |
|---|---|
| `elem_alloc<T>(n: usize) -> usize` | Allocates room for `n >= 1` elements, uninitialised, returns the base. (C `malloc`.) |
| `elem_realloc<T>(base, old_n, new_n) -> usize` | Resizes to `new_n` elements, keeps the first `old_n`, returns the new base. Never called with `old_n == 0`. |
| `elem_free<T>(base)` | Frees a block from `elem_alloc` / `elem_realloc`. |
| `elem_read<T>(base, i) -> T` | Bitwise copy of element `i` out of storage (a move; runs no drop and no clone). |
| `elem_write<T>(base, i, v: T)` | Stores `v` into element `i` (a move in; the old contents are not dropped). |
| `elem_offset<T>(base, n) -> usize` | The base advanced by `n` elements (`base + n` for `u8`, `base + n * words(T)` otherwise). |
| `elem_ref<'a, T>(base, i) -> &'a T` | Reference to element `i`. For `T = u8` the reference must address a packed byte: the code generator needs a representation for `&u8` into packed storage (byte address) distinct from `&u8` to a local (word address), or must restrict it. `elem_ref_mut` likewise. |
| `elem_ref_mut<'a, T>(base, i) -> &'a mut T` | Same, mutable. |

Zero sized types (unit structs, `()`) must occupy at least one word.

### Places and values

| Intrinsic | Semantics |
|---|---|
| `read_ref<T>(r: &T) -> T` | Bitwise copy out of `*r` (a move; no drop of the source). |
| `write_ref<T>(r: &mut T, v: T)` | Bitwise store into `*r`, **without dropping** the old value. |
| `forget<T>(v: T)` | Consumes `v` without running its destructor. |
| `size_of<T>() -> usize` | The size of a value of type `T` in bytes: 8 per word, as every scalar is one word (a `u8` field also takes a word). |
| `transmute<T, U>(v: T) -> U` | Reinterpret the bits. Used only for `u32 -> char`. |

### Slices and strings

A `&[T]` is the pair (storage base, element count); `&str` is (byte address, byte
length); `&mut [T]` like `&[T]`.

| Intrinsic | Semantics |
|---|---|
| `slice_base<T>(s: &[T]) -> usize` | The base field. |
| `slice_len<T>(s: &[T]) -> usize` | The length field. |
| `slice_from_raw<'a, T>(base, len) -> &'a [T]` | Builds the pair; `len` may be 0 and `base` may be 0. |
| `slice_from_raw_mut<'a, T>(base, len) -> &'a mut [T]` | Same, mutable. |
| `str_base(s: &str) -> usize` | Byte address of the first byte. |
| `str_len(s: &str) -> usize` | Byte length. |
| `str_from_raw<'a>(base, len) -> &'a str` | Builds the pair; no UTF-8 check. |

### Box

| Intrinsic | Semantics |
|---|---|
| `box_new<T>(v: T) -> Box<T>` | Allocates a block for `T` (at least one word), moves `v` in. |
| `box_deref<'a, T: ?Sized>(b: &'a Box<T>) -> &'a T` | The pointee (`T` may be `dyn Trait`, `[U]` or `str`; the fat part comes from the box). |
| `box_deref_mut<'a, T: ?Sized>(b: &'a mut Box<T>) -> &'a mut T` | Same, mutable. |
| `box_drop<T: ?Sized>(b: &mut Box<T>)` | Drops the pointee (through the vtable for `dyn`) and frees the block. Called from `Drop for Box<T>`. |

### Integers (`T` is any integer type)

| Intrinsic | Semantics |
|---|---|
| `wrapping_add<T>(a, b) -> T`, `wrapping_sub`, `wrapping_mul` | Result modulo 2^bits of `T`, sign-extended or masked to the type's range as for `as` casts. |
| `add_with_overflow<T>(a, b) -> (T, bool)`, `sub_with_overflow`, `mul_with_overflow` | The wrapped result and whether the mathematical result left `T`'s range. Both `i64` and `u64` must be handled without a wider type. |

`wrapping_sub::<u64>(0, x)` is used to negate (also for `i64::MIN`).

### Floats (`f64`; `f32` is an `f64` too)

| Intrinsic | Semantics |
|---|---|
| `f64_to_bits(v) -> u64`, `f64_from_bits(b) -> f64` | IEEE 754 bit pattern. |
| `sqrtf64(v)`, `floorf64(v)`, `ceilf64(v)` | The usual functions. |
| `fmt_f64(v: f64, prec: usize, has_prec: bool, mode: u8, buf: usize) -> usize` | Writes the ASCII text of **`|v|`** (the sign is added by the caller; NaN gives `NaN`, infinity `inf`) at byte address `buf` (capacity 400 bytes) and returns its length. `mode` 0: Display (`1`, `1.5`, `0.1`; shortest round-trip digits, never exponent form; with `has_prec` exactly `prec` decimals); 1: Debug (as Display but integral values get `.0`, and exponent form `1e21` / `1e-7` outside 1e-5..1e16 as in Rust); 2: LowerExp (`1.5e0`); 3: UpperExp (`1.5E0`). |
| `parse_f64(base: usize, len: usize, out: &mut f64) -> bool` | Parses the bytes as a Rust float literal (`1`, `-1.5e3`, `inf`, `NaN`, `+.5`); false when invalid. |

### I/O and process

| Intrinsic | Semantics |
|---|---|
| `write_bytes(port: i64, base: usize, len: usize)` | Writes `len` bytes from byte address `base` to the port: 1 is standard output, 2 is standard error. One `OUT` of size BYTE per byte (or 8 bytes at a time when aligned). |
| `read_byte(port: i64) -> i32` | Reads one byte from the port (0 is standard input), `-1` at end of input. |
| `exit(code: i32) -> !` | Ends the program with the exit status. |
| `abort() -> !` | Ends the program abnormally (status 134). |

## Items needed by the macros (design-expand.md)

`fmt::{Result, Error, Write, Formatter::{new, write_str, set_spec, clear_spec, pad,
pad_integral, alternate, width, precision, debug_struct, debug_tuple, debug_list},
Display, Debug, LowerHex, UpperHex, Octal, Binary, LowerExp, UpperExp}`,
`io::{RawOut, stdout_raw, stderr_raw}`,
`panicking::{panic_str, begin, end, assert_failed, assert_failed_msg}`,
`string::String` (with `fmt::Write`), `vec::{Vec::new, Vec::from_array, from_elem}`.

Also provided: `panicking::unwrap_failed(msg, &dyn Debug)`, used by
`Result::unwrap` and `expect` (`msg: {value:?}`).

`Formatter` field layout is private and flat (`out`, `fill`, `align`, `sign`, `alt`,
`zero`, `width`, `has_width`, `prec`, `has_prec`). For `derive(Debug)` lower to
`f.debug_struct("Name").field("x", &self.x).finish()`, `f.debug_tuple("Name")
.field(&self.0).finish()`, `f.write_str("Unit")`, and `f.debug_list().entries(it).finish()`.
`{:#?}` (pretty) is implemented through `PadAdapter`, so `derive(Debug)` needs
nothing more. Missing for now: `debug_map`, `debug_set`, `{:x?}`.

## Contents

| Module | Provides |
|---|---|
| `marker`, `clone`, `ops`, `cmp`, `default`, `convert`, `borrow`, `mem`, `process` | the traits and functions of the same names in std; `cmp::Reverse`; `mem::{replace, swap, take, forget, drop, size_of}` |
| `option`, `result` | the enums and the usual methods |
| `fmt`, `io`, `env`, `panicking` | formatting (`fmt::ok()` is `Ok(())` as `fmt::Result`), `io::{Read, BufRead, Write}` (`read_to_string`, `read_line`, `lines`, `write_all`, `flush`), `Stdin`/`Stdout`/`Stderr` (`lock`; `Stdout`/`Stderr` deref to `RawOut` so that `write!(w, ..)` accepts an owned handle), `BufReader`, `BufWriter`, `Write for Vec<u8>`, `env::args()` (only the program name, the simulator has no command line), panics |
| `num`, `f64`, `char` | integer methods (`MIN`, `MAX`, `pow`, `checked_{add,sub,mul,div,rem,neg,abs,pow}`, `wrapping_{add,sub,mul,neg,abs,pow}`, `overflowing_*`, `saturating_*`, `abs`, `signum`, `rem_euclid`, `abs_diff`, `count_ones`, `count_zeros`, `leading_zeros`, `trailing_zeros`, `from_str_radix`), `u8::is_ascii_*` / `to_ascii_*` / `eq_ignore_ascii_case`, `FromStr` for integers and `f64`, `f64` methods (`abs sqrt floor ceil round trunc fract powi powf exp ln log2 log10 log cbrt hypot sin cos tan asin acos atan atan2 sinh cosh tanh min max clamp copysign mul_add to_radians to_degrees recip rem_euclid`; the transcendental functions are series computed in `core.rs`, accurate to a few ulp but not bit for bit with libm) and `consts`, char classification and conversion |
| `str`, `string` | `impl str` (len, chars, char_indices, bytes, lines, split, split_whitespace, find, contains, starts_with, ends_with, strip_*, trim*, parse, replace, repeat, ASCII case), `String` (`push`, `push_str`, `pop`, `insert`, `insert_str`, `remove`, `retain`, `truncate`, `split_off`, `drain(range)`, `replace_range`; `insert`/`insert_str`/`split_off`/`drain` panic with `assertion failed: self.is_char_boundary(idx)` off a boundary), `ToString`, `Pattern` for `char`, `&str`, `&String`, `FromIterator` for `char`, `&char`, `&str`, `String`, `Extend` for `char`, `&char`, `&str`, `String` |
| `iter` | `Iterator` with map filter filter_map flat_map flatten inspect scan enumerate zip chain take skip step_by take_while skip_while rev peekable cycle cloned copied count last nth fold reduce for_each any all find find_map position rposition max min max_by_key min_by_key max_by min_by sum product partition unzip collect; ranges as iterators; `FromIterator` for `Vec`, `String`, `Result`, `Option`; `Extend`; `once`, `repeat`; `IntoIterator for Option` |
| `slice`, `vec`, `boxed`, `error` | `impl<T> [T]` (len, iter, get, first, last, split_first, split_last, split_at, split_at_mut, contains, starts_with, ends_with, swap, reverse, rotate_left, rotate_right, fill, windows, chunks, chunks_exact, sort*, binary_search, to_vec), `join` / `concat` for slices of `String`, `&str`, `&String` (via `JoinItem`) and of `Vec<T>` / `&[T]`, `Vec` (also `drain(range)`, `split_off`, `dedup_by`, `dedup_by_key`, `extend_from_slice`, `swap_remove`, `retain`, `insert`, `truncate`), `Box` (`AsRef`, `AsMut`), `Error` with `Box<dyn Error>` conversions; `ops::RangeArg` gives `(start, end)` of a usize range for `drain` |
| `prelude` | the std prelude re-exports (`Option::{Some, None}`, `Result::{Ok, Err}`, `Vec`, `String`, `Box`, `ToString`, `ToOwned`, the traits) |

| `collections`, `rc` (lib/collections.rs, lib/rc.rs) | `HashMap`, `HashSet`, `BTreeMap`, `VecDeque`, `Hash`, `Rc`, `RefCell` and their iterators |

Not provided: `TryFrom`, `std::thread`, `i128`, `f32` as a distinct type, Unicode case
mapping (`to_uppercase` is ASCII only, `char::is_alphabetic` treats every non-ASCII
char as alphabetic), and `Index<usize>` impls for `[T]`.

## Performance notes

The ref forms of the arithmetic operator impls (`&a + b`, `a + &b`, `&a + &b`)
are 165 of the impls that macro expansion generates (5 operators, 3 ref forms,
11 types). If translation time matters, drop them from `binop_impls!` for
types other than `i32`, `i64`, `usize` and `f64`.
