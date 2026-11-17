//! Opcode execution (ports the `exec` dispatch and all 39 `op*` handlers of
//! `Simulator.java`), with interleaved cycle accounting (Task 5).

use super::{CpuException, CpuResult, Simulator, R_PC, R_SF, R_SP};
use crate::decode::{
    is_const_kind, is_fp_kind, is_none_kind, is_o_kind, is_q_kind, is_reg_kind, is_y_kind,
    Decoded, FPR_COUNT, GPR_COUNT,
};

impl Simulator {
    fn to_reg_index(&self, _kind: i32, val12: i32) -> usize {
        (val12 & 0x3F) as usize
    }

    fn get_const(&self, kind: i32, val12: i32) -> CpuResult<i32> {
        if !is_const_kind(kind) {
            return Err(CpuException("Illegal constant argument.".into()));
        }
        Ok(val12)
    }

    fn get_r_kind(&self, kind: i32, val12: i32) -> CpuResult<i64> {
        if !is_reg_kind(kind) {
            return Err(CpuException("Illegal A/R argument.".into()));
        }
        Ok(self.r[self.to_reg_index(kind, val12)])
    }

    fn set_r_kind(&mut self, kind: i32, val12: i32, bits: i64) -> CpuResult<()> {
        if is_reg_kind(kind) {
            let r = self.to_reg_index(kind, val12);
            self.r[r] = bits;
            self.set_flags_int(bits, false);
            Ok(())
        } else {
            Err(CpuException("Illegal A/R argument.".into()))
        }
    }

    fn get_fp_kind(&self, kind: i32, val12: i32) -> CpuResult<f64> {
        if !is_fp_kind(kind) {
            return Err(CpuException("Illegal F argument.".into()));
        }
        Ok(self.f[self.to_reg_index(kind, val12)])
    }

    fn set_fp_kind(&mut self, kind: i32, val12: i32, v: f64) -> CpuResult<()> {
        if is_fp_kind(kind) {
            let r = self.to_reg_index(kind, val12);
            self.f[r] = v;
            self.set_flags_fp(v);
            Ok(())
        } else {
            Err(CpuException("Illegal F argument.".into()))
        }
    }

    fn get_y(&self, kind: i32, val12: i32) -> CpuResult<i64> {
        if is_fp_kind(kind) {
            Ok(self.f[self.to_reg_index(kind, val12)].to_bits() as i64)
        } else if is_reg_kind(kind) {
            Ok(self.r[self.to_reg_index(kind, val12)])
        } else {
            Err(CpuException("Illegal Y argument.".into()))
        }
    }

    fn set_y(&mut self, kind: i32, val12: i32, bits: i64) -> CpuResult<()> {
        if is_fp_kind(kind) {
            let r = self.to_reg_index(kind, val12);
            self.f[r] = f64::from_bits(bits as u64);
            self.set_flags_fp(self.f[r]);
            Ok(())
        } else if is_reg_kind(kind) {
            let r = self.to_reg_index(kind, val12);
            self.r[r] = bits;
            self.set_flags_int(bits, false);
            Ok(())
        } else {
            Err(CpuException("Illegal Y argument.".into()))
        }
    }

    fn get_o(&self, kind: i32, val12: i32) -> CpuResult<i64> {
        if is_reg_kind(kind) {
            Ok(self.r[self.to_reg_index(kind, val12)])
        } else if is_const_kind(kind) {
            Ok(val12 as i64)
        } else if is_none_kind(kind) {
            Ok(0)
        } else {
            Err(CpuException("Illegal O argument.".into()))
        }
    }

    fn get_q(&self, kind: i32, val12: i32) -> CpuResult<i64> {
        if is_fp_kind(kind) {
            Ok(self.f[self.to_reg_index(kind, val12)].to_bits() as i64)
        } else if is_reg_kind(kind) {
            Ok(self.r[self.to_reg_index(kind, val12)])
        } else if is_const_kind(kind) {
            Ok(val12 as i64)
        } else {
            Err(CpuException("Illegal Q argument.".into()))
        }
    }

    fn any_fp(d: &Decoded) -> bool {
        is_fp_kind(d.a) || is_fp_kind(d.b) || is_fp_kind(d.c) || is_fp_kind(d.d)
    }

    /// Dispatch one decoded instruction (ports `exec`), charging extra cycles.
    pub fn exec(&mut self, d: &Decoded) -> CpuResult<()> {
        match d.op {
            0 => self.op_nop_debug(d)?,
            1 => {
                self.op_clear(d)?;
                self.add_cycles("clear");
            }
            2 => {
                self.op_move(d)?;
                self.add_cycles("move");
            }
            3 => {
                self.op_load(d, false)?;
                self.add_cycles("load");
            }
            4 => {
                self.op_store(d, false)?;
                self.add_cycles("store");
            }
            5 => {
                self.op_pop(d)?;
                self.add_cycles("pop");
            }
            6 => {
                self.op_push(d)?;
                self.add_cycles("push");
            }
            7 => self.op_jump(d)?,
            8 => self.op_call(d)?,
            9 => {
                self.op_return(d)?;
                self.add_cycles("return");
            }
            10 => self.op_interrupt(d)?,
            11 => {
                self.running = false;
                self.add_cycles("stop");
            }
            12 => {
                self.op_negate(d)?;
                self.add_cycles(if Self::any_fp(d) { "fnegate" } else { "negate" });
            }
            13 => {
                self.op_add(d)?;
                self.add_cycles(if Self::any_fp(d) { "fadd" } else { "add" });
            }
            14 => {
                self.op_sub(d)?;
                self.add_cycles(if Self::any_fp(d) { "fsubtract" } else { "subtract" });
            }
            15 => {
                self.op_mult(d)?;
                self.add_cycles(if Self::any_fp(d) { "fmultiply" } else { "multiply" });
            }
            16 => {
                self.op_div_or_recip(d)?;
                // Matches Simulator.java case 16:
                //   addCycles(anyFP(d) ? "fdivide" : "divide")
                // RECIP's operand is FP, so anyFP is true and it charges
                // "fdivide" like any other FP divide.
                self.add_cycles(if Self::any_fp(d) { "fdivide" } else { "divide" });
            }
            17 => {
                self.op_compl(d)?;
                self.add_cycles("compliment");
            }
            18 => {
                self.bitwise(d)?;
                self.add_cycles("and");
            }
            19 => {
                self.bitwise(d)?;
                self.add_cycles("or");
            }
            20 => {
                self.bitwise(d)?;
                self.add_cycles("xor");
            }
            21 => {
                self.op_test(d)?;
                self.add_cycles("test");
            }
            22 => {
                self.op_cmp(d)?;
                self.add_cycles("compare");
            }
            23 => {
                self.bitwise(d)?;
                self.add_cycles("lshift");
            }
            24 => {
                self.bitwise(d)?;
                self.add_cycles("rshift");
            }
            25 => {
                self.bitwise(d)?;
                self.add_cycles("arshift");
            }
            26 => {
                self.bitwise(d)?;
                self.add_cycles("lrotate");
            }
            27 => {
                self.bitwise(d)?;
                self.add_cycles("rrotate");
            }
            28 => {
                let io_start = crate::util::nano_time();
                self.op_in(d)?;
                self.total_system_time_ns += crate::util::nano_time() - io_start;
                self.add_cycles("in");
            }
            29 => {
                let io_start = crate::util::nano_time();
                self.op_out(d)?;
                self.total_system_time_ns += crate::util::nano_time() - io_start;
                self.add_cycles("out");
            }
            30 => {
                self.op_pack(d)?;
                self.add_cycles("pack");
            }
            31 => {
                self.op_pack64(d)?;
                self.add_cycles("pack64");
            }
            32 => {
                self.op_unpack(d)?;
                self.add_cycles("unpack");
            }
            33 => {
                self.op_unpack64(d)?;
                self.add_cycles("unpack64");
            }
            34 => {
                self.op_cas(d)?;
                self.add_cycles("cas");
            }
            35 => {
                self.op_endian(d)?;
                self.add_cycles("endian");
            }
            36 => {
                self.op_save(d)?;
                self.add_cycles("save");
            }
            37 => {
                self.op_restore(d)?;
                self.add_cycles("restore");
            }
            38 => {
                self.op_readonly(d)?;
                self.add_cycles("readonly");
            }
            39 => {
                self.op_load(d, true)?;
                self.add_cycles("ldb");
            }
            40 => {
                self.op_store(d, true)?;
                self.add_cycles("stb");
            }
            _ => return Err(CpuException(format!("Unimplemented opcode: {}", d.op))),
        }
        Ok(())
    }

