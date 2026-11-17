fn main() {
    let w = 6;
    let p = 2;
    let v = 3.14159;
    let name = "né";
    println!("héllo wörld ✓ {}", "ünï");
    println!(r"raw {{ {} \n", 1);
    println!(r#"raw "quoted" {}"#, 2);
    println!("{:→^9}|{:é<4}|{:0>5}|{: >3}", 1, 2, 3, 4);
    println!("{:+.2}|{:<#10x}|{:^+10}|{:08.3}", v, 255, 7, v);
    println!("{:1$}|{:.1$}|{:w$.p$}|{:>1$}", 5, 3, v, 9);
    println!("{name:>w$}|{v:.p$}|{0:?}|{0}", name);
    println!("{:5}|{:<5}|{:^5}|{:>5}", "ab", "cd", "ef", "gh");
    println!("{:#?}", name);
    println!("{:e} {:E}", 1500.0, 0.00025);
    println!("{:#b} {:#o} {:#X}", 5, 64, 255);
    println!("{:?} {:?}", "q\"uote", 'c');
    println!("{}%", 50);
    println!("{{}} {{{}}}", 1);
    println!("{} {}", "a", "b",);
    println!("{x}{y}{x}", x = 1, y = 2);
    println!("{0}{1}{0}{x}", 1, 2, x = 3);
    println!("{:width$.prec$}", v, width = 10, prec = 3);
    println!("{:>1$} {:>1$}", 1, 5);
    println!("{}", b"bytes".len());
    println!("a\tb\\n \u{1F600}");
}
