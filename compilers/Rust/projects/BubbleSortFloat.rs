//! BubbleSortFloat.rs
//!
//! Sorts 10 arrays of 9-14 random doubles in [0, 1) with bubble sort and prints
//! each array (6 decimals) before and after.  Rust version of
//! documentation/projects/BubbleSortFloat.asm.
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

fn bubble_sort_float(a: &mut [f64]) {
    let n = a.len();
    for i in 1..n {
        for j in 0..n - i {
            if a[j] > a[j + 1] {
                a.swap(j, j + 1);
            }
        }
    }
}

fn main() {
    let mut rng = Rng(20251104);
    for i in 1..=10 {
        let len = rng.range(9, 14);
        let mut a = create_random_array(&mut rng, len);
        println!("Test {}", i);
        println!("random: {}", join(&a));
        bubble_sort_float(&mut a);
        println!("sorted: {}", join(&a));
    }
}
