fn main() {
    let t = (true, 1);
    match t {
        (true, _) => {}
        (false, 0) => {}
    }
}
