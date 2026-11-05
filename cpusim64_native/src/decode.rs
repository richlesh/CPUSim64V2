//! Instruction decode (ports `Simulator.Decoded`) plus register-name tables and
//! encode helpers (`encT0..encT3`).

use std::collections::HashMap;

pub const GPR_COUNT: usize = 32;
pub const FPR_COUNT: usize = 32;

/// Sign-extend the low `bits` of `v` to a full i64 (ports `Simulator.signExtend`).
pub fn sign_extend(v: i64, bits: u32) -> i64 {
    let m = 1i64 << (bits - 1);
    let mask = if bits >= 64 { -1i64 } else { (1i64 << bits) - 1 };
    let v = v & mask;
    (v ^ m).wrapping_sub(m)
}

#[inline]
pub fn is_none_kind(k: i32) -> bool {
    k == 0
}
#[inline]
pub fn is_const_kind(k: i32) -> bool {
    k == 1
}
#[inline]
pub fn is_reg_kind(k: i32) -> bool {
    k == 2
}
#[inline]
pub fn is_fp_kind(k: i32) -> bool {
    k == 3
}
#[inline]
pub fn is_x_kind(k: i32) -> bool {
    k == 2 || k == 3
}
#[inline]
pub fn is_y_kind(k: i32) -> bool {
    k == 2 || k == 3
}
#[inline]
pub fn is_o_kind(k: i32) -> bool {
    k == 1 || k == 2
}
#[inline]
pub fn is_q_kind(k: i32) -> bool {
    k != 0
}

/// Register symbol table: R0..R28, SF, SP, PC, and index 32 = SR.
pub fn register_names() -> &'static [&'static str; GPR_COUNT + 1] {
    &REGISTERS
}

/// FP register symbol table: F0..F31.
pub fn fp_register_names() -> &'static [&'static str; FPR_COUNT] {
    &REGISTERS_FP
}

static REGISTERS: [&str; GPR_COUNT + 1] = [
    "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10", "R11", "R12", "R13", "R14",
    "R15", "R16", "R17", "R18", "R19", "R20", "R21", "R22", "R23", "R24", "R25", "R26", "R27",
    "R28", "SF", "SP", "PC", "SR",
];

static REGISTERS_FP: [&str; FPR_COUNT] = [
    "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14",
    "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "F25", "F26", "F27",
    "F28", "F29", "F30", "F31",
];

/// Condition code mnemonics for disassembly of conditional JUMP/CALL.
pub static CONDITION: [&str; 11] = [
    "u", "z", "nz", "n", "p", "nn", "np", "o", "no", "pe", "po",
];

/// A decoded instruction word (ports `Simulator.Decoded`).
#[derive(Debug, Clone, Default)]
pub struct Decoded {
    pub tt: i32,
    pub op: u8,
    pub a: i32,
    pub b: i32,
    pub c: i32,
    pub d: i32,
    pub v0: i32,
    pub v1: i32,
    pub v2: i32,
    pub v3: i32,
    pub c1: i64,
    pub c2: i64,
    pub c3: i32,
    pub arg_count: i32,
}

impl Decoded {
    pub fn decode(w: i64) -> Decoded {
        let mut d = Decoded::default();
        d.decode_into(w);
        d
    }

