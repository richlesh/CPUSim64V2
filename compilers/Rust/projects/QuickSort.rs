//! QuickSort.rs
//!
//! Sorts 10 arrays of 15-30 random integers in [1, 999] with quicksort (Lomuto
//! partition, the last element is the pivot) and prints each array before and
//! after.  Rust version of documentation/projects/QuickSort.asm.
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
}

fn create_random_array(rng: &mut Rng, n: i64) -> Vec<i64> {
    (0..n).map(|_| rng.range(1, 999)).collect()
}

/// the elements separated by commas
fn join(a: &[i64]) -> String {
    a.iter().map(|x| x.to_string()).collect::<Vec<String>>().join(",")
}

/// partitions a around its last element; returns the pivot's final index
fn partition(a: &mut [i64]) -> usize {
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

fn quick_sort(a: &mut [i64]) {
    if a.len() > 1 {
        let p = partition(a);
        let (left, right) = a.split_at_mut(p);
        quick_sort(left);
        quick_sort(&mut right[1..]);
    }
}

fn main() {
    let mut rng = Rng(20251104);
    for i in 1..=10 {
        let len = rng.range(15, 30);
        let mut a = create_random_array(&mut rng, len);
        println!("Test {}", i);
        println!("random: {}", join(&a));
        quick_sort(&mut a);
        println!("sorted: {}", join(&a));
    }
}
