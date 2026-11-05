//! CPU-state / dump / disassembly formatting (ports the printing half of
//! `Simulator.java`). Output must match the Java `Formatter` byte-for-byte in
//! the region the validation harness compares.

use super::{Simulator, FPR_COUNT, GPR_COUNT, R_SF, R_SP};
use crate::decode::{
    is_const_kind, is_fp_kind, is_reg_kind, register_names, Decoded,
};
use crate::javacompat::format::{format_g, pad};

const HEX_SIZE: usize = 16;
const DEC_SIZE: usize = 21;
const FLOAT_SIZE: usize = 23;
const FRACTION_SIZE: usize = 16;

fn fmt_reg_name(name: &str) -> String {
    // "%3s: " — right-justify name in width 3, then ": ".
    format!("{:>3}: ", name)
}

fn fmt_hex(v: i64) -> String {
    format!("{:016x}", v as u64)
}

fn fmt_dec(v: i64) -> String {
    // "%21d" — right-justified decimal in width 21.
    pad(&v.to_string(), DEC_SIZE, false, false)
}

fn fmt_float(v: f64) -> String {
    // "%23.16g"
    pad(&format_g(v, FRACTION_SIZE), FLOAT_SIZE, false, false)
}

impl Simulator {
    /// Build the register/stack dump as a string (ports `printCPUState`).
    pub fn cpu_state_string(&mut self) -> String {
        let regs = register_names();
        let mut out = String::new();
        out.push_str(&format!("\nCPU State    SR: {}\n", self.format_sr()));

        // Heading: "%3s  %16s %21s %3s  %23s   %5s  %16s %21s %23s\n"
        let labels = ["R", "hex", "dec", "FP", "float", "Stack", "hex", "dec", "float"];
        out.push_str(&format!(
            "{:>3}  {:>hex$} {:>dec$} {:>3}  {:>flt$}   {:>5}  {:>hex$} {:>dec$} {:>flt$}\n",
            labels[0],
            labels[1],
            labels[2],
            labels[3],
            labels[4],
            labels[5],
            labels[6],
            labels[7],
            labels[8],
            hex = HEX_SIZE,
            dec = DEC_SIZE,
            flt = FLOAT_SIZE,
        ));

        let mut stack_base = self.r[R_SP] as i64 + FPR_COUNT as i64;
        if stack_base >= self.stack_base {
            stack_base = self.stack_base - 1;
        }

        for i in 0..FPR_COUNT {
            if i >= R_SF {
                // fmtCPUAlt: reg hex, 21-space padded dec placeholder, fp float.
                out.push_str(&fmt_reg_name(regs[i]));
                out.push_str(&fmt_hex(self.r[i]));
                out.push(' ');
                out.push_str(&" ".repeat(DEC_SIZE));
                out.push(' ');
                out.push_str(&fmt_reg_name(crate::decode::fp_register_names()[i]));
                out.push_str(&fmt_float(self.f[i]));
            } else {
                // fmtCPU: reg hex dec fp float.
                out.push_str(&fmt_reg_name(regs[i]));
                out.push_str(&fmt_hex(self.r[i]));
                out.push(' ');
                out.push_str(&fmt_dec(self.r[i]));
                out.push(' ');
                out.push_str(&fmt_reg_name(crate::decode::fp_register_names()[i]));
                out.push_str(&fmt_float(self.f[i]));
            }

            let idx = stack_base - i as i64;
            if idx > self.r[R_SP] {
                let v = self.mem_read(idx).unwrap_or(0);
                let f = f64::from_bits(v as u64);
                // fmtStack: " %2sSP+%02d: %016x %21d %23.16g"
                let marker = if idx == self.r[R_SF] { "=>" } else { "  " };
                out.push_str(&format!(
                    " {:>2}SP+{:02}: {} {} {}",
                    marker,
                    idx - self.r[R_SP],
                    fmt_hex(v),
                    fmt_dec(v),
                    fmt_float(f)
                ));
            } else if idx == self.r[R_SP] && idx == self.r[R_SF] {
                out.push_str(" =>");
            }
            out.push('\n');
        }
        out
    }

    pub fn print_cpu_state(&mut self) {
        let s = self.cpu_state_string();
        print!("{s}");
        use std::io::Write;
        let _ = std::io::stdout().flush();
    }

    pub fn print_memory_dump(&mut self, addr: i64, count: i32) {
        print!("Memory dump at {:08x}, {} words:\n", addr as u32, count);
        for i in 0..count as i64 {
            let v = self.mem_read(addr + i).unwrap_or(0);
            print!("  [{:08x}]: {:016x}  ({})\n", (addr + i) as u32, v as u64, v);
        }
    }

    pub fn print_heap_walk(&mut self, addr: i64) {
        println!("Heap walk:");
        let mut p = if addr < self.heap_start { self.heap_start } else { addr };
        while p < self.stack_base {
            let _prev = self.mem_read(p).unwrap_or(0);
            let next = self.mem_read(p + 1).unwrap_or(0);
            let size = self.mem_read(p + 2).unwrap_or(0);
            print!(
                "  [{:08x}]: {}, size={} words\n",
                p as u32,
                if size > 0 { "IN USE" } else { "FREE" },
                size.abs()
            );
            if next <= 0 {
                break;
            }
            p = next;
        }
    }

