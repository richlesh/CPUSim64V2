// sorting: sort, sort_by, sort_by_key, sort_unstable, dedup, binary_search, custom orderings
use std::cmp::Ordering;

#[derive(Debug, Clone)]
struct Person {
    name: String,
    age: u32,
}

fn insertion_sort(v: &mut Vec<i32>) {
    for i in 1..v.len() {
        let mut j = i;
        while j > 0 && v[j - 1] > v[j] {
            v.swap(j - 1, j);
            j -= 1;
        }
    }
}

fn quicksort(v: &mut [i32]) {
    if v.len() <= 1 {
        return;
    }
    let pivot = v[v.len() - 1];
    let mut store = 0;
    for i in 0..v.len() - 1 {
        if v[i] < pivot {
            v.swap(i, store);
            store += 1;
        }
    }
    let last = v.len() - 1;
    v.swap(store, last);
    quicksort(&mut v[..store]);
    quicksort(&mut v[store + 1..]);
}

fn merge_sort(v: Vec<i32>) -> Vec<i32> {
    if v.len() <= 1 {
        return v;
    }
    let mid = v.len() / 2;
    let right = merge_sort(v[mid..].to_vec());
    let left = merge_sort(v[..mid].to_vec());
    let mut out = Vec::new();
    let (mut i, mut j) = (0, 0);
    while i < left.len() && j < right.len() {
        if left[i] <= right[j] {
            out.push(left[i]);
            i += 1;
        } else {
            out.push(right[j]);
            j += 1;
        }
    }
    out.extend_from_slice(&left[i..]);
    out.extend_from_slice(&right[j..]);
    out
}

fn main() {
    let data = vec![38, 27, 43, 3, 9, 82, 10, 3];
    let mut a = data.clone();
    a.sort();
    println!("{:?}", a);
    let mut b = data.clone();
    b.sort_by(|x, y| y.cmp(x));
    println!("{:?}", b);
    let mut c = data.clone();
    insertion_sort(&mut c);
    println!("{:?}", c);
    let mut d = data.clone();
    quicksort(&mut d);
    println!("{:?}", d);
    println!("{:?}", merge_sort(data.clone()));
    let mut e = data.clone();
    e.sort_unstable();
    e.dedup();
    println!("{:?}", e);
    println!("{:?} {:?}", e.binary_search(&43), e.binary_search(&5));

    let mut people = vec![
        Person { name: "Cy".to_string(), age: 30 },
        Person { name: "Al".to_string(), age: 25 },
        Person { name: "Bo".to_string(), age: 30 },
        Person { name: "Di".to_string(), age: 22 },
    ];
    people.sort_by_key(|p| p.age);
    println!("{:?}", people.iter().map(|p| p.name.as_str()).collect::<Vec<_>>());
    people.sort_by(|p, q| match q.age.cmp(&p.age) {
        Ordering::Equal => p.name.cmp(&q.name),
        other => other,
    });
    for p in &people {
        println!("{} {}", p.name, p.age);
    }
    let mut words = vec!["pear", "fig", "banana", "kiwi"];
    words.sort();
    println!("{:?}", words);
    words.sort_by_key(|w| w.len());
    println!("{:?}", words);
    let mut fl = vec![2.5, -1.0, 9.75, 0.0];
    fl.sort_by(|x, y| x.partial_cmp(y).unwrap());
    println!("{:?}", fl);
    println!("{:?}", 3.cmp(&5));
    println!("{:?}", "b".cmp("a"));
    println!("{:?}", 2.0f64.partial_cmp(&2.0));
}
