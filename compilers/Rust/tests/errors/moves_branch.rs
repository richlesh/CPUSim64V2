fn f(s: String) {}
fn main() {
    let s = String::from("a");
    if true {
        f(s);
    }
    let n = s.len();
}
