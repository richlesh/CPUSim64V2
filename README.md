# CPUSim64  
*A 64-bit virtual CPU, assembler, and simulation environment*

©2026 Richard Lesh.  All rights reserved.

CPUSim64 is a complete software toolchain for experimenting with CPU architecture and assembly language design.  

It includes:

- A 64-bit virtual CPU with a simple instruction set  
- A Java-based assembler built using ANTLR4 grammars  
- A cycle-accurate simulation engine  
- A simple IDE with code editor, console, and integrated run/debug  
- A native CLI tool for command-line workflows  
- Native installers for macOS, Windows, and Linux  
- A Maven-driven build system producing runnable JAR file

The project is designed for educational use, computer architecture instruction, and experimentation with ISA design and microarchitecture concepts.

---

## 🚀 Features

### **Integrated Development Environment**
- Code editor with syntax highlighting for CPUSim64 assembly
- Line number margin  
- Console pane with program output and interactive input  
- Assemble, Run, Debug, and Trace from the menu  
- Find and Replace with regex/grep support and capture groups  
- Cmd+click (macOS) / Ctrl+click to navigate `#include` files  
- System include files open in a read-only viewer  
- Configurable fonts, sizes, and syntax highlighting colors  
- Settings persisted to `~/.cpusim64-settings.json`  
- Tab key inserts spaces to 4-character tab stops  
- Undo/Redo support  
- Splash screen and license key system  

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

### **Simulation Engine**
- Cycle-accurate execution  
- Debug mode with stepping and register inspection  
- Trace mode for instruction-by-instruction tracing  
- Disassemble feature to disassemble object files

### **CLI Tool (`cpusim64`)**
- Native C++ command-line interface  
- Commands: `assemble`, `run`, `debug`, `trace`, `disassemble`, `preprocess`  
- Installable via the IDE's CLI Tools menu  

### **Native Installers**
- macOS DMG (signed and notarized)  
- Windows MSI (Azure code signed)  
- Linux DEB and RPM  
- File type associations for `.asm`, `.o64`, and `.sym` files  
- Built via GitHub Actions workflows

---

## 📦 Project Structure (CPUSim64V2)

The CPUSim64V2.zip archive is organized to clearly separate documentation, source code, examples, scripts, and build artifacts.

```CPUSim64V2/
├── LICENSE
├── NOTICE
├── README.md
│
├── assemble.sh
├── assembler_tree.sh
├── clean.sh
├── debug.sh
├── disassemble.sh
│
├── documentation/
│   ├── Architecture.html
│   ├── Directive_Reference.html
│   ├── Instruction_Set_Reference.html
│   ├── Interrupt_Reference.html
│   ├── Library_Reference.html
│   ├── Programmer_Guide.html
│   ├── Instruction Format New.txt
│   └── examples/
│
├── lib/
│   └── CPUSim64-2.2.0.jar
│
└── src/
    ├── generated-sources/
    │
    ├── main/
    │   ├── antlr4/
    │   ├── java/
    │   └── resources/
    │
    └── test/
        ├── java/
        └── resources/
```
⸻

🔹 Top-Level Files

| File        | Purpose                                                     |
|-------------|-------------------------------------------------------------|
| `LICENSE`   | Apache License 2.0                                          |
| `NOTICE`    | Required attribution and licensing notices                  |
| `README.md` | Project overview and usage instructions                     |
| `*.sh`      | Convenience scripts for assembling, debugging, and cleaning |

⸻

📚 documentation/

HTML and reference material describing the CPUSim64V2 architecture, instruction set, directives, interrupts, libraries, and programming model.

These files may not with direct viewing in some browsers as they make extensive use of AJAX to load example files and output.  See http://cpusim64.lesh.cloud/ for online hosted documentation.

⸻

🧪 examples/

Sample CPUSim64 assembly programs (.asm) with corresponding output files (.out) demonstrating:

-	Instruction usage
-	Control flow
-	I/O
-	Interrupts
-	Library calls

⸻

📦 lib/

Assembly-level runtime and standard libraries used by programs assembled for CPUSim64V2.

⸻

🛠 src/

Java source code for the toolchain:
- Assembler (ANTLR grammar, lexer/parser, visitors)
- Simulator (CPU core, memory model, devices)
- Common utilities shared across components
