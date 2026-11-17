fn main() {
    let x = 5;
    println!("hello");
    println!("x = {}, y = {}", x, x + 1);
    let v = vec![1, 2, 3];
    let w = vec![0; 10];
    assert!(x == 5);
    assert_eq!(x, 5, "x is {}", x);
    print!("{}\n", format!("{:>4}", x));
    let nested = vec![vec![1, 2], vec![3]];
    let s = format!("{}", vec![(1, 2), (3, 4)].len());
    println!("{}", { let a = 1; a + 1 });
    println!();
    if let Some(m) = matches_it!(x) {
        println!("{}", m);
    }
    match x {
        pat_macro!(1) => {}
        _ => {}
    }
    some::path::mac! { a b c };
    mac_stmt![1, 2];
}
