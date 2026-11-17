// String and &str operations
fn is_palindrome(s: &str) -> bool {
    let cleaned: Vec<char> = s
        .chars()
        .filter(|c| c.is_alphanumeric())
        .map(|c| c.to_ascii_lowercase())
        .collect();
    let n = cleaned.len();
    for i in 0..n / 2 {
        if cleaned[i] != cleaned[n - 1 - i] {
            return false;
        }
    }
    true
}

fn count_vowels(s: &str) -> usize {
    s.chars().filter(|c| "aeiouAEIOU".contains(*c)).count()
}

fn caesar(s: &str, k: u8) -> String {
    let mut out = String::new();
    for c in s.chars() {
        if c.is_ascii_lowercase() {
            out.push((((c as u8 - b'a' + k) % 26) + b'a') as char);
        } else if c.is_ascii_uppercase() {
            out.push((((c as u8 - b'A' + k) % 26) + b'A') as char);
        } else {
            out.push(c);
        }
    }
    out
}

fn main() {
    let mut s = String::new();
    s.push('a');
    s.push_str("bc");
    s += "def";
    s = s + "g";
    println!("{} {}", s, s.len());
    println!("{}", s.to_uppercase());
    println!("{}", "MiXed".to_lowercase());
    let t = "  padded text \n";
    println!("[{}] [{}] [{}]", t.trim(), t.trim_start(), t.trim_end());
    println!("{}", "hello world".replace("o", "0"));
    println!("{:?}", "a,b,,c".split(',').collect::<Vec<&str>>());
    println!("{:?}", "  one  two three ".split_whitespace().collect::<Vec<_>>());
    println!("{:?}", "l1\nl2\r\nl3".lines().collect::<Vec<_>>());
    println!("{} {}", "hello".starts_with("he"), "hello".ends_with("lo"));
    println!("{:?} {:?}", "hello".find('l'), "hello".find("xyz"));
    println!("{}", "hello".contains("ell"));
    println!("{}", &"hello world"[6..]);
    println!("{}", &"hello world"[..5]);
    println!("{:?}", "abc".chars().rev().collect::<String>());
    println!("{:?}", "abc".bytes().map(|b| b as u32).sum::<u32>());
    println!("{}", "ab".repeat(3));
    println!("{}", "apple" < "banana");
    println!("{}", String::from("x") == "x");
    println!("{}", is_palindrome("A man, a plan, a canal: Panama"));
    println!("{}", is_palindrome("hello"));
    println!("{}", count_vowels("Programming Language"));
    println!("{}", caesar("Hello, World!", 3));
    println!("{:?}", "key=value".split_once('='));
    println!("{:?}", "prefix_rest".strip_prefix("prefix_"));
    let mut owned = String::from("hello");
    owned.push('>');
    println!("{}", owned);
    println!("{:?}", owned.pop());
    owned.clear();
    println!("{} {}", owned.is_empty(), owned.len());
    let num = 42;
    let msg = format!("{}:{:>4}:{:<4}:", "id", num, num);
    println!("{}", msg);
    println!("{}", 12.to_string() + &7.to_string());
    println!("{:?}", "x=1;y=2".split(';').map(|p| p.len()).collect::<Vec<_>>());
    let c = 'Z';
    println!("{} {} {}", c.is_alphabetic(), c.is_numeric(), c.to_ascii_lowercase());
    println!("{:?} {:?}", '7'.to_digit(10), 'g'.to_digit(16));
    println!("{:?}", std::char::from_digit(5, 10));
}
