//! QuickSortFloat.rs
//!
//! Sorts 10 arrays of 9-14 random doubles in [0, 1) with quicksort (Lomuto
//! partition, the last element is the pivot) and prints each array (6 decimals)
//! before and after.  Rust version of documentation/projects/QuickSortFloat.asm.
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

    /// a random integer in [low, high]
    fn range(&mut self, low: i64, high: i64) -> i64 {
        low + self.next() % (high - low + 1)
    }

    /// a random double in [0, 1)
    fn unit(&mut self) -> f64 {
        self.next() as f64 / 2147483648.0
    }
}

fn create_random_array(rng: &mut Rng, n: i64) -> Vec<f64> {
    (0..n).map(|_| rng.unit()).collect()
}

/// the elements with 6 decimals, separated by commas
fn join(a: &[f64]) -> String {
    a.iter().map(|x| format!("{:.6}", x)).collect::<Vec<String>>().join(",")
}

/// partitions a around its last element; returns the pivot's final index
fn partition(a: &mut [f64]) -> usize {
    let hi = a.len() - 1;
    let pivot = a[hi];
    let mut i = 0;
    for j in 0..hi {
        if a[j] <= pivot {
            a.swap(i, j);
            i += 1;
        }
    }
    a.swap(i, hi);
    i
}

fn quick_sort_float(a: &mut [f64]) {
    if a.len() > 1 {
        let p = partition(a);
        let (left, right) = a.split_at_mut(p);
        quick_sort_float(left);
        quick_sort_float(&mut right[1..]);
    }
}

fn main() {
    let mut rng = Rng(20251104);
    for i in 1..=10 {
        let len = rng.range(9, 14);
        let mut a = create_random_array(&mut rng, len);
        println!("Test {}", i);
        println!("random: {}", join(&a));
        quick_sort_float(&mut a);
        println!("sorted: {}", join(&a));
    }
}