    // ---- 0: NOP / DEBUG ----
    fn op_nop_debug(&mut self, d: &Decoded) -> CpuResult<()> {
        let no_ops = d.tt == 0 && d.a == 0 && d.b == 0 && d.c == 0 && d.d == 0;
        if no_ops {
            return Ok(()); // NOP
        }
        if !self.debug && !self.trace {
            return Ok(());
        }
        // DEBUG forms: register prints or memory/heap dumps.
        if d.tt == 0 {
            if is_const_kind(d.b) && is_none_kind(d.c) && is_none_kind(d.d) && is_y_kind(d.a) {
                let addr = if is_reg_kind(d.a) {
                    self.get_r_kind(d.a, d.v0)?
                } else {
                    d.v0 as i64
                };
                let count = d.v1;
                if count == -1 {
                    self.print_heap_walk(addr);
                } else {
                    self.print_memory_dump(addr, count);
                }
            } else {
                let mut out = String::new();
                self.print_x(d.a, d.v0, &mut out);
                self.print_x(d.b, d.v1, &mut out);
                self.print_x(d.c, d.v2, &mut out);
                self.print_x(d.d, d.v3, &mut out);
                out.push('\n');
                print!("{out}");
            }
        } else if d.tt == 2 {
            let addr = d.c2;
            let count = d.v0;
            if count == -1 {
                self.print_heap_walk(addr);
            } else {
                self.print_memory_dump(addr, count);
            }
        } else {
            self.print_cpu_state();
        }
        Ok(())
    }

    // ---- 1: CLEAR ----
    fn op_clear(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 && d.a == 0 {
            self.clear_cpu_state();
            return Ok(());
        }
        for (k, v) in [(d.a, d.v0), (d.b, d.v1), (d.c, d.v2), (d.d, d.v3)] {
            if is_reg_kind(k) {
                self.set_r_kind(k, v, 0)?;
            }
            if is_fp_kind(k) {
                self.set_fp_kind(k, v, 0.0)?;
            }
        }
        self.sr = super::SR_Z;
        Ok(())
    }

