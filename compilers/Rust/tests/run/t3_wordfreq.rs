// word frequency without HashMap: sorted Vec of (word, count), plus text stats
fn bump(counts: &mut Vec<(String, usize)>, word: &str) {
    for entry in counts.iter_mut() {
        if entry.0 == word {
            entry.1 += 1;
            return;
        }
    }
    counts.push((word.to_string(), 1));
}

fn main() {
    let text = "the quick brown fox jumps over the lazy dog the fox";
    let mut counts: Vec<(String, usize)> = Vec::new();
    for w in text.split_whitespace() {
        bump(&mut counts, w);
    }
    counts.sort_by(|a, b| b.1.cmp(&a.1).then(a.0.cmp(&b.0)));
    for (w, n) in counts.iter().take(4) {
        println!("{:<6}{:>3}", w, n);
    }
    let total: usize = counts.iter().map(|(_, n)| n).sum();
    println!("total {} unique {}", total, counts.len());
    let longest = text.split(' ').max_by_key(|w| w.len()).unwrap();
    println!("{}", longest);
    let caps: Vec<String> = text
        .split(' ')
        .map(|w| {
            let mut cs = w.chars();
            match cs.next() {
                Some(f) => f.to_ascii_uppercase().to_string() + cs.as_str(),
                None => String::new(),
            }
        })
        .collect();
    println!("{}", caps.join(" "));
    let mut letters = [0usize; 26];
    for b in text.bytes() {
        if b >= b'a' && b <= b'z' {
            letters[(b - b'a') as usize] += 1;
        }
    }
    let mut best = 0;
    for i in 1..26 {
        if letters[i] > letters[best] {
            best = i;
        }
    }
    println!("{} {}", (b'a' + best as u8) as char, letters[best]);
}
