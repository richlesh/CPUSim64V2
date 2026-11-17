// Vec operations
fn main() {
    let mut v: Vec<i32> = Vec::new();
    for i in 1..=5 {
        v.push(i * i);
    }
    println!("{:?} len={}", v, v.len());
    println!("{:?}", v.pop());
    v.insert(1, 100);
    println!("{:?}", v);
    println!("{}", v.remove(0));
    println!("{:?}", v);
    v.extend(vec![7, 8]);
    v.extend_from_slice(&[9, 10]);
    println!("{:?}", v);
    v.truncate(5);
    println!("{:?}", v);
    v.swap(0, 4);
    println!("{:?}", v);
    v.reverse();
    println!("{:?}", v);
    println!("{} {}", v[0], v[v.len() - 1]);
    v[2] = -1;
    println!("{:?}", v);
    println!("{}", v.contains(&-1));
    println!("{:?}", v.iter().position(|&x| x == -1));
    v.retain(|x| *x > 0);
    println!("{:?}", v);
    v.dedup();
    let z = vec![0u8; 4];
    println!("{:?}", z);
    let w = vec!["a", "b", "c"];
    println!("{}", w.join("-"));
    println!("{:?} {:?}", w.first(), w.last());
    let mut grid = vec![vec![0; 3]; 3];
    for i in 0..3 {
        for j in 0..3 {
            grid[i][j] = i * 3 + j;
        }
    }
    println!("{:?}", grid);
    let drained: Vec<i32> = vec![v.remove(0), v.remove(0)];
    println!("{:?} {:?}", drained, v);
    let mut a = vec![1, 2, 3];
    let mut b = vec![4, 5];
    a.append(&mut b);
    println!("{:?} {:?}", a, b);
    println!("{}", a.is_empty());
    let c = a.clone();
    println!("{}", a == c);
    let s: Vec<String> = a.iter().map(|x| format!("<{}>", x)).collect();
    println!("{}", s.concat());
    println!("{:?} {:?}", a.first(), &a[1..]);
    println!("{:?}", a.iter().rev().take(2).collect::<Vec<_>>());
    let mut cap = Vec::with_capacity(10);
    cap.push('x');
    println!("{:?} {}", cap, cap.capacity() >= 10);
    let nested: Vec<(i32, String)> = vec![(1, "one".to_string()), (2, "two".to_string())];
    for (n, name) in &nested {
        println!("{} {}", n, name);
    }
    a.clear();
    println!("{:?}", a);
}
