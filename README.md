# CPUSimV2  
*A 64-bit virtual CPU, assembler, and simulation environment*

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
- Support for integer arithmetic, branching, memory operations, and pseudo-instructions  
- Configurable registers, flags, memory size, and device interfaces  

### **Assembler**
- Implemented in Java with **ANTLR4** lexer and parser  
- Strict syntax checking and helpful diagnostics  
- Support for labels, immediates, directives, and constants  
- Emits flattened machine code for the CPUSimV2 execution core

### **Simulation Engine**
- Cycle-accurate execution  
- Stepping and register inspection  
- Optional verbose mode for instruction-by-instruction tracing  

### **Development Tools**
- Command-line runner  
- Maven project structure (`pom.xml`)  
- Scripts for building and packaging  

---

## 📦 Project Structure