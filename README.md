![app_icon_256](ide/src/main/resources/app_icon_256.png)

# CPUSim64

*A 64-bit virtual CPU, assembler, and simulation environment*

©2026 Richard Lesh.  All rights reserved.

CPUSim64 is a complete software toolchain for experimenting with CPU architecture and assembly language design.  

It includes:

- A 64-bit virtual CPU with a simple instruction set  
- A Java-based assembler built using ANTLR4 grammars  
- A cycle-accurate simulation engine  
- A native Rust reimplementation of the simulator (`cpusim64_native`) for faster, standalone execution  
- A simple IDE with code editor, console, and integrated run/debug  
- A pluggable compiler framework in the IDE, with a **C11 compiler written in CPUSim64 assembly** and its own C standard library (`compilers/C11`), and an **MMIX translator** that runs Knuth's MMIXAL programs (`compilers/MMIX`)  
- A native CLI tool for command-line workflows  
- An agentic AI assistant with RAG-based context retrieval for writing and debugging assembly code  
- Native installers for macOS, Windows, and Linux (x64 and ARM64)  
- A multi-module Maven build system producing runnable JAR files

The project is designed for educational use, computer architecture instruction, and experimentation with ISA design and microarchitecture concepts.

---

## 🚀 Features

### **Integrated Development Environment**
- Code editor with syntax highlighting for CPUSim64 assembly
- Tabbed editor: File → Open opens a file in a new tab (or in the editor if it is empty); a CPUSim64 window opens `.asm` and `.def` files, a compiler window only its own source files (`.c` and `.h` for C11). File → New opens a new CPUSim64 window, File → New Compiler a new compiler window  
- Run and Debug act on the focused tab and are enabled only for a program: an `.asm` file with `__MAIN__`, or one that is not a library or module (not a `.def` file, no include guard, no `#export`); a C file that defines `main()` (the name is `main_function` in the plug-in's `config.properties`); an MMIXAL file with the label `Main` in column 1 (`entry_label`)  
- Cmd+click (macOS) / Ctrl+click on a file name in an `#include` opens it in a tab; system libraries (`<system/io.asm>`) open read-only, and `<stdio.h>` in a C11 window opens the compiler's `lib/stdio.h`  
- The debugger switches to (or opens) the tab of the source file that is executing, and follows the breakpoints of every open tab  
- The AI chat works on the focused tab
- Line number margin  
- Custom terminal console with ANSI terminal support and fast rendering  
  - ANSI foreground and background colors (standard and bright)
  - Carriage return (`\r`) support for progress bars and spinners  
  - Cursor hide/show escape sequences  
  - Full ANSI SGR support: bold, dim, italic, underline, blink, reverse, hidden, strikethrough  
  - Clear to end of line and clear line escape sequences  
- Assemble, Run, Debug, and Trace from the menu  
- Run/Stop toggle button — Stop terminates the running program and all child processes  
- Ctrl+C to interrupt running program, Ctrl+D to signal EOF  
- Configurable Heap Size (MiB) and Stack Size (kiB) in the toolbar  
- Find and Replace with regex/grep support and capture groups  
- Shift Selection Left/Right (Cmd+[/]) for block indent/dedent  
- Configurable fonts, sizes, and syntax highlighting colors  
- Settings persisted to `~/.cpusim64-settings.json`  
- Tab key inserts spaces to 4-character tab stops  
- Smart backspace: deletes to previous tab stop in leading whitespace  
- Tabs converted to spaces on file open  
- Triple-click-drag selects whole lines  
- Undo/Redo support (text-snapshot based)  
- File Open/Save remembers last used directory  
- Splash screen and license key system  
- **Pluggable compilers** — File → New Compiler lists the compiler plug-ins found in the `compilers/` folder (see below)  
- Run/Debug link relocatable modules automatically (sources with `#export`/`#extern`)  

### **Compiler plug-ins**
A compiler written in CPUSim64 assembly (for example the C11 compiler in `compilers/C11`) can be added to the IDE without changing it.
The IDE looks for a `compilers` folder next to `CPUSim64IDE.jar` (installed app) or, when run from IntelliJ IDEA or a source checkout, next to the `ide` folder (override with `-Dcpusim64.compilers=DIR`).
Every sub-folder with a `config.properties` is a plug-in; the folder name is the menu item (File → New Compiler → *name*) and the window title.

| Plug-in file | Purpose |
|---|---|
| `config.properties` | `compiler_source` – the compiler's main `.asm` file (required, `.asm` optional)<br>`syntax_language` – RSyntaxTextArea style for highlighting, e.g. `text/c`<br>`file_extensions` – optional, e.g. `c,h` (first is the Save As default)<br>`compiler_mem`, `compiler_stack` – optional run sizes (default `16M`, `256K`)<br>`main_function` – name of the entry function (default `main`): Run and Debug are enabled for a source that defines it<br>`entry_label` – instead of `main_function`: Run and Debug are enabled for a source with a line that starts with this label (MMIX: `Main`) |
| `system_prompt.md` | system prompt for the AI chat of that window |
| `obj/` | the assembled compiler; rebuilt when missing or older than any `.asm`/`.def` file of the plug-in (kept in `~/.cpusim64/compilers/<name>` if the plug-in folder is read-only) |

Run in such a window saves the file, assembles the compiler if needed, runs it with the source file path as its argument (it must write `<source>.asm` next to the source and exit with 0), assembles the result into `obj/<name>.o64` and runs it with the Args from the toolbar, using the Java or native simulator as selected in Settings.
Errors from the compiler's assembly or from the compiler are shown in the console and stop the run.
Debug compiles the same way and assembles the result. If the compiler puts `.source "file", line` directives in its output (as the C11 compiler and the MMIX translator do), the debugger shows the source file in the compiler window: the execution line and the breakpoints use the source lines (stepping is by instruction). Otherwise the generated assembly opens in a regular window with the debugger.
A compiler window also has an **Options** field for extra compiler arguments, such as other source files (relative to the source file) and `-lstd` for the C11 compiler.

In any window, a source file that contains `#export` or `#extern` is a relocatable module: Run and Debug assemble it with `-r` and link it with the modules it `#include`s into `obj/<name>.o64` (with merged symbol and source-map files for the debugger); link errors appear in the console and stop the run.

### **C11 Compiler (`compilers/C11`)**
A C compiler written entirely in CPUSim64 assembly.  It compiles a core subset of C11 to CPUSim64 `.asm` (run it with `run.sh cc prog.c`, or from the IDE through File → New Compiler → C11).
- Hand-written recursive-descent parser following the ANTLR grammars-v4 C grammar, a built-in preprocessor, type checking, and stack-machine code generation; it stops at the first error with `file:line: error: message`  
- Types: `int`, `long`, `char`, `double`, pointers, arrays, structs, unions, enums, typedefs, function pointers, `_Bool`; compound literals, designated initializers, variadic functions, `_Static_assert`, unsigned arithmetic and conversions  
- Not supported (reported as errors): `float`, `long double`, `_Complex`, `_Atomic`, `_Generic`, bit-fields, variable length arrays, K&R parameter lists and GNU extensions  
- Every scalar is one 64-bit word; `char` holds a Unicode codepoint  
- Standard library written in C in `lib/` and compiled only when used: `stdio` (exact C `printf`/`scanf` families, files with `fseek`/`ftell`, `snprintf`, `getline`), `stdlib` (`malloc`, `qsort`, `strto*`, `rand`/`srand`, `atexit`, `getenv`), `string`, `math` (C99), `ctype`, `time`, `setjmp`, `assert`, `stdarg`, `stddef`, `stdint`, `stdbool`, `limits`, `float`  
- `time()` returns the real wall-clock time (UTC) so `srand(time(NULL))` works; `getenv()` reads the host environment  
- A header `#include <name.h>` is found in `lib/` (plus any `--lib=DIR`), and `lib/name.c` is appended once, compiling only the functions the program uses  
- Separate compilation: `cc main.c util.c` compiles each file into its own linker module (external symbols exported, `static` ones private); `-lstd` links the C library as shared library modules instead of appending it  
- 97 regression tests: `compilers/C11/tests/run_tests.sh`  

### **MMIX Translator (`compilers/MMIX`)**
Runs programs written in MMIXAL, the assembly language of Knuth's 64-bit MMIX computer, by translating them to CPUSim64 assembly (File → New Compiler → MMIX, or `compilers/MMIX/obj/mmixc.o64 prog.mms` on a simulator).
- The translator is written in portable C (`src/`) and compiled by the C11 compiler; its MMIXAL front end assembles exactly as Knuth's `mmixal` (`PREFIX`, `GREG`, local labels, future references, the same error messages)  
- Full user-mode MMIX: integer, logic, branch and memory instructions, the register stack (`PUSHJ`, `POP`, `SAVE`, `UNSAVE`), all floating-point operations with the four rounding modes, the exception flags and trip handlers in `rA` (`TRIP`, `RESUME`), and the `TRAP` I/O system calls of the MMIXware simulator (`Fopen` … `Ftell`, `Halt`)  
- The arithmetic run-time library (`rt/mmix_rt.c`) is a port of Knuth's `mmix-arith`, so results, rounding and flags are those of the real `mmix`  
- Kernel features (virtual memory, interrupts, the timer registers) are not supported  
- Tests compare with the real `mmixal` and `mmix`: `compilers/MMIX/tests/run_tests.sh` and the randomised `tests/random_diff.sh`  

### **Virtual CPU**
- Custom 64-bit RISC-style architecture  
- Support for integer/float arithmetic, branching, memory operations, and pseudo-instructions  
- Configurable registers, memory size, and device interfaces  

### **Assembler**
- Implemented in Java with **ANTLR4** lexer and parser  
- Strict syntax checking and helpful diagnostics  
- Support for labels, immediates, directives, and constants  
- Preprocessor with `#include`, `#define`, `#if`/`#else`/`#endif`, loops, macros, and functions  
- Emits gzip-compressed `.o64` object files
- Per-file start-up functions (`__INIT__`) run before the program; a program that defines `__MAIN__` exits with its return value

### **Linker**
- `assemble.sh -r name` writes a relocatable module (`obj/name.ro64`), with its relocation information in a JSON `.rel` file
- `link.sh -o prog a.ro64 b.ro64 ...` links modules into one ordinary `.o64` with merged `.sym`/`.sym1`/`.sym2`/`.srcmap` files, so the simulators and the debugger need no changes
- `assemble.sh -o prog a b ...` assembles and links in one step (`-o` is required with more than one input)
- `#export name, ...` / `#export *` and `#extern name, ...` control which labels modules share; all labels are private by default
- `#include`d `.asm` libraries are linked as their own modules, which are assembled automatically (system and ADT libraries are cached in `~/.cpusim64/modules/`)
- Link errors for duplicate or undefined symbols, two `__MAIN__`s, and addresses that do not fit their field

### **Simulation Engine**
- Cycle-accurate execution  
- Random-access files: `iOPEN_RANDOM_ACCESS_FILE`, `iSEEK` and `iTELL` (with `SEEK_BEGIN`/`SEEK_END`) in both the Java and native simulators  
- `iTIME` returns the wall-clock time in nanoseconds since the Unix epoch (UTC)  
- `iENV` fetches an environment variable as a string (cached, so fetching it again returns the same string; do not free it)  
- Matching `openRandomAccessFile`, `seek` and `tell` functions in `system/io.asm`; interrupt numbers in `system/system.def` and `system/io.def`  
- Debug mode with stepping and register inspection  
- Trace mode for instruction-by-instruction tracing  
- Disassemble feature to disassemble object files

### **Native Rust Simulator (`cpusim64_native`)**
- Standalone Rust port of the CPUSim64 interpreter for faster, dependency-free execution  
- Full behavioral parity with the Java simulator: all 39 opcodes, the complete interrupt/syscall surface, endian-aware port I/O, file I/O, the Fibonacci-block heap allocator, and the fork/thread/shared-memory concurrency model  
- Byte-for-byte identical `printCPUState`, dump, and trace output — including a port of the OpenJDK Schubfach `Double.toString` algorithm so even subnormal floats format exactly like Java  
- Reads the same gzip-compressed `.o64` object files and `.sym1` symbol maps; accepts the same CLI flags (`--debug`, `--trace`, `--verbose`, `--mem=`, `--stack=`)  
- Cycle-timing table built in by default, overridable at runtime by dropping an `instructions.data` file next to the executable  
- Validation harness compares output against the committed reference `.out` files  
- Implements the same interrupts as the Java simulator, including random-access files, `iTIME` and `iENV`; the validation tests require exact output and cycle parity for all 35 programs  
- **"Use Native CPUSim64 Simulator"** setting in the IDE — when enabled, Run/Debug/Trace launch `cpusim64_native` instead of the in-process Java simulator  
- Bundled into the IDE JAR and all native installers, and installable via the IDE's CLI Tools menu  

### **Symbolic Debugger**
- Integrated GUI debugger launched from the IDE  
- Disassembly view with live instruction markers and label annotations  
- Bidirectional breakpoint synchronization between source code and disassembly  
- Breakpoints on preprocessor directives (`#call`, `#macro`, etc.) map to generated instructions  
- Step Into, Step Over, Step Out, and Resume execution controls  
- Green execution line indicator in the source editor (coexists with breakpoints and line numbers)  
- Live register display with click-to-toggle decimal/hexadecimal values  
- Stack display with high-to-low address ordering and click-to-cycle decimal/hex/float values  
- Source-map-based address resolution (`.srcmap` file emitted during assembly)  
- Window layout (size, dividers, column widths) persisted across sessions  
- Source editor auto-scrolls to current execution line  
- Program I/O redirected to the IDE console pane during debugging  
- Memory window: right-click a register to inspect memory at that address  
- Memory window click-to-cycle value format: decimal, hex, float, Unicode character  
- Memory window right-click menu with String viewer and pointer-follow (Memory)  
- Heap block header highlighting (gray=allocated, green=free)  
- Status register (SR) display with PZSO flag indicators  

### **Agentic AI Assistant**
- Built-in AI chat panel in the IDE for writing and debugging CPUSim64 assembly  
- Supports multiple LLM vendors: OpenAI, Anthropic, Google, DeepSeek, Alibaba, and local Ollama  
- Generic OpenAI-compatible API vendor support via YAML configuration  
- Configurable model and API key via dedicated AI Chat Settings dialog  
- RAG-based context retrieval using embedding search or BM25 keyword fallback  
- Embedding and keyword index caching in `~/.cpusim64/`  
- Context-aware: automatically provides current source code and console output to the AI  
- AI responses with code blocks present Allow/Reject buttons for applying changes directly to the editor  
- Auto-apply fulltext code blocks for seamless code replacement  
- DiffApplier with regex-based hunk parsing and freeform diff support  
- Animated "thinking" indicator with cancel support  
- Markdown rendering in responses with bold, italic, code spans, and LaTeX math  
- Conversation history maintained across messages within a session  
- System prompt includes full CPUSim64 documentation, examples, and projects for accurate assistance  
- In a compiler window (File → New Compiler) the chat uses that plug-in's own `system_prompt.md`  
- Clear button to reset conversation  
- Status bar showing system prompt, program, and output sizes  
- Separate AI font settings (sans-serif default) with code font for code blocks  
- Markdown table rendering in monospace for column alignment  
- Configurable chat text colors  
- Developer mode with prompt/response logging  

### **CLI Tool (`cpusim64`)**
- Native C++ command-line interface  
- Commands: `assemble`, `run`, `debug`, `trace`, `disassemble`, `preprocess`  
- Installable via the IDE's CLI Tools menu — installs the `cpusim64_native` Rust simulator and its `instructions.data` timing table alongside it  

### **Native Installers**
- macOS DMG (signed and notarized) — ARM64  
- Windows Inno Setup EXE (Azure code signed) — x64 and ARM64; supports `/ALLUSERS` (all users) and `/CURRENTUSER` (one user) command-line install scopes  
- Linux DEB and RPM — x64 and ARM64  
- File type associations for `.asm`, `.o64`, `.sym`, `.sym1`, `.sym2`, `.srcmap`, and `.def` files  
- The `compilers/` folder (without tests) is installed next to the application JAR  
- Built via GitHub Actions workflows

---

## 📦 Project Structure (CPUSim64V2)

The project uses a multi-module Maven layout separating the AI chat library, simulator/assembler core, native Rust simulator, and IDE.

```
CPUSim64V2/
├── LICENSE
├── NOTICE
├── README.md
├── pom.xml                          (parent POM)
│
├── aichat/                          (AI Chat Module)
│   ├── pom.xml
│   └── src/main/java/com/glowingcat/aichat/
│       ├── AIChatPanel.java
│       ├── DocumentRetriever.java   (RAG retrieval)
│       ├── EmbeddingClient.java
│       ├── KeywordIndex.java
│       ├── LLMClientFactory.java
│       ├── VendorRegistry.java
│       └── ...
│
├── cpusim64/                        (Simulator Module)
│   ├── pom.xml
│   ├── src/main/antlr4/            (ANTLR grammars)
│   ├── src/main/cpp/               (CLI tool source)
│   ├── src/main/java/cloud/lesh/CPUSim64/
│   │   ├── Assembler.java
│   │   ├── Simulator.java
│   │   ├── Preprocessor.java
│   │   └── ...
│   ├── src/main/resources/
│   │   ├── documentation/
│   │   ├── system/                  (system libraries)
│   │   └── adt/                     (data structure libraries)
│   ├── src/test/
│   └── validation/
│
├── cpusim64_native/                 (Native Rust Simulator Module)
│   ├── Cargo.toml                   (Rust crate manifest)
│   ├── pom.xml                      (Maven wrapper — runs `cargo build`)
│   └── src/
│       ├── main.rs                  (CLI entry ≈ Simulation.java)
│       ├── decode.rs                (instruction decode)
│       ├── cpu/                     (exec, heap, memory, cycles, format, interrupts, concurrency)
│       ├── ports/                   (stdio / file / raw port handlers)
│       └── javacompat/              (Java Formatter + Schubfach Double.toString)
│
├── compilers/                       (IDE compiler plug-ins: C11/, MMIX/)
│
├── ide/                             (IDE Module)
│   ├── pom.xml
│   └── src/main/java/com/glowingcat/cpusim64ide/
│       ├── IDEApp.java
│       ├── EditorPanel.java
│       ├── DebuggerWindow.java
│       ├── TerminalPanel.java
│       └── ...
│
├── resources/                       (jpackage resources)
│   ├── entitlements.plist
│   ├── macos/
│   └── linux/
│
├── .github/workflows/
│   ├── package.yml                  (native installer builds)
│   └── release.yml
│
└── *.sh / *.bat                     (convenience scripts)
```

---

### Module Overview

| Module            | Artifact                   | Description                                              |
|-------------------|----------------------------|----------------------------------------------------------|
| `aichat`          | `com.glowingcat:aichat`    | Reusable AI chat panel with LLM integration and RAG      |
| `cpusim64`        | `cloud.lesh:CPUSim64`        | Assembler, simulator, CLI tool, ANTLR grammars           |
| `cpusim64_native` | `cloud.lesh:cpusim64_native` | Native Rust port of the interpreter (built via cargo)    |
| `ide`             | `cloud.lesh:CPUSim64IDE`     | JavaFX/Swing GUI IDE, depends on the modules above       |

---

## 🛠 Building

### Prerequisites
- JDK 21+
- Maven 3.9+
- Rust toolchain (`cargo`) — required to build the `cpusim64_native` module. On a machine without Rust, skip it with `-Dcpusim64.native.skip=true`.

### Build
```bash
mvn clean package
```
The `cpusim64_native` Maven module runs `cargo build --release` and the IDE module bundles the resulting executable (plus its `instructions.data` timing table) into the IDE JAR.

### Build for release (JavaFX provided by runtime):
```bash
mvn clean package -P release -DskipTests=true
```

### Build without the native simulator (no Rust toolchain):
```bash
mvn clean package -Dcpusim64.native.skip=true
```

### Run the IDE locally:
```bash
./ide.sh
```

### Key dependencies
- JavaFX **21.0.13** (LTS)
- ANTLR **4.13.2**
- JUnit **5.14.0**, commonmark **0.24.0**
- Rust crate deps (pinned): `flate2 1.0.30`, `regex 1.10.4`

---

## 📚 Documentation

HTML reference documentation for the CPUSim64 architecture, instruction set, directives, interrupts, libraries, and programming model is bundled in `cpusim64/src/main/resources/documentation/`.

Online hosted documentation: http://cpusim64.lesh.cloud/

---

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE) for details.

© 2026 Richard Lesh
