//! CPU simulator core (ports `Simulator.java`): state, flags, memory access,
//! program loading. Opcode execution (Task 4), cycles (Task 5), heap (Task 6),
//! and formatting (Task 9) are added in sibling modules.

pub mod concurrency;
pub mod cycles;
pub mod exec;
pub mod format;
pub mod heap;
pub mod interrupts;
pub mod memory;
pub mod portio;

use std::collections::HashMap;
use std::sync::Arc;

use memory::SharedMem;

pub const GPR_COUNT: usize = 32;
pub const FPR_COUNT: usize = 32;

pub const R_SF: usize = 29;
pub const R_SP: usize = 30;
pub const R_PC: usize = 31;

pub const SR_P: i64 = 1 << 0; // parity (1 = odd)
pub const SR_Z: i64 = 1 << 1; // zero
pub const SR_S: i64 = 1 << 2; // signed (negative)
pub const SR_O: i64 = 1 << 3; // overflow

/// A simulated CPU fault (ports `Simulator.CPUException`). The message is
/// prefixed with the PC by the thrower where Java does so.
#[derive(Debug, Clone)]
pub struct CpuException(pub String);

impl std::fmt::Display for CpuException {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.0)
    }
}

impl std::error::Error for CpuException {}

pub type CpuResult<T> = Result<T, CpuException>;

/// Snapshot of register/FP/SR/cycles state (ports `Simulator.CPUState`).
#[derive(Clone)]
pub struct CpuStateSnapshot {
    pub r: [i64; GPR_COUNT],
    pub f: [f64; FPR_COUNT],
    pub sr: i64,
    pub cycles: i64,
}

pub struct Simulator {
    // Register files.
    pub r: [i64; GPR_COUNT],
    pub f: [f64; FPR_COUNT],
    pub sr: i64,

    // Memory.
    pub mem: Arc<SharedMem>,
    pub stack: Vec<i64>,
    pub heap_start: i64,
    pub heap_limit: i64,
    pub stack_size: i64,
    pub stack_base: i64,

    // Heap bookkeeping (populated in Task 6).
    //
    // `free_list` is shared across all threads of a program (threads clone the
    // `Arc`; a forked process gets a deep copy). The mutex is held across an
    // entire alloc/free/realloc so the multi-step read-modify-write of the
    // shared heap block metadata (which lives in the shared `mem`) is atomic —
    // mirroring Java, where threads share the `freeList` object and heap array
    // mutations are synchronized. Without this, concurrent allocations from
    // `printf` and friends corrupt the heap.
    pub heap_list: i64,
    pub free_list: std::sync::Arc<std::sync::Mutex<Vec<i64>>>,

    // Protected memory ranges (READONLY).
    pub protected_memory: Vec<(i64, i64)>,

    // Execution controls.
    pub running: bool,
    pub debug: bool,
    pub trace: bool,

    // Accounting.
    pub cycles: i64,
    pub total_system_time_ns: i64,
    pub start_clock_ns: i64,

    // Command-line args passed to the simulated program.
    pub args: Vec<String>,

    // Symbol map for disassembly/trace.
    pub reverse_symbol_map: HashMap<i64, String>,

    // Identity / concurrency (used in Task 11).
    pub pid: i64,
    pub is_root: bool,

    // Port handlers (Task 7): ports 0/1/2 default to stdio.
    pub ports: std::collections::HashMap<i32, crate::ports::Port>,

    // Lazily-allocated argv pointer (Task 10 iARGS/freeArgs).
    pub argv_ptr: i64,

    // iENV cache: variable name -> address of its value string (0 if not set).
    pub env_cache: HashMap<String, i64>,
}

