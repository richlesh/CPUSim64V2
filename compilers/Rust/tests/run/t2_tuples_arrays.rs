// tuples and arrays
fn divmod(a: i32, b: i32) -> (i32, i32) {
    (a / b, a % b)
}

fn sum3(t: (i32, i32, i32)) -> i32 {
    t.0 + t.1 + t.2
}

fn main() {
    let t = (1, "two", 3.0);
    println!("{} {} {}", t.0, t.1, t.2);
    let (q, r) = divmod(47, 5);
    println!("{} {}", q, r);
    println!("{}", sum3((1, 2, 3)));
    let nested = ((1, 2), (3, 4));
    println!("{}", (nested.0).1 + (nested.1).0);
    let mut m = (10, 20);
    m.0 += 5;
    m.1 -= 5;
    println!("{:?}", m);
    println!("{}", (1, 2) < (1, 3));
    println!("{}", (1, 2) == (1, 2));

    let a = [1, 2, 3, 4, 5];
    println!("{} {}", a.len(), a[2]);
    let mut b = [0; 6];
    for i in 0..b.len() {
        b[i] = i * i;
    }
    println!("{:?}", b);
    let mut total = 0;
    for x in a.iter() {
        total += *x;
    }
    println!("{}", total);
    for x in a {
        total += x;
    }
    println!("{}", total);
    let grid = [[1, 2, 3], [4, 5, 6]];
    let mut diag = 0;
    for i in 0..2 {
        for j in 0..3 {
            if i == j {
                diag += grid[i][j];
            }
        }
    }
    println!("{}", diag);
    let mut c = a;
    c[0] = 100;
    println!("{} {}", a[0], c[0]);
    println!("{}", a == [1, 2, 3, 4, 5]);
    let [first, .., last] = a;
    println!("{} {}", first, last);
    let words = ["apple", "fig", "kiwi"];
    let mut longest = words[0];
    for w in words.iter() {
        if w.len() > longest.len() {
            longest = w;
        }
    }
    println!("{}", longest);
    println!("{:?}", grid);
}
