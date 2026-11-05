//! Object file (.o64) and reverse symbol map (.sym1) loading.
//!
//! Ports `AsmIO.readU64BE` (a GZIP stream of big-endian i64 words read until
//! EOF) and `Simulator.readReverseLabelMapFromFile`.

use flate2::read::GzDecoder;
use std::collections::HashMap;
use std::fs::File;
use std::io::{self, BufRead, BufReader, Read};
use std::path::Path;

/// Read a `.o64` object file: a GZIP stream of big-endian i64 words.
///
/// Mirrors `AsmIO.readU64BE`, which reads `DataInputStream.readLong()` (always
/// big-endian) until `EOFException`.
pub fn read_u64_be<P: AsRef<Path>>(path: P) -> io::Result<Vec<i64>> {
    let file = File::open(path)?;
    let mut decoder = GzDecoder::new(file);
    let mut bytes = Vec::new();
    decoder.read_to_end(&mut bytes)?;

    // The Java reader stops at EOF; a partial trailing word (< 8 bytes) is
    // silently dropped because readLong() throws EOFException mid-word.
    let mut words = Vec::with_capacity(bytes.len() / 8);
    let full = bytes.len() / 8;
    for i in 0..full {
        let off = i * 8;
        let mut buf = [0u8; 8];
        buf.copy_from_slice(&bytes[off..off + 8]);
        words.push(i64::from_be_bytes(buf));
    }
    Ok(words)
}

/// Read a `.sym1` reverse label map file (`<long>: <name>` per line).
///
/// Mirrors `Simulator.readReverseLabelMapFromFile`: blank lines and lines
/// starting with `#` are skipped; lines without a colon are skipped; a key that
/// fails to parse as a long is skipped (with a stderr note in Java — we skip
/// silently to avoid polluting the validation-compared output).
pub fn read_reverse_label_map<P: AsRef<Path>>(path: P) -> io::Result<HashMap<i64, String>> {
    let file = File::open(path)?;
    let reader = BufReader::new(file);
    let mut map = HashMap::new();
    for line in reader.lines() {
        let line = line?;
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        let colon = match line.find(':') {
            Some(c) => c,
            None => continue,
        };
        let key_str = line[..colon].trim();
        let val_str = line[colon + 1..].trim();
        if let Ok(key) = key_str.parse::<i64>() {
            map.insert(key, val_str.to_string());
        }
    }
    Ok(map)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;

    fn validation_dir() -> PathBuf {
        // crate dir is .../cpusim64_native; validation is .../cpusim64/validation
        let mut p = PathBuf::from(env!("CARGO_MANIFEST_DIR"));
        p.pop();
        p.push("cpusim64");
        p.push("validation");
        p
    }

    #[test]
    fn decode_t13_add_object() {
        let mut p = validation_dir();
        p.push("obj");
        p.push("T13_ADD.o64");
        let words = read_u64_be(&p).expect("read T13_ADD.o64");
        // Reference T13_ADD.out reports "Read 28 words from T13_ADD.o64".
        assert_eq!(words.len(), 28, "T13_ADD word count");
    }

    #[test]
    fn decode_t00_nop_object() {
        let mut p = validation_dir();
        p.push("obj");
        p.push("T00_NOP.o64");
        let words = read_u64_be(&p).expect("read T00_NOP.o64");
        assert!(!words.is_empty(), "T00_NOP should have words");
    }

    #[test]
    fn parse_sym1() {
        let mut p = validation_dir();
        p.push("obj");
        p.push("T13_ADD.sym1");
        let map = read_reverse_label_map(&p).expect("read T13_ADD.sym1");
        // Every value should be non-empty; keys parse as integers.
        for (_k, v) in &map {
            assert!(!v.is_empty());
        }
    }
}
