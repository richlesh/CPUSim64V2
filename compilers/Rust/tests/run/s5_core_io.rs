use std::io::{self, BufRead, Read, Write};
use std::env;
fn main() {
    let stdin = io::stdin();
    let mut first = String::new();
    stdin.lock().read_line(&mut first).unwrap();
    println!("first=[{}]", first.trim_end());
    let mut count = 0;
    let mut total = 0i64;
    {
        // the lock is released before read_to_string below (std would deadlock otherwise)
        let mut lines = stdin.lock().lines();
        while let Some(line) = lines.next() {
            let line = line.unwrap();
            if line == "END" { break; }
            count += 1;
            for tok in line.split_whitespace() {
                if let Ok(n) = tok.parse::<i64>() { total += n; }
            }
        }
    }
    println!("count={} total={}", count, total);
    let mut rest = String::new();
    io::stdin().read_to_string(&mut rest).unwrap();
    println!("rest={:?}", rest);
    let out = io::stdout();
    let mut w = out.lock();
    writeln!(w, "hello {}", 5).unwrap();
    write!(w, "a").unwrap();
    write!(w, "b\n").unwrap();
    w.write_all(b"bytes\n").unwrap();
    w.flush().unwrap();
    let mut err = io::stderr();
    writeln!(err, "to stderr").unwrap();
    io::stdout().flush().unwrap();
    let args: Vec<String> = env::args().collect();
    println!("{}", args.len());
    println!("{}", env::args().skip(1).count());
}
