// read_line loop: echo with line numbers, word counts, stops at "quit" or EOF
use std::io;

fn main() {
    let mut n = 0;
    let mut words = 0;
    loop {
        let mut line = String::new();
        let got = io::stdin().read_line(&mut line).unwrap();
        if got == 0 {
            println!("eof");
            break;
        }
        let t = line.trim_end();
        if t == "quit" {
            println!("bye");
            break;
        }
        n += 1;
        let w = t.split_whitespace().count();
        words += w;
        println!("{:>2}: {} ({} words)", n, t.to_uppercase(), w);
    }
    println!("{} lines, {} words", n, words);
}
