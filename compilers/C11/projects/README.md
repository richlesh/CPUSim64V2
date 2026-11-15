# C11 versions of the documentation projects

The programs in `cpusim64/src/main/resources/documentation/projects/*.asm`,
written in C for the CPUSim64 C11 compiler.  Each `NAME.c` does what
`NAME.asm` does and prints the same output.

```
../../run.sh cc projects/Fibonacci.c           # from compilers/C11: writes projects/Fibonacci.asm
../../../assemble.sh Fibonacci                  # from projects/
../../../bin/cpusim64_native obj/Fibonacci.o64
```

or open `NAME.c` in a C11 window of the IDE and Run it.

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
- `Pi` prints `3.1415926535897931`: C's `printf("%.16f")` prints the exact
  decimal value of the double, while the assembly version prints Java's
  shorter form (`...930`).  `expected/Pi.txt` holds the C output.
- The programs with threads or processes (BBP_hex_digits_threaded,
  CAS_printf_stress, Hailstone4/5/5bad, mandelbrot*) use `<thread.h>`.  Their
  output order and the number of threads depend on the scheduler and the
  host, so the tests check only what cannot vary: the `Max ... found at ...`
  line, the `Position` lines, the counters and the complete set of printed
  lines, and (for the Mandelbrot programs) that the image file is identical
  to `reference/*.pgm`.  `Hailstone5bad` has no reference of its own and is
  compared with `Hailstone5`.
- `CAS_printf_stress` serializes the printing with a mutex.  The simulator
  writes a line character by character, so on the Java simulator lines
  printed by several threads at once get mixed; define `NO_PRINT_MUTEX` to
  see it.  (The assembly version prints without a lock unless
  `USE_PRINT_MUTEX` is defined.)
- The console animations (progress_bar, progress_bar2, working_bar, Spinner)
  busy-wait on `clock()` for their delays (the C library has no sleep); the
  tests set `NO_DELAY` to skip them.

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
| BBP_hex_digits_threaded | 100 blocks of pi digits, one thread per core, each taking the next block with an atomic counter |
| CAS_printf_stress | atomic counters and `printf` under load from one thread per core |
| Hailstone4, Hailstone5, Hailstone5bad | threaded Hailstone search: one share per thread (4), work units from an atomic counter (5) or from a locked stack (5bad) |
| mandelbrot, mandelbrot_binary | Mandelbrot image (text or binary PGM) made by one `fork`ed process per core |
| mandelbrot_histogram, mandelbrot_histogram_balanced | Mandelbrot image with histogram coloring, computed by threads (fixed bands, or one row at a time from an atomic counter) |
