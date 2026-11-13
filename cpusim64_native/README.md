# cpusim64_native

A native **Rust** port of the CPUSim64 virtual-CPU **interpreter / simulator**
(the execution half of the Java `cpusim64` module — *not* the ANTLR assembler or
preprocessor).

It loads pre-assembled `.o64` object files (gzip-compressed big-endian `u64`
words) plus their `.sym1` reverse-label maps and executes them, reproducing the
Java `Simulator` / `Simulation` behavior: all 39 opcodes, the full ~130-interrupt
syscall surface, endian-aware port I/O, file I/O, the Fibonacci-block heap
allocator, the fork/thread/shared-memory concurrency model, exact
`printCPUState` / dump / trace formatting, and CPU cycle accounting.

## Layout

```
cpusim64_native/
├── Cargo.toml
├── src/
│   ├── main.rs            CLI entry (≈ Simulation.java)
│   ├── lib.rs
│   ├── object.rs          .o64 gzip-BE reader + .sym1 loader (≈ AsmIO)
│   ├── opcode.rs          opcode table (≈ Opcode.java)
│   ├── decode.rs          instruction decode + encode helpers (≈ Decoded)
│   ├── util.rs            decodeSI, build info, asinh/acosh/atanh
│   ├── cpu/
│   │   ├── mod.rs         Simulator state, flags, memory, loadProgram
│   │   ├── memory.rs      shared heap (atomic) + shared-memory globals
│   │   ├── exec.rs        all 39 opcode handlers + run loop + stepOne
│   │   ├── cycles.rs      instructions.data extra-cycle table
│   │   ├── heap.rs        Fibonacci alloc/realloc/free/coalesce/walk
│   │   ├── portio.rs      port registry + IN/OUT glue
│   │   ├── format.rs      printCPUState / dumps / disassembly
│   │   ├── interrupts.rs  full ~130-interrupt dispatch (≈ StdInterruptHandler)
│   │   └── concurrency.rs fork/thread/join/wait/wake (≈ Simulator threads)
│   ├── ports/mod.rs       stdio/file/raw port handlers, endian + pushback
│   └── javacompat/
│       ├── mod.rs         String.hashCode / compareTo
│       ├── format.rs      Java Formatter: %d/%x/%e/%f/%g/%s/%c, MessageFormat
│       ├── schubfach.rs   Java-exact Double.toString digits (Schubfach)
│       └── regexadapter.rs java.util.regex subset via the `regex` crate
└── tests/validation.rs   runs validation/*.asm vs reference/*.out
```

## Build & run

```bash
cargo build --release
# Run an object file the way debug.sh does:
./target/release/cpusim64_native path/to/prog.o64 --verbose --debug --mem=1M
```

CLI flags mirror the Java `Simulation`: `--debug`, `--trace`, `--verbose`,
`--mem=<SI>`, `--stack=<SI>` (SI suffixes K/M/G/… are decimal, e.g. `1M` =
1,000,000 words, matching `Utils.decodeSI`).

## Validation

```bash
cargo test                      # unit tests + validation harness
cargo test --test validation    # just the reference comparison
```

The harness (`tests/validation.rs`) runs every
`../cpusim64/validation/*.asm` (via its generated `obj/<name>.o64`; run
`../cpusim64/validation/runtests.py` once to create them) with the same
flags `debug.sh` uses — `--verbose --debug --mem=16M --stack=8K --mem=1M` (the
last `--mem` wins) — and compares the region between the `Read N words …` line
and the `User CPU Cycles:` line against `../cpusim64/validation/reference/*.out`,
exactly as `runtests.py` does. It also checks the `User CPU Cycles:` value.

### Results

* **Output parity: 35 / 35 deterministic tests match the reference
  byte-for-byte** (the full `printCPUState` register/stack/float dump plus all
  program output).
* **Cycle parity: 35 / 35 match exactly** with the built-in cycle table.
* `T105_SYNC` (in `validation/bad/`) is **excluded** — see "Known deviations".