impl Simulator {
    /// Primary constructor (ports `Simulator(memoryWords, heapStart, stackSize, args)`).
    pub fn new(memory_words: usize, heap_start: i64, stack_size: usize, args: Vec<String>) -> Simulator {
        let mem = SharedMem::new(memory_words);
        let mut sim = Simulator {
            r: [0; GPR_COUNT],
            f: [0.0; FPR_COUNT],
            sr: SR_Z,
            mem,
            stack: vec![0; stack_size],
            heap_start,
            heap_limit: memory_words as i64,
            stack_size: stack_size as i64,
            stack_base: memory_words as i64 + stack_size as i64,
            heap_list: 0,
            free_list: std::sync::Arc::new(std::sync::Mutex::new(Vec::new())),
            protected_memory: Vec::new(),
            running: false,
            debug: false,
            trace: false,
            cycles: 0,
            total_system_time_ns: 0,
            start_clock_ns: 0,
            args,
            reverse_symbol_map: HashMap::new(),
            pid: 1,
            is_root: true,
            ports: std::collections::HashMap::new(),
            argv_ptr: 0,
            env_cache: HashMap::new(),
        };
        for p in 0..=2 {
            sim.ports.insert(p, crate::ports::Port::new_std());
        }
        sim
    }

    // ===== Program loading (ports loadProgram(List, loadAddr, reverseSymbolMap)) =====
    pub fn load_program(
        &mut self,
        words: &[i64],
        load_addr: i64,
        reverse_symbol_map: HashMap<i64, String>,
    ) {
        for (i, &w) in words.iter().enumerate() {
            let a = load_addr + i as i64;
            self.mem.set(a as usize, w);
        }
        self.r[R_PC] = load_addr;
        self.r[R_SP] = self.stack_base - 1;
        self.r[R_SF] = self.r[R_SP];
        if words.len() as i64 + load_addr > self.heap_start {
            self.heap_start = words.len() as i64 + load_addr;
        }
        // Heap header: prev, next, size (negative = free).
        self.mem_write(self.heap_start, -1).ok();
        self.mem_write(self.heap_start + 1, -1).ok();
        self.mem_write(self.heap_start + 2, self.heap_start - self.heap_limit)
            .ok();
        self.heap_list = self.heap_start;
        *self.free_list.lock().unwrap() = vec![self.heap_list];
        self.reverse_symbol_map = reverse_symbol_map;
    }

    // ===== Flags (ports setFlags variants) =====
    pub fn set_flags_int(&mut self, x: i64, overflow: bool) {
        let neg = x < 0;
        let zero = x == 0;
        let odd_parity = (x.count_ones() & 1) == 1;
        self.sr = (if odd_parity { SR_P } else { 0 })
            | (if zero { SR_Z } else { 0 })
            | (if neg { SR_S } else { 0 })
            | (if overflow { SR_O } else { 0 });
    }

    pub fn set_flags_from_subtract(&mut self, a: i64, b: i64, result: i64) {
        // 2's-complement overflow on a - b.
        let of = ((a ^ b) & (a ^ result) & (1i64 << 63)) != 0;
        self.set_flags_int(result, of);
    }

    pub fn set_flags_fp(&mut self, x: f64) {
        let neg = x < 0.0;
        let zero = x == 0.0;
        let bits = x.to_bits() as i64;
        let odd_parity = (bits.count_ones() & 1) == 1;
        let overflow = x.is_infinite();
        self.sr = (if odd_parity { SR_P } else { 0 })
            | (if zero { SR_Z } else { 0 })
            | (if neg { SR_S } else { 0 })
            | (if overflow { SR_O } else { 0 });
    }

    pub fn format_sr(&self) -> String {
        let p = (self.sr & SR_P) != 0;
        let z = (self.sr & SR_Z) != 0;
        let s = (self.sr & SR_S) != 0;
        let o = (self.sr & SR_O) != 0;
        format!(
            "{}{}{}{}",
            if o { "O" } else { "o" },
            if s { "S" } else { "s" },
            if z { "Z" } else { "z" },
            if p { "P" } else { "p" }
        )
    }

    /// Evaluate a condition code (ports `testCond`).
    pub fn test_cond(&self, z: i32) -> bool {
        let p = (self.sr & SR_P) != 0;
        let zf = (self.sr & SR_Z) != 0;
        let s = (self.sr & SR_S) != 0;
        let o = (self.sr & SR_O) != 0;
        match z {
            0 => true,
            1 => zf,
            2 => !zf,
            3 => s,
            4 => !s && !zf,
            5 => !s,
            6 => s || zf,
            7 => o,
            8 => !o,
            9 => !p,
            10 => p,
            _ => false,
        }
    }

