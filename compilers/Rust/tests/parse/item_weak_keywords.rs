union U { a: u32 }
pub union PU { a: u8 }
fn f() {
    let union = 1;
    let auto = 2;
    let macro_rules = 3;
    union + 1;
    auto = 4;
    union;
    const { 5 };
    let c = const { 6 };
    unsafe { }
    async { };
    async move { };
}
fn union() {}
struct auto;
trait auto2 {}
auto trait AutoT {}
type union = i32;
const auto: i32 = 1;
