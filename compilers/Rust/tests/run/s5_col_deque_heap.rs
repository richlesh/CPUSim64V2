use std::cmp::Reverse;
use std::collections::{BinaryHeap, VecDeque};

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord)]
struct Job { prio: u32, name: String }

fn main() {
    let mut d: VecDeque<i32> = VecDeque::new();
    for i in 1..=5 { d.push_back(i); }
    d.push_front(0);
    d.push_front(-1);
    println!("{:?} len {}", d, d.len());
    println!("{:?} {:?}", d.front(), d.back());
    println!("{:?} {:?}", d.pop_front(), d.pop_back());
    println!("{} {}", d[0], d[3]);
    d[1] = 100;
    println!("{:?}", d);
    println!("{:?}", d.iter().rev().collect::<Vec<_>>());
    for x in d.iter_mut() { *x += 1; }
    println!("{:?} {}", d, d.contains(&101));
    let total: i32 = d.iter().sum();
    println!("{}", total);
    // wrap around and growth
    let mut w: VecDeque<u32> = VecDeque::new();
    let mut check = 0u64;
    for i in 0..100u32 {
        w.push_back(i);
        if i % 3 == 0 { check += w.pop_front().unwrap() as u64; }
        if i % 7 == 0 { w.push_front(1000 + i); }
    }
    println!("{} {} {:?} {:?}", w.len(), check, w.front(), w.back());
    let v: Vec<u32> = w.into_iter().take(5).collect();
    println!("{:?}", v);
    let mut q: VecDeque<(usize, usize)> = VecDeque::new();
    q.push_back((0, 0));
    let mut visited = 0;
    while let Some((x, y)) = q.pop_front() {
        visited += 1;
        if x < 3 { q.push_back((x + 1, y)); }
        if y < 3 && x == 0 { q.push_back((x, y + 1)); }
    }
    println!("visited {}", visited);
    let e: VecDeque<i32> = VecDeque::from(vec![1, 2, 3]);
    let f: VecDeque<i32> = (1..=3).collect();
    println!("{} {:?}", e == f, e.clone());
    let mut s: VecDeque<String> = VecDeque::new();
    s.push_back("a".to_string());
    s.push_front("b".to_string());
    println!("{:?} {:?}", s, s.get(1));
    println!("{:?}", d.get(99));

    let mut h = BinaryHeap::new();
    for x in [5, 1, 8, 3, 9, 2, 7] { h.push(x); }
    println!("{} {:?}", h.len(), h.peek());
    let mut out = Vec::new();
    while let Some(x) = h.pop() { out.push(x); }
    println!("{:?}", out);
    let h2: BinaryHeap<i32> = vec![4, 10, 3, 5, 1].into_iter().collect();
    println!("{:?}", h2.into_sorted_vec());
    let mut mh = BinaryHeap::new();
    for x in [5, 1, 8, 3] { mh.push(Reverse(x)); }
    println!("{:?}", mh.pop().map(|r| r.0));
    println!("{:?}", mh.peek().map(|r| r.0));
    let mut jobs = BinaryHeap::new();
    jobs.push(Job { prio: 2, name: "write".to_string() });
    jobs.push(Job { prio: 9, name: "deploy".to_string() });
    jobs.push(Job { prio: 5, name: "test".to_string() });
    jobs.push(Job { prio: 9, name: "alert".to_string() });
    while let Some(j) = jobs.pop() { println!("{} {}", j.prio, j.name); }
    // dijkstra style
    let mut pq = BinaryHeap::new();
    pq.push(Reverse((0u32, 0usize)));
    let adj: Vec<Vec<(usize, u32)>> = vec![vec![(1, 4), (2, 1)], vec![(3, 1)], vec![(1, 2), (3, 5)], vec![]];
    let mut dist = vec![u32::MAX; 4];
    while let Some(Reverse((dd, u))) = pq.pop() {
        if dd >= dist[u] { continue; }
        dist[u] = dd;
        for &(v, wgt) in &adj[u] { pq.push(Reverse((dd + wgt, v))); }
    }
    println!("{:?}", dist);
    let mut big = BinaryHeap::new();
    for i in 0..200u32 { big.push((i * 37) % 101); }
    let mut last = u32::MAX;
    let mut ok = true;
    while let Some(x) = big.pop() { if x > last { ok = false; } last = x; }
    println!("{}", ok);
}
