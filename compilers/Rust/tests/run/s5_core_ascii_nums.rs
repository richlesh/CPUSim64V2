fn main() {
    for b in [b'a', b'Z', b'5', b' ', b'\n', b'#', 200u8] {
        println!("{} {} {} {} {} {} {} {} {}", b, b.is_ascii_alphabetic(), b.is_ascii_alphanumeric(), b.is_ascii_digit(),
            b.is_ascii_whitespace(), b.is_ascii_uppercase(), b.is_ascii_lowercase(),
            b.to_ascii_uppercase(), b.to_ascii_lowercase());
    }
    println!("{} {}", b'a'.eq_ignore_ascii_case(&b'A'), b'a'.eq_ignore_ascii_case(&b'b'));
    for c in ['a', 'Z', '5', ' ', '\t', '#', 'é'] {
        println!("{:?} {} {} {} {} {} {} {} {}", c, c.is_ascii_alphabetic(), c.is_ascii_alphanumeric(), c.is_ascii_digit(),
            c.is_ascii_whitespace(), c.is_ascii_uppercase(), c.is_ascii_lowercase(),
            c.to_ascii_uppercase(), c.to_ascii_lowercase());
    }
    println!("{}", 'a'.eq_ignore_ascii_case(&'A'));
    println!("{:?} {:?}", (-5i32).checked_abs(), i32::MIN.checked_abs());
    println!("{:?} {:?}", 5i32.checked_neg(), i32::MIN.checked_neg());
    println!("{:?} {:?}", 0u32.checked_neg(), 1u32.checked_neg());
    println!("{:?} {:?}", 3i32.checked_pow(4), 3i32.checked_pow(40));
    println!("{:?} {:?}", 2u8.checked_pow(7), 2u8.checked_pow(8));
    println!("{} {}", 5i32.wrapping_neg(), i32::MIN.wrapping_neg());
    println!("{} {}", 1u32.leading_zeros(), 0u32.leading_zeros());
    println!("{} {}", 8u32.trailing_zeros(), 0u32.trailing_zeros());
    println!("{} {}", 1u8.leading_zeros(), 255u8.leading_zeros());
    println!("{} {}", 1u64.leading_zeros(), 16i64.trailing_zeros());
    println!("{} {}", (-1i32).leading_zeros(), 1i16.leading_zeros());
    println!("{:?} {:?}", i32::from_str_radix("ff", 16), i32::from_str_radix("-101", 2));
    println!("{:?} {:?}", u8::from_str_radix("zz", 36), u8::from_str_radix("12", 2));
    println!("{:?} {:?}", u64::from_str_radix("777", 8), i64::from_str_radix("", 10));
    println!("{:?}", u32::from_str_radix("FfFf", 16));
    println!("{} {}", 255u8.to_string(), (-17i64).to_string());
    println!("{} {}", i64::MAX.to_string(), u64::MAX);
    println!("{} {}", 12345u32.count_ones(), 7i8.pow(2));
    println!("{} {}", 17i32.rem_euclid(5), (-17i32).rem_euclid(5));
    println!("{} {}", 10u32.abs_diff(3), 3i32.abs_diff(-4));
    println!("{:?} {:?}", 200u8.checked_add(100), 200u8.saturating_add(100));
    println!("{}", u8::MAX.wrapping_add(2));
    println!("{}", format!("{:>6}|{:<6}|{:^6}|{:06}", 42, 42, 42, 42));
    println!("{}", "The Quick".to_ascii_uppercase());
    println!("{}", "The Quick".to_ascii_lowercase());
    println!("{}", "abc".eq_ignore_ascii_case("ABC"));
    println!("{:?}", "A1 b".bytes().filter(|b| b.is_ascii_alphabetic()).collect::<Vec<u8>>());
}
