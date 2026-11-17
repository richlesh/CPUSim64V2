// chars, UTF-8, bytes, char_indices
fn main() {
    let s = "héllo wörld 日本語";
    println!("{} {}", s.len(), s.chars().count());
    for (i, c) in s.char_indices().filter(|(_, c)| !c.is_ascii()) {
        println!("{} {} {}", i, c, c.len_utf8());
    }
    println!("{}", s.chars().rev().collect::<String>());
    let bytes = "aé".as_bytes();
    println!("{:?}", bytes);
    println!("{:?}", "aé".bytes().collect::<Vec<u8>>());
    println!("{:?}", String::from_utf8(vec![104, 105]).unwrap());
    println!("{}", 'é' as u32);
    println!("{:?}", std::char::from_u32(0x65E5));
    println!("{:?}", char::from_u32(0x1F600));
    println!("{}", ('a' as u8 + 3) as char);
    println!("{}", s.is_char_boundary(2));
    println!("{}", &s[0..1]);
    println!("{}", s.starts_with('h'));
    let up: String = "straße".chars().map(|c| c.to_ascii_uppercase()).collect();
    println!("{}", up);
    println!("{:?}", "日本語".chars().map(|c| c as u32).collect::<Vec<_>>());
    let mut out = String::new();
    for c in "tab\tnew\nquote\"".chars() {
        match c {
            '\t' => out.push_str("\\t"),
            '\n' => out.push_str("\\n"),
            '"' => out.push_str("\\\""),
            _ => out.push(c),
        }
    }
    println!("{}", out);
    println!("{:?}", "tab\tnew\n");
    println!("{:?}", 'é');
    println!("{:>6}|{:<6}|{:^6}|", "日本", "é", "ab");
    println!("{}", "😀".len());
    println!("{:?}", "a😀b".chars().nth(1));
    let mut chars: Vec<char> = "dcba".chars().collect();
    chars.sort();
    println!("{}", chars.iter().cloned().collect::<String>());
    println!("{}", 'a' < 'b' && 'é' > 'z');
    println!("{}", char::from(65u8));
    println!("{}", ('a'..='e').collect::<String>());
}
