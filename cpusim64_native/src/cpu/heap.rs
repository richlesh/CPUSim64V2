//! Fibonacci-block heap allocator (ports the heap half of `Simulator.java`).
//!
//! Heap block layout (word offsets from block pointer `p`):
//!   p+0 = prev (free-list style linkage; -1 for null)
//!   p+1 = next
//!   p+2 = size  (includes the 3-word header; negative => free block)
//!   p+3.. = data
//!
//! The free list is a Java `LinkedList<Long>` used as a stack: `push` prepends,
//! iteration is head->tail, `remove(value)` removes the first matching element.
//! We model it with `free_list: Vec<i64>` where index 0 is the head.

use super::{CpuException, CpuResult, Simulator};

/// Fibonacci sizing table (ports the static `fibonacci[]`).
const FIBONACCI: [i64; 39] = [
    8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946, 17711, 28657,
    46368, 75025, 121393, 196418, 317811, 514229, 832040, 1346269, 2178309, 3524578, 5702887,
    9227465, 14930352, 24157817, 39088169, 63245986, 102334155, 165580141, 267914296, 433494437,
    701408733,
];

fn fibonacci_size(size: i64) -> i64 {
    let mut i = 0;
    while i < FIBONACCI.len() && FIBONACCI[i] < size {
        i += 1;
    }
    if i == FIBONACCI.len() {
        size
    } else {
        FIBONACCI[i]
    }
}

impl Simulator {
    // free list helpers mirroring LinkedList<Long> used as a stack
    // Free-list helpers mirroring the Java `LinkedList<Long>` used as a stack.
    // They operate on an already-locked free list so the caller can hold the
    // lock across a whole alloc/free (making the multi-step heap-metadata
    // mutation atomic across threads).
    fn free_list_push(free: &mut Vec<i64>, p: i64) {
        free.insert(0, p);
    }
    fn free_list_remove_value(free: &mut Vec<i64>, p: i64) {
        if let Some(pos) = free.iter().position(|&x| x == p) {
            free.remove(pos);
        }
    }

    /// Allocate `requested_words` of heap storage; returns a data pointer or 0
    /// on failure (ports `alloc`).
    pub fn alloc(&mut self, requested_words: i64) -> CpuResult<i64> {
        // Lock the (possibly thread-shared) free list for the whole allocation
        // so the read-modify-write of shared heap metadata is atomic.
        let fl = self.free_list.clone();
        let mut free = fl.lock().unwrap();

        if free.is_empty() {
            return Ok(0);
        }
        if requested_words < 1 || requested_words > (i32::MAX as i64) - 3 {
            return Err(CpuException(format!(
                "Illegal memory allocation: {}",
                requested_words
            )));
        }
        let num_words = fibonacci_size(requested_words + 3);

        // Iterate the free list front-to-back (snapshot to allow mutation).
        let snapshot: Vec<i64> = free.clone();
        for p in snapshot {
            let size = self.mem_read(p + 2)?;
            if size < 0 && -size >= num_words {
                if -size - num_words < FIBONACCI[0] {
                    // Use whole block.
                    self.mem_write(p + 2, -size)?;
                    Self::free_list_remove_value(&mut free, p);
                    return Ok(p + 3);
                } else {
                    // Split into one alloc + one free block.
                    let new_p = p + num_words;
                    self.mem_write(new_p, p)?;
                    let next = self.mem_read(p + 1)?;
                    self.mem_write(new_p + 1, next)?;
                    self.mem_write(new_p + 2, size + num_words)?;
                    self.mem_write(p + 1, new_p)?;
                    self.mem_write(p + 2, num_words)?;
                    Self::free_list_remove_value(&mut free, p);
                    Self::free_list_push(&mut free, new_p);
                    return Ok(p + 3);
                }
            }
        }
        Ok(0)
    }

    /// Reallocate (ports `realloc`).
    pub fn realloc(&mut self, old_addr: i64, new_size: i64) -> CpuResult<i64> {
        let mut new_addr = old_addr;
        let old_size = self.mem_read(old_addr - 1)?;
        let new_size = fibonacci_size(new_size + 3);
        if old_size < new_size {
            new_addr = self.alloc(new_size - 3)?;
            if new_addr != 0 {
                self.memmove(new_addr, old_addr, old_size - 3)?;
                self.free(old_addr)?;
            }
        } else if old_size > new_size {
            new_addr = self.alloc(new_size - 3)?;
            if new_addr != 0 {
                self.memmove(new_addr, old_addr, new_size - 3)?;
                self.free(old_addr)?;
            }
        }
        Ok(new_addr)
    }