The references are produced by the Java simulator: run each test through
`debug.sh` with `USE_NATIVE=false` (which is what `runtests.py` does when
`debug.sh` uses Java) and save the output as `reference/<name>.out`.  They
must be regenerated whenever the system library (`system/*.asm`) changes
size, because the CPU dumps contain code addresses.

Regenerating the object files from source (optional) requires the Java toolchain
and mirrors `runtests.py`:

```bash
cd ../cpusim64/validation && ../../assemble.sh <name> --DEBUG
```

## Parity scope

Fully ported and behavior-matched against the Java reference:

* **Decode / opcodes** — all instruction types (tt 0–3), sign extension, and all
  39 handlers with Java-exact two's-complement / overflow semantics (including
  the pre-existing Java `MULT` overflow quirk for the immediate forms).
* **Flags** — P/Z/S/O including parity via population count and subtract
  overflow; `testCond` for all 11 condition codes.
* **Heap** — Fibonacci-rounded block sizes, prev/next/size free list with
  coalescing, `realloc`, `memmove`/`memclear`, length-prefixed UTF-8 strings.
* **Ports** — endian-aware multi-byte read/write with the pushback buffer and
  zero-extended values; stdio / text-file / raw-file handlers; `ENDIAN` toggle.
* **Interrupts** — constants, math (incl. `asinh`/`acosh`/`atanh`,
  `IEEEremainder`), heap ops, argc/argv, put/get (int/dec/hex/fp/string/line),
  `printf`/`sprintf`/`format`, file ops, and the string/regex family.
* **Formatting** — Java `Formatter` `%d/%x/%X/%e/%E/%f/%g/%G/%s/%c`,
  `MessageFormat`, `String.hashCode`, `String.compareTo`, and a port of the
  OpenJDK **Schubfach** `Double.toString` digit generator so that even subnormal
  floats (e.g. `Double.MIN_VALUE` → `4.9E-324`) render identically to Java.
* **Concurrency** — `thread`/`join`/`wait`, `fork` (copy-on-write heap),
  `allocShared`; the heap cells are shared via atomics so CAS-based spinlocks
  behave atomically across threads.

## Known deviations

These are documented, intentional differences surfaced during reconciliation:

1. **`T105_SYNC` is excluded from validation.** It is a deliberate data-race
   test: its *unsynchronized* half races 20 threads on a shared counter, so its
   printed outcome (`RACE` vs `NO RACE`) is nondeterministic run-to-run — even
   under Java. The committed `reference/T105_SYNC.out` captured one particular
   run, so byte-comparing against it is unsound. The test source lives under
   `validation/bad/`. The underlying primitives (threads, join, atomic CAS) are
   implemented and run without hanging.

2. **Cycle timing table (`instructions.data`).** The built-in default
   extra-cycle table matches the committed
   `cpusim64/src/main/resources/instructions.data` exactly (`load:1`,
   `fdivide:19`, etc.), and `RECIP` is charged as `fdivide` — identical to
   `Simulator.java` (`case 16 -> addCycles(anyFP(d) ? "fdivide" : "divide")`,
   where `RECIP`'s FP operand makes `anyFP` true).

   The table can be overridden at runtime: if a file named `instructions.data`
   sits in the **same directory as the executable**, its `key:cycles` lines are
   parsed (skipping blanks and `#` comments) and overlaid on the built-in
   defaults (mirroring how Java loads the resource). This lets you retune
   timings without recompiling.

   The reference `.out` files are generated with the same default table, so
   the validation harness checks cycle counts without any override; a separate
   test installs an override file (`load:0`, `fdivide:9`) to check that the
   override mechanism changes the timings.

3. **Regex.** `java.util.regex` is approximated with the Rust `regex` crate.
   Constructs that crate does not support (backreferences, lookaround) surface
   as an error rather than silently differing; none of the current validation
   scripts rely on them.

4. **Nondeterministic/host interrupts.** `iRANDOM`/`iRAND`/`iCLOCK` are
   deterministic placeholders (wall-clock is stripped by the harness anyway);
   `iSYSTEM` and temp-file/dir interrupts are unsupported in the native port.

## License

Apache-2.0, matching the upstream CPUSim64 Java sources.