    // ---- 2: MOVE ----
    fn op_move(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            match d.arg_count() {
                2 => {
                    if is_reg_kind(d.b) {
                        let k = self.get_r_kind(d.b, d.v1)?;
                        if is_reg_kind(d.a) {
                            self.set_r_kind(d.a, d.v0, k)?;
                            return Ok(());
                        } else if is_fp_kind(d.a) {
                            self.set_fp_kind(d.a, d.v0, k as f64)?;
                            return Ok(());
                        }
                    } else if is_fp_kind(d.b) {
                        let fp = self.get_fp_kind(d.b, d.v1)?;
                        if is_reg_kind(d.a) {
                            self.set_r_kind(d.a, d.v0, fp as i64)?;
                            return Ok(());
                        } else if is_fp_kind(d.a) {
                            self.set_fp_kind(d.a, d.v0, fp)?;
                            return Ok(());
                        }
                    }
                }
                3 => {
                    if is_reg_kind(d.a) && is_reg_kind(d.b) && is_reg_kind(d.c) {
                        let v = self.get_r_kind(d.b, d.v1)? + self.get_r_kind(d.c, d.v2)?;
                        self.set_r_kind(d.a, d.v0, v)?;
                        return Ok(());
                    }
                }
                4 => {
                    if is_const_kind(d.a) && is_y_kind(d.b) && is_q_kind(d.c) && is_q_kind(d.d) {
                        let (k, fp);
                        if self.test_cond(d.v0) {
                            if is_fp_kind(d.c) {
                                let v = self.get_fp_kind(d.c, d.v2)?;
                                fp = v;
                                k = v as i64;
                            } else {
                                let v = self.get_o(d.c, d.v2)?;
                                k = v;
                                fp = v as f64;
                            }
                        } else if is_fp_kind(d.d) {
                            let v = self.get_fp_kind(d.d, d.v3)?;
                            fp = v;
                            k = v as i64;
                        } else {
                            let v = self.get_o(d.d, d.v3)?;
                            k = v;
                            fp = v as f64;
                        }
                        if is_reg_kind(d.b) {
                            self.set_r_kind(d.b, d.v1, k)?;
                            return Ok(());
                        } else if is_fp_kind(d.b) {
                            self.set_fp_kind(d.b, d.v1, fp)?;
                            return Ok(());
                        }
                    }
                }
                _ => {}
            }
        } else if d.tt == 2 {
            let k = d.c2;
            if is_reg_kind(d.a) {
                self.set_r_kind(d.a, d.v0, k)?;
                return Ok(());
            } else if is_fp_kind(d.a) {
                self.set_fp_kind(d.a, d.v0, k as f64)?;
                return Ok(());
            }
        } else if d.tt == 3 {
            if is_reg_kind(d.a) && is_reg_kind(d.b) {
                let v = self.get_r_kind(d.b, d.v1)? + d.c3 as i64;
                self.set_r_kind(d.a, d.v0, v)?;
                return Ok(());
            }
        }
        Err(CpuException("Illegal MOVE arguments.".into()))
    }

    // ---- 39/40: LDB / STB helpers ----
    fn load_at(&mut self, ea: i64, byte_mode: bool) -> CpuResult<i64> {
        if byte_mode {
            self.mem_read_byte(ea)
        } else {
            self.mem_read(ea)
        }
    }

    fn store_at(&mut self, ea: i64, val: i64, byte_mode: bool) -> CpuResult<()> {
        if byte_mode {
            self.mem_write_byte(ea, val)
        } else {
            self.mem_write(ea, val)
        }
    }

    // ---- 3: LOAD ----
    fn op_load(&mut self, d: &Decoded, byte_mode: bool) -> CpuResult<()> {
        if d.tt == 2 {
            let addr = d.c2;
            let word = self.load_at(addr, byte_mode)?;
            self.set_y(d.a, d.v0, word)?;
            return Ok(());
        } else if d.tt == 0 {
            let count = d.arg_count();
            if count == 2 && is_y_kind(d.a) && is_reg_kind(d.b) {
                let addr = self.get_r_kind(d.b, d.v1)?;
                let w = self.load_at(addr, byte_mode)?;
                self.set_y(d.a, d.v0, w)?;
                return Ok(());
            }
            if count == 3 && is_y_kind(d.a) && is_reg_kind(d.b) && is_reg_kind(d.c) {
                let base = self.get_r_kind(d.b, d.v1)?;
                let off = self.get_r_kind(d.c, d.v2)?;
                let w = self.load_at(base + off, byte_mode)?;
                self.set_y(d.a, d.v0, w)?;
                return Ok(());
            }
        } else if d.tt == 3 {
            if is_y_kind(d.a) && is_o_kind(d.b) {
                let base = self.get_o(d.b, d.v1)?;
                let addr = base + d.c3 as i64;
                let w = self.load_at(addr, byte_mode)?;
                self.set_y(d.a, d.v0, w)?;
                return Ok(());
            }
        }
        Err(CpuException("Illegal LOAD arguments.".into()))
    }

    // ---- 4: STORE ----
    fn op_store(&mut self, d: &Decoded, byte_mode: bool) -> CpuResult<()> {
        if d.tt == 2 {
            let val = self.get_q(d.a, d.v0)?;
            self.store_at(d.c2, val, byte_mode)?;
            return Ok(());
        }
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 2 && is_q_kind(d.a) && is_reg_kind(d.b) {
                let addr = self.get_r_kind(d.b, d.v1)?;
                let val = self.get_q(d.a, d.v0)?;
                self.store_at(addr, val, byte_mode)?;
                return Ok(());
            }
            if count == 3 && is_q_kind(d.a) && is_reg_kind(d.b) && is_reg_kind(d.c) {
                let base = self.get_r_kind(d.b, d.v1)?;
                let addr = base + self.get_r_kind(d.c, d.v2)?;
                let val = self.get_q(d.a, d.v0)?;
                self.store_at(addr, val, byte_mode)?;
                return Ok(());
            }
        } else if d.tt == 3 {
            if is_q_kind(d.a) && is_reg_kind(d.b) {
                let base = self.get_r_kind(d.b, d.v1)?;
                let addr = base + d.c3 as i64;
                let val = self.get_q(d.a, d.v0)?;
                self.store_at(addr, val, byte_mode)?;
                return Ok(());
            } else if is_q_kind(d.a) && is_const_kind(d.b) {
                let base = self.get_o(d.b, d.v1)?;
                let addr = base + d.c3 as i64;
                let val = self.get_q(d.a, d.v0)?;
                self.store_at(addr, val, byte_mode)?;
                return Ok(());
            }
        }
        Err(CpuException("Illegal STORE arguments.".into()))
    }

    // ---- 5: POP ----
    fn op_pop(&mut self, d: &Decoded) -> CpuResult<()> {
        self.r[R_SP] += 1;
        let val = self.mem_read(self.r[R_SP])?;
        if d.tt == 0 {
            if d.a == 0 {
                return Ok(());
            } else if is_y_kind(d.a) {
                self.set_y(d.a, d.v0, val)?;
                return Ok(());
            }
        }
        Err(CpuException("POP form not implemented".into()))
    }

    // ---- 6: PUSH ----
    fn op_push(&mut self, d: &Decoded) -> CpuResult<()> {
        let val = if d.tt == 1 {
            d.c1
        } else if d.tt == 0 && is_y_kind(d.a) {
            self.get_y(d.a, d.v0)?
        } else {
            return Err(CpuException("Illegal PUSH instruction".into()));
        };
        self.mem_write(self.r[R_SP], val)?;
        self.r[R_SP] -= 1;
        if self.r[R_SP] < self.heap_limit {
            return Err(CpuException("Stack/Heap collision".into()));
        }
        Ok(())
    }

    // ---- 7: JUMP ----
    fn op_jump(&mut self, d: &Decoded) -> CpuResult<()> {
        let conditional = d.tt != 1 && is_const_kind(d.a);
        if conditional {
            if !self.test_cond(self.get_const(d.a, d.v0)?) {
                self.add_cycles("jump0");
                return Ok(());
            }
            self.add_cycles("jump1");
        } else {
            self.add_cycles("jump");
        }
        self.set_jump_target(d)
    }

    fn set_jump_target(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 1 {
            self.r[R_PC] = d.c1;
        } else if is_const_kind(d.a) {
            let addr =
                self.get_o(d.b, d.v1)? + self.get_o(d.c, d.v2)? + d.c1 + d.c2 + d.c3 as i64;
            self.r[R_PC] = addr;
        } else if is_reg_kind(d.a) {
            let addr =
                self.get_o(d.a, d.v0)? + self.get_o(d.b, d.v1)? + d.c1 + d.c2 + d.c3 as i64;
            self.r[R_PC] = addr;
        } else {
            return Err(CpuException("Illegal JUMP arguments.".into()));
        }
        Ok(())
    }

    // ---- 8: CALL ----
    fn op_call(&mut self, d: &Decoded) -> CpuResult<()> {
        let conditional = d.tt != 1 && is_const_kind(d.a);
        if conditional && !self.test_cond(self.get_const(d.a, d.v0)?) {
            self.add_cycles("call0");
            return Ok(());
        }
        self.add_cycles(if conditional { "call1" } else { "call" });
        self.mem_write(self.r[R_SP], self.r[R_PC])?;
        self.r[R_SP] -= 1;
        self.mem_write(self.r[R_SP], self.r[R_SF])?;
        self.r[R_SP] -= 1;
        self.r[R_SF] = self.r[R_SP];
        self.set_jump_target(d)
    }

    // ---- 9: RETURN ----
    fn op_return(&mut self, _d: &Decoded) -> CpuResult<()> {
        self.r[R_SP] = self.r[R_SF];
        self.r[R_SP] += 1;
        self.r[R_SF] = self.mem_read(self.r[R_SP])?;
        self.r[R_SP] += 1;
        self.r[R_PC] = self.mem_read(self.r[R_SP])?;
        if self.r[R_PC] < 0 {
            let code = self.r[R_PC] as i32;
            self.exit(code);
        }
        Ok(())
    }

    // ---- 10: INTERRUPT ----
    fn op_interrupt(&mut self, d: &Decoded) -> CpuResult<()> {
        let mut code: i64 = -1;
        let mut conditional = false;
        if d.tt == 1 {
            code = d.c1;
        } else if d.tt == 2 {
            conditional = true;
            code = if self.test_cond(self.get_const(d.a, d.v0)?) {
                d.c2
            } else {
                0
            };
        } else if d.tt == 0 {
            let count = d.arg_count();
            if count == 1 && is_o_kind(d.a) {
                code = self.get_o(d.a, d.v0)?;
            } else if count == 2 && is_o_kind(d.b) {
                conditional = true;
                code = if self.test_cond(self.get_const(d.a, d.v0)?) {
                    self.get_o(d.b, d.v1)?
                } else {
                    0
                };
            }
        }
        if code > 0 {
            self.add_cycles(if conditional { "interrupt1" } else { "interrupt" });
            let start = crate::util::nano_time();
            self.dispatch_interrupt(code as i32)?;
            self.total_system_time_ns += crate::util::nano_time() - start;
        } else if code == 0 {
            self.add_cycles("interrupt0");
        } else {
            return Err(CpuException("Illegal INTERRUPT arguments.".into()));
        }
        Ok(())
    }

    // ---- 12: NEGATE ----
    fn op_negate(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 && is_fp_kind(d.a) {
            let f = self.to_reg_index(d.a, d.v0);
            self.f[f] = -self.f[f];
            Ok(())
        } else if d.tt == 0 && is_reg_kind(d.a) {
            let r = self.to_reg_index(d.a, d.v0);
            let before = self.r[r];
            let res = before.wrapping_neg();
            self.r[r] = res;
            self.set_flags_int(res, before == i64::MIN);
            Ok(())
        } else {
            Err(CpuException("NEGATE form not implemented".into()))
        }
    }

    // Helpers for the arithmetic families: these centralize the overflow logic.
    fn add_int(&mut self, dst: usize, before: i64, rhs: i64) {
        let res = before.wrapping_add(rhs);
        let of = ((before ^ res) & (rhs ^ res)) < 0;
        self.r[dst] = res;
        self.set_flags_int(res, of);
    }
    fn sub_int(&mut self, dst: usize, before: i64, rhs: i64) {
        let res = before.wrapping_sub(rhs);
        let of = ((before ^ rhs) & (before ^ res)) < 0;
        self.r[dst] = res;
        self.set_flags_int(res, of);
    }
    // MULT overflow in the tt==0 paths uses res!=0 && res/rhs!=before.
    fn mult_int_t0(&mut self, dst: usize, before: i64, rhs: i64) {
        let res = before.wrapping_mul(rhs);
        let of = res != 0 && (rhs == 0 || res.wrapping_div(rhs) != before);
        self.r[dst] = res;
        self.set_flags_int(res, of);
    }
    // MULT overflow in the tt==2/tt==3 paths uses the Java quirk res==0 && ...
    fn mult_int_timm(&mut self, dst: usize, before: i64, rhs: i64) {
        let res = before.wrapping_mul(rhs);
        let of = res == 0 && (rhs == 0 || res.wrapping_div(rhs) != before);
        self.r[dst] = res;
        self.set_flags_int(res, of);
    }
    fn div_int(&mut self, dst: usize, before: i64, rhs: i64) -> CpuResult<()> {
        if rhs == 0 {
            return Err(CpuException("Division by zero".into()));
        }
        let of = before == i64::MIN && rhs == -1;
        let res = if of { i64::MIN } else { before.wrapping_div(rhs) };
        self.r[dst] = res;
        self.set_flags_int(res, of);
        Ok(())
    }

    // ---- 13: ADD ----
    fn op_add(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 2 {
                if is_reg_kind(d.a) && is_reg_kind(d.b) {
                    let rd = self.to_reg_index(d.a, d.v0);
                    let before = self.r[rd];
                    let rhs = self.r[self.to_reg_index(d.b, d.v1)];
                    self.add_int(rd, before, rhs);
                    return Ok(());
                } else if is_fp_kind(d.a) {
                    let rd = self.to_reg_index(d.a, d.v0);
                    let rhs = self.fp_or_int_rhs(d.b, d.v1);
                    let res = self.f[rd] + rhs;
                    self.f[rd] = res;
                    self.set_flags_fp(res);
                    return Ok(());
                }
            } else if count == 3 {
                if is_reg_kind(d.a) && is_reg_kind(d.b) && is_reg_kind(d.c) {
                    let rd = self.to_reg_index(d.b, d.v1);
                    let before = self.r[rd];
                    let rhs = self.r[self.to_reg_index(d.c, d.v2)];
                    let dst = self.to_reg_index(d.a, d.v0);
                    self.add_int(dst, before, rhs);
                    return Ok(());
                } else if is_fp_kind(d.a) && is_fp_kind(d.b) {
                    let rd = self.to_reg_index(d.b, d.v1);
                    let rhs = self.fp_or_int_rhs(d.c, d.v2);
                    let res = self.f[rd] + rhs;
                    self.f[self.to_reg_index(d.a, d.v0)] = res;
                    self.set_flags_fp(res);
                    return Ok(());
                }
            }
        } else if d.tt == 2 && is_y_kind(d.a) {
            if is_reg_kind(d.a) {
                let rd = self.to_reg_index(d.a, d.v0);
                let before = self.r[rd];
                self.add_int(rd, before, d.c2);
                return Ok(());
            } else if is_fp_kind(d.a) {
                let rd = self.to_reg_index(d.a, d.v0);
                let res = self.f[rd] + d.c2 as f64;
                self.f[rd] = res;
                self.set_flags_fp(res);
                return Ok(());
            }
        } else if d.tt == 3 {
            if is_reg_kind(d.a) && is_reg_kind(d.b) {
                let rd = self.to_reg_index(d.b, d.v1);
                let before = self.r[rd];
                let dst = self.to_reg_index(d.a, d.v0);
                self.add_int(dst, before, d.c3 as i64);
                return Ok(());
            } else if is_fp_kind(d.a) && is_fp_kind(d.b) {
                let rd = self.to_reg_index(d.b, d.v1);
                let res = self.f[rd] + d.c3 as f64;
                self.f[self.to_reg_index(d.a, d.v0)] = res;
                self.set_flags_fp(res);
                return Ok(());
            }
        }
        Err(CpuException("ADD form not implemented".into()))
    }

    fn fp_or_int_rhs(&self, kind: i32, val12: i32) -> f64 {
        if is_fp_kind(kind) {
            self.f[self.to_reg_index(kind, val12)]
        } else {
            self.r[self.to_reg_index(kind, val12)] as f64
        }
    }

    // ---- 14: SUB ----
    fn op_sub(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 2 {
                if is_reg_kind(d.a) && is_reg_kind(d.b) {
                    let rd = self.to_reg_index(d.a, d.v0);
                    let before = self.r[rd];
                    let rhs = self.r[self.to_reg_index(d.b, d.v1)];
                    self.sub_int(rd, before, rhs);
                    return Ok(());
                } else if is_fp_kind(d.a) {
                    let rd = self.to_reg_index(d.a, d.v0);
                    let rhs = self.fp_or_int_rhs(d.b, d.v1);
                    let res = self.f[rd] - rhs;
                    self.f[rd] = res;
                    self.set_flags_fp(res);
                    return Ok(());
                }
            } else if count == 3 {
                if is_reg_kind(d.a) && is_reg_kind(d.b) && is_reg_kind(d.c) {
                    let rd = self.to_reg_index(d.b, d.v1);
                    let before = self.r[rd];
                    let rhs = self.r[self.to_reg_index(d.c, d.v2)];
                    let dst = self.to_reg_index(d.a, d.v0);
                    self.sub_int(dst, before, rhs);
                    return Ok(());
                } else if is_fp_kind(d.a) && is_fp_kind(d.b) {
                    let rd = self.to_reg_index(d.b, d.v1);
                    let rhs = self.fp_or_int_rhs(d.c, d.v2);
                    let res = self.f[rd] - rhs;
                    self.f[self.to_reg_index(d.a, d.v0)] = res;
                    self.set_flags_fp(res);
                    return Ok(());
                }
            }
        } else if d.tt == 2 && is_y_kind(d.a) {
            if is_reg_kind(d.a) {
                let rd = self.to_reg_index(d.a, d.v0);
                let before = self.r[rd];
                self.sub_int(rd, before, d.c2);
                return Ok(());
            } else if is_fp_kind(d.a) {
                let rd = self.to_reg_index(d.a, d.v0);
                let res = self.f[rd] - d.c2 as f64;
                self.f[rd] = res;
                self.set_flags_fp(res);
                return Ok(());
            }
        } else if d.tt == 3 {
            if is_reg_kind(d.a) && is_reg_kind(d.b) {
                let rd = self.to_reg_index(d.b, d.v1);
                let before = self.r[rd];
                let dst = self.to_reg_index(d.a, d.v0);
                self.sub_int(dst, before, d.c3 as i64);
                return Ok(());
            } else if is_fp_kind(d.a) && is_fp_kind(d.b) {
                let rd = self.to_reg_index(d.b, d.v1);
                let res = self.f[rd] - d.c3 as f64;
                self.f[self.to_reg_index(d.a, d.v0)] = res;
                self.set_flags_fp(res);
                return Ok(());
            }
        }
        Err(CpuException("SUB form not implemented".into()))
    }

    // ---- 15: MULT ----
    fn op_mult(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 2 {
                if is_reg_kind(d.a) && is_reg_kind(d.b) {
                    let rd = self.to_reg_index(d.a, d.v0);
                    let before = self.r[rd];
                    let rhs = self.r[self.to_reg_index(d.b, d.v1)];
                    self.mult_int_t0(rd, before, rhs);
                    return Ok(());
                } else if is_fp_kind(d.a) {
                    let rd = self.to_reg_index(d.a, d.v0);
                    let rhs = self.fp_or_int_rhs(d.b, d.v1);
                    let res = self.f[rd] * rhs;
                    self.f[rd] = res;
                    self.set_flags_fp(res);
                    return Ok(());
                }
            } else if count == 3 {
                if is_reg_kind(d.a) && is_reg_kind(d.b) && is_reg_kind(d.c) {
                    let rd = self.to_reg_index(d.b, d.v1);
                    let before = self.r[rd];
                    let rhs = self.r[self.to_reg_index(d.c, d.v2)];
                    let dst = self.to_reg_index(d.a, d.v0);
                    self.mult_int_t0(dst, before, rhs);
                    return Ok(());
                } else if is_fp_kind(d.a) && is_fp_kind(d.b) {
                    let rd = self.to_reg_index(d.b, d.v1);
                    let rhs = self.fp_or_int_rhs(d.c, d.v2);
                    let res = self.f[rd] * rhs;
                    self.f[self.to_reg_index(d.a, d.v0)] = res;
                    self.set_flags_fp(res);
                    return Ok(());
                }
            }
        } else if d.tt == 2 && is_y_kind(d.a) {
            if is_reg_kind(d.a) {
                let rd = self.to_reg_index(d.a, d.v0);
                let before = self.r[rd];
                self.mult_int_timm(rd, before, d.c2);
                return Ok(());
            } else if is_fp_kind(d.a) {
                let rd = self.to_reg_index(d.a, d.v0);
                let res = self.f[rd] * d.c2 as f64;
                self.f[rd] = res;
                self.set_flags_fp(res);
                return Ok(());
            }
        } else if d.tt == 3 {
            if is_reg_kind(d.a) && is_reg_kind(d.b) {
                let rd = self.to_reg_index(d.b, d.v1);
                let before = self.r[rd];
                let dst = self.to_reg_index(d.a, d.v0);
                self.mult_int_timm(dst, before, d.c3 as i64);
                return Ok(());
            } else if is_fp_kind(d.a) && is_fp_kind(d.b) {
                let rd = self.to_reg_index(d.b, d.v1);
                let res = self.f[rd] * d.c3 as f64;
                self.f[self.to_reg_index(d.a, d.v0)] = res;
                self.set_flags_fp(res);
                return Ok(());
            }
        }
        Err(CpuException("MULT form not implemented".into()))
    }

    // ---- 16: DIV / RECIP ----
    fn op_div_or_recip(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 1 {
                let rd = self.to_reg_index(d.a, d.v0);
                let res = 1.0 / self.get_fp_kind(d.a, d.v0)?;
                self.f[rd] = res;
                self.set_flags_fp(res);
                return Ok(());
            } else if count == 2 {
                if is_reg_kind(d.a) && is_reg_kind(d.b) {
                    let rd = self.to_reg_index(d.a, d.v0);
                    let before = self.r[rd];
                    let rhs = self.r[self.to_reg_index(d.b, d.v1)];
                    self.div_int(rd, before, rhs)?;
                    return Ok(());
                } else if is_fp_kind(d.a) {
                    let rd = self.to_reg_index(d.a, d.v0);
                    let rhs = self.fp_or_int_rhs(d.b, d.v1);
                    let res = self.f[rd] / rhs;
                    self.f[rd] = res;
                    self.set_flags_fp(res);
                    return Ok(());
                }
            } else if count == 3 {
                if is_reg_kind(d.a) && is_reg_kind(d.b) && is_reg_kind(d.c) {
                    let rd = self.to_reg_index(d.b, d.v1);
                    let before = self.r[rd];
                    let rhs = self.r[self.to_reg_index(d.c, d.v2)];
                    let dst = self.to_reg_index(d.a, d.v0);
                    self.div_int(dst, before, rhs)?;
                    return Ok(());
                } else if is_fp_kind(d.a) && is_fp_kind(d.b) {
                    let rd = self.to_reg_index(d.b, d.v1);
                    let rhs = self.fp_or_int_rhs(d.c, d.v2);
                    let res = self.f[rd] / rhs;
                    self.f[self.to_reg_index(d.a, d.v0)] = res;
                    self.set_flags_fp(res);
                    return Ok(());
                }
            } else if count == 4 {
                let a = self.get_r_kind(d.c, d.v2)?;
                let b = self.get_o(d.d, d.v3)?;
                if b == 0 {
                    return Err(CpuException("Division by zero".into()));
                }
                let of = a == i64::MIN && b == -1;
                let q = if of { i64::MIN } else { a.wrapping_div(b) };
                let rem = if of { 0 } else { a.wrapping_rem(b) };
                self.set_r_kind(d.a, d.v0, q)?;
                self.set_r_kind(d.b, d.v1, rem)?;
                let quotient = self.get_r_kind(d.a, d.v0)?;
                self.set_flags_int(quotient, of);
                return Ok(());
            }
        } else if d.tt == 2 && is_y_kind(d.a) {
            if is_reg_kind(d.a) {
                let rd = self.to_reg_index(d.a, d.v0);
                let before = self.r[rd];
                self.div_int(rd, before, d.c2)?;
                return Ok(());
            } else if is_fp_kind(d.a) {
                let rd = self.to_reg_index(d.a, d.v0);
                let res = self.f[rd] / d.c2 as f64;
                self.f[rd] = res;
                self.set_flags_fp(res);
                return Ok(());
            }
        } else if d.tt == 3 {
            if is_reg_kind(d.a) && is_reg_kind(d.b) {
                let rd = self.to_reg_index(d.b, d.v1);
                let before = self.r[rd];
                let dst = self.to_reg_index(d.a, d.v0);
                self.div_int(dst, before, d.c3 as i64)?;
                return Ok(());
            } else if is_fp_kind(d.a) && is_fp_kind(d.b) {
                let rd = self.to_reg_index(d.b, d.v1);
                let res = self.f[rd] / d.c3 as f64;
                self.f[self.to_reg_index(d.a, d.v0)] = res;
                self.set_flags_fp(res);
                return Ok(());
            }
        }
        Err(CpuException("DIV/RECIP form not implemented".into()))
    }

    // ---- 17: COMPL ----
    fn op_compl(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 && is_reg_kind(d.a) {
            let v = !self.get_r_kind(d.a, d.v0)?;
            self.set_r_kind(d.a, d.v0, v)?;
            return Ok(());
        }
        Err(CpuException("COMPL form not implemented".into()))
    }

    // ---- 18/19/20/23..27: AND/OR/XOR/shifts/rotates ----
    fn bitwise(&mut self, d: &Decoded) -> CpuResult<()> {
        let (mut lhs, mut rhs) = (0i64, 0i64);
        let mut is_good = false;
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 2 {
                lhs = self.get_r_kind(d.a, d.v0)?;
                rhs = self.get_r_kind(d.b, d.v1)?;
                is_good = true;
            } else if count == 3 {
                lhs = self.get_o(d.b, d.v1)?;
                rhs = self.get_o(d.c, d.v2)?;
                is_good = true;
            }
        } else if d.tt == 2 {
            lhs = self.get_r_kind(d.a, d.v0)?;
            rhs = d.c2;
            is_good = true;
        } else if d.tt == 3 {
            lhs = self.get_r_kind(d.b, d.v1)?;
            rhs = d.c3 as i64;
            is_good = true;
        }
        if !is_good {
            return Err(CpuException("Illegal bitwise arguments".into()));
        }
        if rhs < 0 && d.op >= 23 {
            return Err(CpuException(
                "Illegal negative shift on bitwise operator".into(),
            ));
        }
        let res = match d.op {
            18 => lhs & rhs,
            19 => lhs | rhs,
            20 => lhs ^ rhs,
            23 => ((lhs as u64) << (rhs as u64 & 63)) as i64,
            24 => ((lhs as u64) >> (rhs as u64 & 63)) as i64, // logical >>>
            25 => lhs >> (rhs & 63),                           // arithmetic >>
            26 => (lhs as u64).rotate_left((rhs as u32) & 63) as i64,
            27 => (lhs as u64).rotate_right((rhs as u32) & 63) as i64,
            _ => return Err(CpuException("Illegal bitwise operator".into())),
        };
        self.set_r_kind(d.a, d.v0, res)
    }

    // ---- 21: TEST ----
    fn op_test(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt != 1 {
            if is_fp_kind(d.a) {
                let f = self.get_fp_kind(d.a, d.v0)?;
                self.set_flags_fp(f);
                return Ok(());
            } else {
                let bits = self.get_o(d.a, d.v0)?;
                self.set_flags_int(bits, false);
                return Ok(());
            }
        }
        Err(CpuException("TEST form not implemented".into()))
    }

    // ---- 22: CMP ----
    fn op_cmp(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 && !is_fp_kind(d.a) && !is_fp_kind(d.b) {
            let a1 = self.get_q(d.a, d.v0)?;
            let a2 = self.get_q(d.b, d.v1)?;
            let res = a1.wrapping_sub(a2);
            self.set_flags_from_subtract(a1, a2, res);
            return Ok(());
        }
        if d.tt == 2 && !is_fp_kind(d.a) {
            let a = self.get_y(d.a, d.v0)?;
            let c = d.c2;
            let res = a.wrapping_sub(c);
            self.set_flags_from_subtract(a, c, res);
            return Ok(());
        }
        if d.tt == 0 && is_fp_kind(d.a) && is_fp_kind(d.b) {
            let f1 = self.f[self.to_reg_index(d.a, d.v0)];
            let f2 = self.f[self.to_reg_index(d.b, d.v1)];
            let res = (f1 - f2).signum() as i64;
            // Java Math.signum of 0.0 is 0.0; of negative is -1; positive +1.
            let res = if (f1 - f2) == 0.0 { 0 } else { res };
            self.set_flags_int(res, false);
            return Ok(());
        }
        Err(CpuException("CMP form not implemented".into()))
    }

    // ---- 28: IN ----
    fn op_in(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 && d.arg_count() == 3 {
            let bytes = self.get_o(d.b, d.v1)? as i32;
            let port = self.get_o(d.c, d.v2)? as i32;
            if is_fp_kind(d.a) && bytes != 8 {
                return Err(CpuException("IN for FP must be 8 bytes".into()));
            } else if !(0..=8).contains(&bytes) {
                return Err(CpuException("IN number of bytes must be 0-8".into()));
            }
            let val = self.port_in(port, bytes)?;
            self.set_y(d.a, d.v0, val)?;
            return Ok(());
        }
        Err(CpuException("IN form not implemented".into()))
    }

    // ---- 29: OUT ----
    fn op_out(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 && d.arg_count() == 3 {
            let bytes = self.get_o(d.b, d.v1)? as i32;
            let port = self.get_o(d.c, d.v2)? as i32;
            let val = self.get_q(d.a, d.v0)?;
            if is_fp_kind(d.a) && bytes != 8 {
                return Err(CpuException("OUT for FP must be 8 bytes".into()));
            } else if !(0..=8).contains(&bytes) {
                return Err(CpuException("OUT number of bytes must be 0-8".into()));
            }
            self.port_out(port, val, bytes)?;
            return Ok(());
        }
        Err(CpuException("OUT form not implemented".into()))
    }

    // ---- 30: PACK ----
    fn op_pack(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 2 {
                let v = ((self.get_r_kind(d.a, d.v0)? & 0xFFFF) << 16)
                    | (self.get_r_kind(d.b, d.v1)? & 0xFFFF);
                self.set_r_kind(d.a, d.v0, v)?;
                return Ok(());
            } else if count == 4 {
                let v = ((self.get_r_kind(d.a, d.v0)? & 0xFF) << 24)
                    | ((self.get_r_kind(d.b, d.v1)? & 0xFF) << 16)
                    | ((self.get_r_kind(d.c, d.v2)? & 0xFF) << 8)
                    | (self.get_r_kind(d.d, d.v3)? & 0xFF);
                self.set_r_kind(d.a, d.v0, v)?;
                return Ok(());
            }
        }
        Err(CpuException("PACK form not implemented".into()))
    }

    // ---- 31: PACK64 ----
    fn op_pack64(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 2 {
                let v = ((self.get_r_kind(d.a, d.v0)? & 0xFFFFFFFF) << 32)
                    | (self.get_r_kind(d.b, d.v1)? & 0xFFFFFFFF);
                self.set_r_kind(d.a, d.v0, v)?;
                return Ok(());
            } else if count == 4 {
                let v = ((self.get_r_kind(d.a, d.v0)? & 0xFFFF) << 48)
                    | ((self.get_r_kind(d.b, d.v1)? & 0xFFFF) << 32)
                    | ((self.get_r_kind(d.c, d.v2)? & 0xFFFF) << 16)
                    | (self.get_r_kind(d.d, d.v3)? & 0xFFFF);
                self.set_r_kind(d.a, d.v0, v)?;
                return Ok(());
            }
        }
        Err(CpuException("PACK64 form not implemented".into()))
    }

    // ---- 32: UNPACK ----
    fn op_unpack(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 2 {
                let v = self.get_r_kind(d.a, d.v0)?;
                self.set_r_kind(d.a, d.v0, (v >> 16) & 0xFFFF)?;
                self.set_r_kind(d.b, d.v1, v & 0xFFFF)?;
                return Ok(());
            } else if count == 4 {
                let mut v = self.get_r_kind(d.a, d.v0)?;
                self.set_r_kind(d.d, d.v3, v & 0xFF)?;
                v >>= 8;
                self.set_r_kind(d.c, d.v2, v & 0xFF)?;
                v >>= 8;
                self.set_r_kind(d.b, d.v1, v & 0xFF)?;
                v >>= 8;
                self.set_r_kind(d.a, d.v0, v & 0xFF)?;
                return Ok(());
            }
        }
        Err(CpuException("UNPACK form not implemented".into()))
    }

    // ---- 33: UNPACK64 ----
    fn op_unpack64(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 2 {
                let v = self.get_r_kind(d.a, d.v0)?;
                self.set_r_kind(d.a, d.v0, (v >> 32) & 0xFFFFFFFF)?;
                self.set_r_kind(d.b, d.v1, v & 0xFFFFFFFF)?;
                return Ok(());
            } else if count == 4 {
                let mut v = self.get_r_kind(d.a, d.v0)?;
                self.set_r_kind(d.d, d.v3, v & 0xFFFF)?;
                v >>= 16;
                self.set_r_kind(d.c, d.v2, v & 0xFFFF)?;
                v >>= 16;
                self.set_r_kind(d.b, d.v1, v & 0xFFFF)?;
                v >>= 16;
                self.set_r_kind(d.a, d.v0, v & 0xFFFF)?;
                return Ok(());
            }
        }
        Err(CpuException("UNPACK64 form not implemented".into()))
    }

    // ---- 34: CAS ----
    fn op_cas(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 3 || count == 4 {
                let old_val = self.get_o(d.a, d.v0)?;
                let new_val = self.get_o(d.b, d.v1)?;
                let base = self.get_r_kind(d.c, d.v2)?;
                let offset = if count == 4 { self.get_o(d.d, d.v3)? } else { 0 };
                let addr = base + offset;
                // Parity with Java opCAS: the compare-and-swap must be a single
                // indivisible operation, not a separate read then write.
                //   * addr < 0      -> shared memory: lock held across cmp+write.
                //   * addr < limit  -> heap: atomic compare_exchange on the cell.
                //   * else          -> stack: per-thread, plain cmp+write.
                let ok = if addr < 0 {
                    crate::cpu::memory::shared_mem_compare_and_set(addr, old_val, new_val)
                        .map_err(CpuException)?
                } else if addr < self.heap_limit {
                    let idx = addr as usize;
                    if idx >= self.mem.len() {
                        return Err(CpuException(format!(
                            "Illegal CAS access of {:08x}",
                            addr
                        )));
                    }
                    self.mem.compare_and_set(idx, old_val, new_val)
                } else {
                    let idx = (addr - self.heap_limit) as usize;
                    if idx >= self.stack.len() {
                        return Err(CpuException(format!(
                            "Illegal CAS access of {:08x}",
                            addr
                        )));
                    }
                    let cur = self.stack[idx];
                    if cur == old_val {
                        self.stack[idx] = new_val;
                        true
                    } else {
                        false
                    }
                };
                self.set_flags_int(new_val, ok);
                return Ok(());
            }
        }
        Err(CpuException("CAS form not implemented".into()))
    }

    // ---- 35: ENDIAN ----
    fn op_endian(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            if d.arg_count() != 2 {
                return Err(CpuException("ENDIAN must have two arguments".into()));
            }
            let little_endian = self.get_o(d.b, d.v1)? != 0;
            let port = self.get_o(d.a, d.v0)? as i32;
            self.port_set_little_endian(port, little_endian);
        }
        Ok(())
    }

    // ---- 36: SAVE ----
    pub fn op_save(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 1 || count == 2 {
                if d.a != d.b {
                    return Err(CpuException("SAVE operands must be same type".into()));
                }
                if is_reg_kind(d.a) {
                    let hi = if count == 1 { 28 } else { d.v1 };
                    for i in d.v0..=hi {
                        self.push(self.r[i as usize])?;
                    }
                    return Ok(());
                } else if is_fp_kind(d.a) {
                    let hi = if count == 1 { (FPR_COUNT - 1) as i32 } else { d.v1 };
                    for i in d.v0..=hi {
                        self.fpush(self.f[i as usize])?;
                    }
                    return Ok(());
                }
            }
        }
        Err(CpuException("SAVE form not implemented".into()))
    }

    // ---- 37: RESTORE ----
    pub fn op_restore(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 0 {
            let count = d.arg_count();
            if count == 1 || count == 2 {
                if d.a != d.b {
                    return Err(CpuException("RESTORE operands must be same type".into()));
                }
                if is_reg_kind(d.a) {
                    let hi = if count == 1 { 28 } else { d.v1 };
                    for i in (d.v0..=hi).rev() {
                        self.r[i as usize] = self.pop()?;
                    }
                    return Ok(());
                } else if is_fp_kind(d.a) {
                    let hi = if count == 1 { (FPR_COUNT - 1) as i32 } else { d.v1 };
                    for i in (d.v0..=hi).rev() {
                        self.f[i as usize] = self.fpop()?;
                    }
                    return Ok(());
                }
            }
        }
        Err(CpuException("RESTORE form not implemented".into()))
    }

    // ---- 38: READONLY ----
    fn op_readonly(&mut self, d: &Decoded) -> CpuResult<()> {
        if d.tt == 1 {
            self.protect(self.r[R_PC], d.c1)?;
            Ok(())
        } else {
            Err(CpuException("READONLY form not implemented".into()))
        }
    }

    pub fn exit(&mut self, code: i32) {
        self.set_r(0, code as i64);
        self.running = false;
    }

    // ===== Run loop / stepping =====
    pub fn run(&mut self, start_pc: i64) -> CpuResult<i32> {
        self.running = true;
        self.cycles = 0;
        self.total_system_time_ns = 0;
        self.start_clock_ns = crate::util::nano_time();
        self.r[R_PC] = start_pc;
        let mut decoded = Decoded::default();
        while self.running {
            let pc = self.r[R_PC];
            let instr = self.fetch(pc)?;
            self.r[R_PC] = pc + 1;
            decoded.decode_into(instr);
            if self.trace {
                self.trace_line(pc, &decoded);
            }
            if let Err(e) = self.exec(&decoded) {
                // Match the Java emulator: emit the symbolic location, PC, and
                // disassembled instruction to stderr before the exception
                // message propagates.
                let label =
                    Decoded::find_nearest_label(&self.reverse_symbol_map, pc).unwrap_or_default();
                let disasm = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                    decoded.disassemble(&self.reverse_symbol_map)
                }))
                .unwrap_or_else(|_| "DECODE ERROR".to_string());
                eprintln!("Exception at PC={} {}: {}", pc, label, disasm);
                return Err(e);
            }
        }
        if self.debug {
            self.print_cpu_state();
        }
        if self.is_root {
            self.free_args();
        }
        Ok(self.r[0] as i32)
    }

    pub fn init_for_debug(&mut self, start_pc: i64) {
        self.running = true;
        self.cycles = 0;
        self.total_system_time_ns = 0;
        self.start_clock_ns = crate::util::nano_time();
        self.r[R_PC] = start_pc;
    }

    pub fn step_one(&mut self) -> CpuResult<i32> {
        if !self.running {
            return Ok(-1);
        }
        let pc = self.r[R_PC];
        let instr = self.fetch(pc)?;
        self.r[R_PC] = pc + 1;
        let mut d = Decoded::default();
        d.decode_into(instr);
        self.exec(&d)?;
        Ok(d.op_code() as i32)
    }

    // Helper used by exec to know whether R0..R28 (GPR minus SF/SP/PC) — kept for
    // clarity though unused directly; silences dead-code if referenced later.
    #[allow(dead_code)]
    fn gpr_data_count() -> usize {
        GPR_COUNT - 3
    }
}

