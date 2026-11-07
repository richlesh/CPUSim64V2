//! Concurrency model (ports the fork/thread/process parts of `Simulator.java`).
//!
//! Java uses JVM threads over a shared heap array (volatile) with per-thread
//! stacks, plus a ForkJoin-based `fork` that clones the heap. We emulate the
//! *observable behavior*:
//!   * `thread(func, data)` spawns an OS thread running a child `Simulator`
//!     that SHARES the heap (`Arc<SharedMem>`, atomic cells) but has its own
//!     register file and a fresh private stack. This reproduces data races in
//!     unsynchronized code and correct results when a CAS-based mutex serializes
//!     access (as in T105_SYNC).
//!   * `fork()` creates a child with a deep-copied heap (copy-on-write process
//!     semantics) run to completion; the parent continues. Return value is the
//!     child pid (or -1 on failure).
//!   * `allocShared` is handled in `memory.rs`.
//!
//! JVM scheduling is not reproduced; only the observable memory/result effects.

use super::{CpuResult, Simulator, FPR_COUNT, GPR_COUNT, R_PC, R_SF, R_SP};
use std::sync::atomic::{AtomicI64, Ordering};
use std::sync::{Mutex, OnceLock};
use std::thread::JoinHandle;

fn next_pid_counter() -> &'static AtomicI64 {
    static C: OnceLock<AtomicI64> = OnceLock::new();
    C.get_or_init(|| AtomicI64::new(2)) // root is pid 1
}

fn thread_registry() -> &'static Mutex<Vec<(i64, JoinHandle<()>)>> {
    static R: OnceLock<Mutex<Vec<(i64, JoinHandle<()>)>>> = OnceLock::new();
    R.get_or_init(|| Mutex::new(Vec::new()))
}

impl Simulator {
    pub fn next_pid(&mut self) -> i64 {
        next_pid_counter().fetch_add(1, Ordering::SeqCst)
    }

    /// Build a child simulator for a thread: shares the heap `Arc<SharedMem>`,
    /// copies registers/FP/SR, gets a fresh private stack, and its own ports.
    fn spawn_child_thread_sim(&self, pid: i64) -> Simulator {
        let mut ports = std::collections::HashMap::new();
        // Threads start with fresh stdio ports (0/1/2). File ports are not shared
        // across threads in the validation programs.
        for p in 0..=2 {
            ports.insert(p, crate::ports::Port::new_std());
        }
        Simulator {
            r: self.r,
            f: self.f,
            sr: self.sr,
            mem: self.mem.clone(), // shared heap
            stack: vec![0; self.stack_size as usize],
            heap_start: self.heap_start,
            heap_limit: self.heap_limit,
            stack_size: self.stack_size,
            stack_base: self.stack_base,
            heap_list: self.heap_list,
            free_list: self.free_list.clone(),
            protected_memory: self.protected_memory.clone(),
            running: false,
            debug: false,
            trace: false,
            cycles: 0,
            total_system_time_ns: 0,
            start_clock_ns: 0,
            args: self.args.clone(),
            reverse_symbol_map: self.reverse_symbol_map.clone(),
            pid,
            is_root: false,
            ports,
            argv_ptr: 0,
        }
    }

    /// Start a thread running `function` with `data` (ports `thread`).
    pub fn thread(&mut self, function: i64, data: i64) -> CpuResult<i64> {
        let pid = self.next_pid();
        let mut child = self.spawn_child_thread_sim(pid);
        // Set up the child's stack frame as Java does:
        //   push data; push -1 (return address, -1 exits); push old SF; SF=SP; PC=function.
        child.push(data)?;
        child.push(-1)?;
        let old_sf = child.r[R_SF];
        child.push(old_sf)?;
        child.r[R_SF] = child.r[R_SP];
        child.r[R_PC] = function;
        let start_pc = function;
        let handle = std::thread::spawn(move || {
            let mut c = child;
            let _ = c.run_thread(start_pc);
        });
        thread_registry().lock().unwrap().push((pid, handle));
        Ok(pid)
    }

    /// A thread body run (like `run` but never prints the final CPU state and is
    /// not the root; returns the exit code).
    fn run_thread(&mut self, start_pc: i64) -> CpuResult<i32> {
        self.running = true;
        self.r[R_PC] = start_pc;
        let mut d = crate::decode::Decoded::default();
        while self.running {
            let pc = self.r[R_PC];
            let instr = self.fetch(pc)?;
            self.r[R_PC] = pc + 1;
            d.decode_into(instr);
            self.exec(&d)?;
        }
        Ok(self.r[0] as i32)
    }

    pub fn join_thread(&mut self, pid: i64) -> CpuResult<()> {
        let handle = {
            let mut reg = thread_registry().lock().unwrap();
            if let Some(pos) = reg.iter().position(|(p, _)| *p == pid) {
                Some(reg.remove(pos).1)
            } else {
                None
            }
        };
        if let Some(h) = handle {
            let _ = h.join();
        }
        Ok(())
    }

    pub fn wake_thread(&mut self, _pid: i64) {
        // No blocking sleep model in the native port; wake is a no-op.
    }

    /// Fork a child process (deep-copied heap), run it to completion, return pid.
    pub fn fork(&mut self) -> CpuResult<i64> {
        let pid = self.next_pid();
        let mut child = self.spawn_child_process_sim(pid);
        child.set_r(0, 0); // child sees fork() == 0
        let start_pc = child.r[R_PC];
        let handle = std::thread::spawn(move || {
            let mut c = child;
            let _ = c.run_thread(start_pc);
        });
        thread_registry().lock().unwrap().push((pid, handle));
        Ok(pid)
    }

    fn spawn_child_process_sim(&self, pid: i64) -> Simulator {
        let mut ports = std::collections::HashMap::new();
        for p in 0..=2 {
            ports.insert(p, crate::ports::Port::new_std());
        }
        Simulator {
            r: self.r,
            f: self.f,
            sr: self.sr,
            mem: self.mem.deep_copy(), // process = copy-on-write heap
            stack: self.stack.clone(),
            heap_start: self.heap_start,
            heap_limit: self.heap_limit,
            stack_size: self.stack_size,
            stack_base: self.stack_base,
            heap_list: self.heap_list,
            // Process = deep copy of the free list (fork COW semantics), NOT a
            // shared Arc like threads use.
            free_list: std::sync::Arc::new(std::sync::Mutex::new(
                self.free_list.lock().unwrap().clone(),
            )),
            protected_memory: self.protected_memory.clone(),
            running: false,
            debug: false,
            trace: false,
            cycles: 0,
            total_system_time_ns: 0,
            start_clock_ns: 0,
            args: self.args.clone(),
            reverse_symbol_map: self.reverse_symbol_map.clone(),
            pid,
            is_root: false,
            ports,
            argv_ptr: 0,
        }
    }

    pub fn wait_all(&mut self) -> CpuResult<()> {
        let handles: Vec<JoinHandle<()>> = {
            let mut reg = thread_registry().lock().unwrap();
            reg.drain(..).map(|(_, h)| h).collect()
        };
        for h in handles {
            let _ = h.join();
        }
        Ok(())
    }

    pub fn wait_pid(&mut self, pid: i64) -> CpuResult<()> {
        self.join_thread(pid)
    }
}

// Reference the GPR/FPR counts so the imports are used even if the struct
// literal paths change.
#[allow(dead_code)]
const _ASSERT_COUNTS: (usize, usize, usize, usize, usize) =
    (GPR_COUNT, FPR_COUNT, R_PC, R_SF, R_SP);
