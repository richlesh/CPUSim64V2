# IVT Plan: Vectored Interrupts, Control Registers and OS ROMs

Status: work items 1 to 11 are implemented on branch `feature/ivt`, except that the debugger part of item 11 is limited to showing the table base in the CPU state dump. Phase 3 (strict mode) is not started. See "Implementation notes" at the end for where the code differs from the text above.

## 1. Goal

Let an operating system written in CPUSim64 code (first target: an MS-DOS-style clone) install its own interrupt handlers, while the roughly 150 built-in simulator services stay available as "hardware ROM" routines. Existing programs keep working during the migration.

## 2. Current behavior

- `INTERRUPT n` (opcode 10, `op_interrupt` in `cpusim64_native/src/cpu/exec.rs`, and the Java `Simulator`) runs host service `n` entirely in the host and continues. `n > 0` dispatches, `n == 0` is a no-op (also the result of a false conditional), `n < 0` raises "Illegal INTERRUPT arguments".
- `RETURN` does `SP <- SF; pop SF; pop PC`. It does not touch `sr`. A `RETURN` to a negative PC ends the process with that value as the exit code.
- Host services do not modify `sr`, `SP` or `SF` (no references in `interrupts.rs`).
- CPU state: r0..r31 (r29 = SF, r30 = SP, r31 = PC), f0..f31, `sr` (P, Z, S, O flags). The stack grows downward; `PUSH` stores at `[SP]` and then decrements.

## 3. Decisions

### 3.1 Interrupt numbering

- `INT -n` runs built-in host service `n` (the "ROM" routines). It never consults the vector table.
- `INT 1..255` calls through the vector table. Phase 1: an empty entry falls through to the host service with the same number, so existing programs run unchanged. A vector installed by an OS overrides that number.
- `INT 0` remains a no-op. Table entry 0 is the process-termination hook (3.6).
- Positive numbers above 255 are an error.

### 3.2 Vector table

- 256 words of ordinary RAM. Entry `n` holds the word address of the handler, 0 = not installed.
- Read in place at each `INT` (`memory[ivt_base + n]`), so an OS patches vectors with a plain `STORE` (DOS get/set-vector, chaining, TSR hooks).
- A negative (shared-memory) base is rejected. A base with `base + 255` out of range is rejected.

### 3.3 Control registers and new instructions

- `ivt_base` is hidden simulator state next to `sr`, not a general register. 0 means no table.
- `LDCR rd, n` and `STCR n, rs` read and write control register `n`. Register 0 = IVT base. Other numbers are reserved (future: interrupt enable, timer period, privilege). A write to an undefined number is an error.
- `IRET` returns from an interrupt (3.4).
- The ISA grows from 41 to 44 opcodes, following the same process as `LDB`/`STB` (opcodes 39 and 40).
- No privilege checks, as in DOS. A privilege bit can be added to a control register later.

### 3.4 Entry and exit sequence (every INT, negative or positive)

- Entry: push the return PC, then `sr`, then `SF`; set `SF = SP`.
- Positive `INT`: `PC <- memory[ivt_base + n]`. Negative `INT`: run the host service.
- Exit (`IRET`): `SP <- SF`; pop `SF`; pop `sr`; pop PC. For a host service the simulator runs the same pop sequence after the service finishes. `SP` is not pushed; it is restored through `SF`.
- Registers r0..r28 are not saved. Arguments and results travel in registers, as for host services today. A handler preserves whatever it uses beyond the documented outputs.
- Handlers run on the interrupted code's stack, which needs 3 free words per `INT`. Because the frame matches `CALL`, `#call` and function macros should work inside handlers (**Verify** against the macro layer).
- Caller-visible contract for any `INT`: afterwards PC is the next instruction, and `SP`, `SF` and `sr` are as before. A handler installed at vector `n` can therefore replace or wrap `INT -n` transparently.

### 3.5 Cycle model

- Pushes and pops are charged with the normal rule (1 cycle per memory word): 6 cycles per `INT` round trip, plus the usual fetch for `INT` and for an `IRET` instruction.
- `interrupt` and `interrupt1` in `instructions.data` become 0 (users can raise them for extra cycles). `interrupt0` is unchanged.
- Cycle totals of every program using host services change. Regenerate the validation reference outputs. Java and native totals must match exactly.
- `LDCR`, `STCR` and `IRET` get timing entries in `instructions.data` and in the built-in default table.

