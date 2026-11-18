// slices: borrowing, sub-slices, mutation, windows, chunks, split patterns
fn sum(s: &[i32]) -> i32 {
    let mut t = 0;
    for x in s {
        t += x;
    }
    t
}

fn max_of(s: &[i32]) -> i32 {
    let mut m = s[0];
    for &x in &s[1..] {
        if x > m {
            m = x;
        }
    }
    m
}

fn double_all(s: &mut [i32]) {
    for x in s.iter_mut() {
        *x *= 2;
    }
}

fn reverse(s: &mut [i32]) {
    let n = s.len();
    for i in 0..n / 2 {
        s.swap(i, n - 1 - i);
    }
}

fn is_sorted(s: &[i32]) -> bool {
    s.windows(2).all(|w| w[0] <= w[1])
}

fn main() {
    let mut a = [5, 3, 8, 1, 9, 2];
    println!("{} {}", sum(&a), max_of(&a));
    println!("{}", sum(&a[1..4]));
    println!("{}", sum(&a[..2]));
    println!("{}", sum(&a[4..]));
    println!("{:?}", &a[2..=3]);
    double_all(&mut a[..3]);
    println!("{:?}", a);
    reverse(&mut a);
    println!("{:?}", a);
    println!("{} {}", is_sorted(&a), is_sorted(&[1, 2, 2, 3]));
    for w in a.windows(3) {
        print!("{} ", w[0] + w[1] + w[2]);
    }
    println!();
    for c in a.chunks(4) {
        print!("{:?} ", c);
    }
    println!();
    println!("{:?} {:?}", a.first(), a.last());
    println!("{:?} {:?}", a.get(2), a.get(10));
    println!("{}", a.contains(&16));
    let empty: &[i32] = &[];
    println!("{} {:?}", empty.len(), empty.first());
    let (left, right) = a.split_at(2);
    println!("{:?} {:?}", left, right);
    let mut v = a.to_vec();
    v.sort();
    println!("{:?}", v);
    println!("{:?}", v.binary_search(&9));
    println!("{}", v.iter().rev().map(|x| x.to_string()).collect::<Vec<String>>().join(","));
    if let [x, y, rest @ ..] = &v[..] {
        println!("{} {} {}", x, y, rest.len());
    }
}
