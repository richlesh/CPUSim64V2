# C++ versions of the documentation projects

The programs in `cpusim64/src/main/resources/documentation/projects/*.asm`,
written in C++14 for the CPUSim64 C++ translator.  Each `NAME.cpp` does what
`NAME.asm` (and `compilers/C11/projects/NAME.c`) does and prints the same
output.  They use iostreams, `std::string`, `std::vector`, classes and RAII
where that is natural, and `printf` from `<cstdio>` where the output is a
C-style format (`%.6f`, `%016lX`) or comes from several threads.

```
../obj/cppc-native Fibonacci.cpp                # or: ../../../bin/cpusim64_native ../obj/cppc.o64 Fibonacci.cpp
cd ../../C11 && ../../bin/cpusim64_native obj/cc.o64 ../C++14/projects/Fibonacci.gen.c && cd -
mv Fibonacci.gen.asm Fibonacci.asm
../../../assemble.sh Fibonacci
../../../bin/cpusim64_native obj/Fibonacci.o64
```

or open `NAME.cpp` in a C++14 window of the IDE and Run it.

## Tests

```
./run_projects.sh [name ...]       # translate, compile, run, and compare with the reference
SIM=java ./run_projects.sh         # on the Java simulator
CPPC=native ./run_projects.sh      # with the native build of the translator (faster)
```

`run_projects.sh` is the C11 script (`compilers/C11/projects/run_projects.sh`)
with the translation step added.  It runs each program with the arguments of
`documentation/projects/gen_output.sh` and compares the output, the result
code and the files the program writes with `compilers/C11/projects/reference/`
(the output of the assembly versions), using the same checks for the
programs with random numbers, threads and processes.  See the C11
`README.md` for those checks.

- `expected/Pi.txt` is the C11 one: `printf("%.16f")` prints the exact decimal
  value of the double (`3.1415926535897931`), the assembly version Java's
  shorter form.
- The thread and process programs include `<thread.h>`, the C library's
  threads, `fork`, atomics and mutexes, declared for C++ in
  `compilers/C++14/lib/thread.h`.  `thread_create` takes a plain function
  pointer, so the thread functions are `static` functions, not lambdas.
  Data the threads share is global or on the heap: the main thread's stack
  is not shared (Hailstone4 and Hailstone5 allocate their cache with `new`).
- `CAS_printf_stress` serializes the printing with a mutex (a small RAII
  guard) unless `NO_PRINT_MUTEX` is defined, as the C11 version does.
- The console animations busy-wait on `clock()`; the tests set `NO_DELAY` to
  skip the delays.
- There is no `Hailstone5bad` (it has no assembly original).

## Translator problem found

`(a + b).c_str()` and other member calls on a parenthesized temporary
destroy a second temporary that was never constructed
(`std_string__dtor(&__t1)` after the call in the generated C), which crashes
in `free`.  The Mandelbrot programs name the string instead
(`std::string name = filename + ".pgm";`).
