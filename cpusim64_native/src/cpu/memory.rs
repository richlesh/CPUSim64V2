//! Memory model (ports the memory half of `Simulator.java`).
//!
//! Java layout:
//!   * `mem[0..heapLimit)` — the heap region, shared across threads (volatile).
//!   * `stack[]` — per-thread, indexed `addr - heapLimit`.
//!   * negative addresses — a single global shared-memory `LongArray`, indexed
//!     `addr - Long.MIN_VALUE`.
//!
//! We model the heap as an `Arc<SharedMem>` of `AtomicI64` cells so that threads
//! (Task 11) can share it with the same volatile semantics; the stack is an
//! owned `Vec<i64>`; shared memory is a process-global `Mutex<Vec<i64>>`.

use std::sync::atomic::{AtomicI64, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

/// Heap region shared across threads of one program.
pub struct SharedMem {
    pub cells: Vec<AtomicI64>,
}

impl SharedMem {
    pub fn new(words: usize) -> Arc<SharedMem> {
        let mut cells = Vec::with_capacity(words);
        for _ in 0..words {
            cells.push(AtomicI64::new(0));
        }
        Arc::new(SharedMem { cells })
    }

    #[inline]
    pub fn get(&self, i: usize) -> i64 {
        self.cells[i].load(Ordering::SeqCst)
    }

    #[inline]
    pub fn set(&self, i: usize, v: i64) {
        self.cells[i].store(v, Ordering::SeqCst);
    }

    /// Deep copy the heap (ports the `mem = cloneMe.mem.clone()` path used by
    /// `fork`/process creation).
    pub fn deep_copy(&self) -> Arc<SharedMem> {
        let mut cells = Vec::with_capacity(self.cells.len());
        for c in &self.cells {
            cells.push(AtomicI64::new(c.load(Ordering::SeqCst)));
        }
        Arc::new(SharedMem { cells })
    }

    pub fn len(&self) -> usize {
        self.cells.len()
    }

    pub fn is_empty(&self) -> bool {
        self.cells.is_empty()
    }
}

/// Process-global shared memory (ports the static `LongArray sharedMem`).
/// Addresses are negative; index = `addr - i64::MIN`.
fn shared_mem() -> &'static Mutex<Vec<i64>> {
    static SHARED: OnceLock<Mutex<Vec<i64>>> = OnceLock::new();
    SHARED.get_or_init(|| Mutex::new(Vec::new()))
}

pub fn shared_mem_read(addr: i64) -> Result<i64, String> {
    let a = addr.wrapping_sub(i64::MIN);
    let guard = shared_mem().lock().unwrap();
    if a < 0 || (a as usize) >= guard.len() {
        return Err(format!("Illegal shared memory read access of {:08x}", addr));
    }
    Ok(guard[a as usize])
}

pub fn shared_mem_write(addr: i64, val: i64) -> Result<(), String> {
    let a = addr.wrapping_sub(i64::MIN);
    let mut guard = shared_mem().lock().unwrap();
    if a < 0 || (a as usize) >= guard.len() {
        return Err(format!("Illegal shared memory write access of {:08x}", addr));
    }
    guard[a as usize] = val;
    Ok(())
}

/// Allocate `size` words of shared memory, returning the base negative address
/// (ports `allocShared`).
pub fn alloc_shared(size: i64) -> i64 {
    let mut guard = shared_mem().lock().unwrap();
    let new_alloc = guard.len();
    guard.resize(new_alloc + size as usize, 0);
    i64::MIN + new_alloc as i64
}

/// Reset the process-global shared memory (test helper; Java keeps it static for
/// the JVM lifetime, but tests want isolation).
pub fn reset_shared_mem() {
    shared_mem().lock().unwrap().clear();
}