    // ===== Memory access (ports fetch/memRead/memWrite) =====
    pub fn fetch(&mut self, pc: i64) -> CpuResult<i64> {
        if pc >= 0 && pc < self.heap_start {
            self.cycles += 1;
            return Ok(self.mem.get(pc as usize));
        }
        self.mem_read(pc)
    }

    pub fn mem_read(&mut self, addr: i64) -> CpuResult<i64> {
        if addr < 0 {
            return memory::shared_mem_read(addr).map_err(CpuException);
        }
        let a = addr;
        let val = if a < self.heap_limit {
            if (a as usize) >= self.mem.len() {
                return Err(CpuException(format!(
                    "Illegal memory read access of {:08x}",
                    addr
                )));
            }
            self.mem.get(a as usize)
        } else {
            let idx = a - self.heap_limit;
            if idx < 0 || idx as usize >= self.stack.len() {
                return Err(CpuException(format!(
                    "Illegal memory read access of {:08x}",
                    addr
                )));
            }
            self.stack[idx as usize]
        };
        self.cycles += 1;
        Ok(val)
    }

    pub fn mem_write(&mut self, addr: i64, val: i64) -> CpuResult<()> {
        if addr < 0 {
            return memory::shared_mem_write(addr, val).map_err(|_| {
                CpuException(format!("Illegal shared memory write access of {:08x}", addr))
            });
        }
        if addr >= self.stack_base {
            return Err(CpuException(format!(
                "Illegal stack write access of {:08x}",
                addr
            )));
        }
        for &(lo, hi) in &self.protected_memory {
            if addr >= lo && addr < hi {
                return Err(CpuException(format!(
                    "Write access violation of {:08x}",
                    addr
                )));
            }
        }
        let a = addr;
        if a < self.heap_limit {
            if (a as usize) >= self.mem.len() {
                return Err(CpuException(format!(
                    "Illegal memory write access of {:08x}",
                    addr
                )));
            }
            self.mem.set(a as usize, val);
        } else {
            let idx = a - self.heap_limit;
            if idx < 0 || idx as usize >= self.stack.len() {
                return Err(CpuException(format!(
                    "Illegal memory write access of {:08x}",
                    addr
                )));
            }
            self.stack[idx as usize] = val;
        }
        self.cycles += 1;
        Ok(())
    }

    pub fn protect(&mut self, addr: i64, limit: i64) -> CpuResult<()> {
        if addr < 0 || addr > self.heap_start {
            return Err(CpuException(format!("Illegal protect base addr at {}", addr)));
        }
        if limit < addr || limit > self.heap_start {
            return Err(CpuException(format!("Illegal protect limit at {}", limit)));
        }
        self.protected_memory.push((addr, limit));
        Ok(())
    }

    // ===== Register/stack convenience (ports getR/setR/push/pop/fpush/fpop) =====
    pub fn get_r(&self, r: usize) -> i64 {
        self.r[r & 0x1F]
    }
    pub fn set_r(&mut self, r: usize, v: i64) {
        self.r[r & 0x1F] = v;
        self.set_flags_int(v, false);
    }
    pub fn get_fp(&self, fr: usize) -> f64 {
        self.f[fr & 0x1F]
    }
    pub fn set_fp(&mut self, fr: usize, v: f64) {
        self.f[fr & 0x1F] = v;
        self.set_flags_fp(v);
    }

    pub fn push(&mut self, val: i64) -> CpuResult<()> {
        self.mem_write(self.r[R_SP], val)?;
        self.r[R_SP] -= 1;
        Ok(())
    }
    pub fn pop(&mut self) -> CpuResult<i64> {
        self.r[R_SP] += 1;
        self.mem_read(self.r[R_SP])
    }
    pub fn fpush(&mut self, val: f64) -> CpuResult<()> {
        self.mem_write(self.r[R_SP], val.to_bits() as i64)?;
        self.r[R_SP] -= 1;
        Ok(())
    }
    pub fn fpop(&mut self) -> CpuResult<f64> {
        self.r[R_SP] += 1;
        let bits = self.mem_read(self.r[R_SP])?;
        Ok(f64::from_bits(bits as u64))
    }