#[cfg(test)]
mod exec_tests {
    use super::*;
    use crate::cpu::Simulator;
    use crate::decode::{enc_t0, enc_t1, enc_t2};
    use crate::opcode;
    use std::collections::HashMap;

    fn run_prog(words: &[i64]) -> Simulator {
        let mut s = Simulator::new(4096, 0, 1024, vec![]);
        s.load_program(words, 0, HashMap::new());
        s.run(0).expect("run");
        s
    }

    #[test]
    fn move_add_stop() {
        // MOVE R0, #5  (tt2, a=reg(2), v0=0, c2=5)
        // ADD  R0, #3  (tt2, a=reg(2), v0=0, c2=3)
        // STOP          (tt1, op=11)
        let prog = vec![
            enc_t2(opcode::MOVE, 2, 0, 5),
            enc_t2(opcode::ADD, 2, 0, 3),
            enc_t1(opcode::STOP, 0),
        ];
        let s = run_prog(&prog);
        assert_eq!(s.r[0], 8);
        // 8 = 0b1000 -> odd parity (1 bit) => P set; non-zero, positive.
        assert_eq!(s.sr & crate::cpu::SR_P, crate::cpu::SR_P);
        assert_eq!(s.sr & crate::cpu::SR_Z, 0);
    }

    #[test]
    fn subtract_to_zero_sets_z() {
        // MOVE R0,#7 ; SUB R0,#7 ; STOP
        let prog = vec![
            enc_t2(opcode::MOVE, 2, 0, 7),
            enc_t2(opcode::SUBTRACT, 2, 0, 7),
            enc_t1(opcode::STOP, 0),
        ];
        let s = run_prog(&prog);
        assert_eq!(s.r[0], 0);
        assert_eq!(s.sr & crate::cpu::SR_Z, crate::cpu::SR_Z);
    }

