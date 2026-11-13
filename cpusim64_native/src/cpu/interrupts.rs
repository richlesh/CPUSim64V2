//! Interrupt dispatch (ports `StdInterruptHandler`). Non-concurrency interrupts
//! are implemented here; fork/thread/shared-memory interrupts are added in
//! Task 11 (`concurrency` wiring), but their IDs are matched here and routed to
//! the concurrency methods.

use super::{CpuException, CpuResult, Simulator, R_SF};
use crate::javacompat::format::{java_printf, message_format, number_placeholders, Arg};
use crate::javacompat::{regexadapter, string_compare_to, string_compare_to_ignore_case, string_hash_code};
use crate::util;
use regex::Regex;

// ===== interrupt IDs (ports the iXXX static finals) =====
const I_INT_MIN: i32 = 1;
const I_INT_MAX: i32 = 2;
const I_FLOAT_MIN: i32 = 3;
const I_FLOAT_LOWEST: i32 = 4;
const I_FLOAT_MAX: i32 = 5;
const I_NEGATIVE_INFINITY: i32 = 6;
const I_POSITIVE_INFINITY: i32 = 7;
const I_NAN: i32 = 8;

const I_CYCLES: i32 = 10;
const I_CLOCK: i32 = 11;
const I_SAVE: i32 = 12;
const I_SAVE_FP: i32 = 13;
const I_RESTORE: i32 = 14;
const I_RESTORE_FP: i32 = 15;
const I_PRINT_CPU_STATE: i32 = 16;

const I_ALLOC: i32 = 20;
const I_REALLOC: i32 = 21;
const I_FREE: i32 = 22;
const I_MEMMOVE: i32 = 23;
const I_MEMCLEAR: i32 = 24;
const I_ALLOC_COUNT: i32 = 25;
const I_FREE_COUNT: i32 = 26;
const I_ALLOC_SIZE: i32 = 27;
const I_FREE_SIZE: i32 = 28;
const I_WALK_HEAP: i32 = 29;
const I_ARGC: i32 = 30;
const I_ARGS: i32 = 31;
const I_EXIT: i32 = 32;
const I_SYSTEM: i32 = 33;
const I_GET_PID: i32 = 34;
const I_NEXT_PID: i32 = 35;
const I_GET_NUM_CORES: i32 = 36;
const I_FORK: i32 = 37;
const I_WAIT: i32 = 38;
const I_WAIT_PID: i32 = 39;
const I_THREAD: i32 = 40;
const I_JOIN_THREAD: i32 = 41;
const I_SLEEP: i32 = 42;
const I_WAKE_THREAD: i32 = 43;
const I_ALLOC_SHARED: i32 = 44;

const I_TERM_COLS: i32 = 50;
const I_TERM_ROWS: i32 = 51;
const I_TERM_SETCURSOR: i32 = 52;
const I_TERM_CLEAR: i32 = 53;

const I_PI: i32 = 100;
const I_E: i32 = 101;
const I_ABS_FP: i32 = 102;
const I_ABS: i32 = 103;
const I_CEIL: i32 = 104;
const I_FLOOR: i32 = 105;
const I_ROUND: i32 = 106;
const I_SQRT: i32 = 107;
const I_EXP: i32 = 108;
const I_LOG: i32 = 109;
const I_POW: i32 = 110;
const I_REMAINDER: i32 = 111;
const I_MAX_FP: i32 = 112;
const I_MIN_FP: i32 = 113;
const I_MAX: i32 = 114;
const I_MIN: i32 = 115;
const I_RANDOM: i32 = 116;
const I_RAND: i32 = 117;
const I_TO_DEGREES: i32 = 118;
const I_TO_RADIANS: i32 = 119;
const I_SIN: i32 = 120;
const I_COS: i32 = 121;
const I_TAN: i32 = 122;
const I_ASIN: i32 = 123;
const I_ACOS: i32 = 124;
const I_ATAN: i32 = 125;
const I_ATAN2: i32 = 126;
const I_SINH: i32 = 127;
const I_COSH: i32 = 128;
const I_TANH: i32 = 129;
const I_ASINH: i32 = 130;
const I_ACOSH: i32 = 131;
const I_ATANH: i32 = 132;

const I_PUT_NL: i32 = 200;
const I_PUT_INT: i32 = 201;
const I_PUT_DEC: i32 = 202;
const I_PUT_HEX: i32 = 203;
const I_PUT_FP: i32 = 204;
const I_PUTS: i32 = 205;
const I_PUT_LINE: i32 = 206;
const I_GET_INT: i32 = 207;
const I_GET_DEC: i32 = 208;
const I_GET_HEX: i32 = 209;
const I_GET_FP: i32 = 210;
const I_GETS: i32 = 211;
const I_GET_LINE: i32 = 212;
const I_PRINTF: i32 = 213;
const I_COND_PRINTF: i32 = 214;

const I_OPEN_FILE_READ: i32 = 220;
const I_OPEN_FILE_WRITE: i32 = 221;
const I_OPEN_FILE_APPEND: i32 = 222;
const I_OPEN_RAW_FILE_READ: i32 = 223;
const I_OPEN_RAW_FILE_WRITE: i32 = 224;
const I_OPEN_RAW_FILE_APPEND: i32 = 225;
const I_CLOSE_FILE: i32 = 226;
const I_FLUSH: i32 = 227;
const I_DELETE_FILE: i32 = 228;
const I_MAKE_DIR: i32 = 229;
const I_DELETE_DIR: i32 = 230;
const I_IS_DIR: i32 = 231;
const I_IS_FILE: i32 = 232;
const I_FILE_EXISTS: i32 = 233;
const I_FILES: i32 = 234;
const I_TEMP_DIR: i32 = 235;
const I_TEMP_FILE: i32 = 236;
const I_FEOF: i32 = 237;
const I_FERROR: i32 = 238;
const I_OPEN_RANDOM_ACCESS_FILE: i32 = 239;
const I_SEEK: i32 = 240;
const I_TELL: i32 = 241;

