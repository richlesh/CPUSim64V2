//! Native Rust port of the CPUSim64 virtual CPU interpreter.
//!
//! This crate ports the *interpreter/simulator* half of the Java CPUSim64
//! project (not the ANTLR assembler/preprocessor). It reads pre-assembled
//! `.o64` object files and executes them with full behavioral parity.

pub mod cpu;
pub mod decode;
pub mod javacompat;
pub mod object;
pub mod opcode;
pub mod ports;
pub mod util;
