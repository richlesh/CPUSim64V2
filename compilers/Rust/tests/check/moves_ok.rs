fn consume(s: String) -> usize { s.len() }

fn reassign_after_move() -> usize {
    let mut s = String::from("a");
    let n = consume(s);
    s = String::from("b");
    n + s.len()
}

fn copy_types() -> i32 {
    let a = 5;
    let b = a;
    a + b
}

fn clone_keeps_original() -> usize {
    let s = String::from("a");
    let t = s.clone();
    s.len() + t.len()
}

fn late_init() -> i32 {
    let x: i32;
    x = 5;
    x + 1
}

fn print_twice() {
    let s = String::from("a");
    println!("{}", s);
    println!("{}", s);
}

fn main() {
    let total = reassign_after_move() + clone_keeps_original();
    println!("{} {} {}", total, copy_types(), late_init());
    print_twice();
}