const I_FMT_DEC: i32 = 301;
const I_FMT_HEX: i32 = 302;
const I_FMT_FLOAT: i32 = 303;
const I_PARSE_INT: i32 = 304;
const I_PARSE_DEC: i32 = 305;
const I_PARSE_HEX: i32 = 306;
const I_PARSE_FLOAT: i32 = 307;
const I_SPRINTF: i32 = 308;
const I_FORMAT: i32 = 309;
const I_TO_LOWER: i32 = 310;
const I_TO_UPPER: i32 = 311;
const I_TO_LOWER_STR: i32 = 312;
const I_TO_UPPER_STR: i32 = 313;
const I_STRCOPY: i32 = 314;
const I_STRCMP: i32 = 315;
const I_STRICMP: i32 = 316;
const I_SUBSTRING: i32 = 317;
const I_PREFIX: i32 = 318;
const I_SUFFIX: i32 = 319;
const I_CHAR_SEARCH: i32 = 320;
const I_LAST_CHAR_SEARCH: i32 = 321;
const I_SUBSTRING_SEARCH: i32 = 322;
const I_LAST_SUBSTRING_SEARCH: i32 = 323;
const I_GET_CODEPOINTS: i32 = 324;
const I_FROM_CODEPOINTS: i32 = 325;
const I_COUNT_GLYPHS: i32 = 326;
const I_HASHCODE: i32 = 327;
const I_TRIM: i32 = 328;

const I_MATCHES: i32 = 350;
const I_REPLACE_FIRST: i32 = 351;
const I_REPLACE_ALL: i32 = 352;
const I_SPLIT: i32 = 353;
const I_JOIN: i32 = 354;
const I_STRCAT: i32 = 355;

impl Simulator {
    fn err(&self, msg: impl Into<String>) -> CpuException {
        CpuException(msg.into())
    }

