fn main() {
    let s = String::from("a");
    let c = move || s.len();
    let n = s.len();
}
