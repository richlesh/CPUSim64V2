# CPUSim64 C Compiler

A C compiler written in CPUSim64 assembly language. It reads a C source file
and writes CPUSim64 assembly, which the regular CPUSim64 tools assemble and run.

```
../../run.sh cc prog.c            # compile prog.c -> prog.asm
../../run.sh prog                 # assemble and run the generated program
../../run.sh prog arg1 arg2       # command line arguments reach main(argc, argv)
```

Run the `cc` command from this directory (`run.sh` looks for `cc.asm` in the
current directory); the C file itself may be anywhere, and `prog.asm` is
written next to it.

Debugging dumps (instead of code generation):

```
../../run.sh cc prog.c --pp       # preprocessed source
../../run.sh cc prog.c --tokens   # token list (line, class, spelling)
../../run.sh cc prog.c --ast      # typed abstract syntax tree
```

`--lib=DIR` selects the library directory (see *Library headers* below); it
may be combined with the dump options, in any order after the file name.

The compiler stops at the first error, prints `file:line: error: message` to
STDERR, deletes any partial output and finishes with result 1.  Note that the
simulator's own exit status is always 0; the program result is the
`Result: N` line printed by `run.sh`.

## run.sh settings

`run.sh` uses `MEMSIZE=16M` and `STACKSIZE=256K` (64-bit words).  The
compiler needs a few million words of heap for large inputs and a deep
stack for its recursive-descent parser; compiled programs get the same
limits (deep recursion such as 50,000 nested calls works).

## Architecture

| File | Purpose |
|------|---------|
| `cc.asm` | main program; includes every other module (CPUSim64 has no linker) |
| `defs.def` | token, AST, type and symbol layouts; vector macros |
| `util.asm` | zeroed allocation, growable vectors, string builder, identifier interning |
| `errors.asm` | fail-first error reporting |
| `source.asm` | file loading and path helpers |
| `lexer.asm` | tokens per `CLexer.g4` (subset) |
| `pp.asm` | token-level preprocessor |
| `prelude.asm` | built-in C header text: macros, types, library declarations, C library bodies |
| `types.asm`, `symtab.asm` | type objects and scoped symbol table |
| `parse_expr.asm`, `parse_decl.asm`, `parse_stmt.asm` | recursive-descent parser following `CParser.g4`, precedence climbing for binary operators |
| `sema.asm` | type checking and implicit conversions (applied while parsing) |
| `ast.asm` | AST nodes and the `--ast` dumper |
| `codegen.asm` | stack-machine code generation |
| `runtime_emit.asm` | assembly runtime library, copied into programs on demand |
| `lib/` | library headers and sources in C (`math.h`, `math.c`) |

Pipeline: source -> lexer -> preprocessor -> parser + semantic analysis ->
code generation per function -> output file.  Library functions written in C
(in the prelude or in `lib/*.c`) are compiled only when a program uses them.

### Generated code

* Values are computed into `r0` (integers, pointers, aggregate addresses) or
  `f0` (double); temporaries go on the stack.
* Arguments are pushed right to left; the callee reads them at `SF[3]`,
  `SF[4]`, ... and keeps locals at `SF[0]`, `SF[-1]`, ...  This is the
  `#def_func` convention, so CPUSim64 library functions are called directly.
* A function returning a struct receives a hidden result pointer.
* Globals are `#global` data; string literals are `.dcs` (packed) or `.dcw`
  (word-per-character) data, emitted only in the forms the program uses.

## Supported language

Types: `char`, `short`, `int`, `long`, `long long` (all 64-bit), `signed`,
`unsigned`, `_Bool`/`bool`, `double`, `void`, pointers, arrays
(multi-dimensional), `struct`, `union`, `enum`, `typedef`, function pointers.
Qualifiers (`const`, `volatile`, `restrict`) and `static`, `extern`, `inline`
are accepted.

Statements: everything, including `switch`, `goto`/labels, `break`/`continue`,
and declarations in `for`.

Expressions: all C operators, casts, `sizeof`, compound literals, designated
initializers, brace elision, string-literal concatenation, `__func__`,
variadic functions via `<stdarg.h>`.

Preprocessor: `#include "file"` (searched relative to the including file, then
the main file) and `<file>` (library headers from `lib/`, other standard
headers are built in), object-like and
function-like `#define` with `#`, `##` and `__VA_ARGS__`, `#undef`, `#if`,
`#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`, `defined`, `#error`;
`#pragma`, `#line` and `#warning` are ignored.  `__LINE__` and `__FILE__` are
predefined.

Not supported (reported as errors): `float`, `long double`, `_Complex`,
`_Atomic`, bit-fields, variable length arrays, K&R parameter lists,
`_Generic` and GNU extensions.

## Library headers

`#include <name>` or `#include <name.h>` looks for `lib/name.h`.  If it
exists:

* `lib/name.h` is included at the point of the `#include`, like any header;
* `lib/name.c`, if it exists, is appended to the end of the translation unit
  (once, however often the header is included), because CPUSim64 has no
  linker.  Its functions are compiled only if the program uses them, so a
  program that calls only `sqrt` gets only the `sqrt` code.

A `<...>` header without a `lib/` file is one of the built-in standard
headers (`stdio.h`, `stdlib.h`, `string.h`, ...) and is accepted silently;
those declarations are part of the prelude.

The library directory is the `lib/` directory next to the compiler's `obj/`
directory, found from the path of `cc.o64`; `--lib=DIR` overrides it.

