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
- An agentic AI assistant for writing and debugging assembly code  
- Native installers for macOS, Windows, and Linux  
- A Maven-driven build system producing runnable JAR file

The project is designed for educational use, computer architecture instruction, and experimentation with ISA design and microarchitecture concepts.

---

## 🚀 Features

### **Integrated Development Environment**
- Code editor with syntax highlighting for CPUSim64 assembly
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
- Cmd+click (macOS) / Ctrl+click to navigate `#include` files  
- System include files open in a read-only viewer  
- Configurable fonts, sizes, and syntax highlighting colors  
- Settings persisted to `~/.cpusim64-settings.json`  
- Tab key inserts spaces to 4-character tab stops  
- Smart backspace: deletes to previous tab stop in leading whitespace  
- Tabs converted to spaces on file open  
- Triple-click-drag selects whole lines  
- Undo/Redo support (text-snapshot based)  
- File Open/Save remembers last used directory  
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
- Configurable model and API key via Settings  
- Context-aware: automatically provides current source code and console output to the AI  
- AI responses with code blocks present Allow/Reject buttons for applying changes directly to the editor  
- Animated "thinking" indicator with cancel support  
- Markdown rendering in responses with bold, italic, code spans, and LaTeX math  
- Conversation history maintained across messages within a session  
- System prompt includes full CPUSim64 documentation, examples, and projects for accurate assistance  
- Clear button to reset conversation  
- Status bar showing system prompt, program, and output sizes  
- Separate AI font settings (sans-serif default) with code font for code blocks  
- Markdown table rendering in monospace for column alignment  

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
│   └── CPUSim64-2.5.0.jar
│
└── src/
    ├── generated-sources/
    │
    ├── main/
    │   ├── antlr4/
    │   ├── cpp/
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
