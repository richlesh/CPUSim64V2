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

`--lib=DIR` adds a library directory (see *Library headers* below); it may
be combined with the dump options, in any order after the file name.

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
| `prelude.asm` | predefined macros only (`__CPUSIM64__`, `__STDC__`, ...) |
| `types.asm`, `symtab.asm` | type objects and scoped symbol table |
| `parse_expr.asm`, `parse_decl.asm`, `parse_stmt.asm` | recursive-descent parser following `CParser.g4`, precedence climbing for binary operators |
| `sema.asm` | type checking and implicit conversions (applied while parsing) |
| `ast.asm` | AST nodes and the `--ast` dumper |
| `codegen.asm` | stack-machine code generation |
| `runtime_emit.asm` | assembly runtime library, copied into programs on demand |
| `lib/` | library headers and sources in C: `stdio`, `math`, `stdlib`, `string`, `time`, `ctype` (`.h` + `.c`) and the header-only `setjmp`, `stdarg`, `stddef`, `stdint`, `stdbool`, `limits`, `float`, `assert` |

Pipeline: source -> lexer -> preprocessor -> parser + semantic analysis ->
code generation per function -> output file.  Library functions written in C
(in `lib/*.c`) are compiled only when a program uses them.

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

Everything else, including `size_t`, `NULL`, `va_list`, `INT_MAX`, `bool`
and the `isdigit` family, comes from these headers (the built-in prelude
defines only the `__CPUSIM64__` / `__STDC__` macros), so as in standard C
a program must include `<stddef.h>`, `<limits.h>`, `<ctype.h>` and so on.
The headers that need `size_t` or `va_list` include `<stddef.h>` and
`<stdarg.h>` themselves.

The library directory is the `lib/` directory next to the compiler's `obj/`
directory, found from the path of `cc.o64`.  `--lib=DIR` adds a directory
that is searched first (it may be given more than once).

A function in `lib/name.h` that has no body in `lib/name.c` is provided by the
assembly runtime (`runtime_emit.asm`, label `__crt_<name>`).  If the program
defines a function with the same name, the program's definition is used and
the library body is ignored.

The appended `.c` file is preprocessed after the program, so it sees the
program's macros.  Library sources therefore use names that start with a
library-specific prefix (`__sio_`, `__m_`, `__sl_`, `__st_`, `__tm_`) for all locals,
helpers and data.

As in standard C, math functions must be declared: calling `sqrt` without
`#include <math.h>` is an "implicit declaration" error.

## Memory model

CPUSim64 memory is word addressed, so **every scalar is one 64-bit word**:
`sizeof(char) == sizeof(int) == sizeof(double) == sizeof(void *) == 1`.
`sizeof` and `malloc` sizes are therefore in words, and programs written with
`sizeof` stay correct.  A `char` holds one Unicode codepoint (UTF-32); `char`
arrays and `char *` strings store one character per word and end with 0.
`CHAR_BIT` is 64 and `INT_MAX` is 2^63 - 1.

## Strings

There are two string representations, and C code only ever sees the first:

* **C strings** - 0-terminated arrays of `char`, one Unicode codepoint
  (UTF-32) per 64-bit word.  Every `char *`, `char[]` and string literal is
  one, and the whole C library (`string.h`, `stdio.h`, ...) works on them.
* **CPUSim64 strings** - a length word followed by UTF-8 bytes packed 8 per
  word (`.dcs` data).  Only the CPUSim64 operating system interrupts and
  assembly library use them.

The conversion happens inside a few assembly runtime routines at the
operating-system boundary: file names are converted when a file is opened,
deleted or tested, output text is converted when a formatted block is
written, and `argv` strings are converted at startup.  Characters are read
and written one codepoint at a time (`IN`/`OUT` with size `CHAR`), so text
files are UTF-8 on disk and one codepoint per `char` in memory.

The internal type `__pstr` (a packed string) is still accepted for code that
calls CPUSim64 functions directly, but no standard library function uses it.

## C library

`<stdio.h>` (`lib/stdio.h`, `lib/stdio.c`):
* files: `fopen` (modes `r w a`, with `+`, `b`, `x`), `freopen`, `fclose`,
  `fflush`, `setbuf`/`setvbuf` (accepted, no effect), `remove`, `rename`,
  `tmpfile`, `tmpnam`, `fileno`, `stdin`, `stdout`, `stderr`;
* characters and lines: `fgetc`, `getc`, `getchar`, `ungetc`, `fgets`,
  `getline`, `getdelim`, `fputc`, `putc`, `putchar`, `fputs`, `puts`;