    /// Full interrupt dispatch (ports `StdInterruptHandler.dispatch`).
    pub fn dispatch_interrupt(&mut self, id: i32) -> CpuResult<()> {
        match id {
            I_INT_MIN => self.set_r(0, i64::MIN),
            I_INT_MAX => self.set_r(0, i64::MAX),
            I_FLOAT_MIN => self.set_fp(0, -f64::MAX),
            I_FLOAT_LOWEST => self.set_fp(0, f64::from_bits(1)), // Double.MIN_VALUE
            I_FLOAT_MAX => self.set_fp(0, f64::MAX),
            I_NEGATIVE_INFINITY => self.set_fp(0, f64::NEG_INFINITY),
            I_POSITIVE_INFINITY => self.set_fp(0, f64::INFINITY),
            I_NAN => self.set_fp(0, f64::NAN),
            I_CYCLES => self.set_r(0, self.cycles),
            I_CLOCK => self.set_r(0, self.get_clock()), // elapsed wall clock in ns (ports getClock)
            I_SAVE => {
                let d = crate::decode::Decoded::decode(crate::decode::enc_t0(
                    crate::opcode::SAVE,
                    2,
                    2,
                    0,
                    0,
                    0,
                    (super::GPR_COUNT - 4) as i32,
                    0,
                    0,
                ));
                self.op_save(&d)?;
            }
            I_SAVE_FP => {
                let d = crate::decode::Decoded::decode(crate::decode::enc_t0(
                    crate::opcode::SAVE,
                    3,
                    3,
                    0,
                    0,
                    0,
                    (super::FPR_COUNT - 1) as i32,
                    0,
                    0,
                ));
                self.op_save(&d)?;
            }
            I_RESTORE => {
                let d = crate::decode::Decoded::decode(crate::decode::enc_t0(
                    crate::opcode::RESTORE,
                    2,
                    2,
                    0,
                    0,
                    0,
                    (super::GPR_COUNT - 4) as i32,
                    0,
                    0,
                ));
                self.op_restore(&d)?;
            }
            I_RESTORE_FP => {
                let d = crate::decode::Decoded::decode(crate::decode::enc_t0(
                    crate::opcode::RESTORE,
                    3,
                    3,
                    0,
                    0,
                    0,
                    (super::FPR_COUNT - 1) as i32,
                    0,
                    0,
                ));
                self.op_restore(&d)?;
            }
            I_PRINT_CPU_STATE => self.print_cpu_state(),
            I_ALLOC => {
                let n = self.get_r(1);
                let p = self.alloc(n)?;
                self.set_r(0, p);
            }
            I_REALLOC => {
                let a = self.get_r(1);
                let n = self.get_r(2);
                let p = self.realloc(a, n)?;
                self.set_r(0, p);
            }
            I_FREE => {
                let a = self.get_r(1);
                self.free(a)?;
            }
            I_MEMMOVE => {
                let dest = self.get_r(1);
                let src = self.get_r(2);
                let count = self.get_r(3);
                self.memmove(dest, src, count)?;
            }
            I_MEMCLEAR => {
                let dest = self.get_r(1);
                let count = self.get_r(2);
                self.memclear(dest, count)?;
            }
            I_ALLOC_COUNT => {
                let v = self.count_heap_blocks(true, false)?;
                self.set_r(0, v);
            }
            I_FREE_COUNT => {
                let v = self.count_heap_blocks(false, true)?;
                self.set_r(0, v);
            }
            I_ALLOC_SIZE => {
                let v = self.count_heap_size(true, false)?;
                self.set_r(0, v);
            }
            I_FREE_SIZE => {
                let v = self.count_heap_size(false, true)?;
                self.set_r(0, v);
            }
            I_WALK_HEAP => self.walk_heap()?,
            I_ARGC => {
                let argc = self.args.len() as i64;
                self.set_r(0, argc);
            }
            I_ARGS => self.int_args()?,
            I_EXIT => {
                let code = self.get_r(1) as i32;
                self.exit(code);
            }
            I_SYSTEM => return Err(self.err("iSYSTEM not supported in native port")),
            I_GET_PID => self.set_r(0, self.pid),
            I_NEXT_PID => {
                let p = self.next_pid();
                self.set_r(0, p);
            }
            I_GET_NUM_CORES => self.set_r(0, num_cpus()),
            I_FORK => {
                let p = self.fork()?;
                self.set_r(0, p);
            }
            I_WAIT => self.wait_all()?,
            I_WAIT_PID => {
                let p = self.get_r(1);
                self.wait_pid(p)?;
            }
            I_THREAD => {
                let func = self.get_r(1);
                let data = self.get_r(2);
                let p = self.thread(func, data)?;
                self.set_r(0, p);
            }
            I_JOIN_THREAD => {
                let p = self.get_r(1);
                self.join_thread(p)?;
            }
            I_SLEEP => {
                // Sleep R1 milliseconds (ports `Thread.sleep(cpu.getR(1))`),
                // interruptible by wakeThread() and by a cooperative stop so a
                // mutex waiter wakes promptly instead of blocking the full
                // backoff duration.
                let ms = self.get_r(1);
                self.interruptible_sleep(ms);
            }
            I_WAKE_THREAD => {
                let p = self.get_r(1);
                self.wake_thread(p);
            }
            I_ALLOC_SHARED => {
                let n = self.get_r(1);
                let addr = crate::cpu::memory::alloc_shared(n);
                self.set_r(0, addr);
            }
            I_TERM_COLS => self.set_r(0, 80),
            I_TERM_ROWS => self.set_r(0, 24),
            I_TERM_SETCURSOR => {
                let row = self.get_r(0) + 1;
                let col = self.get_r(1) + 1;
                print!("\u{1b}[{};{}H", row, col);
                use std::io::Write;
                let _ = std::io::stdout().flush();
            }
            I_TERM_CLEAR => {
                print!("\u{1b}[2J\u{1b}[H");
                use std::io::Write;
                let _ = std::io::stdout().flush();
            }
            // ===== math =====
            I_PI => self.set_fp(0, std::f64::consts::PI),
            I_E => self.set_fp(0, std::f64::consts::E),
            I_ABS_FP => self.set_fp(0, self.get_fp(1).abs()),
            I_ABS => self.set_r(0, self.get_r(1).wrapping_abs()),
            I_CEIL => self.set_fp(0, self.get_fp(1).ceil()),
            I_FLOOR => self.set_fp(0, self.get_fp(1).floor()),
            I_ROUND => {
                let v = self.get_fp(1);
                self.set_fp(0, (v.abs() + 0.5).floor().copysign(v));
            }
            I_SQRT => self.set_fp(0, self.get_fp(1).sqrt()),
            I_EXP => self.set_fp(0, self.get_fp(1).exp()),
            I_LOG => self.set_fp(0, self.get_fp(1).ln()),
            I_POW => self.set_fp(0, self.get_fp(1).powf(self.get_fp(2))),
            I_REMAINDER => {
                let a = self.get_fp(1);
                let b = self.get_fp(2);
                self.set_fp(0, ieee_remainder(a, b));
            }
            I_MAX_FP => self.set_fp(0, self.get_fp(1).max(self.get_fp(2))),
            I_MIN_FP => self.set_fp(0, self.get_fp(1).min(self.get_fp(2))),
            I_MAX => self.set_r(0, self.get_r(1).max(self.get_r(2))),
            I_MIN => self.set_r(0, self.get_r(1).min(self.get_r(2))),
            I_RANDOM => self.set_fp(0, util::random()), // F0 = random [0.0, 1.0)
            I_RAND => {
                // R0 = floor(random() * (R2 - R1 + 1)) + R1  -> integer in [R1, R2].
                let lo = self.get_r(1);
                let hi = self.get_r(2);
                let span = (hi - lo + 1) as f64;
                let v = (util::random() * span).floor() as i64 + lo;
                self.set_r(0, v);
            }
            I_TO_DEGREES => self.set_fp(0, self.get_fp(1).to_degrees()),
            I_TO_RADIANS => self.set_fp(0, self.get_fp(1).to_radians()),
            I_SIN => self.set_fp(0, self.get_fp(1).sin()),
            I_COS => self.set_fp(0, self.get_fp(1).cos()),
            I_TAN => self.set_fp(0, self.get_fp(1).tan()),
            I_ASIN => self.set_fp(0, self.get_fp(1).asin()),
            I_ACOS => self.set_fp(0, self.get_fp(1).acos()),
            I_ATAN => self.set_fp(0, self.get_fp(1).atan()),
            I_ATAN2 => self.set_fp(0, self.get_fp(1).atan2(self.get_fp(2))),
            I_SINH => self.set_fp(0, self.get_fp(1).sinh()),
            I_COSH => self.set_fp(0, self.get_fp(1).cosh()),
            I_TANH => self.set_fp(0, self.get_fp(1).tanh()),
            I_ASINH => self.set_fp(0, util::asinh(self.get_fp(1))),
            I_ACOSH => self.set_fp(0, util::acosh(self.get_fp(1))),
            I_ATANH => self.set_fp(0, util::atanh(self.get_fp(1))),
            // ===== put/get =====
            I_PUT_NL => {
                let port = self.get_r(1) as i32;
                if self.has_port(port) {
                    self.port_out(port, b'\n' as i64, 0)?;
                }
            }
            I_PUT_INT => {
                let port = self.get_r(1) as i32;
                let v = self.get_r(2);
                let base = self.get_r(3) as u32;
                let s = to_radix_string(v, base);
                self.put_str(port, &s)?;
            }
            I_PUT_DEC => {
                let port = self.get_r(1) as i32;
                let s = self.get_r(2).to_string();
                self.put_str(port, &s)?;
            }
            I_PUT_HEX => {
                let port = self.get_r(1) as i32;
                let v = self.get_r(2);
                let digits = self.get_r(3);
                let s = if digits <= 0 {
                    format!("{:X}", v as u64)
                } else {
                    format!("{:0width$X}", v as u64, width = digits as usize)
                };
                self.put_str(port, &s)?;
            }
            I_PUT_FP => {
                let port = self.get_r(1) as i32;
                let prec = self.get_r(2) as usize;
                let s = crate::javacompat::format::format_f(self.get_fp(1), prec);
                self.put_str(port, &s)?;
            }
            I_PUTS => {
                let port = self.get_r(1) as i32;
                let pos = self.get_r(2);
                let s = self.convert_string(pos)?;
                self.put_str(port, &s)?;
                if let Some(ph) = self.get_port_handler_mut(port) {
                    ph.flush()?;
                }
            }
            I_PUT_LINE => {
                let port = self.get_r(1) as i32;
                let pos = self.get_r(2);
                let s = self.convert_string(pos)?;
                self.put_str(port, &s)?;
                self.port_out(port, b'\n' as i64, 0)?;
            }
            I_GET_INT => self.get_int()?,
            I_GET_DEC => {
                self.set_r(2, 10);
                self.dispatch_interrupt(I_GET_INT)?;
            }
            I_GET_HEX => {
                self.set_r(2, 16);
                self.dispatch_interrupt(I_GET_INT)?;
            }
            I_GET_FP => self.get_fp_interrupt()?,
            I_GETS => self.gets()?,
            I_GET_LINE => self.get_line()?,
            I_PRINTF => {
                let port = self.mem_read(self.get_r(R_SF) + 3)?;
                let s = self.sprintf(4)?;
                self.put_str(port as i32, &s)?;
            }
            I_COND_PRINTF => {
                let cond = self.mem_read(self.get_r(R_SF) + 3)? != 0;
                if cond {
                    let port = self.mem_read(self.get_r(R_SF) + 4)?;
                    let s = self.sprintf(5)?;
                    self.put_str(port as i32, &s)?;
                }
            }
            // ===== files =====
            I_OPEN_FILE_READ => self.open_file(0, false)?,
            I_OPEN_FILE_WRITE => self.open_file(1, false)?,
            I_OPEN_FILE_APPEND => self.open_file(2, false)?,
            I_OPEN_RAW_FILE_READ => self.open_file(0, true)?,
            I_OPEN_RAW_FILE_WRITE => self.open_file(1, true)?,
            I_OPEN_RAW_FILE_APPEND => self.open_file(2, true)?,
            I_CLOSE_FILE => {
                let port = self.get_r(1) as i32;
                if let Some(ph) = self.get_port_handler_mut(port) {
                    ph.close()?;
                    self.set_port_handler(port, None);
                } else {
                    return Err(self.err(format!("Port {} not mapped!", port)));
                }
            }
            I_FLUSH => {
                let port = self.get_r(1) as i32;
                if let Some(ph) = self.get_port_handler_mut(port) {
                    ph.flush()?;
                } else {
                    return Err(self.err(format!("Port {} not mapped!", port)));
                }
            }
            I_FEOF => {
                let port = self.get_r(1) as i32;
                if let Some(ph) = self.get_port_handler_mut(port) {
                    let eof = ph.is_eof();
                    self.set_r(0, if eof { -1 } else { 0 });
                } else {
                    return Err(self.err(format!("Port {} not mapped!", port)));
                }
            }
            I_FERROR => {
                let port = self.get_r(1) as i32;
                if let Some(ph) = self.get_port_handler_mut(port) {
                    let e = ph.is_error();
                    self.set_r(0, if e { -1 } else { 0 });
                } else {
                    return Err(self.err(format!("Port {} not mapped!", port)));
                }
            }
            I_OPEN_RANDOM_ACCESS_FILE => {
                let port = self.get_r(1) as i32;
                if self.has_port(port) {
                    return Err(self.err(format!("Port {} already mapped!", port)));
                }
                let filespec = self.convert_string(self.get_r(2))?;
                match crate::ports::Port::new_random(&filespec) {
                    Ok(p) => {
                        self.set_port_handler(port, Some(p));
                        self.set_r(0, -1);
                    }
                    Err(_) => self.set_r(0, 0),
                }
            }
            I_SEEK => {
                let port = self.get_r(1) as i32;
                let target = self.get_r(2);
                if let Some(ph) = self.get_port_handler_mut(port) {
                    let r = ph.seek(target)?;
                    self.set_r(0, r);
                } else {
                    return Err(self.err(format!("Port {} not mapped!", port)));
                }
            }
            I_TELL => {
                let port = self.get_r(1) as i32;
                if let Some(ph) = self.get_port_handler_mut(port) {
                    let r = ph.tell()?;
                    self.set_r(0, r);
                } else {
                    return Err(self.err(format!("Port {} not mapped!", port)));
                }
            }
            I_DELETE_FILE => {
                let s = self.convert_string(self.get_r(1))?;
                let ok = std::path::Path::new(&s).is_file() && std::fs::remove_file(&s).is_ok();
                self.set_r(0, if ok { -1 } else { 0 });
            }
            I_MAKE_DIR => {
                let s = self.convert_string(self.get_r(1))?;
                let ok = std::fs::create_dir(&s).is_ok();
                self.set_r(0, if ok { -1 } else { 0 });
            }
            I_DELETE_DIR => {
                let s = self.convert_string(self.get_r(1))?;
                let ok = std::path::Path::new(&s).is_dir() && std::fs::remove_dir(&s).is_ok();
                self.set_r(0, if ok { -1 } else { 0 });
            }
            I_IS_DIR => {
                let s = self.convert_string(self.get_r(1))?;
                self.set_r(0, if std::path::Path::new(&s).is_dir() { -1 } else { 0 });
            }
            I_IS_FILE => {
                let s = self.convert_string(self.get_r(1))?;
                self.set_r(0, if std::path::Path::new(&s).is_file() { -1 } else { 0 });
            }
            I_FILE_EXISTS => {
                let s = self.convert_string(self.get_r(1))?;
                self.set_r(0, if std::path::Path::new(&s).exists() { -1 } else { 0 });
            }
            I_FILES => self.list_files()?,
            I_TEMP_DIR => {
                // R1 = prefix; create a unique temp directory in the system temp
                // dir, return its absolute path (R0), or 0 on failure
                // (ports Files.createTempDirectory).
                let prefix = self.convert_string(self.get_r(1))?;
                match create_temp_dir(&prefix) {
                    Some(path) => {
                        let p = self.alloc_string(&path, 0)?;
                        self.set_r(0, p);
                    }
                    None => self.set_r(0, 0),
                }
            }
            I_TEMP_FILE => {
                // R1 = prefix, R2 = extension/suffix; create a unique temp file
                // in the system temp dir, return its absolute path (R0), or 0 on
                // failure (ports File.createTempFile).
                let prefix = self.convert_string(self.get_r(1))?;
                let suffix = self.convert_string(self.get_r(2))?;
                match create_temp_file(&prefix, &suffix) {
                    Some(path) => {
                        let p = self.alloc_string(&path, 0)?;
                        self.set_r(0, p);
                    }
                    None => self.set_r(0, 0),
                }
            }
            // ===== strings / formatting =====
            I_FMT_DEC => {
                let s = self.get_r(1).to_string();
                let p = self.alloc_string(&s, 0)?;
                self.set_r(0, p);
            }
            I_FMT_HEX => {
                let v = self.get_r(1);
                let digits = self.get_r(2) as usize;
                let s = format!("{:0width$X}", v as u64, width = digits);
                let p = self.alloc_string(&s, 0)?;
                self.set_r(0, p);
            }
            I_FMT_FLOAT => {
                let prec = self.get_r(1) as usize;
                let s = crate::javacompat::format::format_f(self.get_fp(1), prec);
                let p = self.alloc_string(&s, 0)?;
                self.set_r(0, p);
            }
            I_PARSE_INT => {
                let s = self.convert_string(self.get_r(1))?;
                let v = parse_int_java(&s).ok_or_else(|| self.err(format!("Can't parse int '{s}'")))?;
                self.set_r(0, v);
            }
            I_PARSE_DEC => {
                let s = self.convert_string(self.get_r(1))?;
                let v: i64 = s.trim().parse().map_err(|_| self.err(format!("Can't parse dec '{s}'")))?;
                self.set_r(0, v);
            }
            I_PARSE_HEX => {
                let s = self.convert_string(self.get_r(1))?;
                let v = i64::from_str_radix(s.trim(), 16).map_err(|_| self.err(format!("Can't parse hex '{s}'")))?;
                self.set_r(0, v);
            }
            I_PARSE_FLOAT => {
                let s = self.convert_string(self.get_r(1))?;
                let v: f64 = s.trim().parse().map_err(|_| self.err(format!("Can't parse float '{s}'")))?;
                self.set_fp(0, v);
            }
            I_SPRINTF => {
                let s = self.sprintf(3)?;
                let p = self.alloc_string(&s, 0)?;
                self.set_r(0, p);
            }
            I_FORMAT => {
                let s = self.format_msg(3)?;
                let p = self.alloc_string(&s, 0)?;
                self.set_r(0, p);
            }
            I_TO_LOWER => {
                let c = char::from_u32(self.get_r(1) as u32).unwrap_or('\0');
                let lc = c.to_lowercase().next().unwrap_or(c);
                self.set_r(0, lc as i64);
            }
            I_TO_UPPER => {
                let c = char::from_u32(self.get_r(1) as u32).unwrap_or('\0');
                let uc = c.to_uppercase().next().unwrap_or(c);
                self.set_r(0, uc as i64);
            }
            I_TO_LOWER_STR => {
                let s = self.convert_string(self.get_r(1))?.to_lowercase();
                let p = self.alloc_string(&s, 0)?;
                self.set_r(0, p);
            }
            I_TO_UPPER_STR => {
                let s = self.convert_string(self.get_r(1))?.to_uppercase();
                let p = self.alloc_string(&s, 0)?;
                self.set_r(0, p);
            }
            I_STRCMP => {
                let s1 = self.convert_string(self.get_r(1))?;
                let s2 = self.convert_string(self.get_r(2))?;
                self.set_r(0, string_compare_to(&s1, &s2) as i64);
            }
            I_STRICMP => {
                let s1 = self.convert_string(self.get_r(1))?;
                let s2 = self.convert_string(self.get_r(2))?;
                self.set_r(0, string_compare_to_ignore_case(&s1, &s2) as i64);
            }
            I_STRCOPY => {
                // Not in the Java dispatch switch body shown; treat as strcat-like
                // no-op copy: allocate a copy of the string in r1.
                let s = self.convert_string(self.get_r(1))?;
                let p = self.alloc_string(&s, 0)?;
                self.set_r(0, p);
            }
            I_SUBSTRING => {
                let s = self.convert_string(self.get_r(1))?;
                let chars: Vec<char> = s.chars().collect();
                let len = chars.len() as i64;
                let mut start = self.get_r(2);
                start = start.min(len).max(0);
                let mut stop = self.get_r(3);
                if stop < 0 {
                    stop = len;
                }
                stop += start;
                stop = stop.min(len).max(start);
                let sub: String = chars[start as usize..stop as usize].iter().collect();
                let p = self.alloc_string(&sub, 0)?;
                self.set_r(0, p);
            }
            I_PREFIX => {
                let s = self.convert_string(self.get_r(1))?;
                let n = self.get_r(2) as usize;
                let sub: String = s.chars().take(n).collect();
                let p = self.alloc_string(&sub, 0)?;
                self.set_r(0, p);
            }
            I_SUFFIX => {
                let s = self.convert_string(self.get_r(1))?;
                let chars: Vec<char> = s.chars().collect();
                let n = self.get_r(2) as usize;
                let sub: String = chars[chars.len() - n..].iter().collect();
                let p = self.alloc_string(&sub, 0)?;
                self.set_r(0, p);
            }
            I_CHAR_SEARCH => {
                let s = self.convert_string(self.get_r(1))?;
                let c = char::from_u32(self.get_r(2) as u32).unwrap_or('\0');
                let i = (self.get_r(3).max(0) as usize).min(s.chars().count());
                let v = index_of_char(&s, c, i);
                self.set_r(0, v);
            }
            I_LAST_CHAR_SEARCH => {
                let s = self.convert_string(self.get_r(1))?;
                let c = char::from_u32(self.get_r(2) as u32).unwrap_or('\0');
                let i = self.get_r(3);
                let v = last_index_of_char(&s, c, i);
                self.set_r(0, v);
            }
            I_SUBSTRING_SEARCH => {
                let s = self.convert_string(self.get_r(1))?;
                let sub = self.convert_string(self.get_r(2))?;
                let i = (self.get_r(3).max(0) as usize).min(s.chars().count());
                let v = index_of_str(&s, &sub, i);
                self.set_r(0, v);
            }
            I_LAST_SUBSTRING_SEARCH => {
                let s = self.convert_string(self.get_r(1))?;
                let sub = self.convert_string(self.get_r(2))?;
                let i = self.get_r(3);
                let v = last_index_of_str(&s, &sub, i);
                self.set_r(0, v);
            }
            I_GET_CODEPOINTS => {
                let s = self.convert_string(self.get_r(1))?;
                let cps: Vec<i64> = s.chars().map(|c| c as i64).collect();
                let arr = self.alloc(cps.len() as i64 + 1)?;
                self.mem_write(arr, cps.len() as i64)?;
                for (i, cp) in cps.iter().enumerate() {
                    self.mem_write(arr + i as i64 + 1, *cp)?;
                }
                self.set_r(0, arr);
            }
            I_FROM_CODEPOINTS => {
                let addr = self.get_r(1);
                let len = self.mem_read(addr)?;
                let mut s = String::new();
                for i in 0..len {
                    let cp = self.mem_read(addr + i + 1)? as u32;
                    if let Some(c) = char::from_u32(cp) {
                        s.push(c);
                    }
                }
                let p = self.alloc_string(&s, 0)?;
                self.set_r(0, p);
            }
            I_COUNT_GLYPHS => {
                let s = self.convert_string(self.get_r(1))?;
                // Approximate grapheme count with char count (no ICU in native).
                self.set_r(0, s.chars().count() as i64);
            }
            I_HASHCODE => {
                let s = self.convert_string(self.get_r(1))?;
                self.set_r(0, string_hash_code(&s) as i64);
            }
            I_TRIM => {
                let s = self.convert_string(self.get_r(1))?;
                let t = java_trim(&s);
                let p = self.alloc_string(&t, 0)?;
                self.set_r(0, p);
            }
            I_MATCHES => {
                let s = self.convert_string(self.get_r(1))?;
                let re = self.convert_string(self.get_r(2))?;
                let m = regexadapter::find(&re, &s).map_err(|e| self.err(e))?;
                self.set_r(0, if m { -1 } else { 0 });
            }
            I_REPLACE_FIRST => {
                let s = self.convert_string(self.get_r(1))?;
                let re = self.convert_string(self.get_r(2))?;
                let repl = self.convert_string(self.get_r(3))?;
                let out = regexadapter::replace_first(&s, &re, &repl).map_err(|e| self.err(e))?;
                let p = self.alloc_string(&out, 0)?;
                self.set_r(0, p);
            }
            I_REPLACE_ALL => {
                let s = self.convert_string(self.get_r(1))?;
                let re = self.convert_string(self.get_r(2))?;
                let repl = self.convert_string(self.get_r(3))?;
                let out = regexadapter::replace_all(&s, &re, &repl).map_err(|e| self.err(e))?;
                let p = self.alloc_string(&out, 0)?;
                self.set_r(0, p);
            }
            I_SPLIT => {
                let s = self.convert_string(self.get_r(1))?;
                let re = self.convert_string(self.get_r(2))?;
                let limit = self.get_r(3) as i32;
                let parts = regexadapter::split(&s, &re, limit).map_err(|e| self.err(e))?;
                let arr = self.alloc(parts.len() as i64 + 1)?;
                for (i, part) in parts.iter().enumerate() {
                    let sp = self.alloc_string(part, 0)?;
                    self.mem_write(arr + i as i64 + 1, sp)?;
                }
                self.mem_write(arr, parts.len() as i64)?;
                self.set_r(0, arr);
            }
            I_JOIN => {
                let delim = self.convert_string(self.get_r(2))?;
                let mut a = self.get_r(1);
                let len = self.mem_read(a)?;
                let mut s = String::new();
                for i in 1..=len {
                    if i != 1 {
                        s.push_str(&delim);
                    }
                    a += 1;
                    let strp = self.mem_read(a)?;
                    s.push_str(&self.convert_string(strp)?);
                }
                let p = self.alloc_string(&s, 0)?;
                self.set_r(0, p);
            }
            I_STRCAT => {
                let s1 = self.convert_string(self.get_r(1))?;
                let s2 = self.convert_string(self.get_r(2))?;
                let s = format!("{s1}{s2}");
                let buf = self.get_r(3);
                let p = self.alloc_string(&s, buf)?;
                self.set_r(0, p);
            }
            _ => return Err(self.err(format!("Unknown interrupt {id}"))),
        }
        Ok(())
    }