    #[test]
    fn multiply() {
        // MOVE R0,#6 ; MULT R0,#7 ; STOP
        let prog = vec![
            enc_t2(opcode::MOVE, 2, 0, 6),
            enc_t2(opcode::MULTIPLY, 2, 0, 7),
            enc_t1(opcode::STOP, 0),
        ];
        let s = run_prog(&prog);
        assert_eq!(s.r[0], 42);
    }

    #[test]
    fn bitwise_and_shift() {
        // MOVE R0,#0xFF ; AND R0,#0x0F ; LSHIFT R0,#4 ; STOP => 0xF0
        let prog = vec![
            enc_t2(opcode::MOVE, 2, 0, 0xFF),
            enc_t2(opcode::AND, 2, 0, 0x0F),
            enc_t2(opcode::LSHIFT, 2, 0, 4),
            enc_t1(opcode::STOP, 0),
        ];
        let s = run_prog(&prog);
        assert_eq!(s.r[0], 0xF0);
    }

    #[test]
    fn push_pop_roundtrip() {
        // MOVE R0,#123 ; PUSH R0 ; MOVE R0,#0 ; POP R0 ; STOP
        let prog = vec![
            enc_t2(opcode::MOVE, 2, 0, 123),
            enc_t0(opcode::PUSH, 2, 0, 0, 0, 0, 0, 0, 0),
            enc_t2(opcode::MOVE, 2, 0, 0),
            enc_t0(opcode::POP, 2, 0, 0, 0, 0, 0, 0, 0),
            enc_t1(opcode::STOP, 0),
        ];
        let s = run_prog(&prog);
        assert_eq!(s.r[0], 123);
    }

