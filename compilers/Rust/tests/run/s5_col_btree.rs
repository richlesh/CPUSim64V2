use std::collections::{BTreeMap, BTreeSet};

fn main() {
    let mut m: BTreeMap<String, i32> = BTreeMap::new();
    for (i, w) in "delta alpha charlie bravo echo alpha delta alpha".split(' ').enumerate() {
        *m.entry(w.to_string()).or_insert(0) += i as i32;
    }
    println!("{:?}", m);
    println!("{:#?}", m);
    for (k, v) in &m {
        println!("{} => {}", k, v);
    }
    println!("{:?} {:?}", m.first_key_value(), m.last_key_value());
    println!("{:?} {:?}", m.get("alpha"), m.get("zulu"));
    println!("{} {}", m["echo"], m.len());
    println!("{:?}", m.keys().collect::<Vec<_>>());
    println!("{:?}", m.values().rev().collect::<Vec<_>>());
    println!("{:?}", m.iter().next_back());
    if let Some(v) = m.get_mut("bravo") { *v *= 100; }
    for v in m.values_mut() { *v += 1; }
    println!("{:?}", m);
    println!("{:?} {:?}", m.remove("charlie"), m.remove("charlie"));
    println!("{:?}", m.pop_first());
    println!("{:?}", m.pop_last());
    println!("{:?}", m);

    let mut n: BTreeMap<i32, &str> = BTreeMap::new();
    for k in [50, 10, 40, 20, 30, 10, 60, 5] {
        n.insert(k, "x");
    }
    println!("{:?}", n.keys().copied().collect::<Vec<i32>>());
    println!("{:?}", n.insert(30, "y"));
    println!("{} {}", n.contains_key(&30), n.contains_key(&31));
    n.retain(|k, _| k % 20 != 0);
    println!("{:?}", n);
    let c = n.clone();
    println!("{}", c == n);
    let sq: BTreeMap<i32, i32> = (1..=5).map(|i| (i, i * i)).collect();
    println!("{:?}", sq);
    let mut total = 0;
    for (k, v) in sq.into_iter() { total += k * v; }
    println!("{}", total);
    let mut big = BTreeMap::new();
    for i in (0..300).rev() { big.insert(i * 3 % 301, i); }
    let ks: Vec<i32> = big.keys().cloned().collect();
    let mut sorted = ks.clone();
    sorted.sort();
    println!("{} {}", ks == sorted, big.len());
    println!("{:?}", big.iter().take(3).collect::<Vec<_>>());

    let mut s: BTreeSet<i32> = BTreeSet::new();
    for x in [5, 3, 8, 1, 9, 3, 5, 7] { s.insert(x); }
    println!("{:?} {}", s, s.len());
    println!("{:?} {:?}", s.first(), s.last());
    println!("{} {}", s.contains(&8), s.contains(&4));
    println!("{} {}", s.remove(&8), s.remove(&8));
    let t: BTreeSet<i32> = [7, 8, 9, 10].into_iter().collect();
    println!("{:?}", s.union(&t).collect::<Vec<_>>());
    println!("{:?}", s.intersection(&t).collect::<Vec<_>>());
    println!("{:?}", s.difference(&t).collect::<Vec<_>>());
    println!("{:?}", s.symmetric_difference(&t).collect::<Vec<_>>());
    println!("{:?}", s.iter().rev().collect::<Vec<_>>());
    println!("{:#?}", t);
    let words: BTreeSet<&str> = "the cat sat on the mat".split(' ').collect();
    println!("{:?}", words);
    let ws: BTreeSet<String> = words.iter().map(|w| w.to_uppercase()).collect();
    println!("{:?} {}", ws, ws.contains("CAT"));
    println!("{:?}", s.pop_first());
    println!("{:?}", s.into_iter().collect::<Vec<_>>());
    let e: BTreeMap<i32, i32> = BTreeMap::new();
    println!("{:?} {:?}", e, e.first_key_value());
}