    // ---- helpers ----
    fn put_str(&mut self, port: i32, s: &str) -> CpuResult<()> {
        if !self.has_port(port) {
            return Ok(());
        }
        // Write the whole formatted string as one atomic unit so concurrent
        // threads never interleave mid-line (ports the per-`printf` output unit;
        // STDOUT/STDERR are serialized through a process-global console lock).
        let ph = self.get_port_handler_mut(port).ok_or_else(|| {
            CpuException(format!("OUT: no handler registered for port {}", port))
        })?;
        ph.write_str(s)
    }

    /// Ports `sprintf(msgOffset)`: read fmt at [SF+msgOffset], pull args from the
    /// stack frame per conversion.
    fn sprintf(&mut self, msg_offset: i64) -> CpuResult<String> {
        let sf = self.get_r(R_SF);
        let fmt_ptr = self.mem_read(sf + msg_offset)?;
        let fmt = self.convert_string(fmt_ptr)?;
        let rx = Regex::new(r"(?i)%[-+0 ]?\d*\.?\d*([cdxefgs])").unwrap();
        let mut args: Vec<Arg> = Vec::new();
        let mut num_args = 0i64;
        for cap in rx.captures_iter(&fmt) {
            let conv = cap.get(1).unwrap().as_str().to_lowercase();
            let slot = self.mem_read(sf + num_args + msg_offset + 1)?;
            let arg = match conv.as_str() {
                "c" => Arg::Char(slot as i32),
                "s" => Arg::Str(self.convert_string(slot)?),
                "e" | "f" | "g" => Arg::Float(f64::from_bits(slot as u64)),
                _ => Arg::Int(slot),
            };
            args.push(arg);
            num_args += 1;
        }
        Ok(java_printf(&fmt, &args))
    }

