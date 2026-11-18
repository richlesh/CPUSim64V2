// std::env::args(): the program, then the command line arguments
fn main() {
    let v: Vec<String> = std::env::args().collect();
    println!("{}", v.len());
    println!("{}", v[0].ends_with(".o64"));
    for a in v.iter().skip(1) {
        println!("[{}] {} chars", a, a.chars().count());
    }
    let n: i64 = std::env::args().nth(1).unwrap().parse().unwrap();
    println!("{}", n * 2);
    println!("{}", std::env::args().len());
    let last = std::env::args().rev().next().unwrap();
    println!("{}", last);
    let rest: Vec<String> = std::env::args().skip(2).collect();
    println!("{}", rest.join(","));
}
