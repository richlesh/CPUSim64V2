fn main() {
    let mut v: Vec<i32> = (1..=10).collect();
    let dr: Vec<i32> = v.drain(2..5).collect();
    println!("{:?} {:?}", dr, v);
    let dr2: Vec<i32> = v.drain(..2).collect();
    println!("{:?} {:?}", dr2, v);
    v.drain(3..);
    println!("{:?}", v);
    let mut a = vec![1, 2, 3, 4, 5];
    if let Some((f, rest)) = a.split_first() { println!("{} {:?}", f, rest); }
    if let Some((l, rest)) = a.split_last() { println!("{} {:?}", l, rest); }
    let (x, y) = a.split_at(2);
    println!("{:?} {:?}", x, y);
    {
        let (p, q) = a.split_at_mut(2);
        p[0] = 10;
        q[0] = 30;
        p[1] += q[1];
    }
    println!("{:?}", a);
    a.extend_from_slice(&[7, 7, 8]);
    println!("{:?}", a);
    a.dedup();
    println!("{:?}", a);
    let mut b = vec![10, 11, 20, 21, 22, 30];
    b.dedup_by_key(|x| *x / 10);
    println!("{:?}", b);
    b.retain(|&x| x != 20);
    println!("{:?}", b);
    b.insert(1, 99);
    b.insert(3, 98);
    println!("{:?}", b);
    let sr = b.swap_remove(0);
    println!("{} {:?}", sr, b);
    b.truncate(2);
    println!("{:?}", b);
    let words = vec![String::from("a"), String::from("b")];
    println!("{}", words.join("+"));
    for x in b.iter().rev() { print!("{} ", x); }
    println!();
    let mut c = [1, 2, 3, 4, 5, 6];
    c.rotate_left(2);
    println!("{:?}", c);
    c.rotate_right(1);
    println!("{:?}", c);
    c.fill(7);
    println!("{:?}", c);
    for x in c.iter_mut() { *x += 1; }
    println!("{:?}", c);
    let d = [1, 2, 3, 4, 5, 6, 7];
    for ch in d.chunks_exact(3) { println!("{:?}", ch); }
    println!("{} {}", d.contains(&4), d.contains(&9));
    println!("{} {}", d.starts_with(&[1, 2]), d.starts_with(&[2]));
    println!("{}", d.ends_with(&[6, 7]));
    let sl = &d[1..4];
    for x in sl.iter().rev() { print!("{} ", x); }
    println!();
    let mut it = d.iter().rev().skip(1);
    println!("{:?}", it.next());
    let mut m = vec![vec![1, 2], vec![3]];
    m[0].push(5);
    m.swap(0, 1);
    println!("{:?}", m);
    let fl: Vec<i32> = m.concat();
    println!("{:?}", fl);
    let mut st = vec!["b".to_string(), "a".to_string(), "b".to_string()];
    st.sort();
    st.dedup();
    println!("{:?}", st);
    let mut e: Vec<i32> = Vec::new();
    e.extend(vec![1, 2]);
    e.extend(&[3, 4]);
    e.extend((5..7).map(|x| x * 2));
    println!("{:?}", e);
}