    /// Ports `format(msgOffset)` (MessageFormat `{}`), all args strings.
    fn format_msg(&mut self, msg_offset: i64) -> CpuResult<String> {
        let sf = self.get_r(R_SF);
        let fmt_ptr = self.mem_read(sf + msg_offset)?;
        let fmt = self.convert_string(fmt_ptr)?;
        let rx = Regex::new(r"\{([0-9]*)\}").unwrap();
        let mut num_args = 0i64;
        let mut max_args = 0i64;
        for cap in rx.captures_iter(&fmt) {
            let g = cap.get(1).unwrap().as_str();
            if g.is_empty() {
                num_args += 1;
            } else {
                let i: i64 = g.parse().unwrap_or(0);
                max_args = max_args.max(i + 1);
            }
        }
        num_args = num_args.max(max_args);
        let mut args: Vec<String> = Vec::new();
        for i in 0..num_args {
            let ptr = self.mem_read(sf + i + msg_offset + 1)?;
            args.push(self.convert_string(ptr)?);
        }
        let _ = number_placeholders(&fmt); // (message_format numbers internally)
        Ok(message_format(&fmt, &args))
    }

    fn int_args(&mut self) -> CpuResult<()> {
        let argc = self.args.len() as i64;
        if self.argv_ptr == 0 && argc > 0 {
            let argv = self.alloc(argc)?;
            self.argv_ptr = argv;
            for i in 0..argc {
                let arg = self.args[i as usize].clone();
                let sp = self.alloc_string(&arg, 0)?;
                self.mem_write(argv + i, sp)?;
            }
        }
        let v = self.get_r(1);
        let out = if self.argv_ptr > 0 && v >= 0 && v < argc {
            self.mem_read(self.argv_ptr + v)?
        } else {
            0
        };
        self.set_r(0, out);
        Ok(())
    }