    /// Ports `printX` (DEBUG register/value print, appended to `out`).
    pub fn print_x(&mut self, kind: i32, v: i32, out: &mut String) {
        if kind == 0 {
            return;
        }
        let reg = Decoded::get_register_or_value(kind, v as i64);
        if is_fp_kind(kind) {
            let f = (v & 0x3F) as usize;
            if f < FPR_COUNT {
                // Java "%s: %.17g  "
                out.push_str(&format!("{}: {}  ", reg, format_g(self.f[f], 17)));
            }
        } else if is_reg_kind(kind) {
            let r = (v & 0x3F) as usize;
            if r == 32 {
                out.push_str(&format!("SR: {}", self.format_sr()));
            } else if r < GPR_COUNT - 3 {
                out.push_str(&format!("{}: {}  ", reg, self.r[r]));
            } else if r < GPR_COUNT {
                out.push_str(&format!("{}: {:08x}  ", reg, self.r[r] as u32));
            }
        } else if is_const_kind(kind) {
            out.push_str(&format!("C(12)={}  ", v));
        }
    }

    /// Trace-line output (ports the `trace` block of `run`).
    pub fn trace_line(&mut self, pc: i64, d: &Decoded) {
        // Java: "%5d:%-60.60s<addr> " then disassembly.
        let label = Decoded::find_nearest_label(&self.reverse_symbol_map, pc);
        let label_str = label.unwrap_or_default();
        // %-60.60s: left-justify, truncate to 60.
        let truncated: String = label_str.chars().take(60).collect();
        let padded = format!("{:<60}", truncated);
        print!("{:>5}:{}{:08x} ", self.get_pid(), padded, pc as u32);
        match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            d.disassemble(&self.reverse_symbol_map)
        })) {
            Ok(dis) => println!("{dis}"),
            Err(_) => print!("DECODE ERROR\n"),
        }
    }

    pub fn get_pid(&self) -> i64 {
        self.pid
    }
}

impl Decoded {
    /// Disassemble this instruction (ports `Decoded.disassemble`).
    pub fn disassemble(&self, reverse: &std::collections::HashMap<i64, String>) -> String {
        use crate::decode::CONDITION;
        use crate::opcode;
        let mut buffer = String::new();
        buffer.push_str(&format!("{:<10}\t", self.op_name()));
        match self.tt {
            0 => {
                if self.a != 0 {
                    buffer.push_str(&self.get(0));
                    if self.b != 0 {
                        buffer.push_str(", ");
                        buffer.push_str(&self.get(1));
                        if self.c != 0 {
                            buffer.push_str(", ");
                            buffer.push_str(&self.get(2));
                            if self.d != 0 {
                                buffer.push_str(", ");
                                buffer.push_str(&self.get(3));
                            }
                        }
                    }
                }
            }
            1 => {
                let v0 = if self.op == opcode::JUMP
                    || self.op == opcode::CALL
                    || self.op == opcode::READONLY
                {
                    let mut s = format!("0x{:x}", self.c1);
                    if let Some(label) = Decoded::find_nearest_label(reverse, self.c1) {
                        s += &format!(" ({})", label);
                    }
                    s
                } else {
                    format!("{} (0x{:X})", self.c1, self.c1)
                };
                buffer.push_str(&v0);
            }
            2 => {
                let mut v0 = self.get(0);
                if self.a == 1 && (self.op == opcode::JUMP || self.op == opcode::CALL) {
                    v0 = CONDITION[self.v0 as usize].to_string();
                }
                buffer.push_str(&v0);
                buffer.push_str(", ");
                let v1 = if self.op == opcode::JUMP || self.op == opcode::CALL {
                    let mut s = format!("0x{:x}", self.c2);
                    if let Some(label) = Decoded::find_nearest_label(reverse, self.c2) {
                        s += &format!(" ({})", label);
                    }
                    s
                } else {
                    format!("{} (0x{:X})", self.c2, self.c2)
                };
                buffer.push_str(&v1);
            }
            3 => {
                let mut v0 = self.get(0);
                if self.a == 1 && (self.op == opcode::JUMP || self.op == opcode::CALL) {
                    v0 = CONDITION[self.v0 as usize].to_string();
                }
                buffer.push_str(&v0);
                buffer.push_str(", ");
                buffer.push_str(&self.get(1));
                buffer.push_str(", ");
                let v2 = if self.op == opcode::JUMP || self.op == opcode::CALL {
                    format!("0x{:x}", self.c3)
                } else {
                    format!("{} (0x{:X})", self.c3, self.c3)
                };
                buffer.push_str(&v2);
            }
            _ => {}
        }
        buffer
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cpu::Simulator;
    use std::collections::HashMap;

    #[test]
    fn heading_line_matches_reference() {
        let mut s = Simulator::new(1_000_000, 0, 8000, vec![]);
        s.load_program(&[0i64], 0, HashMap::new());
        let out = s.cpu_state_string();
        let heading = out.lines().nth(2).unwrap();
        // From reference T13_ADD.out.
        assert_eq!(
            heading,
            "  R               hex                   dec  FP                    float   Stack               hex                   dec                   float"
        );
    }

    #[test]
    fn r0_line_format() {
        let mut s = Simulator::new(1_000_000, 0, 8000, vec![]);
        s.load_program(&[0i64], 0, HashMap::new());
        let out = s.cpu_state_string();
        // R0 line begins " R0: 0000000000000000                     0  F0:       0.000000000000000"
        let r0 = out.lines().find(|l| l.starts_with(" R0:")).unwrap();
        assert!(r0.starts_with(" R0: 0000000000000000                     0  F0:       0.000000000000000"));
    }
}
