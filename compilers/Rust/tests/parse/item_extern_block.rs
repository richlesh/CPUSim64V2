extern "C" {
    fn puts(s: *const u8) -> i32;
    fn printf(fmt: *const u8, ...) -> i32;
    fn named_variadic(fmt: *const u8, args: ...);
    static errno: i32;
    static mut counter: u32;
    pub fn exported(x: i32);
    pub static EXP: u8;
    type Opaque;
    #[link_name = "other"]
    fn renamed();
}
extern {
    fn bare();
}
unsafe extern "C" {
    fn safe_decl();
}
extern "system" { }
extern "Rust" fn rust_abi() {}
unsafe extern "C" fn unsafe_c() {}
#[link(name = "m")]
extern "C" { fn cos(x: f64) -> f64; }