    fn get_int(&mut self) -> CpuResult<()> {
        let base = self.get_r(2) as u32;
        let buf = self.alloc(65)?;
        self.set_r(2, buf);
        self.gets()?;
        let s = self.convert_string(self.get_r(2))?;
        let v = i64::from_str_radix(s.trim(), base)
            .map_err(|_| self.err(format!("Can't convert \"{s}\" to an integer base {base}!")))?;
        self.free(self.get_r(2))?;
        self.set_r(0, v);
        Ok(())
    }

    fn get_fp_interrupt(&mut self) -> CpuResult<()> {
        let buf = self.alloc(65)?;
        self.set_r(2, buf);
        self.gets()?;
        let s = self.convert_string(self.get_r(2))?;
        let v: f64 = s
            .trim()
            .parse()
            .map_err(|_| self.err(format!("Can't convert \"{s}\" to a floating point!")))?;
        self.free(self.get_r(2))?;
        self.set_fp(0, v);
        Ok(())
    }

    fn gets(&mut self) -> CpuResult<()> {
        let port = self.get_r(1) as i32;
        if !self.has_port(port) {
            return Ok(());
        }
        let mut codepoints: Vec<i32> = Vec::new();
        let mut last_cp;
        loop {
            let cp = self.get_port_handler_mut(port).unwrap().read_char()?;
            last_cp = cp;
            if cp == -1 {
                break;
            }
            let c = char::from_u32(cp as u32).unwrap_or('\0');
            if c.is_whitespace() {
                if codepoints.is_empty() {
                    continue;
                } else {
                    break;
                }
            }
            codepoints.push(cp);
        }
        let s: String = codepoints
            .iter()
            .filter_map(|&c| char::from_u32(c as u32))
            .collect();
        let buf = self.get_r(2);
        let new_alloc = self.alloc_string(&s, buf)?;
        self.set_r(2, new_alloc);
        if last_cp == -1 && s.is_empty() {
            self.set_r(0, -1);
        } else {
            self.set_r(0, s.chars().count() as i64);
        }
        Ok(())
    }

