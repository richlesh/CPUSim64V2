# Implementation Plan: VB.NET -> CPUSim64 compiler (compilers/VBNET)

## Problem Statement

Add a compiler plug-in that translates VB.NET source (`.vb`) into CPUSim64
assembly, so VB.NET programs can be run and debugged in the IDE on both
simulators.  The compiler follows the dotnet/vblang `spec/vb.g4` grammar (VB
spec v11).  It is written in portable C and compiled by our C11 compiler,
like the MMIX translator.  It covers a broad VB.NET language, delivered in
phases, with a small .NET base class library and a garbage collector.

## Requirements

- Grammar: dotnet/vblang `spec/vb.g4` (copy in `grammar/vb.g4`).
- Scope: broad VB.NET: classes, generics, collections, lambdas, LINQ, a small
  .NET base class library.
- Parser: hand-written recursive descent in C following the grammar rules;
  ANTLR parses the same test corpus on the host to check that both parsers
  accept and reject the same inputs (`tests/antlr_check.sh`).
- Memory: mark-sweep GC in the run-time library.
- Tests: hand-checked `.expected` files (no .NET on the development machine).
- Staging: four phases, each ending in a working compiler.
- Excluded: Async/Await, Iterator/Yield, reflection, threading, XML
  literals, `My.*`, WinForms, late binding (Option Strict Off on Object),
  On Error/Err, Decimal, Date.

## Design

Pipeline: `.vb` files -> lexer (logical lines, contextual keywords) ->
parser (recursive descent per vb.g4, AST) -> semantic analysis (names,
overloads, conversions, generic instantiation) -> lowering (closures,
queries, Try, For Each, properties, events) -> code generation (stack
machine, `.source` lines) -> `prog.asm` + `rt/vb_rt.asm` + `lib/*.vb` ->
assemble/link -> Java or native simulator.

1. Front end: logical lines (`_` and implicit continuation), case-insensitive
   and contextual keywords, all literal forms, type characters, comments,
   `#Const`/`#If`/`#Region`.  Debug options `--tokens`, `--ast`,
   `--parse-only`.
2. Semantics: scoped symbols, VB widening/narrowing conversions, Option
   Strict/Explicit, simplified overload resolution, type inference, generics
   by monomorphization.
3. Values: Integer/Short/Byte/Long in 64-bit words with range checks
   (OverflowException); Single maps to Double; Char is a codepoint; String is
   an immutable heap object (length + one codepoint per word); structures are
   multi-word values; boxing allocates.  `Double.ToString` is .NET's
   shortest round-trip format.
4. Objects: two header words (type descriptor, GC word).  Type descriptors
   hold parent, vtable, itables, name and the reference map of the fields.
5. Exceptions: handler stack (setjmp pattern), `Finally` lowered for normal
   exit, Exit/Return and exceptions; run-time errors are exceptions.
6. GC: precise for heap objects, conservative for the stack and globals,
   `--gc-stress`.  Own arena, see the spike below.
7. Library: `rt/vb_rt.c` (C, compiled by C11) for GC, strings, exceptions and
   I/O; `lib/*.vb` for the .NET subset, compiled only when used.
   Whole-program compilation of all named `.vb` files.

## Task 1 spike results

### Grammar

`vb.g4` is a documentation grammar: every rule except `start` is a lexer
rule (capitalised names) and it has 74 placeholder literals such as
`'<Unicode classes Lu,Ll,Lt,Lm,Lo,Nl>'`.  ANTLR 4.13.2 accepts it with ~40
"token unreachable" warnings, but the result cannot parse VB.  We use the
fallback: derived grammars `grammar/VBLexer.g4` and `grammar/VBParser.g4`,
written with the lexer (Task 2) and the parser (Task 3).  The full cleanup
list is in `grammar/README.md`.

### GC allocation

The simulator heap (`iALLOC`/`iFREE`, `cpusim64_native/src/cpu/heap.rs` and
the Java port) is first-fit over one free list, with a 3-word block header and
block sizes rounded up to Fibonacci numbers (minimum 8 words).  The C11
`malloc`/`free` call it directly.

Measured with a C11 program that allocates N blocks of 4 words, frees every
other one, then allocates blocks of 10 words:

| N blocks | simulator | total time |
|---------:|-----------|-----------:|
|   20,000 | native    |    0.034 s |
|  200,000 | native    |    1.447 s |
|  200,000 | Java      |    0.160 s |

The native simulator grows superlinearly with the number of free blocks
(its free list is a `Vec` with `insert(0)` and a linear `remove`), and the
Fibonacci rounding wastes up to 38% for small objects (a 6-word object takes
a 13-word block).

Decision: the GC manages its own arena.
- It takes large chunks (e.g. 64K words) from `iALLOC` and divides them into
  pages of one size class each (1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64,
  ..., 256 words); free cells are on a per-class free list, so allocation and
  sweeping are O(1) per object.
