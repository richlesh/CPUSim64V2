# CPUSim64 VB.NET Compiler

A Visual Basic .NET compiler for CPUSim64.  It compiles `.vb` programs to
CPUSim64 assembly, which runs on the Java and the native simulator and in
the IDE's debugger with the `.vb` source lines.  The compiler is written in
the C subset of the CPUSim64 C11 compiler (`compilers/C11`) and is itself a
CPUSim64 program (`vbc.asm`).

It follows the Visual Basic Language Specification version 11
([dotnet/vblang](https://github.com/dotnet/vblang)) and its grammar
(`grammar/vb.g4`), with a few later additions: interpolated strings, `?.`,
`NameOf`, `&B` literals, digit separators and multi-line strings.

## Using it

In the IDE: File → New Compiler → VBNET opens a VB.NET window.  Run and
Debug are enabled for a file with `Sub Main` (or `Function Main`).  Other
source files of the program go in the Options field.  The `samples/` folder
has programs to start from: `Hello.vb`, `Bank.vb`, `Shapes.vb`, `Linq.vb`
and `Guess.vb`.

From the command line (`compilers/VBNET`):

```
../../bin/cpusim64_native obj/vbc.o64 --mem=16M --stack=256K prog.vb [more.vb ...] [options]
../../assemble.sh prog
../../bin/cpusim64_native obj/prog.o64 [arguments]
```

The compiler needs about 4M words of memory (`--mem`; the IDE gives it
16M, the simulator's own default is too small).  `prog.asm` is written next
to the first source file.  `obj/vbc.o64` is made
from `vbc.asm` by `../../assemble.sh vbc` (the IDE and the test script do it
when it is missing or older).  A program reads standard input and writes
standard output; `Function Main` returns the exit code.

Options:

| Option | Meaning |
|---|---|
| `--define=NAME[=VALUE]` | a conditional compilation constant (`#If`) |
| `--tokens`, `--syntax`, `--ast` | print the tokens, the parse tree or the typed tree of the first file |
| `--parse-only` | check the syntax only |
| `--gc-stress` | the program collects garbage at every allocation (for testing) |
| `--rt=DIR`, `--lib=DIR` | the run-time library and the VB library sources (default `rt/` and `lib/` of the plug-in) |

Errors are reported as `file:line: error: message`, mostly with the
messages of Microsoft's compiler, and the compiler stops at the first one.
A run-time error ends the program with `Unhandled exception.
System.OverflowException: ...` (the exception's `ToString()`) and exit
code 1.

## The language

- Modules, classes, structures, interfaces, enums, delegates, namespaces,
  `Imports`, `Partial` classes, programs of several files.
- Fields, constants, methods (`ByRef`, `Optional`, `ParamArray`, named
  arguments, overloading), constructors, properties (indexed, `Default`,
  auto-implemented), events (`RaiseEvent`, `AddHandler`, `Handles`,
  `WithEvents`), operator overloading and conversion operators.
- Inheritance (`Overridable`, `Overrides`, `MustOverride`, `MyBase`),
  interface implementation, `TypeOf`, `DirectCast`, `TryCast`, `CType`.
- Generic types and methods with constraints and type inference.
- All the statements except `On Error`/`Resume`: `If`, `Select Case`, the
  loops, `Exit`, `Continue`, `GoTo`, `Try`/`Catch`/`When`/`Finally`,
  `Throw`, `Using`, `With`, `SyncLock`, `ReDim`, `Static` locals.
- Lambdas and closures, `AddressOf`, extension methods, nullable values
  (`Integer?`), `?.`, anonymous types, object and collection initializers,
  local type inference, LINQ (methods and query expressions).
- Option Strict, Explicit, Infer and Compare; the conversions and operator
  types of VB, with overflow checks.

Not supported: `Async`/`Await`, `Iterator`/`Yield`, late binding, `On
Error`, `Decimal` and `Date`, XML literals, reflection (`GetType`),
`My`, arithmetic on nullable values, Custom events, variance, thread pools
and tasks (`ThreadPool`, `Task`), `Monitor.Wait`/`Pulse`.  `New Object()`
and `New String(c, n)` are not supported (lock an object of a class of
your own; use `StrDup(n, c)`).  They
are reported as errors ("... not supported yet").

## The library

- `Console`, `Math`, `Convert`, `String` and `Char` methods, `Parse`/
  `TryParse`, `Environment`, `GC`, .NET format strings (invariant culture),
  and the `Microsoft.VisualBasic` functions (`Len`, `Mid`, `InStr`, `Val`,
  `Format`, `Rnd`, `vbCrLf`, ...).
- `System.Exception` and the standard exception classes.
- `List`, `Dictionary`, `HashSet`, `Stack`, `Queue`, `KeyValuePair`,
  `StringBuilder`, `Random` (the .NET generator: seeded sequences are the
  same as in .NET), `Array`, `Tuple`, `Nullable`.
- `Action`, `Func`, `Predicate`, `Comparison`, `EventHandler`.
- `System.Linq.Enumerable`: the query operators and aggregates.
- `File` (also `ReadAllBytes`, `WriteAllBytes`), `StreamReader`,
  `StreamWriter`, `Environment.GetEnvironmentVariable`.
- Threads: `System.Threading.Thread` (`ThreadStart` and
  `ParameterizedThreadStart`, `Start`, `Join`, `Sleep`, `CurrentThread`,
  `ManagedThreadId`), `Interlocked` (`Increment`, `Decrement`, `Add`,
  `Exchange`, `CompareExchange`, `Read`), `Monitor` (`Enter`, `Exit`,
  `TryEnter`) and `SyncLock`, `Environment.ProcessorCount`.  Console output
  is thread-safe (a line is never mixed with another thread's).  While other
  threads run the garbage collector waits; it runs again when they have all
  ended.
- `CPUSim64.Process.Fork()` (0 in the child, the child's id in the parent),
  `Wait(pid)` and `WaitAll()`: processes with a copy of the memory, as the
  simulator's `iFORK` makes them (not a .NET API).

## How it works

| Part | Files |
|---|---|
| Main program, options | `src/vbc.c`, `src/vbc.h` |
| Lexer (logical lines, `#If`) | `src/lexer.c`, `src/keywords.inc` |
| Parser (recursive descent following `vb.g4`) | `src/parser.c`, `src/ast.c`, `src/nodes.inc` |
| Types, symbols and semantics | `src/types.c`, `src/sema_*.c` |
| Generics (monomorphization) | `src/sema_generic.c` |
| Lambdas, delegates, anonymous types | `src/sema_lambda.c` |
| Query expressions | `src/sema_query.c` |
| Code generation | `src/codegen.c`, `src/codegen_stmt.c` |
| Run-time library (C, compiled by C11) | `rt/vb_rt.c`, `vb_gc.c`, `vb_fmt.c`, `vb_strings.c`, `vb_io.c`, `vb_thread.c` → `rt/vb_rt.asm` |
| Library in VB, part of every program | `lib/System.vb`, `lib/Collections.vb`, `lib/Linq.vb`, `lib/Threading.vb` |
| Grammars for checking the parser with ANTLR | `grammar/` |

- Every value is one 64-bit word.  `Integer`, `Short`, `Byte` and the
  others are checked against their ranges; `Single` values are rounded to
  32 bits.  `Char` is a Unicode codepoint, `String` an object of codepoints.
- Objects have two header words (a type descriptor and the garbage
  collector's word).  Structures are values in boxes that are copied.
  Virtual methods use vtables, interfaces itables.
- Generic types and methods are instantiated for each set of type
  arguments.  A lambda is a method whose `Me` is an environment object of
  the captured variables.
- Exceptions use a chain of handler records; `Finally` runs on every exit.
  The head of the chain is a stack word, so every thread has its own.
- Threads run on the simulator's threads (the C11 library's `<_Thread.h>`);
  while they run, allocation is locked and the collector does not run.
  `SyncLock` is `Monitor.Enter`, then a `Try` with `Monitor.Exit` in its
  `Finally`.
- A mark-and-sweep garbage collector (precise for the heap, conservative
  for the stack) runs when the allocation since the last collection exceeds
  the live data.
- Code is generated only for the methods that can run, so the library
  costs a program only what it uses.

`plan.md` describes the design and the tasks in more detail.

## Building and testing

```
./build.sh             # vbc.asm and rt/vb_rt.asm, with the C11 compiler
./build.sh --rt        # only the run-time library
./build.sh --native    # obj/vbc-native, with the host C compiler (development)

tests/run_tests.sh [filter]    # the tests with the CPUSim64 compiler (about 4 minutes)
VBC=native tests/run_tests.sh  # with obj/vbc-native (about a minute)
SIM=java tests/run_tests.sh    # programs run on the Java simulator
GC_STRESS=1 tests/run_tests.sh run/   # a garbage collection at every allocation
tests/antlr_check.sh           # the parser against the ANTLR grammar
```

- `tests/dump/`: `--tokens`, `--syntax` and `--ast` output.
- `tests/errors/`: programs that must fail with a given message.
- `tests/run/`: programs that are compiled, run and compared with
  `.expected` (optional `.exit`, `.stdin`, `.argv`, `.stderr`, `.vbcargs`,
  `.mem`, `.nostress`); the source map must give the `.vb` lines.
- `tests/grammar/`: files that the parser and ANTLR must both accept
  (`ok/`) or both reject (`bad/`).
- `samples/`: must compile and assemble.

There is no .NET on the development machine: the expected output was worked
out by hand from the .NET documentation and checked against the program
output.