    fn get_line(&mut self) -> CpuResult<()> {
        let port = self.get_r(1) as i32;
        if !self.has_port(port) {
            self.set_r(0, 0);
            return Ok(());
        }
        let mut codepoints: Vec<i32> = Vec::new();
        let mut last_cp;
        loop {
            let cp = self.get_port_handler_mut(port).unwrap().read_char()?;
            last_cp = cp;
            if cp == -1 || cp == b'\n' as i32 {
                break;
            }
            if cp == b'\r' as i32 {
                continue;
            }
            codepoints.push(cp);
        }
        if last_cp == -1 && codepoints.is_empty() {
            self.set_r(0, 0);
        } else {
            let s: String = codepoints
                .iter()
                .filter_map(|&c| char::from_u32(c as u32))
                .collect();
            let buf = self.get_r(2);
            let new_alloc = self.alloc_string(&s, buf)?;
            self.set_r(0, new_alloc);
            self.set_r(2, new_alloc);
        }
        Ok(())
    }

    fn open_file(&mut self, mode: i32, raw: bool) -> CpuResult<()> {
        let port = self.get_r(1) as i32;
        if self.has_port(port) {
            return Err(self.err(format!("Port {} already mapped!", port)));
        }
        let filespec = self.convert_string(self.get_r(2))?;
        let ph = if raw {
            crate::ports::Port::new_raw(&filespec, mode)
        } else {
            crate::ports::Port::new_file(mode, &filespec)
        };
        match ph {
            Ok(p) => {
                self.set_port_handler(port, Some(p));
                self.set_r(0, -1);
            }
            Err(_) => {
                self.set_r(0, 0);
            }
        }
        Ok(())
    }

    fn list_files(&mut self) -> CpuResult<()> {
        let mut s = self.convert_string(self.get_r(1))?;
        if !s.ends_with('/') {
            s.push('/');
        }
        let entries: Vec<String> = match std::fs::read_dir(&s) {
            Ok(rd) => rd
                .filter_map(|e| e.ok())
                .map(|e| format!("{}{}", s, e.file_name().to_string_lossy()))
                .collect(),
            Err(_) => Vec::new(),
        };
        let arr = self.alloc(entries.len() as i64 + 1)?;
        for (i, name) in entries.iter().enumerate() {
            let sp = self.alloc_string(name, 0)?;
            self.mem_write(arr + i as i64 + 1, sp)?;
        }
        self.mem_write(arr, entries.len() as i64)?;
        self.set_r(0, arr);
        Ok(())
    }

    /// Free the lazily-allocated argv (ports `freeArgs`).
    pub fn free_args(&mut self) {
        if self.argv_ptr == 0 {
            return;
        }
        let argc = self.args.len() as i64;
        for i in 0..argc {
            if let Ok(str_addr) = self.mem_read(self.argv_ptr + i) {
                if str_addr > 0 {
                    let _ = self.free(str_addr);
                }
            }
        }
        let argv = self.argv_ptr;
        let _ = self.free(argv);
        self.argv_ptr = 0;
    }
}