- Each page has an object-start bitmap and a mark bitmap; the conservative
  root test finds the chunk by binary search in a sorted chunk table and
  then tests the bitmap.
- Objects over 256 words get their own `iALLOC` block, kept in a sorted
  table for the root test and released with `iFREE` when unmarked.
- Until Task 12 the run time uses a bump allocator over the same chunks.

## Task Breakdown

Phase 1: core language
- [x] Task 1: plug-in skeleton, build pipeline, test harness, `entry_pattern`
  IDE key, grammar spike and GC allocation spike.
- [x] Task 2: lexer (`src/lexer.c`, `--tokens`, `--define`; conditional
  compilation, implicit and explicit line continuation, all literal forms).
- [x] Task 3: parser (`src/parser.c`, `src/ast.c`, `--ast`, `--parse-only`).
  It covers the whole syntax that is in scope, not only the core: classes,
  interfaces, properties, events, operators, generics, lambdas, queries and
  interpolated strings.  `grammar/VBLexer.g4` + `grammar/VBParser.g4` and
  `tests/antlr_check.sh` check it against ANTLR (82 files agree).
- [x] Task 4: types, symbols and semantic checks (`src/types.c`,
  `src/sema_*.c`, `--ast` now prints the typed tree, `--syntax` the parse
  tree).  Modules, methods, fields, constants (folded, with forward
  references), locals, the VB conversion classes and Option Strict /
  Explicit / Infer / Compare, the operator type tables, overload resolution
  (Optional, ParamArray, named arguments, ByRef copy-back), arrays and array
  literals, For / For Each inference, Select Case, Exit / Continue / GoTo
  checks, the builtin members of Console (all .NET overloads), String.Length,
  array Length and the MaxValue/MinValue constants.  Classes, Try, With,
  lambdas and the other later features are reported as "not supported
  yet".
- [x] Task 5: code generation (`src/codegen.c`, `src/codegen_stmt.c`) and
  the run-time library (`rt/vb_rt.c`, compiled to the module
  `rt/vb_rt.asm`).  Stack-machine code with `.source` lines (the test
  harness checks the source map), checked integral arithmetic
  (OverflowException, DivideByZeroException), the VB conversions (CInt
  rounds to even, Single values rounded to binary32 at compile and run
  time), .NET's shortest round-trip Double/Single formatting, boxing,
  composite formats ({0,5}), interpolated strings, Console.Write/WriteLine/
  ReadLine, command line arguments, Function Main's exit code, and
  unhandled exceptions ("Unhandled exception. System.X: message", exit
  code 1).  Most of Task 6 is in place too: calls with ByRef (and
  copy-back), Optional, ParamArray and named arguments, recursion, Static
  locals, all loops, Select Case, Exit/Continue/GoTo, 1-D, multi-dimensional
  and jagged arrays, array literals, ReDim [Preserve], Erase.
- [x] Task 6: procedures, arrays and flow control (done with Task 5, see above; tests run/control, run/arrays).
- [x] Task 7: strings and the library functions: the String methods
  (Substring, IndexOf, Split, Replace, PadLeft, Trim, ...), String.Join/
  Format/Compare/IsNullOrEmpty, the Microsoft.VisualBasic functions (Len,
  Left, Mid, InStr, UCase, Chr, Asc, Split, Join, Val, Hex, Int, Fix,
  IsNumeric, Format, Rnd/Randomize with VB's generator, vbCrLf,
  ControlChars), Char tests, Parse/TryParse, Convert.ToXxx, Math, the Like
  operator, Environment.Exit/NewLine/GetCommandLineArgs, Char() <-> String,
  and the .NET format strings (D X F N E G P C R and custom formats,
  midpoints away from zero, invariant culture) in ToString(format),
  String.Format, interpolated strings and Format.  rt/vb_fmt.c and
  rt/vb_strings.c are included by rt/vb_rt.c.  The Random class waits for
  objects (Task 8).

Phase 2: object-oriented language
- [x] Task 8: enums, classes and structures (`src/sema_type.c`).  Objects
  are tagged with a type descriptor (filled in at start-up; array tags are
  negative now).  Enums: members with values (implicit +1), conversions to
  and from the underlying type, the bitwise operators keeping the enum,
  ToString/interpolation/String.Format giving the member name (VB's
  Console.WriteLine(e) prints the number: the Integer overload wins).
  Classes: instance and Shared fields, methods, overloaded constructors (an
  implicit Sub New, field initializers run first), Shared Sub New, regular,
  indexed, ReadOnly/WriteOnly and auto-implemented properties (backing field
  _Name), Me, New with object initializers, Private access, ReadOnly fields,
  nested types (Outer+Inner names), Shared Sub Main in a class, the cast
  Object -> class.  Structures: values in boxes that are copied on
  assignment, ByVal, Return and boxing (vbrt_struct_copy, deep for nested
  structures), zero values for locals, fields and array elements.
