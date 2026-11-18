fn main() {
    println!("hello");
    println!();
    print!("a");
    print!("{} and {}", 1, 2);
    println!("{{literal}} {}", "x");
    println!("{0} {1} {0}", "a", "b");
    let name = "bob";
    let width = 8;
    println!("{name} is {name:?}");
    println!("{x} {y}", x = 1, y = 2 + 3);
    println!("{:>width$}|{:<5}|{:^7}|{:*^9}|", 1, 2, 3, 4);
    println!("{:05} {:+} {:#x} {:#010b} {:e}", 42, 7, 255, 5, 1.5);
    println!("{:.2} {:8.3} {:.*} {:.prec$}", 1.0, 2.0, 3, 4.0, 5.0, prec = 1);
    println!("{:#?} {:?} {:x} {:X} {:o}", 1, 2, 3, 4, 5);
    eprintln!("err {}", 1);
    eprint!("err");
    println!("{}", format!("{}-{}", 1, 2));
}
