fn count_words(text: &str) -> Vec<(String, usize)> {
    let mut counts: Vec<(String, usize)> = Vec::new();
    for w in text.split_whitespace() {
        let word = w.to_lowercase();
        let mut found = false;
        for entry in counts.iter_mut() {
            if entry.0 == word { entry.1 += 1; found = true; break; }
        }
        if !found { counts.push((word, 1)); }
    }
    counts.sort_by(|a, b| b.1.cmp(&a.1).then(a.0.cmp(&b.0)));
    counts
}
fn longest<'a>(words: &[&'a str]) -> Option<&'a str> {
    let mut best: Option<&str> = None;
    for w in words {
        match best { Some(b) if b.len() >= w.len() => {}, _ => best = Some(*w) }
    }
    best
}
fn main() {
    let text = String::from("the cat and The dog and the bird");
    let counts = count_words(&text);
    for (w, n) in &counts { println!("{}: {}", w, n); }
    let total: usize = counts.iter().map(|(_, n)| *n).sum();
    let words: Vec<&str> = text.split(' ').collect();
    println!("{} {:?}", total, longest(&words));
}