    fn combine_blocks(&mut self, free: &mut Vec<i64>, p: i64) -> CpuResult<()> {
        if p <= 0 {
            return Ok(());
        }
        let size = self.mem_read(p + 2)?;
        if size >= 0 {
            return Ok(()); // not free
        }
        let next = self.mem_read(p + 1)?;
        if next > 0 {
            let next_size = self.mem_read(next + 2)?;
            if next_size < 0 {
                self.mem_write(p + 2, next_size + size)?;
                let next_next = self.mem_read(next + 1)?;
                self.mem_write(p + 1, next_next)?;
                if next_next > 0 {
                    self.mem_write(next_next, p)?;
                }
                self.mem_write(next, -1)?;
                self.mem_write(next + 1, -1)?;
                self.mem_write(next + 2, 0)?;
                Self::free_list_remove_value(free, next);
            }
        }
        Ok(())
    }

    /// Free an allocation (ports `free`).
    pub fn free(&mut self, address: i64) -> CpuResult<()> {
        let p = address - 3;
        if p <= 0 {
            return Ok(());
        }
        // Lock the (possibly thread-shared) free list for the whole free so the
        // push + coalescing is atomic across threads.
        let fl = self.free_list.clone();
        let mut free = fl.lock().unwrap();

        let size = self.mem_read(p + 2)?;
        if size > 0 {
            self.mem_write(p + 2, -size)?;
        } else {
            return Ok(());
        }
        Self::free_list_push(&mut free, p);
        self.combine_blocks(&mut free, p)?;
        let prev = self.mem_read(p)?;
        self.combine_blocks(&mut free, prev)?;
        Ok(())
    }

    pub fn count_heap_blocks(&mut self, count_alloc: bool, count_free: bool) -> CpuResult<i64> {
        let mut p = self.heap_start;
        let mut num_alloc = 0;
        let mut num_free = 0;
        while p > 0 {
            if self.mem_read(p + 2)? < 0 {
                num_free += 1;
            } else {
                num_alloc += 1;
            }
            p = self.mem_read(p + 1)?;
        }
        Ok(select(count_alloc, count_free, num_alloc, num_free))
    }

    pub fn count_heap_size(&mut self, count_alloc: bool, count_free: bool) -> CpuResult<i64> {
        let mut p = self.heap_start;
        let mut num_alloc = 0;
        let mut num_free = 0;
        while p > 0 {
            let size = self.mem_read(p + 2)?;
            if size < 0 {
                num_free += -size;
            } else {
                num_alloc += size;
            }
            p = self.mem_read(p + 1)?;
        }
        Ok(select(count_alloc, count_free, num_alloc, num_free))
    }

    pub fn walk_heap(&mut self) -> CpuResult<()> {
        let mut p = self.heap_start;
        print!("Heap Blocks:\n");
        while p > 0 {
            let size = self.mem_read(p + 2)?;
            print!("{:08x}: {}\n", p, size);
            p = self.mem_read(p + 1)?;
        }
        Ok(())
    }

    pub fn memmove(&mut self, dest: i64, src: i64, size: i64) -> CpuResult<()> {
        let mem_len = self.mem.len() as i64;
        if dest < 0 || dest > mem_len {
            return Err(CpuException("Illegal memmove dest argument!".into()));
        }
        if src < 0 || src > mem_len {
            return Err(CpuException("Illegal memmove src argument!".into()));
        }
        if src + size > mem_len || dest + size > mem_len {
            return Err(CpuException("Illegal memmove size argument!".into()));
        }
        // System.arraycopy handles overlap correctly; emulate via a temp buffer.
        let mut buf = Vec::with_capacity(size as usize);
        for i in 0..size {
            buf.push(self.mem.get((src + i) as usize));
        }
        for i in 0..size {
            self.mem.set((dest + i) as usize, buf[i as usize]);
        }
        Ok(())
    }

