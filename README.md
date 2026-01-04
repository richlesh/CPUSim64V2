# CPUSimV2  
*A 64-bit virtual CPU, assembler, and simulation environment*

©2026 Richard Lesh.  All rights reserved.

CPUSimV2 is a complete software toolchain for experimenting with CPU architecture and assembly language design.  

It includes:

- A 64-bit virtual CPU with a simple instruction set  
- A Java-based assembler built using ANTLR4 grammars  
- A cycle-accurate simulation engine  
- A Maven-driven build system producing runnable JAR file

The project is designed for educational use, computer architecture instruction, and experimentation with ISA design and microarchitecture concepts.

---

## 🚀 Features

### **Virtual CPU**
- Custom 64-bit RISC-style architecture  
- Support for integer/float arithmetic, branching, memory operations, and pseudo-instructions  
- Configurable registers, memory size, and device interfaces  

### **Assembler**
- Implemented in Java with **ANTLR4** lexer and parser  
- Strict syntax checking and helpful diagnostics  
- Support for labels, immediates, directives, and constants  
- Emits flattened machine code for the CPUSimV2 execution core

### **Simulation Engine**
- Cycle-accurate execution  
- Stepping and register inspection  
- Optional trace mode for instruction-by-instruction tracing  
- Disassemble feature to disassemble object files

### **Development Tools**
- Scripts to preprocess, assemble, run, debug and trace
- Scripts to view preprocessor parse tree and assembler parse tree
- Scripts for cleaning, building and packaging  

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
│   └── CPUSim64-2.0-SNAPSHOT.jar
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

These files are not suitable for direct viewing in a browser as they make extensive use of server-side includes.  See http://cpusim64.lesh.cloud/ for online hosted documentation.

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
