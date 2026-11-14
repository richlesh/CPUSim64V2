# MMIX plug-in (MMIXAL to CPUSim64)

Runs programs written in MMIXAL, the assembly language of Knuth's MMIX
computer, on CPUSim64.  The translator `mmixc` assembles a `.mms` program
exactly as Knuth's `mmixal` does and writes `prog.asm`, a CPUSim64 program
that runs it.  The translator is portable C (`src/`), compiled by the
CPUSim64 C11 compiler into `mmixc.asm`, which the IDE assembles and runs
as this plug-in's compiler (File → New Compiler → MMIX).

## Use

    ../../assemble.sh mmixc                  # once: obj/mmixc.o64
    ../../bin/cpusim64_native obj/mmixc.o64 prog.mms [options]
    ../../run.sh prog                        # assemble and run prog.asm

In the IDE, Run is enabled for a file that defines `Main` in column 1;
the Options field takes the translator options.  As with the real `mmix`,
the program's arguments are `$0` (argc) and `$1` (argv, in the pool
segment); `argv[0]` is the source file name without its extension.  The
exit code is `$255` at `TRAP 0,Halt,0` (the real `mmix` returns its low 8
bits).

Options:

| Option | Meaning |
|---|---|
| `--data=N`, `--pool=N`, `--stack=N` | bytes of memory for the data, pool and stack segments beyond the program's own data (suffixes `K`, `M`; defaults 1M, 512K, 512K).  The stack segment holds the register stack. |
| `--rt=DIR` | directory of the run-time library (default: `rt/` of the plug-in) |
| `--dump` | print the assembled image instead of translating (for the tests) |

The default sizes need about 2 MiB (260K words) of CPUSim64 memory, which
fits the IDE's default 1M-word heap.  Larger `--data` values need a larger
Heap setting.

## Supported

All user-mode instructions: integer, logic, shifts, the conditional set and
zero-or-set families, branches, loads and stores of every size, `CSWAP`,
`GETA`, `GO`, the register stack (`PUSHJ`, `PUSHGO`, `POP`, `SAVE`,
`UNSAVE`, `GET`/`PUT` with the rules for `rL`), every floating-point
instruction with the four rounding modes, short floats, the epsilon
comparisons (`rE`), the arithmetic flags and trips (`rA`, handlers at
`#10`…`#80`, `TRIP` to location 0, `RESUME` after a trip), and the
MMIXware simulator's system calls `Halt`, `Fopen`, `Fclose`, `Fread`,
`Fgets`, `Fgetws`, `Fwrite`, `Fputs`, `Fputws`, `Fseek`, `Ftell` (handle h
is any of 0..255; `StdIn`, `StdOut`, `StdErr` are the console).

Not supported, reported at run time with a message on standard error:
kernel features (virtual memory, `LDVTS`, interrupts, the timer and
privileged special registers), `PUT rG` with a different value, `RESUME`
with an instruction to execute in `rX` (an `rX` whose sign bit is 0, as an
operating system would set it), programs that
execute data or code they stored themselves, and jumps outside the text
segment.  An illegal instruction stops the program with exit code 1 (the
real `mmix` stops and reports it on standard output).

## How it works

- `src/mmixal.c` is the assembler front end; `src/gen.c` writes the
  CPUSim64 program: each MMIX instruction becomes a short sequence labelled
  `M_<address>`, with a comment showing the instruction word and the source
  line.  `.source "prog.mms", N` directives give the assembler the source
  line of each instruction, so the IDE's debugger (Debug in an MMIX window)
  shows the `.mms` line that is executing and takes breakpoints set in the
  `.mms` file (stepping is still by CPUSim64 instruction).  The start-up code and the run-time library have no `.mms` line.
- The MMIX registers live in memory: the local registers are a window of the
  register stack (CPUSim64 `r27` points to `$0`, `r26` is `rL`), the global
  and special registers are arrays of the run-time library.  Local registers
  at and above `rL` always hold zero.
- MMIX memory is a block of the CPUSim64 heap holding the four segments;
  each MMIX octabyte is one 64-bit word, big-endian.  An address outside the
  allocated segments stops the program.
- Indirect jumps (`GO`, `PUSHGO`, `POP`, trips) use a jump table with one
  entry per tetrabyte of code.
- `rt/mmix_rt.c` (compiled to `rt/mmix_rt.asm`) is the run-time library:
  start-up, multiplication and division, the bit-matrix and saturating
  operations, the floating-point operations (a port of Knuth's
  `mmix-arith`), exceptions and trips, `TRAP` and `SAVE`/`UNSAVE`.
  `rt/mmix_io.asm` has the functions that need CPUSim64 instructions or
  interrupts.

## Build

    ./build.sh            # rt/mmix_rt.asm and mmixc.asm via the C11 compiler
    ./build.sh --rt       # only the run-time library
    ./build.sh --native   # obj/mmixc-native with the host cc, for development

## Test

    tests/run_tests.sh [filter]     # MMIXC=native: native translator; SIM=java: Java simulator
    tests/random_diff.sh SEED N LEN # N random programs of LEN instructions vs the real mmix
    tests/oracle.sh dump F.mms      # make F.dump from the real mmixal
    tests/oracle.sh run  F.mms      # make F.expected / F.exit from the real mmix

`asm/` tests compare the assembled image with the real `mmixal`; `errors/`
tests check diagnostics; `run/` tests translate, assemble and run programs
(Knuth's samples and tests of the register stack, trips and `SAVE`/`UNSAVE`)
and compare their output and exit code with the real `mmix`.  The oracle
needs Knuth's `mmixal` and `mmix` (for example `brew install mmix`).
