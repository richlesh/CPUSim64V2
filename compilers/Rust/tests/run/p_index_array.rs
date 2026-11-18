// index out of bounds on a fixed array, through a slice
fn at(s: &[u8], i: usize) -> u8 {
    s[i]
}

fn main() {
    let a = [1u8, 2, 3, 4];
    let mut sum = 0u32;
    for i in 0..=4 {
        sum += at(&a, i) as u32;
        println!("sum {}", sum);
    }
}