    #[test]
    fn cycle_accounting_integer() {
        // MOVE R0,#5 ; ADD R0,#3 ; STOP
        // Each non-fp integer MOVE/ADD/STOP has 0 extra cycles; base +1 per
        // fetch. 3 instructions => 3 fetch cycles, no mem r/w, no extras.
        let prog = vec![
            enc_t2(opcode::MOVE, 2, 0, 5),
            enc_t2(opcode::ADD, 2, 0, 3),
            enc_t1(opcode::STOP, 0),
        ];
        let s = run_prog(&prog);
        assert_eq!(s.cycles, 3);
    }

    #[test]
    fn cycle_accounting_load() {
        // LOAD R0, [#0] (tt2 YC) ; STOP.
        // Built-in default matches instructions.data (load extra = 1), so:
        // fetch LOAD (+1) + memRead (+1) + load extra (1) + fetch STOP (+1) = 4.
        let prog = vec![enc_t2(opcode::LOAD, 2, 0, 0), enc_t1(opcode::STOP, 0)];
        let s = run_prog(&prog);
        assert_eq!(s.cycles, 4);
    }
}
#[cfg(test)]
mod byte_tests {
    //! LDB (39) / STB (40) tests mirroring `ByteAccessTest.java`.
    use crate::cpu::Simulator;
    use crate::decode::{enc_t0, enc_t1, enc_t2, enc_t3, Decoded};
    use crate::opcode;
    use std::collections::HashMap;