### 3.6 Process termination and INT 0

- When the process would end (host `iExit`, or a `RETURN` to a negative PC, which is how `__MAIN__` programs exit with a value) the simulator checks `memory[ivt_base + 0]`.
  - 0: terminate as today.
  - Nonzero: do not terminate. Put the exit code in r0 and jump to that handler. The OS handler restores the `SP` and `SF` saved by its loader and continues with the value.
- Main thread only. A worker thread ending through its sentinel return is unchanged.
- **Verify** that process exit has a single place for the hook, and check the interaction with `atexit` in the C11 runtime.

### 3.7 Threads and fork

- The thread and fork implementations stay as they are.
- Parent: the `INT` pushes its frame, the service creates the thread (or child), and the epilogue pops the frame and returns the PID in a register.
- New thread: starts fresh at the thread function with its own stack and ends with an ordinary `RETURN` to the negative sentinel.
- `fork` child: a copy taken after the push. It returns through the same epilogue with its own return value.
- `ivt_base` is copied into new threads and forked children. Whether a later `STCR 0` is per-thread or process-wide is open (**Verify** how per-process state is stored; `free_list` is an example of a shared `Arc`).

### 3.8 Migration of existing code

1. Phase 1 (additive): add negatives, the table, `LDCR`/`STCR`/`IRET`. Positive numbers fall through to the host when no handler is installed.
2. Phase 2: change the values in `system/system.def` and `system/io.def` to negative numbers and reassemble. Find hard-coded literals by grepping `.asm` files, compiler output templates and the cached modules in `~/.cpusim64/modules/`.
3. Phase 3 (optional): strict mode, where an empty vector is an error. An OS ROM enables it.

## 4. OS ROM boot (Option A)

- The ROM is an ordinary module (`assemble.sh -r`, `link.sh`), started by a new `--rom=` option (or linked ahead of the program) and run before the OS main.
- Start-up code fills a 256-word RAM block with handler addresses using a macro, then installs it (pseudo-syntax; check the real macro and store forms):

      #def_macro SET_VECTOR(n, handler)
          move   r1, handler
          store  r1, IVT[n]
      #end_def_macro
      rom_boot:
          #macro SET_VECTOR(0x21, dos_int21)
          #macro SET_VECTOR(0, dos_exit)
          move   r1, IVT
          STCR   0, r1

- A DOS-style vector such as 21h has one handler that dispatches on a function register through an internal jump table. Each handler ends with `IRET`.
- A handler reaches built-in services with `INT -n`.
- The ROM image can use `protected_memory` ranges for read-only code. The table itself stays writable.
- Static initialized data tables (Option B) need label addresses in data words and relocation support. Deferred.
- The boot-from-disk-image flow and the DOS clone itself (FAT image or host pass-through filesystem, COMMAND.COM equivalent, loader, PSP) are separate follow-on work.

## 5. Work breakdown

1. Verification spike (read-only): free opcode values and operand forms; `INT -5` literal assembly; `CALL`/`RETURN`/`#call` frame setup; thread-spawn and fork state copying; the process-exit path; fork, thread and exit services against the frame contract.
2. Design doc updates and Instruction Set Reference changes.
3. Assembler: `LDCR`, `STCR`, `IRET`, negative `INT` literals, grammar, encoder, disassembler, syntax highlighter keywords.
4. Rust simulator: `ivt_base`, vectored `INT`, host-service frame, `IRET`, `LDCR`/`STCR`, exit hook, cycle table.
5. Java simulator: the same, with parity tests (printCPUState, traces and cycles identical).
6. Tests: handler install, call and return; host service called from a handler; `sr`/`SP`/`SF` preserved across a sample of host services; nested `INT`s; exit hook; fork and thread; empty-vector fall-through; out-of-range base.
7. Regenerate the validation references. Run the Java tests, `cargo test`, and the C11 and C++14 suites on both simulators.
8. Documentation: Instruction Set Reference, Interrupt Reference, Architecture, Cycle Timing, Programmer Guide, README opcode count.
9. Phase 2 migration of `system.def` and `io.def`.
10. OS ROM prototype: a vector 0x21 handler, and a loader that runs a program and receives its exit value.
11. Debugger: show `ivt_base`, merge ROM symbols into the source map, step into handlers.

## 6. Risks