    pub fn get_state(&self) -> CpuStateSnapshot {
        CpuStateSnapshot {
            r: self.r,
            f: self.f,
            sr: self.sr,
            cycles: self.cycles,
        }
    }

    pub fn clear_cpu_state(&mut self) {
        for i in 0..GPR_COUNT - 3 {
            self.r[i] = 0;
        }
        self.f = [0.0; FPR_COUNT];
        self.sr = SR_Z;
    }

    // ===== Timing (ports getClock/getSystemClock) =====
    /// Total elapsed wall-clock time of the run, in nanoseconds
    /// (ports `getClock()` = `System.nanoTime() - startClock`).
    pub fn get_clock(&self) -> i64 {
        crate::util::nano_time() - self.start_clock_ns
    }

    /// Accumulated system (I/O + interrupt handler) time, in nanoseconds
    /// (ports `getSystemClock()` = `totalSystemTime`).
    pub fn get_system_clock(&self) -> i64 {
        self.total_system_time_ns
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sim() -> Simulator {
        Simulator::new(1024, 0, 256, vec![])
    }

    #[test]
    fn flags_zero_and_parity() {
        let mut s = sim();
        s.set_flags_int(0, false);
        assert_eq!(s.sr, SR_Z); // zero, even parity, non-neg, no overflow
        s.set_flags_int(1, false);
        // 1 has odd parity, non-zero, positive.
        assert_eq!(s.sr, SR_P);
        s.set_flags_int(3, false);
        // 3 = 0b11 -> even parity (2 bits), non-zero.
        assert_eq!(s.sr & SR_P, 0);
        s.set_flags_int(-1, false);
        // -1: all bits set -> 64 ones -> even parity; negative.
        assert_eq!(s.sr & SR_S, SR_S);
        assert_eq!(s.sr & SR_P, 0);
    }

    #[test]
    fn subtract_overflow() {
        let mut s = sim();
        // i64::MIN - 1 overflows.
        let a = i64::MIN;
        let b = 1i64;
        let res = a.wrapping_sub(b);
        s.set_flags_from_subtract(a, b, res);
        assert_eq!(s.sr & SR_O, SR_O);
    }

    #[test]
    fn test_cond_codes() {
        let mut s = sim();
        s.sr = SR_Z;
        assert!(s.test_cond(0)); // u
        assert!(s.test_cond(1)); // z
        assert!(!s.test_cond(2)); // nz
        s.sr = SR_S;
        assert!(s.test_cond(3)); // n
        assert!(!s.test_cond(5)); // nn
    }

    #[test]
    fn format_sr_string() {
        let mut s = sim();
        s.sr = SR_Z;
        // Only Z set => O=o, S=s, Z=Z, P=p
        assert_eq!(s.format_sr(), "oszp".chars().enumerate().map(|(i,c)| if i==2 {'Z'} else {c}).collect::<String>());
    }

    #[test]
    fn load_program_sets_pc_sp() {
        let mut s = sim();
        let words = vec![5i64, 6, 7];
        s.load_program(&words, 0, HashMap::new());
        assert_eq!(s.r[R_PC], 0);
        assert_eq!(s.r[R_SP], s.stack_base - 1);
        assert_eq!(s.mem.get(0), 5);
        assert_eq!(s.mem.get(2), 7);
    }

    #[test]
    fn mem_roundtrip_heap_and_stack() {
        let mut s = sim();
        s.load_program(&[0i64], 0, HashMap::new());
        s.mem_write(10, 1234).unwrap();
        assert_eq!(s.mem_read(10).unwrap(), 1234);
        // Stack region: address >= heap_limit.
        let sp = s.stack_base - 1;
        s.mem_write(sp, 99).unwrap();
        assert_eq!(s.mem_read(sp).unwrap(), 99);
    }
}