    fn stop() -> i64 {
        enc_t1(opcode::STOP, 0)
    }

    fn mk() -> Simulator {
        Simulator::new(4096, 0, 1024, vec![])
    }

    // MOVE Rn, #imm
    fn mv(r: i32, imm: i64) -> i64 {
        enc_t2(opcode::MOVE, 2, r, imm)
    }
    // LDB/STB Ra, Rb[#imm]   (tt3)
    fn t3(op: u8, a: i32, b: i32, imm: i32) -> i64 {
        enc_t3(op, 2, 2, a, b, imm)
    }
    // LDB/STB Ra, Rb[Rc]     (tt0, three registers)
    fn rrr(op: u8, a: i32, b: i32, c: i32) -> i64 {
        enc_t0(op, 2, 2, 2, 0, a, b, c, 0)
    }

    fn run_with(prog: &[i64], init: &[(usize, i64)]) -> Simulator {
        let mut s = mk();
        s.load_program(prog, 0, HashMap::new());
        for &(a, v) in init {
            s.mem.set(a, v);
        }
        s.run(0).expect("run");
        s
    }

    #[test]
    fn ldb_all_offsets_zero_extended() {
        let mut prog = vec![mv(10, 200 * 8)];
        for k in 0..8 {
            prog.push(t3(opcode::LDB, 1 + k, 10, k));
        }
        prog.push(mv(11, 201 * 8));
        prog.push(t3(opcode::LDB, 9, 11, 0));
        prog.push(t3(opcode::LDB, 12, 11, 3));
        prog.push(stop());
        let s = run_with(
            &prog,
            &[(200, 0x0102030405060708), (201, 0xFFEEDDCCBBAA9988u64 as i64)],
        );
        for k in 0..8usize {
            assert_eq!(s.r[1 + k], 1 + k as i64, "offset {k}");
        }
        assert_eq!(s.r[9], 0xFF); // zero-extended, not sign-extended
        assert_eq!(s.r[12], 0xCC);
    }

