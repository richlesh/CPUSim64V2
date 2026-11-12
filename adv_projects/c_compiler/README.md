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

Pipeline: source -> lexer -> preprocessor -> parser + semantic analysis ->
code generation per function -> output file.  Library functions written in C
in the prelude are compiled only when a program uses them.

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
the main file) and `<file>` (standard headers are built in), object-like and
function-like `#define` with `#`, `##` and `__VA_ARGS__`, `#undef`, `#if`,
`#ifdef`, `#ifndef`, `#elif`, `#else`, `#endif`, `defined`, `#error`;
`#pragma`, `#line` and `#warning` are ignored.  `__LINE__` and `__FILE__` are
predefined.

Not supported (reported as errors): `float`, `long double`, `_Complex`,
`_Atomic`, bit-fields, variable length arrays, K&R parameter lists,
`_Generic` and GNU extensions.

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
`<math.h>`: `sqrt`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `atan2`,
`sinh`, `cosh`, `tanh`, `exp`, `log`, `log10`, `log2`, `pow`, `fabs`,
`floor`, `ceil`, `round`, `trunc`, `fmod`, `hypot`, `cbrt`, `fmin`, `fmax`,
`M_PI` etc.
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
  `.expected` output (`.exit`, `.argv` and `.stdin` files are optional).
  Where C semantics allow, the expected output was produced by compiling
  the same program natively with gcc.
* `tests/cli/` - command line handling