- [x] Task 9: inheritance, interfaces and namespaces.  Inherits (fields of
  the base first, members and overloads through the base chain, Overloads),
  MustInherit/NotInheritable, Overridable/Overrides/NotOverridable/
  MustOverride with vtables (slots 0-2: ToString, Equals, GetHashCode, so
  that the run time's ToString and Equals call the overrides), MyBase and
  MyClass (direct calls), MyBase.New / Me.New as the first statement (else
  the implicit MyBase.New()), Protected, interfaces with Inherits and
  Implements clauses (methods and properties, on classes and structures),
  itables per class (with the overrides), TypeOf ... Is/IsNot, TryCast,
  DirectCast and CType with run-time checks over the base chain and the
  interfaces, Partial classes (parts in several files), operator
  overloading (unary, binary, Widening/Narrowing CType), Object.Equals/
  GetHashCode/ReferenceEquals, and programs of several files with
  namespaces and Imports aliases.  The descriptor layout is in
  rt/vb_rt.c.
- [x] Task 10: exceptions.  lib/System.vb (part of every program, found next
  to rt/ or with --lib=DIR) has System.Exception and the standard exception
  classes with the .NET messages; the run-time library's errors become
  these exceptions (VBRuntimeHelpers.MakeException).  Try/Catch (typed,
  When filters, catch variables)/Finally, Throw and rethrow, Exit Try.  Try
  pushes a 4-word handler record (catch address, SP, SF, previous) on the
  chain __vb_handler; __vb_throw pops it and jumps.  Exit, Continue, Return
  and GoTo out of a Try go through its Finally (a reason code and a
  dispatch at the end of the Finally).  An unhandled exception prints
  "Unhandled exception. " and its ToString() (exit code 1).  Every program
  now contains the exception classes (hello.asm: 2,815 lines); compiling
  only the used library code is for Task 14.
- [x] Task 11: resource and iteration statements, rewritten by the semantic
  analysis into blocks (N_BLOCK), hidden locals ($t1, ...) and Try/Finally:
  For Each over collections (the GetEnumerator/MoveNext/Current pattern or
  System.Collections.IEnumerable, the enumerator disposed if it implements
  IDisposable), Using (declarations or an expression, several resources,
  disposed in reverse order, also on exceptions), With (nested, on
  structure variables in place), SyncLock (evaluated; single-threaded).
  System.IDisposable, IEnumerable and IEnumerator are in lib/System.vb.
  Continue, IsNot, If(a, b), Like and Static locals were done earlier.

Phase 3: GC, generics and library
- [x] Task 12: garbage collector (rt/vb_gc.c): mark and sweep, not moving.
  64K-word chunks from the simulator heap, 4096-word pages of one size
  class (4 .. 256 words), larger objects in blocks of their own; per-class
  free lists, empty pages reused by any class.  Marking is precise in the
  heap (descriptors list the reference fields: d[9], d[10]; array tags tell
  whether elements are references) and conservative for the roots (the
  whole used stack, including the library's C frames, and the #global data
  between __vb_gstart and __vb_gend; interior pointers count).  A
  collection runs when the allocation since the last one exceeds the live
  size (at least 1M words) or memory runs out.  GC.Collect,
  GC.GetTotalMemory, GC.CollectionCount(0).  --gc-stress collects at every
  allocation; tests/run_tests.sh with GC_STRESS=1 runs the run tests that
  way (all pass).  run/gc allocates twice its 2M-word memory (gc.mem).
- [x] Task 13: generics, by monomorphization (src/sema_generic.c).  Generic
  classes, structures, interfaces and methods are templates that are never
  analyzed; each use with type arguments instantiates a copy of the
  template's tree in which the type parameters name the argument types.
  Instances are cached and catch up with the passes (advance() in
  sema_main.c, the method bodies in a work loop).  Arity overloading (Foo,
  Foo(Of T), and imported names chosen by arity), constraints (Class,
  Structure, New, types; primitives satisfy IComparable(Of T) and
  IEquatable(Of T)), type inference for generic methods (T, T(), Foo(Of T)
  through base classes and interfaces, the dominant type), explicit type
  arguments, generic methods in generic types, .NET names
  (Stack`1[System.Int32]).  lib/System.vb adds IComparable, IComparable(Of T),
  IEquatable(Of T), IEnumerable(Of T), IEnumerator(Of T); the primitives have
  CompareTo and Equals.  Not supported: Overridable and interface generic
  methods, Partial generic types, Nullable(Of T) (Task 14), variance.
- [x] Task 14: base class library, in VB (lib/Collections.vb, with
  lib/System.vb part of every program) on run-time helpers in C (rt/vb_io.c
  for files).  List(Of T), Dictionary(Of K, V) (the .NET entry layout:
  enumeration in insertion order, freed entries reused), KeyValuePair,
  HashSet, Stack, Queue (the .NET messages, modification checks during
  enumeration), StringBuilder, Random (the .NET subtractive generator: seeded
  sequences as in .NET), Array.Sort/Reverse/IndexOf/BinarySearch/Resize/
  Copy/Clear, Tuple, String.Join/Concat of IEnumerable(Of T) (a library
  class System.<Type>_Members extends a built-in type), File, StreamReader,
  StreamWriter, FileNotFoundException.  The language: Default properties,
  collection initializers (From {...}), arrays as IEnumerable(Of T) (through
  ArrayWrapper(Of T)).  Code is generated only for the methods that can run
  (reachable from Main, the field initializers, the library's callback and
  the virtual and interface methods of the types created), so the library
  costs a program only what it uses (hello.asm: 3,124 lines; compiling it
  takes 0.6 s on the native simulator).

Phase 4: functional features
- [x] Task 15: delegates, lambdas, closures and events (src/sema_lambda.c).
  Delegate types are classes derived from System.MulticastDelegate (fields
  target, method address, next) with a built-in Invoke that calls the whole
  invocation list; Delegate.Combine/Remove; Action, Func, Predicate,
  Comparison, Converter, EventHandler in lib/System.vb.  AddressOf (instance,
  Shared through thunks, virtual and interface targets, built-in members
  through a generated lambda), New D(AddressOf M), single- and multi-line
  Function/Sub lambdas, typed by the target delegate or as Func/Action, also
  in generic type inference (parameter types from what is known, result
  types from a tentative binding).  Closures: captured variables live in
  environment objects made per block or loop body (a fresh one per
  iteration), linked to the enclosing ones; Me is captured too.  Events:
  Event E(params) / Event E As D (field EEvent), AddHandler, RemoveHandler,
  RaiseEvent, Shared events, Handles Me.E, WithEvents fields with Handles
  (handlers moved when the field is assigned).  List and Array gained Find,
  FindAll, FindIndex, Exists, TrueForAll, RemoveAll, ForEach, ConvertAll and
  Sort(Comparison).
- [x] Task 16: inference and initializers.  Dim inference, object and
  collection initializers, NameOf and If(...) came with earlier tasks.  New:
  extension methods (<Extension()> on module methods, also generic and
  ByRef; the instance methods win, then the extension methods of the modules
  in scope and imports), Nullable(Of T) / T? in lib/Collections.vb (T -> T?,
  Nothing -> T?, T? -> T through Value, boxing to Object or Nothing,
  Is Nothing, If(n, default), & of T?; lifted arithmetic is not supported),
  ?. and ?( (the whole rest of the chain is skipped; a value type result
  becomes T?; also as a statement), anonymous types (New With {.A = x,
  Key .B = y, name}: a generic class per shape, made from generated source,
  with ToString "{ A = x, B = y }" and Equals/GetHashCode of the Key
  fields).
- [x] Task 17: LINQ.  lib/Linq.vb: System.Linq.Enumerable (Where, Select,
  SelectMany, Take, Skip, TakeWhile, SkipWhile, Distinct, Concat, Union,
  Intersect, Except, Reverse, OfType, Cast, OrderBy/ThenBy(Descending),
  GroupBy, Join, GroupJoin, ToList, ToArray, ToDictionary, ToHashSet, First,
  Last, Single, ElementAt (OrDefault), Any, All, Contains, Count,
  SequenceEqual, Sum, Average, Min, Max, Aggregate, Range, Repeat, Empty)
  with lazy iterators (Iterator(Of T): a fresh state per enumeration).
  Query expressions (src/sema_query.c) are lowered to these calls: From
  (several: SelectMany), Where, Select, Let, Order By, Distinct, Take/Skip
  [While], Join, Group Join ... Into, Group [items] By keys Into aggregates,
  Aggregate ... Into; several range variables travel as anonymous types.
  Overload resolution prefers the delegate whose return type is the
  lambda's own; inference follows interface bases and builds parameter
  types such as IEnumerable(Of TInner); imported extension methods are not
  promoted as plain names.
- [x] Task 18: release polish.  README.md for the plug-in, the full
  system_prompt.md for the AI chat, samples/ (Hello, Bank, Shapes, Linq,
  Guess; tests/run_tests.sh checks that they compile and assemble), the
  VB.NET section of the main README, the debugger's source lines for every
  file of a program (library code keeps its assembly lines), Random exactly
  as .NET for all seeds (32-bit wraparound of the seeding), version 1.0.
  The compiler needs no more than the default 16M/256K of the IDE (the
  largest test compiles with 0.7M words of heap in 0.7 s).
