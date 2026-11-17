//! Tower_of_Hanoi.rs
//!
//! Solves the Tower of Hanoi puzzle for n disks recursively and prints the pegs
//! about ten times along the way (and at the end).  The pegs are an n x 3
//! array: slot 0 is the top of a peg, 0 means no disk.
//! Rust version of documentation/projects/Tower_of_Hanoi.asm.
//!
//! Input (first line of standard input): the number of disks

use std::io::BufRead;

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("Tower_of_Hanoi")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

struct Hanoi {
    num_discs: usize,
    move_number: i64,
    print_mod: i64, // the pegs are printed after every print_mod moves
    pegs: Vec<i64>, // pegs[peg * num_discs + slot]
}

impl Hanoi {
    fn get_disk(&self, peg: usize, slot: usize) -> i64 {
        self.pegs[peg * self.num_discs + slot]
    }

    fn set_disk(&mut self, peg: usize, slot: usize, v: i64) {
        self.pegs[peg * self.num_discs + slot] = v;
    }

    fn print_pegs(&self) {
        let mut out = format!("Move: {}\n", self.move_number);
        for i in 0..self.num_discs {
            for j in 0..3 {
                match self.get_disk(j, i) {
                    0 => out.push('|'),
                    disk => out.push_str(&disk.to_string()),
                }
                out.push_str("   ");
            }
            out.push('\n');
        }
        out.push_str("=========\n\n");
        print!("{}", out);
    }

    /// moves the top disk of src to the top of dst
    fn make_move(&mut self, src: usize, dst: usize) {
        let mut disk = 0;
        let mut pos = self.num_discs - 1;
        self.move_number += 1;
        for i in 0..self.num_discs {
            if self.get_disk(src, i) != 0 {
                disk = self.get_disk(src, i);
                self.set_disk(src, i, 0);
                break;
            }
        }
        for i in 0..self.num_discs {
            if self.get_disk(dst, i) != 0 {
                pos = i - 1;
                break;
            }
        }
        self.set_disk(dst, pos, disk);
        if self.move_number % self.print_mod == 0 {
            self.print_pegs();
        }
    }

    fn solve(&mut self, n: usize, src: usize, dst: usize, aux: usize) {
        if n == 1 {
            self.make_move(src, dst);
        } else {
            self.solve(n - 1, src, aux, dst);
            self.make_move(src, dst);
            self.solve(n - 1, aux, dst, src);
        }
    }
}

fn main() {
    let argv = args();
    if argv.len() != 2 {
        println!("Syntax: Hanoi [disks]");
        std::process::exit(1);
    }
    let mut n: i64 = argv[1].parse().unwrap_or(0);
    if n <= 0 {
        n = 3;
    }
    let mut h = Hanoi { num_discs: n as usize, move_number: 0, print_mod: 1, pegs: vec![0; n as usize * 3] };
    for i in 0..n as usize {
        h.set_disk(0, i, i as i64 + 1);
    }
    // print about 10 times: every steps/10 moves, rounded down to a multiple of 5 (100 for n > 10)
    let steps = (1i64 << n) - 1;
    let mut pm = steps / 10 + 2;
    pm -= pm % (if n > 10 { 100 } else { 5 });
    if pm < 1 {
        pm = 1;
    }
    h.print_mod = pm;
    h.print_pegs();
    h.solve(n as usize, 0, 2, 1);
    if h.move_number % h.print_mod != 0 {
        h.print_pegs();
    }
}