    /// Decode instruction word `w` into this object (ports `decodeInto`).
    pub fn decode_into(&mut self, w: i64) {
        let w = w as u64;
        self.a = 0;
        self.b = 0;
        self.c = 0;
        self.d = 0;
        self.v0 = 0;
        self.v1 = 0;
        self.v2 = 0;
        self.v3 = 0;
        self.c1 = 0;
        self.c2 = 0;
        self.c3 = 0;
        self.arg_count = 0;
        self.tt = ((w >> 62) & 0x3) as i32;
        self.op = ((w >> 56) & 0x3F) as u8;
        match self.tt {
            0 => {
                self.a = ((w >> 54) & 0x3) as i32;
                self.b = ((w >> 52) & 0x3) as i32;
                self.c = ((w >> 50) & 0x3) as i32;
                self.d = ((w >> 48) & 0x3) as i32;
                self.v0 = ((w >> 36) & 0xFFF) as i32;
                self.v1 = ((w >> 24) & 0xFFF) as i32;
                self.v2 = ((w >> 12) & 0xFFF) as i32;
                self.v3 = (w & 0xFFF) as i32;
                if is_const_kind(self.a) {
                    self.v0 = sign_extend(self.v0 as i64, 12) as i32;
                }
                if is_const_kind(self.b) {
                    self.v1 = sign_extend(self.v1 as i64, 12) as i32;
                }
                if is_const_kind(self.c) {
                    self.v2 = sign_extend(self.v2 as i64, 12) as i32;
                }
                if is_const_kind(self.d) {
                    self.v3 = sign_extend(self.v3 as i64, 12) as i32;
                }
            }
            1 => {
                let imm = (w & ((1u64 << 56) - 1)) as i64;
                self.c1 = sign_extend(imm, 56);
            }
            2 => {
                self.a = ((w >> 54) & 0x3) as i32;
                self.v0 = ((w >> 42) & 0xFFF) as i32;
                self.v0 = sign_extend(self.v0 as i64, 12) as i32;
                let imm = (w & ((1u64 << 42) - 1)) as i64;
                self.c2 = sign_extend(imm, 42);
            }
            3 => {
                self.a = ((w >> 54) & 0x3) as i32;
                self.b = ((w >> 52) & 0x3) as i32;
                self.v0 = ((w >> 40) & 0xFFF) as i32;
                self.v0 = sign_extend(self.v0 as i64, 12) as i32;
                self.v1 = ((w >> 28) & 0xFFF) as i32;
                self.v1 = sign_extend(self.v1 as i64, 12) as i32;
                let raw = (w & ((1u64 << 28) - 1)) as i64;
                self.c3 = sign_extend(raw, 28) as i32;
            }
            _ => {}
        }
        self.arg_count = self.compute_arg_count();
    }

    fn compute_arg_count(&self) -> i32 {
        match self.tt {
            0 => {
                if is_none_kind(self.a) {
                    0
                } else if is_none_kind(self.b) {
                    1
                } else if is_none_kind(self.c) {
                    2
                } else if is_none_kind(self.d) {
                    3
                } else {
                    4
                }
            }
            1 => 1,
            2 => 2,
            3 => 3,
            _ => 0,
        }
    }

    pub fn arg_count(&self) -> i32 {
        self.arg_count
    }

    pub fn op_code(&self) -> u8 {
        self.op
    }

    /// Mirrors `Decoded.getOpName()`: code 0 with 0 args is "NOP"; otherwise the
    /// canonical name from the code map.
    pub fn op_name(&self) -> &'static str {
        if self.op == crate::opcode::NOP && self.arg_count() == 0 {
            "NOP"
        } else {
            crate::opcode::name_for_code(self.op)
        }
    }

    /// Render a register name or immediate value (ports `getRegisterOrValue`).
    pub fn get_register_or_value(kind: i32, num: i64) -> String {
        match kind {
            0 => "None".to_string(),
            1 => format!("{} (0x{:X})", num, num),
            2 => REGISTERS[num as usize].to_string(),
            3 => REGISTERS_FP[num as usize].to_string(),
            _ => panic!("Unexpected op type: {kind}"),
        }
    }

    /// Operand `i` rendered as a register/value string (ports `Decoded.get`).
    pub fn get(&self, i: i32) -> String {
        match i {
            0 => Decoded::get_register_or_value(self.a, self.v0 as i64),
            1 => Decoded::get_register_or_value(self.b, self.v1 as i64),
            2 => Decoded::get_register_or_value(self.c, self.v2 as i64),
            3 => Decoded::get_register_or_value(self.d, self.v3 as i64),
            _ => panic!("Illegal op index: {i}"),
        }
    }

    /// Find the nearest label at or below `loc` (ports `findNearestLabel`).
    pub fn find_nearest_label(reverse: &HashMap<i64, String>, loc: i64) -> Option<String> {
        if reverse.is_empty() {
            return None;
        }
        let max_label = *reverse.keys().max().unwrap();
        let mut j = if loc > max_label { max_label } else { loc };
        let mut label = reverse.get(&j).cloned();
        while label.is_none() && j > 0 {
            j -= 1;
            label = reverse.get(&j).cloned();
        }
        label.map(|l| format!("{}+{}", l, loc - j))
    }
}