fn num_cpus() -> i64 {
    std::thread::available_parallelism()
        .map(|n| n.get() as i64)
        .unwrap_or(1)
}

/// Build a random alphanumeric token for unique temp names (uses the shared
/// PRNG so it does not repeat within a run).
fn temp_token() -> String {
    const CHARS: &[u8] = b"0123456789abcdefghijklmnopqrstuvwxyz";
    let mut s = String::with_capacity(12);
    for _ in 0..12 {
        let idx = (crate::util::random() * CHARS.len() as f64) as usize;
        s.push(CHARS[idx.min(CHARS.len() - 1)] as char);
    }
    s
}

/// Create a unique temporary directory in the system temp dir (ports
/// `Files.createTempDirectory(prefix)`). Returns the absolute path, or None on
/// failure. Retries a few times to avoid collisions.
fn create_temp_dir(prefix: &str) -> Option<String> {
    let base = std::env::temp_dir();
    for _ in 0..100 {
        let dir = base.join(format!("{prefix}{}", temp_token()));
        match std::fs::create_dir(&dir) {
            Ok(()) => {
                return Some(
                    std::fs::canonicalize(&dir)
                        .unwrap_or(dir)
                        .to_string_lossy()
                        .into_owned(),
                );
            }
            // AlreadyExists -> try another name; any other error -> give up.
            Err(e) if e.kind() == std::io::ErrorKind::AlreadyExists => continue,
            Err(_) => return None,
        }
    }
    None
}

/// Create a unique temporary file in the system temp dir (ports
/// `File.createTempFile(prefix, suffix)`). Returns the absolute path, or None on
/// failure. The file is created empty; the suffix is used verbatim (Java
/// defaults to ".tmp" only when suffix is null, which never happens here since
/// the caller always passes a string).
fn create_temp_file(prefix: &str, suffix: &str) -> Option<String> {
    use std::fs::OpenOptions;
    let base = std::env::temp_dir();
    for _ in 0..100 {
        let file = base.join(format!("{prefix}{}{suffix}", temp_token()));
        match OpenOptions::new()
            .write(true)
            .create_new(true) // fail if it already exists
            .open(&file)
        {
            Ok(_) => {
                return Some(
                    std::fs::canonicalize(&file)
                        .unwrap_or(file)
                        .to_string_lossy()
                        .into_owned(),
                );
            }
            Err(e) if e.kind() == std::io::ErrorKind::AlreadyExists => continue,
            Err(_) => return None,
        }
    }
    None
}

fn ieee_remainder(a: f64, b: f64) -> f64 {
    // Java Math.IEEEremainder: a - (round(a/b) * b), round-half-even.
    if b == 0.0 || a.is_infinite() {
        return f64::NAN;
    }
    if b.is_infinite() {
        return a;
    }
    let q = (a / b).round_ties_even();
    a - q * b
}

fn to_radix_string(v: i64, radix: u32) -> String {
    if radix == 10 {
        return v.to_string();
    }
    // Java Long.toString(v, radix) handles negatives with a leading '-'.
    let neg = v < 0;
    let mut n = (v as i128).unsigned_abs();
    if n == 0 {
        return "0".to_string();
    }
    let digits = b"0123456789abcdefghijklmnopqrstuvwxyz";
    let mut buf = Vec::new();
    while n > 0 {
        buf.push(digits[(n % radix as u128) as usize]);
        n /= radix as u128;
    }
    if neg {
        buf.push(b'-');
    }
    buf.reverse();
    String::from_utf8(buf).unwrap()
}

fn parse_int_java(s: &str) -> Option<i64> {
    let s = s.trim();
    if let Some(hex) = s.strip_prefix("0x").or_else(|| s.strip_prefix("0X")) {
        i64::from_str_radix(hex, 16).ok()
    } else if let Some(hex) = s.strip_prefix("#") {
        i64::from_str_radix(hex, 16).ok()
    } else {
        s.parse().ok()
    }
}

fn java_trim(s: &str) -> String {
    // Java String.trim removes chars <= U+0020 from both ends.
    let bytes: Vec<char> = s.chars().collect();
    let mut start = 0;
    let mut end = bytes.len();
    while start < end && bytes[start] as u32 <= 0x20 {
        start += 1;
    }
    while end > start && bytes[end - 1] as u32 <= 0x20 {
        end -= 1;
    }
    bytes[start..end].iter().collect()
}

fn index_of_char(s: &str, c: char, from: usize) -> i64 {
    let chars: Vec<char> = s.chars().collect();
    for (i, &ch) in chars.iter().enumerate().skip(from) {
        if ch == c {
            return i as i64;
        }
    }
    -1
}

fn last_index_of_char(s: &str, c: char, from: i64) -> i64 {
    let chars: Vec<char> = s.chars().collect();
    let max = if from < 0 { -1 } else { (from as usize).min(chars.len().saturating_sub(1)) as i64 };
    if max < 0 {
        return -1;
    }
    for i in (0..=max as usize).rev() {
        if chars[i] == c {
            return i as i64;
        }
    }
    -1
}

fn index_of_str(s: &str, sub: &str, from: usize) -> i64 {
    let chars: Vec<char> = s.chars().collect();
    let subchars: Vec<char> = sub.chars().collect();
    if subchars.is_empty() {
        return from.min(chars.len()) as i64;
    }
    let mut i = from;
    while i + subchars.len() <= chars.len() {
        if chars[i..i + subchars.len()] == subchars[..] {
            return i as i64;
        }
        i += 1;
    }
    -1
}

fn last_index_of_str(s: &str, sub: &str, from: i64) -> i64 {
    let chars: Vec<char> = s.chars().collect();
    let subchars: Vec<char> = sub.chars().collect();
    if subchars.is_empty() {
        return from.min(chars.len() as i64);
    }
    let max_start = if from < 0 {
        return -1;
    } else {
        ((from as usize).min(chars.len())) as i64
    };
    let mut i = max_start.min((chars.len() as i64 - subchars.len() as i64).max(-1));
    while i >= 0 {
        let iu = i as usize;
        if iu + subchars.len() <= chars.len() && chars[iu..iu + subchars.len()] == subchars[..] {
            return i;
        }
        i -= 1;
    }
    -1
}
