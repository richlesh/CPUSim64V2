// string reversal with Unicode: by char, by word, by line, palindromes, byte-level checks
fn reverse_chars(s: &str) -> String {
    s.chars().rev().collect()
}

fn reverse_words(s: &str) -> String {
    let words: Vec<&str> = s.split_whitespace().collect();
    let mut out = String::new();
    for (i, w) in words.iter().rev().enumerate() {
        if i > 0 {
            out.push(' ');
        }
        out.push_str(w);
    }
    out
}

fn reverse_each_word(s: &str) -> String {
    let mut out = String::new();
    let mut first = true;
    for w in s.split(' ') {
        if !first {
            out.push(' ');
        }
        first = false;
        out.push_str(&reverse_chars(w));
    }
    out
}

fn reverse_manual(s: &str) -> String {
    let chars: Vec<char> = s.chars().collect();
    let mut out = String::with_capacity(s.len());
    let mut i = chars.len();
    while i > 0 {
        i -= 1;
        out.push(chars[i]);
    }
    out
}

fn is_palindrome(s: &str) -> bool {
    let a: Vec<char> = s.chars().collect();
    let n = a.len();
    (0..n / 2).all(|i| a[i] == a[n - 1 - i])
}

fn utf8_lens(s: &str) -> Vec<usize> {
    s.chars().map(|c| c.len_utf8()).collect()
}

fn main() {
    let samples = ["hello", "héllo wörld", "日本語のテキスト", "a😀b", "", "ñandú"];
    for s in samples.iter() {
        let r = reverse_chars(s);
        println!("[{}] -> [{}] ({} bytes, {} chars)", s, r, s.len(), s.chars().count());
        println!("   {}", reverse_manual(s) == r);
        println!("   back {}", reverse_chars(&r) == *s);
    }
    println!("{}", reverse_words("the quick  brown fox"));
    println!("{}", reverse_each_word("über cool größe"));
    println!("{}", is_palindrome("été"));
    println!("{}", is_palindrome("été!"));
    println!("{}", is_palindrome("日本日"));
    println!("{:?}", utf8_lens("aé日😀"));
    let s = "añb";
    let bytes = s.as_bytes();
    println!("{:?}", bytes);
    let mut rev_bytes = bytes.to_vec();
    rev_bytes.reverse();
    println!("{}", String::from_utf8(rev_bytes).is_err());
    println!("{}", String::from_utf8(bytes.to_vec()).unwrap());
    let mut total = 0u32;
    for c in "日本語".chars() {
        total += c as u32;
    }
    println!("{}", total);
    println!("{}", "日本語".chars().rev().map(|c| c.to_string()).collect::<Vec<_>>().join("/"));
}