* blocks: `fread`, `fwrite`;
* positioning: `fseek`, `ftell`, `rewind`, `fgetpos`, `fsetpos`;
* errors: `feof`, `ferror`, `clearerr`, `perror`;
* formatted output: `printf`, `fprintf`, `sprintf`, `snprintf`, `vprintf`,
  `vfprintf`, `vsprintf`, `vsnprintf` with all C99 conversions (`d i u o x
  X c s p n f F e E g G a A %`), flags (`- + space # 0`), `*` width and
  precision and length modifiers.  Floating-point output is exact and
  rounded half-to-even like glibc (`%.2f` of 0.125 is `0.12`; `%.20f` of 0.1
  prints all its digits).  `%c` prints any codepoint and field widths count
  characters, not UTF-8 bytes;
* formatted input: `scanf`, `fscanf`, `sscanf`, `vscanf`, `vfscanf`,
  `vsscanf` with `d i u o x X p f e g a s c [...] n %`, `*` suppression and
  widths; floating-point input is correctly rounded.

Every file is a CPUSim64 random-access port (`iOPEN_RANDOM_ACCESS_FILE`), so
all modes (`r w a`, `r+ w+ a+`) work as in C: a stream can be read and
written, `fseek` / `ftell` use `iSEEK` / `iTELL`, `r+` updates in place,
`a+` reads anywhere and appends every write, and seeking past the end then
writing fills the gap with zero bytes.  Positions are byte offsets in the
file for text and binary streams alike (as on Unix; a non-ASCII character
takes 2-4 bytes in a text file).  Binary streams (`"b"`) read and write one
byte per `char`, so `fwrite` of an `int` or `long` keeps only its low byte;
`rename` copies the file.

Streams are unbuffered: every output call reaches the operating system
before it returns (one write per `printf`), so nothing is lost at exit and
`stdout` and `stderr` interleave in program order.  Literal `printf` and
`scanf` formats are checked against their arguments at compile time.
`<stdlib.h>` (`lib/stdlib.h`, `lib/stdlib.c`): `malloc`, `calloc`,
`realloc`, `free`, `exit`, `_Exit`, `abort`, `atexit`, `system`, `getenv`
(always `NULL`), `abs`, `labs`, `llabs`, `div`, `ldiv`, `lldiv`, `atoi`,
`atol`, `atoll`, `atof`, `strtol`, `strtoll`, `strtoul`, `strtoull`
(overflow clamps to the limits), `strtod` (also `inf`/`nan`), `rand`,
`srand` (a deterministic 64-bit LCG; without `srand` the sequence is that of
`srand(1)`), `qsort` (heapsort, not stable), `bsearch`, and `EXIT_SUCCESS`,
`EXIT_FAILURE`, `RAND_MAX`, `div_t`/`ldiv_t`/`lldiv_t`.  Returning from
`main` runs the `atexit` handlers, as calling `exit` does.
`<string.h>` (`lib/string.h`, `lib/string.c`): `memcpy`, `memmove`,
`memset`, `memcmp`, `memchr`, `strcpy`, `strncpy`, `strcat`, `strncat`,
`strcmp`, `strncmp`, `strcoll`, `strxfrm`, `strlen`, `strnlen`, `strchr`,
`strrchr`, `strspn`, `strcspn`, `strpbrk`, `strstr`, `strtok`, `strtok_r`,
`strdup`, `strndup`, `strerror`.  Comparisons return -1, 0 or 1.
`<setjmp.h>` (`lib/setjmp.h`, runtime only): `jmp_buf`, `setjmp`,
`longjmp`.  Use `setjmp` only where C allows it (the whole condition of an
`if`/`switch`/loop, an operand of `!` or of a comparison with a constant, an
expression statement, or the right side of a simple assignment); in other
expressions values computed before the call can be lost.
`<time.h>` (`lib/time.h`, `lib/time.c`): `clock` (run time in microseconds,
`CLOCKS_PER_SEC` 1000000), `time`, `difftime`, `mktime`, `timegm`,
`gmtime`, `gmtime_r`, `localtime`, `localtime_r`, `asctime`, `ctime`,
`strftime` (`%a %A %b %B %c %C %d %D %e %F %h %H %I %j %m %M %n %p %r %R %S
%t %T %u %U %w %W %x %X %y %Y %z %Z %%`), `struct tm`, `time_t`, `clock_t`.
The simulator has no calendar clock, so `time()` returns `(time_t)-1` ("not
available", as the C standard allows); all conversions are in UTC, so
`localtime` equals `gmtime` and `mktime` equals `timegm`.
`<ctype.h>` (`lib/ctype.h`, `lib/ctype.c`): `isblank`, `isdigit`, `isalpha`, `isalnum`, `isspace`, `isupper`, `islower`,
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
Also `<assert.h>` (honours `NDEBUG`), `<stdarg.h>`, `<stddef.h>`, `<stdint.h>`,
`<stdbool.h>`, `<limits.h>` and `<float.h>`.

As in standard C, a library function must be declared by its header:
calling `printf` without `#include <stdio.h>`, `malloc` without
`#include <stdlib.h>` or `strlen` without `#include <string.h>` is an
"implicit declaration" error.  Unused library
functions and data are not compiled into the program.

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
