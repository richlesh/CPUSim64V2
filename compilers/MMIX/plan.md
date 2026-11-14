# Implementation Plan: MMIXAL → CPUSim64 translator (compilers/MMIX)
  
##  Problem Statement
  
  Add a compiler plug-in that translates MMIXAL source (.mms) into CPUSim64 assembly, so MMIX programs run in the IDE and on both simulators. The
  translator is written in portable C and compiled to CPUSim64 by our C11 compiler. It covers full user-mode MMIX, and its output is checked against the
  real mmix simulator.
  
##  Requirements (your answers)
  - Location and language: compilers/MMIX, written in C and built with the C11 compiler.
  - Scope (2B): full user-mode MMIX:
    - integer and branch instructions, the full register stack, memory, GREG/LOC and the data directives;
    - TRAP I/O, and all floating-point operations;
    - the arithmetic exception flags and trips in rA, the rounding modes, and short floats.
    - Kernel features (virtual memory, rV, interrupts and timer registers) are excluded.
  
  - Verification (3A): differential testing against the real mmixal and mmix.
  
##  Background (what I found)
  - Source documents: Knuth's mmixal.w and mmix-sim.w are available as text from the toledo/mmix GitHub repository, so the language can be checked against
  the originals.
  - Program start-up:
    - execution begins at the label Main;
    - $0 holds argc and $1 points to the argv array;
    - $255 holds the address of Main;
    - rL starts at 2, and the global registers come from the GREG statements.
  
  - System calls: each TRAP 0,Fxxx,Handle call passes its arguments through $255:
    - a second argument goes in $255;
    - for three arguments, $255 points to an octabyte pair;
    - the result comes back in $255;
    - Halt exits with $255 as the status.
  
  - Test tools: brew install mmix provides mmixal, mmix and mmmix. The MMIXware samples (hello, fib, inout, copy, echo, harm and others) give a ready test
  corpus. mmixal -l writes a listing of every assembled word.
  - CPUSim64 mismatches (from the earlier evaluation):
    - memory is word-addressed;
    - there are 29 integer registers;
    - signed compare and truncating division only;
    - NaN compares as equal to everything;
    - no multiply-high;
    - .dci can't hold a label.
  
##  Proposed Solution
  
  flowchart LR
    S[prog.mms] --> M[mmixc: MMIXAL front end + translator, C compiled by C11]
    M --> A[prog.asm: register file, memory image, code, MMIX-to-code table]
    A --> R[mmix_rt.asm: runtime routines]
    R --> O[assemble + link, then Java or native simulator]
  
  1. Front end: a full MMIXAL assembler in C, producing the same words and data as mmixal.
  2. Translator: each MMIX instruction becomes a short CPUSim64 sequence, or a call to a runtime routine for the complex ones.
  3. Registers:
    - The register file lives in a memory array. Locals are a window at base register RB, and globals sit at fixed addresses.
    - Marginal registers are handled with an invariant that every register at or above rL is zero. Reads then need no check. A write checks rL only where
  a simple analysis can't prove the index is below it.
    - POP re-zeroes the callee's registers.
  
  4. Memory:
    - Each MMIX byte address maps to a word index plus a byte position, big-endian.
    - The four segments (text, data, pool, stack) fold into one contiguous range.
    - Sub-word loads and stores become shift and mask sequences.
    - The initial data image is built from .dci and .dca directives.
  
  5. Jumps: GO, PUSHJ, PUSHGO, returns through rJ, and GETA values use a table from MMIX address to code address, filled in by the generated __INIT__
  because .dci can't hold labels. Only address-taken instruction slots get an entry.
  6. Runtime: an mmix_rt.asm module exports the routines that don't fit inline:
    - MULU high word, 128-bit DIVU, floor division, MOR/MXOR, SADD, BDIF and friends;
    - rounding, short-float conversion and float flags;
    - trap I/O, with handle h mapping to port h (the same 0–255 range).
  
  7. Floating point: directed rounding uses error-free transformations plus a one-ulp adjustment, and the I/O/U/Z/X flags are detected by software checks.
  The oracle verifies each of these.
  8. Trips: when an enabled exception occurs, set rW, rX, rY and rZ, jump to the handler at #10 through #80, and make RESUME work.
  9. Portable-C rules:
    - Use explicit 64-bit types and masks.
    - Avoid byte-level assumptions: a C11 char is a 64-bit word, and an int can't be assumed to wrap at 32 bits.
    - Keep to the C11 subset we support: no bit-fields, VLAs or _Generic.
    - The translator can then be built natively with cc for fast development and compared with its CPUSim64 build.
  
  10. Plug-in files: config.properties (compiler_source: mmixc, file_extensions: mms), a system_prompt.md, and a build script producing a checked-in
  mmixc.asm. One small IDE change is needed: a config key so that Run is enabled when the file defines the label Main, since main_function is a C-function
  check.
  
