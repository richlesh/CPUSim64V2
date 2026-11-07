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

    /// Atomic compare-and-set on a heap cell (ports `atomicMem.compareAndSet`).
    /// Returns `true` if the cell held `expected` and was updated to `new`.
    #[inline]
    pub fn compare_and_set(&self, i: usize, expected: i64, new: i64) -> bool {
        self.cells[i]
            .compare_exchange(expected, new, Ordering::SeqCst, Ordering::SeqCst)
            .is_ok()
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

/// Atomic compare-and-set on process-global shared memory (ports the
/// `synchronized (Simulator.class)` block in `opCAS` for addr < 0). Holds the
/// lock across the compare and the write so the operation is indivisible.
pub fn shared_mem_compare_and_set(addr: i64, expected: i64, new: i64) -> Result<bool, String> {
    let a = addr.wrapping_sub(i64::MIN);
    let mut guard = shared_mem().lock().unwrap();
    if a < 0 || (a as usize) >= guard.len() {
        return Err(format!("Illegal CAS access of {:08x}", addr));
    }
    if guard[a as usize] == expected {
        guard[a as usize] = new;
        Ok(true)
    } else {
        Ok(false)
    }
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

#[cfg(test)]
mod tests {
    use super::*;

    /// Many threads each do CAS-based increments on one shared cell. With a true
    /// atomic compare-and-set, no updates are lost and the final value equals
    /// threads * iters. A non-atomic get-then-set would lose updates under
    /// contention.
    #[test]
    fn shared_mem_cas_is_atomic_under_contention() {
        const THREADS: usize = 8;
        const ITERS: i64 = 50_000;
        let mem = SharedMem::new(1);
        std::thread::scope(|s| {
            for _ in 0..THREADS {
                let mem = &mem;
                s.spawn(move || {
                    for _ in 0..ITERS {
                        // CAS retry loop, mirroring the assembly RAW_COUNTER loop.
                        loop {
                            let old = mem.get(0);
                            if mem.compare_and_set(0, old, old + 1) {
                                break;
                            }
                        }
                    }
                });
            }
        });
        assert_eq!(mem.get(0), THREADS as i64 * ITERS);
    }

    #[test]
    fn compare_and_set_basic() {
        let mem = SharedMem::new(1);
        mem.set(0, 42);
        assert!(!mem.compare_and_set(0, 7, 99)); // wrong expected -> no change
        assert_eq!(mem.get(0), 42);
        assert!(mem.compare_and_set(0, 42, 99)); // correct expected -> swap
        assert_eq!(mem.get(0), 99);
    }
}