A function in `lib/name.h` that has no body in `lib/name.c` is provided by the
assembly runtime (`runtime_emit.asm`, label `__crt_<name>`).  If the program
defines a function with the same name, the program's definition is used and
the library body is ignored.

The appended `.c` file is preprocessed after the program, so it sees the
program's macros.  Library sources therefore use names that start with a
library-specific prefix (`__m_` in `math.c`) for all locals and helpers.

As in standard C, math functions must be declared: calling `sqrt` without
`#include <math.h>` is an "implicit declaration" error.

## Memory model

CPUSim64 memory is word addressed, so **every scalar is one 64-bit word**:
`sizeof(char) == sizeof(int) == sizeof(double) == sizeof(void *) == 1`.
`sizeof` and `malloc` sizes are therefore in words, and programs written with
`sizeof` stay correct.  A `char` holds one Unicode codepoint (UTF-32); `char`
arrays and `char *` strings store one character per word and end with 0.
`CHAR_BIT` is 64 and `INT_MAX` is 2^63 - 1.

## Strings and the pstr rule

The CPUSim64 library uses *packed* strings: a length word followed by UTF-8
bytes, 8 per word.  The compiler keeps both representations apart with an
internal type `__pstr` (packed string):

* A string literal is a `char[]` (word per character) in ordinary C use:
  `char *s = "abc"; s[1]` works as in C.
* Where a library function expects text (`printf` format, `puts`, `fopen`
  file name, ...) the parameter is declared `__pstr` and a literal is passed
  directly as packed `.dcs` data.
* A `char *` passed to such a parameter, or as a `%s` argument to the printf
  family, is converted to a temporary packed string, which is freed after the
  call.
* Format strings are adapted to the CPUSim64 formatter: length modifiers
  (`l`, `ll`, `h`, `z`, ...) are dropped, and `%i`/`%u` become `%d`, `%p`
  becomes `%x`.  A literal format is checked against its arguments at compile
  time, because a mismatch would crash the formatter at run time.
* The formatter is Java's `String.format`, so `%g` prints like `%f`, and `%x`
  of a negative number prints all 64 bits.

## C library

`<stdio.h>`: `printf`, `fprintf`, `sprintf`, `snprintf`, `puts`, `fputs`,
`putchar`, `fputc`, `putc`, `getchar`, `fgetc`, `getc`, `fgets`, `fopen`
(`"r"`, `"w"`, `"a"`), `fclose`, `fflush`, `feof`, `perror`, `stdin`,
`stdout`, `stderr`.
`<stdlib.h>`: `malloc`, `calloc`, `realloc`, `free`, `exit`, `abort`, `rand`,
`srand`, `abs`, `labs`, `atoi`, `atol`, `atof`, `strtol`, `strtoul`, `strtod`,
`qsort`, `bsearch`, `getenv`.
`<string.h>`: `strlen`, `strcmp`, `strncmp`, `strcpy`, `strncpy`, `strcat`,
`strncat`, `strchr`, `strrchr`, `strstr`, `strdup`, `memset`, `memcpy`,
`memmove`, `memcmp`.
`<ctype.h>`: `isdigit`, `isalpha`, `isalnum`, `isspace`, `isupper`, `islower`,
`isxdigit`, `isprint`, `isgraph`, `iscntrl`, `ispunct`, `toupper`, `tolower`.
`<math.h>` (`lib/math.h`, `lib/math.c`): `sin`, `cos`, `tan`, `asin`,
`acos`, `atan`, `atan2`, `sinh`, `cosh`, `tanh`, `asinh`, `acosh`, `atanh`,
`exp`, `exp2`, `expm1`, `log`, `log10`, `log2`, `log1p`, `pow`, `sqrt`,
`cbrt`, `hypot`, `fabs`, `floor`, `ceil`, `round`, `trunc`, `rint`,
`nearbyint`, `lround`, `llround`, `lrint`, `fmod`, `remainder`, `modf`,
`frexp`, `ldexp`, `scalbn`, `copysign`, `fmin`, `fmax`, `fdim`, `fma`,
`isnan`, `isinf`, `isfinite`, `signbit` (functions, not macros), and the
`M_*` constants, `INFINITY`, `NAN`, `HUGE_VAL`.  The simulator's
floating-point compare treats NaN as equal to everything, so use `isnan()`
rather than `x != x`.
Also `<assert.h>`, `<stdarg.h>`, `<stddef.h>`, `<stdint.h>`, `<stdbool.h>`,
`<limits.h>`, `<time.h>` (`time`, `clock`).

A program may define its own function with a library name; its definition
replaces the library version.

## Tests

```
tests/run_tests.sh            # everything
tests/run_tests.sh run/       # only tests whose name contains "run/"
```

* `tests/unit/` - assembly unit tests of the utility layer
* `tests/dump/` - `--pp`, `--tokens` and `--ast` output
* `tests/errors/` - programs that must fail with a given message
* `tests/run/` - programs that are compiled, run and compared with
  `.expected` output.  Optional files: `.exit` (expected result), `.argv`
  and `.stdin` (program input), `.ccargs` (extra compiler arguments;
  `$HERE` is the `tests/` directory) and `.noasm` (strings that must not
  appear in the generated assembly, used to check lazy library compilation).
  Where C semantics allow, the expected output was produced by compiling
  the same program natively with gcc.
* `tests/cli/` - command line handling
* `tests/libfixture/` - a small library used to test the `lib/` mechanism