##  Task Breakdown
  
  - [x] Task 1: plug-in skeleton, build pipeline and test harness.
    - Add compilers/MMIX with its config, a stub mmixc.c that prints the banner and Arguments: line, build.sh (C → C11 → mmixc.asm), and
  tests/run_tests.sh.
    - The tests in this harness use the oracle when it's installed, otherwise checked-in .expected files.
    - Add the IDE entry-label setting for Main with unit tests.
    - Install mmix with brew install mmix and record the versions.
    - Demo: File → New Compiler → MMIX opens a window, Run is enabled on a file with Main, and the stub compile runs.
  
  - [x] Task 2: MMIXAL front end.
    - Cover the lexer, expressions with MMIXAL's two precedence levels, symbols, local labels (1H/1B/1F), PREFIX, IS, LOC, GREG, LOCAL, BSPEC/ESPEC,
  BYTE/WYDE/TETRA/OCTA with their alignment rules, the predefined symbols, and the SET/LDA/GETA forms.
    - A --dump option writes the assembled words.
    - Tests: compare against mmixal -l listings for every corpus program.
    - Demo: identical listings for the MMIXware samples.
  
  - [x] Task 3: core integer translation.
    - Add the register file, memory model and start-up, all loads and stores, integer arithmetic and logic, shifts, the conditional set and zero-or-set
  families, branches, JMP, GETA, and TRAP Halt/Fputs/Fwrite.
    - Demo: hello.mms prints "Hello, world!" in the IDE on Java and native.
  
  - [x] Task 4: subroutines and the register stack.
    - Add PUSHJ, POP, PUSHGO, GO, rJ, the rL/rG rules with the zero-register invariant, GET/PUT of the common special registers, and the address-table
  initialisation.
    - Tests: fibsub.mms, fibgo.mms and recursion programs.
    - Demo: recursive subroutine programs run correctly.
  
  - [x] Task 5: full trap I/O.
    - Add Fopen, Fclose, Fread, Fgets, Fgetws, Fwrite, Fputs, Fputws, Fseek, Ftell, argc/argv start-up, and the Halt exit status.
    - Demo: copy.mms, echo.mms and inout.mms match the oracle.
  
  - [x] Task 6: exact integer arithmetic.
    - Add MULU with rH, DIV and DIVU with rD/rR, floor semantics, SADD, the BDIF family, MOR/MXOR, MUX, and the integer D/V flags in rA.
    - Tests: generated random and boundary programs compared with the oracle.
  
  - [x] Task 7: floating-point core.
    - Add FADD/FSUB/FMUL/FDIV/FSQRT/FREM/FINT, FCMP/FEQL/FUN with NaN handling, the epsilon forms with rE, and FIX/FLOT in nearest-even mode.
    - Tests: differential tests with special values.
  
  - [x] Task 8: rounding modes, float flags and short floats.
    - Add the four rounding modes via rA, the ROUND_* forms, the I/O/U/Z/X/W flags, and LDSF/STSF/SFLOT/SFLOTU.
    - Tests: randomised differential campaign against the oracle.
  
  - [x] Task 9: trips and remaining user-mode instructions.
    - Add trip enables and handler dispatch, TRIP, RESUME, SAVE/UNSAVE, the no-op prefetch and sync instructions, and the remaining special-register
  rules.
    - Unsupported kernel features (rV, interrupts, the timer registers) get a clear error or a documented zero.
    - Tests: trip-handler programs.
  
  - [x] Task 10: IDE and documentation polish.
    - Optionally add an MMIXAL syntax highlighter.
    - Emit .LINE directives (if the assembler accepts them in source) so the debugger shows .mms lines.
    - Add a system_prompt.md for the AI chat, document the translator options (segment size, a memory-requirement warning), and update the READMEs and
  plug-in docs.
  
  - [x] Task 11: final regression.
    - Run the full test suites and a long randomised differential run.
    - Measure translation time and run speed, and check that the checked-in mmixc.asm matches a rebuild.
  
##  Status

  All tasks are done.  Differences from the plan:
  - Floating point is not built on CPUSim64 doubles with error-free
    transformations: rt/mmix_rt.c ports Knuth's mmix-arith to 64-bit
    integers, so every result, rounding mode and flag is the reference one
    (2M random cases compared natively with mmix-arith, and the randomised
    whole-program comparison with mmix).
  - Instead of a per-instruction address table filled in by __INIT__, the
    generated code has a jump table (one JUMP per code tetrabyte); the
    initial memory image is .dci data copied by the run-time library.
  - Source lines for the debugger use the new .source / .source_end
    directives (not .LINE, which the preprocessor does not accept in
    source): the debugger shows the .mms lines.
  - No MMIXAL syntax highlighter (syntax_language stays text/plain).

##  Risks:
  - Translator size under the C11 compiler: it has never compiled a program this large. The native build, run in parallel, mitigates this.
  - Exact float flags: the X flag, the directed roundings and subnormal edge cases are the hardest part, and they depend on the oracle for verification.
  - Memory: the IDE's default heap is about 1M words, so large MMIX programs need a bigger Heap setting. The translator would warn.
  - Unsupported programs: programs that read their own code, or use virtual memory or interrupts, are not supported.
