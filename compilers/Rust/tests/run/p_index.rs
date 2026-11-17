// index out of bounds on a Vec
fn get(v: &Vec<i32>, i: usize) -> i32 {
    v[i]
}

fn main() {
    let v = vec![10, 20, 30];
    println!("{}", get(&v, 2));
    println!("{}", get(&v, 7));
}