// ===== Encoders (ports encT0..encT3) =====

pub fn enc_t0(op: u8, a: i32, b: i32, c: i32, d: i32, v0: i32, v1: i32, v2: i32, v3: i32) -> i64 {
    let mut w: u64 = 0;
    w |= (op as u64 & 0x3F) << 56;
    w |= (a as u64 & 3) << 54;
    w |= (b as u64 & 3) << 52;
    w |= (c as u64 & 3) << 50;
    w |= (d as u64 & 3) << 48;
    w |= (v0 as u64 & 0xFFF) << 36;
    w |= (v1 as u64 & 0xFFF) << 24;
    w |= (v2 as u64 & 0xFFF) << 12;
    w |= v3 as u64 & 0xFFF;
    w as i64
}

pub fn enc_t1(op: u8, imm56: i64) -> i64 {
    let mut w: u64 = 0;
    w |= 1u64 << 62;
    w |= (op as u64 & 0x3F) << 56;
    w |= imm56 as u64 & ((1u64 << 56) - 1);
    w as i64
}

pub fn enc_t2(op: u8, a_kind: i32, v0: i32, imm42: i64) -> i64 {
    let mut w: u64 = 0;
    w |= 2u64 << 62;
    w |= (op as u64 & 0x3F) << 56;
    w |= (a_kind as u64 & 3) << 54;
    w |= (v0 as u64 & 0xFFF) << 42;
    w |= imm42 as u64 & ((1u64 << 42) - 1);
    w as i64
}

pub fn enc_t3(op: u8, a_kind: i32, b_kind: i32, v0: i32, v1: i32, imm28: i32) -> i64 {
    let mut w: u64 = 0;
    w |= 3u64 << 62;
    w |= (op as u64 & 0x3F) << 56;
    w |= (a_kind as u64 & 3) << 54;
    w |= (b_kind as u64 & 3) << 52;
    w |= (v0 as u64 & 0xFFF) << 40;
    w |= (v1 as u64 & 0xFFF) << 28;
    w |= imm28 as u64 & ((1u64 << 28) - 1);
    w as i64
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sign_extend_basic() {
        assert_eq!(sign_extend(0xFFF, 12), -1);
        assert_eq!(sign_extend(0x7FF, 12), 2047);
        assert_eq!(sign_extend(0x800, 12), -2048);
        assert_eq!(sign_extend(1, 56), 1);
    }

    #[test]
    fn roundtrip_t0() {
        // ADD R0, R1 => op=13, a=2(reg),b=2(reg), v0=0, v1=1
        let w = enc_t0(13, 2, 2, 0, 0, 0, 1, 0, 0);
        let d = Decoded::decode(w);
        assert_eq!(d.tt, 0);
        assert_eq!(d.op, 13);
        assert_eq!(d.a, 2);
        assert_eq!(d.b, 2);
        assert_eq!(d.v0, 0);
        assert_eq!(d.v1, 1);
        assert_eq!(d.arg_count(), 2);
    }

    #[test]
    fn roundtrip_t1() {
        let w = enc_t1(7, 0x19);
        let d = Decoded::decode(w);
        assert_eq!(d.tt, 1);
        assert_eq!(d.op, 7);
        assert_eq!(d.c1, 0x19);
        assert_eq!(d.arg_count(), 1);
    }

    #[test]
    fn roundtrip_t2_neg_imm() {
        // Negative 42-bit immediate must sign-extend.
        let w = enc_t2(2, 2, 0, -5);
        let d = Decoded::decode(w);
        assert_eq!(d.tt, 2);
        assert_eq!(d.c2, -5);
    }

    #[test]
    fn roundtrip_t3() {
        let w = enc_t3(13, 2, 2, 0, 1, 7);
        let d = Decoded::decode(w);
        assert_eq!(d.tt, 3);
        assert_eq!(d.c3, 7);
        assert_eq!(d.arg_count(), 3);
    }

    #[test]
    fn nop_name() {
        let d = Decoded::decode(0);
        assert_eq!(d.op_name(), "NOP");
    }
}
