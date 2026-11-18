use std::cmp::Ordering;
struct Person { name: String, age: u32, score: f64 }
fn people() -> Vec<Person> {
    vec![
        Person { name: "ann".to_string(), age: 30, score: 8.5 },
        Person { name: "bob".to_string(), age: 25, score: 9.5 },
        Person { name: "cy".to_string(), age: 30, score: 7.0 },
    ]
}
fn bubble(v: &mut Vec<i32>) {
    let n = v.len();
    for i in 0..n { for j in 0..n - 1 - i { if v[j] > v[j + 1] { let t = v[j]; v[j] = v[j + 1]; v[j + 1] = t; } } }
}
fn main() {
    let mut ps = people();
    ps.sort_by(|a, b| a.age.cmp(&b.age).then_with(|| b.name.cmp(&a.name)));
    println!("{}", ps.iter().map(|p| p.name.clone()).collect::<Vec<_>>().join(","));
    ps.sort_by(|a, b| b.score.partial_cmp(&a.score).unwrap_or(Ordering::Equal));
    ps.sort_by_key(|p| p.name.len());
    let mut v = vec![4, 2, 9, 1];
    bubble(&mut v);
    v.sort_by(|a, b| b.cmp(a));
    let oldest = ps.iter().max_by_key(|p| p.age).unwrap();
    let best = ps.iter().min_by(|a, b| a.score.partial_cmp(&b.score).unwrap()).unwrap();
    println!("{:?} {} {}", v, oldest.name, best.score);
}
