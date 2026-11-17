// unwrap on None
fn find(v: &[i32], x: i32) -> Option<usize> {
    for i in 0..v.len() {
        if v[i] == x {
            return Some(i);
        }
    }
    None
}

fn main() {
    let v = [1, 2, 3];
    println!("{}", find(&v, 2).unwrap());
    println!("{}", find(&v, 9).unwrap());
}
