// read stdin line by line: sum numbers, skip blank lines, report bad lines
use std::io;

fn main() {
    let stdin = io::stdin();
    let mut sum: i64 = 0;
    let mut lines = 0;
    let mut bad = 0;
    loop {
        let mut line = String::new();
        if stdin.read_line(&mut line).unwrap() == 0 {
            break;
        }
        let t = line.trim();
        if t.is_empty() {
            continue;
        }
        lines += 1;
        match t.parse::<i64>() {
            Ok(n) => sum += n,
            Err(_) => {
                bad += 1;
                println!("skipping {:?}", t);
            }
        }
    }
    println!("lines {} bad {} sum {}", lines, bad, sum);
}
