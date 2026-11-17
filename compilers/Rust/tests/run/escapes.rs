// string literal escapes: \n \t \\ \" (and \' \r)
fn main() {
    println!("tab:\there");
    println!("two\nlines");
    println!("back\\slash and \"quotes\" and \'single\'");
    /* a block comment /* nested */ is skipped */
    println!("percent % and 100%d are plain text")
}
