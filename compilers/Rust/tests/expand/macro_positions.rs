// paths, delimiters, shadowing of built-ins, macros in nested expression positions
macro_rules! square {
    ($x:expr) => { $x * $x };
}

mod util {
    macro_rules! twice {
        ($e:expr) => { $e + $e };
    }
    pub fn four() -> i32 { twice!(2) }
    pub mod deeper {
        pub fn six() -> i32 { twice!(3) + square!(0) }
    }
}

fn pick(flag: bool) -> i32 {
    if flag { square!(3) } else { square!(4) }
}

fn arms(n: i32) -> String {
    match n {
        0 => format!("zero"),
        1 => String::from("one"),
        _ => format!("many {}", n),
    }
}

fn nested_calls() -> usize {
    let v = vec![vec![1, 2], vec![3]];
    v.iter().map(|inner| inner.len()).sum::<usize>() + vec![0; 2].len()
}

fn main() {
    ::std::println!("global path");
    std::println!("std path {}", 1);
    core::panic!("core path");
    std::assert!(true);
    std::assert_eq!(1, 1);
    let v = std::vec![1, 2, 3];
    println!{"braces {}", 2}
    println!["brackets {}", 3];
    let s = format!("{}{}", format!("{}", 1), format!("{}", 2));
    let t = if square!(2) > 3 { "big" } else { "small" };
    let u = [square!(1), square!(2), square!(3)];
    let w = (square!(5), vec![square!(6)]);
    let c = |x: i32| square!(x) + 1;
    let r = &square!(7);
    let o = Some(square!(8)).map(|v| v + 1);
    while square!(0) > 1 {
        break;
    }
    for i in 0..square!(3) {
        print!("{} ", i);
    }
    println!();
    return;
}