    pub fn memclear(&mut self, src: i64, size: i64) -> CpuResult<()> {
        let mem_len = self.mem.len() as i64;
        if src < 0 || src > mem_len {
            return Err(CpuException("Illegal memclear src argument!".into()));
        }
        if src + size > mem_len {
            return Err(CpuException("Illegal memclear size argument!".into()));
        }
        for i in 0..size {
            self.mem.set((src + i) as usize, 0);
        }
        Ok(())
    }

    /// Allocate and store a UTF-8 string, length-prefixed (ports `allocString`).
    pub fn alloc_string(&mut self, s: &str, alloc_buffer: i64) -> CpuResult<i64> {
        let utf8 = s.as_bytes();
        let needed_words = (utf8.len() as i64 + 7) / 8 + 1;
        let result = if alloc_buffer <= 0 {
            self.alloc(needed_words)?
        } else if self.mem_read(alloc_buffer - 1)? - 3 >= needed_words {
            alloc_buffer
        } else {
            self.free(alloc_buffer)?;
            self.alloc(needed_words)?
        };
        if result != 0 {
            let mut p = result;
            self.mem_write(p, utf8.len() as i64)?;
            p += 1;
            let mut buffer: i64 = 0;
            let mut index: i32 = 7;
            for &c in utf8 {
                buffer |= ((c as i64) & 0xFF) << (index * 8);
                index -= 1;
                if index < 0 {
                    self.mem_write(p, buffer)?;
                    p += 1;
                    buffer = 0;
                    index = 7;
                }
            }
            if index != 7 {
                self.mem_write(p, buffer)?;
            }
        }
        Ok(result)
    }

    /// Read a length-prefixed UTF-8 string from the heap (ports `convertString`).
    pub fn convert_string(&mut self, addr: i64) -> CpuResult<String> {
        if addr < 0 {
            return Err(CpuException("Illegal string address".into()));
        }
        let mut a = addr;
        let byte_count = self.mem_read(a)?;
        a += 1;
        if byte_count < 0 || byte_count > i32::MAX as i64 {
            return Err(CpuException(format!(
                "Invalid UTF-8 byte count: {}",
                byte_count
            )));
        }
        let mut bytes = Vec::with_capacity(byte_count as usize);
        let mut out_index = 0i64;
        while out_index < byte_count {
            let w = self.mem_read(a)?;
            a += 1;
            let mut b = 7i32;
            while b >= 0 && out_index < byte_count {
                bytes.push(((w >> (b * 8)) & 0xFF) as u8);
                out_index += 1;
                b -= 1;
            }
        }
        Ok(String::from_utf8_lossy(&bytes).into_owned())
    }
}

fn select(count_alloc: bool, count_free: bool, num_alloc: i64, num_free: i64) -> i64 {
    if count_alloc && count_free {
        num_alloc + num_free
    } else if count_alloc {
        num_alloc
    } else if count_free {
        num_free
    } else {
        0
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::cpu::Simulator;
    use std::collections::HashMap;

    fn sim() -> Simulator {
        let mut s = Simulator::new(4096, 0, 1024, vec![]);
        s.load_program(&[0i64], 0, HashMap::new());
        s
    }

    #[test]
    fn fib_sizing() {
        assert_eq!(fibonacci_size(1), 8);
        assert_eq!(fibonacci_size(8), 8);
        assert_eq!(fibonacci_size(9), 13);
        assert_eq!(fibonacci_size(21), 21);
        assert_eq!(fibonacci_size(22), 34);
    }

    #[test]
    fn alloc_free_roundtrip() {
        let mut s = sim();
        let a = s.alloc(5).expect("alloc");
        assert!(a > 0);
        // One allocated block now.
        assert_eq!(s.count_heap_blocks(true, false).unwrap(), 1);
        s.free(a).expect("free");
        // After free + coalesce, no allocated blocks remain.
        assert_eq!(s.count_heap_blocks(true, false).unwrap(), 0);
    }

    #[test]
    fn string_roundtrip() {
        let mut s = sim();
        let p = s.alloc_string("Hello, world!", 0).expect("allocString");
        assert!(p > 0);
        let back = s.convert_string(p).expect("convertString");
        assert_eq!(back, "Hello, world!");
    }

    #[test]
    fn utf8_string_roundtrip() {
        let mut s = sim();
        let p = s.alloc_string("héllo→✓", 0).expect("allocString");
        let back = s.convert_string(p).expect("convertString");
        assert_eq!(back, "héllo→✓");
    }
}