- Cycle totals change for every host-service program, so reference outputs must be regenerated.
- Fork and thread creation must produce the same frame on both sides.
- The exit hook and `atexit` ordering.
- Hard-coded interrupt numbers in generated code or cached modules during Phase 2.
- Per-thread versus per-process `ivt_base`.
- The checked-in `lib/cpusim64-2.12.0.jar` is stale and does not assemble `LDB`/`STB`. Rebuild before validation.

## 7. Open questions

- Free opcode values in the encoding.
- Number of control registers to define now (proposal: only register 0).
- Per-thread or per-process `ivt_base`.
- Whether to add a second argument to bound the table size later.
- Executable format for programs under the OS (`.o64`, or a new header with relocations).

## 8. Implementation notes (Phase 1)

Findings from the verification spike and how the code differs from the design above.

- **Host-service frame is virtual.** Sections 3.4 and 3.7 say a host service gets the same 3-word frame as a handler. That cannot work as written. `iSAVE`, `iSAVE_FP`, `iRESTORE` and `iRESTORE_FP` change `SP` on purpose, and `iPRINTF`, `iCOND_PRINTF` and the other stack-argument services read their arguments relative to the caller's `SF` (`[SF+3]` and up). A pushed frame would hide both. The simulators therefore charge the frame's 6 cycles for a host service but do not push it. The contract in 3.4 (PC advances, `SP`, `SF` and `SR` unchanged) still holds for everything except the four save/restore services, which keep their stack effect. Because nothing is pushed, fork and thread creation from a host service need no epilogue, and the exit hook (3.6) needs no frame handling.
- **Frames are real for vectored handlers.** `INT n` with an installed vector pushes PC, `SR`, `SF` and sets `SF = SP` exactly as in 3.4. `IRET` pops them. A handler can call fork or thread services; the copied stack makes the child return through its own `IRET`.
- **Numbers above 255.** Section 3.1 says positive numbers above 255 are an error, but built-in services such as `iPUT_NL` (200) and `iSTRCAT` (355) are numbered above 255 until Phase 2 renumbers them as negatives. In Phase 1 `INT n` with `n > 255` runs host service `n` directly. Enforce the error after Phase 2.
- **Vector reads are free.** Reading table entry `n` is not charged a memory cycle, so an `INT` costs 6 cycles as in 3.5.
- **IVT base range.** `STCR 0, r` accepts 0 or a word address with `0 <= base <= heap_limit - 256` (heap-range RAM, not the per-thread stack).
- **INT operand.** The assembler now range-checks the constant of `INT` as signed 12 bits, so `INT -5` assembles in the short form. Constants from 2048 to 4095 now use the long form; before, they were encoded in the short form and decoded as negative.
- **Exit hook.** The hook is not disarmed when it fires. The OS handler must clear vector 0 before its final exit, or a second exit re-enters the hook. In the C11 runtime, `exit()` (`lib/stdlib.c`) runs the `atexit` functions and then `_Exit` executes `iEXIT`, so the `atexit` functions run before the hook sees the exit.
- **Conditional INT.** The assembler has no syntax for a conditional `INT`; the encoded forms (ZR, ZC) behave as before and are covered by unit tests built from encoded words.
- **Installed copies.** An installed `bin/instructions.data` (copied from `cpusim64/src/main/resources/instructions.data` by `install.sh`) still holds `interrupt:10` and `interrupt1:10` until it is refreshed, which makes the native simulator disagree with Java on cycle totals.
- **Phase 2 done.** All 150 `#define iNAME N` values in `system.def`, `io.def`, `math.def` and `string.def` are now negative. Hard-coded positive literals in existing `.asm` files still work through the Phase 1 fall-through, so nothing had to be renumbered by hand. Programs, validation output and cycle counts are unchanged on both simulators. Enforcing "positive above 255 is an error" is left for Phase 3 because literals such as `INT 200` still exist in older generated `.asm` files.
- **Debugger.** The CPU state dump prints an `IVT:` line when the table base is nonzero (and nothing otherwise, so existing output is unchanged). Stepping into a handler needs no change, since `INT n` jumps to the handler like a call. Merging ROM symbols into the source map is not done.
- **OS ROM prototype.** `cpusim64/validation/T107_OS_ROM.asm` boots a vector table, installs an INT 21h handler (function 09h print string, 4Ch terminate), and a loader that regains control with its saved `SP`/`SF` when the program terminates.

