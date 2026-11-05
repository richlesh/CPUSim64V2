//! Opcode table (ports `Opcode.java`).
//!
//! Opcode numbers 0..38. Note the Java enum has two names sharing code 0
//! (NOP/DEBUG) and two sharing code 16 (DIVIDE/RECIP). The canonical display
//! name used by the disassembler is derived via `name_for_code`, which matches
//! `Opcode.fromCode(op).getName()` — `codeMap` is built by iterating the enum
//! values in declaration order and the last put wins, so code 0 maps to DEBUG
//! and code 16 maps to RECIP in the Java `codeMap`. However the disassembler's
//! `getOpName()` special-cases code 0 with zero args to "NOP"; see
//! `decode::Decoded::op_name`.

pub const NOP: u8 = 0;
pub const DEBUG: u8 = 0;
pub const CLEAR: u8 = 1;
pub const MOVE: u8 = 2;
pub const LOAD: u8 = 3;
pub const STORE: u8 = 4;
pub const POP: u8 = 5;
pub const PUSH: u8 = 6;
pub const JUMP: u8 = 7;
pub const CALL: u8 = 8;
pub const RETURN: u8 = 9;
pub const INTERRUPT: u8 = 10;
pub const STOP: u8 = 11;
pub const NEGATE: u8 = 12;
pub const ADD: u8 = 13;
pub const SUBTRACT: u8 = 14;
pub const MULTIPLY: u8 = 15;
pub const DIVIDE: u8 = 16;
pub const RECIP: u8 = 16;
pub const COMPL: u8 = 17;
pub const AND: u8 = 18;
pub const OR: u8 = 19;
pub const XOR: u8 = 20;
pub const TEST: u8 = 21;
pub const CMP: u8 = 22;
pub const LSHIFT: u8 = 23;
pub const RSHIFT: u8 = 24;
pub const ARSHIFT: u8 = 25;
pub const LROTATE: u8 = 26;
pub const RROTATE: u8 = 27;
pub const IN: u8 = 28;
pub const OUT: u8 = 29;
pub const PACK: u8 = 30;
pub const PACK64: u8 = 31;
pub const UNPACK: u8 = 32;
pub const UNPACK64: u8 = 33;
pub const CAS: u8 = 34;
pub const ENDIAN: u8 = 35;
pub const SAVE: u8 = 36;
pub const RESTORE: u8 = 37;
pub const READONLY: u8 = 38;

/// Returns the canonical opcode name for a code, matching Java
/// `Opcode.fromCode(op).getName()`.
///
/// In the Java `codeMap`, declaration order means the later duplicate wins:
/// code 0 -> DEBUG, code 16 -> RECIP. Unknown codes -> UNDEFINED.
pub fn name_for_code(op: u8) -> &'static str {
    match op {
        0 => "DEBUG",
        1 => "CLEAR",
        2 => "MOVE",
        3 => "LOAD",
        4 => "STORE",
        5 => "POP",
        6 => "PUSH",
        7 => "JUMP",
        8 => "CALL",
        9 => "RETURN",
        10 => "INTERRUPT",
        11 => "STOP",
        12 => "NEGATE",
        13 => "ADD",
        14 => "SUBTRACT",
        15 => "MULTIPLY",
        16 => "RECIP",
        17 => "COMPL",
        18 => "AND",
        19 => "OR",
        20 => "XOR",
        21 => "TEST",
        22 => "CMP",
        23 => "LSHIFT",
        24 => "RSHIFT",
        25 => "ARSHIFT",
        26 => "LROTATE",
        27 => "RROTATE",
        28 => "IN",
        29 => "OUT",
        30 => "PACK",
        31 => "PACK64",
        32 => "UNPACK",
        33 => "UNPACK64",
        34 => "CAS",
        35 => "ENDIAN",
        36 => "SAVE",
        37 => "RESTORE",
        38 => "READONLY",
        _ => "UNDEFINED",
    }
}