    #[test]
    fn ldb_register_offset_form() {
        let prog = vec![mv(10, 200 * 8), mv(2, 5), rrr(opcode::LDB, 1, 10, 2), stop()];
        let s = run_with(&prog, &[(200, 0x0102030405060708)]);
        assert_eq!(s.r[1], 0x06);
    }

    #[test]
    fn stb_leaves_neighbours_intact() {
        let prog = vec![
            mv(11, 200 * 8),
            mv(5, 0xAA),
            t3(opcode::STB, 5, 11, 3),
            enc_t2(opcode::LOAD, 2, 1, 200),
            mv(5, 0x1FF), // only the low 8 bits are stored
            t3(opcode::STB, 5, 11, 0),
            enc_t2(opcode::LOAD, 2, 2, 200),
            mv(4, 0x1234),
            t3(opcode::STB, 4, 11, 7),
            enc_t2(opcode::LOAD, 2, 3, 200),
            stop(),
        ];
        let s = run_with(&prog, &[(200, 0x0102030405060708)]);
        assert_eq!(s.r[1], 0x010203AA05060708);
        assert_eq!(s.r[2], 0xFF0203AA05060708u64 as i64);
        assert_eq!(s.r[3], 0xFF0203AA05060734u64 as i64);
    }

    #[test]
    fn string_copy_round_trip() {
        // .dcs layout: length word then big-endian packed bytes.
        let text = b"Hello, World!";
        let mut w1: i64 = 0;
        let mut w2: i64 = 0;
        for (i, b) in text.iter().enumerate() {
            if i < 8 {
                w1 |= (*b as i64) << ((7 - i) * 8);
            } else {
                w2 |= (*b as i64) << ((7 - (i - 8)) * 8);
            }
        }
        // r10 = byte addr of text (word 301), r11 = dest byte addr (word 400),
        // r9 = length. The copy loop is unrolled: 13 LDB/STB pairs.
        let mut prog = vec![
            mv(10, 301 * 8),
            mv(11, 400 * 8),
            enc_t2(opcode::LOAD, 2, 9, 300),
        ];
        for i in 0..13 {
            prog.push(mv(12, i));
            prog.push(rrr(opcode::LDB, 1, 10, 12));
            prog.push(rrr(opcode::STB, 1, 11, 12));
        }
        prog.push(enc_t2(opcode::LOAD, 2, 2, 400));
        prog.push(enc_t2(opcode::LOAD, 2, 3, 401));
        prog.push(stop());
        let s = run_with(&prog, &[(300, 13), (301, w1), (302, w2)]);
        assert_eq!(s.r[9], 13);
        assert_eq!(s.r[2], 0x48656C6C6F2C2057);
        assert_eq!(s.r[3], 0x6F726C6421000000);
    }

    #[test]
    fn readonly_violation() {
        let prog = vec![
            enc_t1(opcode::READONLY, 4),
            enc_t2(opcode::STB, 2, 0, 16), // byte addr 16 -> word 2, protected
            stop(),
            stop(),
        ];
        let mut s = mk();
        s.load_program(&prog, 0, HashMap::new());
        let e = s.run(0).unwrap_err();
        assert!(
            e.0.contains("Write access violation of 00000002"),
            "{}",
            e.0
        );
    }

    #[test]
    fn bounds_and_negative_ea_errors() {
        let mut s = mk();
        s.load_program(&[stop()], 0, HashMap::new());
        // Past the end of memory (Java checks the stack range first).
        assert!(s.mem_write_byte(8 * 100000, 1).is_err());
        let e = s.mem_write_byte(-8, 1).unwrap_err();
        assert!(e.0.contains("Illegal shared memory write access"), "{}", e.0);
        assert!(s.mem_read_byte(-8).is_err());
        let e = s.mem_write_byte(4096 * 8 + 1024 * 8, 1).unwrap_err();
        assert!(e.0.contains("Illegal stack write access"), "{}", e.0);
    }

    #[test]
    fn effective_address_shift() {
        let mut s = mk();
        s.load_program(&[stop()], 0, HashMap::new());
        let w = 200i64;
        s.mem_write_byte(w * 8, 0x11).unwrap();
        s.mem_write_byte(w * 8 + 7, 0x22).unwrap();
        s.mem_write_byte((w + 1) * 8 + 4, 0x33).unwrap();
        assert_eq!(s.mem_read(w).unwrap(), 0x1100000000000022);
        assert_eq!(s.mem_read(w + 1).unwrap(), 0x0000000033000000);
    }

    #[test]
    fn adjacent_byte_stores_are_atomic() {
        const ITER: i64 = 200_000;
        let mut a = mk();
        a.load_program(&[stop()], 0, HashMap::new());
        let mut b = mk();
        b.mem = a.mem.clone();
        let w = 200i64;
        std::thread::scope(|sc| {
            for (sim, base) in [(&mut a, 0i64), (&mut b, 4i64)] {
                sc.spawn(move || {
                    for i in 0..ITER {
                        for k in 0..4 {
                            sim.mem_write_byte(w * 8 + base + k, (i + base + k) & 0xFF)
                                .unwrap();
                        }
                    }
                });
            }
        });
        let mut expected: i64 = 0;
        for bi in 0..8 {
            expected = (expected << 8) | ((ITER - 1 + bi) & 0xFF);
        }
        assert_eq!(a.mem.get(w as usize), expected);
    }

    #[test]
    fn disassembly_and_cycles() {
        let ldb = enc_t2(opcode::LDB, 2, 1, 16);
        let stb = enc_t2(opcode::STB, 2, 1, 16);
        let mut d = Decoded::default();
        d.decode_into(ldb);
        assert!(d.disassemble(&HashMap::new()).starts_with("LDB"));
        d.decode_into(stb);
        assert!(d.disassemble(&HashMap::new()).starts_with("STB"));
        // LDB: fetch(1) + memRead(1) + ldb extra(1) + fetch STOP(1) = 4
        let s = run_with(&[enc_t2(opcode::LDB, 2, 0, 0), stop()], &[]);
        assert_eq!(s.cycles, 4);
        // STB: fetch(1) + write(1) + stb extra(0) + fetch STOP(1) = 3
        let s = run_with(&[enc_t2(opcode::STB, 2, 0, 8 * 200), stop()], &[]);
        assert_eq!(s.cycles, 3);
    }
}
