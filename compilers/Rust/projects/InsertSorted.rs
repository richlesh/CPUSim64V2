//! InsertSorted.rs
//!
//! Inserts 30 random integers in [1, 999] one at a time into a sorted, growing
//! array and prints the array after each insertion.  The place of a value is
//! found by binary search.  Rust version of documentation/projects/InsertSorted.asm.
//!
//! CPUSim64's Rust library has no clock and no random generator, so the numbers
//! come from a small linear congruential generator with a fixed seed: they are
//! the same on every run.

struct Rng(u64);

impl Rng {
    /// a random number in [0, 2^31)
    fn next(&mut self) -> i64 {
        self.0 = self.0.wrapping_mul(6364136223846793005).wrapping_add(1442695040888963407);
        (self.0 >> 33) as i64
    }
}

struct SortedArray {
    a: Vec<i64>,
}

impl SortedArray {
    fn insert_sorted(&mut self, val: i64) {
        // the first position whose element is greater than val
        let (mut lo, mut hi) = (0, self.a.len());
        while lo < hi {
            let mid = (lo + hi) / 2;
            if self.a[mid] <= val {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        self.a.insert(lo, val);
    }

    fn to_text(&self) -> String {
        self.a.iter().map(|x| x.to_string()).collect::<Vec<String>>().join(",")
    }
}

fn main() {
    let mut rng = Rng(20251104);
    let mut s = SortedArray { a: Vec::new() };
    println!("Inserting 30 random integers [1,999] one at a time:");
    for _ in 0..30 {
        let v = 1 + rng.next() % 999;
        s.insert_sorted(v);
        println!("Insert {:3}: {}", v, s.to_text());
    }
}
