// process::exit with a non-zero status after output
fn main() {
    println!("starting");
    let code = 3;
    if code != 0 {
        println!("exiting with {}", code);
        std::process::exit(3);
    }
    println!("unreachable");
}
