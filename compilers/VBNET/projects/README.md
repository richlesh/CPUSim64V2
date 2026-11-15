# VB.NET versions of the documentation projects

The programs in `cpusim64/src/main/resources/documentation/projects/*.asm`,
written in Visual Basic .NET for the CPUSim64 VB.NET compiler.  Each
`NAME.vb` does what `NAME.asm` does and prints the same output.  The C
versions are in `compilers/C11/projects`.

```
../../../bin/cpusim64_native ../obj/vbc.o64 --mem=16M --stack=256K Fibonacci.vb   # writes Fibonacci.asm
../../../assemble.sh Fibonacci                                                       # from projects/
../../../bin/cpusim64_native obj/Fibonacci.o64
```

or open `NAME.vb` in a VBNET window of the IDE and Run it.

## Tests

```
./run_projects.sh [name ...]       # compile, run, and compare with reference/
SIM=java ./run_projects.sh         # on the Java simulator
```

`run_projects.sh` runs each program with the arguments of
`documentation/projects/gen_output.sh` and compares the program output and
the result code with `reference/NAME.out` (the output of the assembly
version), and the files the program writes with the reference files.

- The sorting programs (BubbleSort, BubbleSortFloat, QuickSort,
  QuickSortFloat, InsertSorted) use random numbers, so their output is
  checked instead: the counts and ranges of the numbers, and that the sorted
  line is the random line in order.
- `Pi` prints `3.1415926535897931`, the exact decimal value of the double
  (as .NET and C do); the assembly version prints Java's shorter form
  (`...930`).  `expected/Pi.txt` holds the output.
- `Unicode` leaves out the last block of the assembly version, the icons
  from U+1F300: they are outside the Basic Multilingual Plane, where `ChrW`
  fails (as in .NET) and this compiler's strings cannot hold a surrogate
  pair.  `expected/Unicode.txt` is the reference without that block.
- Integer arithmetic is checked in VB.NET, so the programs that need
  wrap-around arithmetic (BBP_hex_digits, hashcode, hashcode2) add in 32-bit
  halves with an explicit carry, and md5 masks its `Long` values.
- Without a file name hashcode, hashcode2 and md5 read the lines of
  standard input and hash them each followed by a newline.
- The console animations (progress_bar, progress_bar2, working_bar,
  Spinner) and CAS_printf_stress wait with `Thread.Sleep`; with the
  environment variable `NO_DELAY` set they do not wait, and the tests set
  it.
- The programs with threads or processes (BBP_hex_digits_threaded,
  CAS_printf_stress, Hailstone4, Hailstone5, mandelbrot*) use
  `System.Threading` and `CPUSim64.Process.Fork`.  Their output order and
  the number of threads depend on the scheduler and the host, so the tests
  check only what cannot vary: the `Max ... found at ...` line, the
  `Position` lines, the counters and the complete set of printed lines,
  and (for the Mandelbrot programs) that the image file is identical to
  `reference/*.pgm`.

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
| BBP_hex_digits_threaded | 100 blocks of pi digits, one thread per core, each taking the next block with `Interlocked.Increment` |
| CAS_printf_stress | `Interlocked` counters and `Console.WriteLine` under load from one thread per core |
| Hailstone4, Hailstone5 | threaded Hailstone search: one share per thread (4), work units from an atomic counter (5) |
| mandelbrot, mandelbrot_binary | Mandelbrot image (text or binary PGM) made by one forked process per core |
| mandelbrot_histogram, mandelbrot_histogram_balanced | Mandelbrot image with histogram coloring, computed by threads (fixed bands, or one row at a time from an atomic counter) |
