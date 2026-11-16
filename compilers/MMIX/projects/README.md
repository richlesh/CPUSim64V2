# MMIX versions of the documentation projects

The programs in `cpusim64/src/main/resources/documentation/projects/*.asm`,
written in MMIXAL for the MMIX translator.  Each `NAME.mms` does what
`NAME.asm` does and prints the same output.

```
../obj/mmixc-native Fibonacci.mms lib/std.mms     # or obj/mmixc.o64 on a simulator
../../../assemble.sh Fibonacci
../../../bin/cpusim64_native obj/Fibonacci.o64
```

or open `NAME.mms` in an MMIX window of the IDE, put the library files of
its `% Options:` line in the Options field (for example
`lib/std.mms lib/args.mms`) and Run it.

## Libraries

MMIXAL has no `include`; a library is a file that is given to the
translator after the program (see "Programs of several files" in
`../README.md`).  Each one starts with its own `PREFIX`, so its names are
`Std:Puts`, `Fmt:PutFixed` ...  A program lists the libraries it needs in
lines `% Options: ...` (more than one line is allowed); `run_projects.sh`
and `real_mmix.sh` read them.

| Library | Routines |
|---|---|
| `lib/std.mms` | `Std:Puts`, `PutNl`, `PutChar` (a code point as UTF-8), `PutDec`, `PutDecW` (right-aligned), `PutDecZ` (zero-filled), `PutHex`, `PutHexU` |
| `lib/args.mms` | `Args:ParseInt` |
| `lib/string.mms` | `Str:Len`, `Lower`, `Find`, `GetLine` (a line of standard input) |
| `lib/file.mms` | `File:OpenIn`, `OpenOut`, `UseStdIn`, `UseStdOut`, `Get`, `Put`, `CloseIn`, `CloseOut` (a byte at a time) |
| `lib/rand.mms` | `Rand:Seed`, `Range`, `Float` (a linear congruential generator) |
| `lib/array.mms`, `lib/farray.mms` | `Arr:Fill`, `Print`, `FArr:Fill`, `Print` (count in `[0]`) |
| `lib/fmt.mms` | `Fmt:PutFixed` (a double with N digits, the exact decimal value rounded half up), `Fmt:Parse` |
| `lib/math.mms` | `Math:PowI`, `Root`, `Cbrt`, `Cos` (a port of fdlibm's, the same bits as the C library), `Acos` |
| `lib/console.mms` | `Con:Init`, `Sleep`, `Sgr` (ESC [ n m), `HideCursor`, `ShowCursor`, `BarPercent`, `Bar` |

Calls follow the MMIX convention: `PUSHJ $X,Std:PutDec` with the
arguments in `$(X+1)`, `$(X+2)` ..., the result in `$X`.  The libraries
keep their data in the data segment at `Data_Segment+#10000` and above,
so a program uses the data segment below that.

## Tests

```
./run_projects.sh [name ...]       # translate, run, and compare with reference/
MMIXC=native ./run_projects.sh     # with the native build of the translator
SIM=java ./run_projects.sh         # on the Java simulator
REAL=1 ./run_projects.sh           # on Knuth's mmixal and mmix instead
./real_mmix.sh NAME [args ...]     # run one program on the real mmix
```

`run_projects.sh` runs each program with the arguments of
`documentation/projects/gen_output.sh` and compares the program output and
the result code with `reference/NAME.out` (the output of the assembly
version), and the files the program writes with the reference files.
A build fails on any error or warning of the translator, and on a source
line of 72 or more characters (`mmixal` drops the end of it).
`REAL=1` checks the same things with the real MMIX tools, so it shows that
a program is correct MMIX and that the translator runs it as `mmix` does.

- The sorting programs (BubbleSort, BubbleSortFloat, QuickSort,
  QuickSortFloat, InsertSorted) use random numbers, so their output is
  checked instead: the counts and ranges of the numbers, and that the sorted
  line is the random line in order.  MMIX has no clock: the numbers are the
  same in every run, unless a seed is given as the argument.
- `Pi` prints `3.1415926535897931`, the exact decimal value of the double,
  while the assembly version prints Java's shorter form (`...930`).
  `expected/Pi.txt` holds the MMIX output (the C11 version prints the same).
- `Hailstone3` keeps 100000 lengths (the assembly version 3000000), so that
  the table fits in the default data segment; the output is the same.
  `PI_spigot` accepts up to 20000 digits.
- The console animations (progress_bar, progress_bar2, working_bar,
  Spinner) pause between frames with a busy loop, as MMIX has no clock
  (`Con:Sleep`, calibrated for the native simulator).  The argument `-n`
  leaves the pauses out; the tests use it.

## Not ported

The programs that need threads or processes have no MMIX version, as
user-mode MMIX has neither: BBP_hex_digits_threaded, CAS_printf_stress,
Hailstone4, Hailstone5, mandelbrot, mandelbrot_binary,
mandelbrot_histogram, mandelbrot_histogram_balanced.

| Program | What it does |
|---|---|
| Fibonacci | the first 45 Fibonacci numbers |
| Renard, Renard2 | Renard series of preferred numbers |
| Pi | pi from the Bailey-Borwein-Plouffe series |
| PI_spigot | digits of pi with the Rabinowitz-Wagon spigot |
| BBP_hex_digits | hex digits of pi at any position (192-bit fixed point) |
| Congruent | congruent numbers with Tunnell's theorem |
| Quadratic, Cubic | roots of quadratic and cubic equations |
| Hailstone, Hailstone2, Hailstone3 | Collatz sequence lengths (Hailstone3 memoized) |
| Tower_of_Hanoi | the Tower of Hanoi, with the pegs printed |
| BubbleSort, BubbleSortFloat, QuickSort, QuickSortFloat, InsertSorted | sorting random arrays |
| Base64, Caesar, rot13, Vigenere, XORCipher | encoders and ciphers for files |
| hashcode, hashcode2, md5 | hashes of a file |
| holy_grail | the Bridge of Death questions |
| Unicode | Unicode character ranges |
| progress_bar, progress_bar2, working_bar, Spinner, rainbow | ANSI console output |
