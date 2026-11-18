//! md5.rs
//!
//! Computes the MD5 digest of the data.  Rust version of
//! documentation/projects/md5.asm.
//!
//! CPUSim64 has no command line and no files: the first line of standard input
//! is the file name that is printed after the digest, the data is the rest of
//! standard input.

use std::io::Read;

const SHIFT: [u32; 64] = [
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, //
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, //
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, //
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21,
];

/// K[i] = floor(2^32 * |sin(i + 1)|)
const K: [u32; 64] = [
    0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE, 0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501, //
    0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE, 0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821, //
    0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA, 0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8, //
    0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED, 0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A, //
    0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C, 0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70, //
    0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05, 0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665, //
    0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039, 0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1, //
    0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1, 0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391,
];

fn args() -> Vec<String> {
    let mut line = String::new();
    let _ = std::io::stdin().read_line(&mut line);
    let mut v = vec![String::from("md5")];
    v.extend(line.split_whitespace().map(|s| s.to_string()));
    v
}

struct Md5 {
    h: [u32; 4],
}

impl Md5 {
    fn new() -> Md5 {
        Md5 { h: [0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476] }
    }

    /// processes one 64-byte block
    fn block(&mut self, blk: &[u8]) {
        let mut m = [0u32; 16];
        for i in 0..16 {
            m[i] = (blk[4 * i] as u32)
                | ((blk[4 * i + 1] as u32) << 8)
                | ((blk[4 * i + 2] as u32) << 16)
                | ((blk[4 * i + 3] as u32) << 24);
        }
        let (mut a, mut b, mut c, mut d) = (self.h[0], self.h[1], self.h[2], self.h[3]);
        for i in 0..64 {
            let (f, g) = if i < 16 {
                (((c ^ d) & b) ^ d, i)
            } else if i < 32 {
                (((b ^ c) & d) ^ c, (5 * i + 1) % 16)
            } else if i < 48 {
                (b ^ c ^ d, (3 * i + 5) % 16)
            } else {
                (c ^ (b | !d), (7 * i) % 16)
            };
            let f = f.wrapping_add(a).wrapping_add(K[i]).wrapping_add(m[g]);
            a = d;
            d = c;
            c = b;
            // f.rotate_left(SHIFT[i]) (u32::rotate_left is missing from lib/core.rs)
            let rotated = (f << SHIFT[i]) | (f >> (32 - SHIFT[i]));
            b = b.wrapping_add(rotated);
        }
        self.h[0] = self.h[0].wrapping_add(a);
        self.h[1] = self.h[1].wrapping_add(b);
        self.h[2] = self.h[2].wrapping_add(c);
        self.h[3] = self.h[3].wrapping_add(d);
    }

    fn update(&mut self, data: &[u8]) {
        // the padding: 0x80, zeros up to 56 bytes mod 64, the length in bits (little endian)
        let mut msg = data.to_vec();
        msg.push(0x80);
        while msg.len() % 64 != 56 {
            msg.push(0);
        }
        let bits = (data.len() as u64) * 8;
        for i in 0..8 {
            msg.push(((bits >> (8 * i)) & 0xFF) as u8);
        }
        for blk in msg.chunks(64) {
            self.block(blk);
        }
    }

    /// the digest: the bytes of each word, lowest first
    fn digest(&self) -> String {
        let mut s = String::new();
        for &w in self.h.iter() {
            for j in 0..4 {
                s.push_str(&format!("{:02X}", (w >> (8 * j)) & 0xFF));
            }
        }
        s
    }
}

fn main() {
    let argv = args();
    let mut data = Vec::new();
    std::io::stdin().read_to_end(&mut data).unwrap();
    let mut md5 = Md5::new();
    md5.update(&data);
    println!();
    if argv.len() < 2 {
        println!("{}", md5.digest());
    } else {
        println!("{} {}", md5.digest(), argv[1]);
    }
}
