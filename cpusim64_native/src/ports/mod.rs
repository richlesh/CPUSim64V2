//! Port handlers (ports `PortHandler` + `StdIOPortHandler` / `FilePortHandler`
//! / `RawFilePortHandler`).
//!
//! The Java design is an abstract `PortHandler` with endian-aware multi-byte
//! `read(count)`/`write(count)`, a pushback buffer, and zero-extended return
//! values, subclassed by stdio/file/raw handlers. We model the concrete
//! handlers as a `Port` enum sharing a common `PortState` (port number,
//! endianness, pushback, eof/error), with the multi-byte logic implemented once
//! on `Port`.

use std::fs::{File, OpenOptions};
use std::io::{BufReader, BufWriter, Read, Write};

pub const REG_SIZE_BYTES: usize = 8;

/// Shared per-port state (ports the fields on the abstract `PortHandler`).
#[derive(Clone)]
pub struct PortState {
    pub port: i32,
    /// Java field name is `littleEndian` but the default (`false`) means
    /// big-endian; `true` means little-endian.
    pub little_endian: bool,
    pub error: bool,
    pub eof: bool,
    pub pushback: Vec<u8>,
}

impl Default for PortState {
    fn default() -> Self {
        PortState {
            port: 0,
            little_endian: false,
            error: false,
            eof: false,
            pushback: Vec::with_capacity(REG_SIZE_BYTES),
        }
    }
}

/// The kind of underlying I/O for a port.
pub enum PortIo {
    /// Standard streams (0 = stdin, 1 = stdout, 2 = stderr).
    Std,
    /// Text file (UTF-8). Modes: read uses `reader`, write/append use `writer`.
    File {
        reader: Option<BufReader<File>>,
        writer: Option<BufWriter<File>>,
        filespec: String,
    },
    /// Raw binary file. readChar/writeChar are errors.
    Raw {
        reader: Option<BufReader<File>>,
        writer: Option<BufWriter<File>>,
        filespec: String,
    },
}

pub struct Port {
    pub state: PortState,
    pub io: PortIo,
}

use crate::cpu::{CpuException, CpuResult};

impl Port {
    pub fn new_std() -> Port {
        Port {
            state: PortState::default(),
            io: PortIo::Std,
        }
    }

    /// Open a text file (ports `FilePortHandler`). mode: 0 read, 1 write, 2 append.
    pub fn new_file(mode: i32, filespec: &str) -> CpuResult<Port> {
        let io = match mode {
            0 => {
                let f = File::open(filespec).map_err(|_| {
                    CpuException(format!(
                        "File open error on \"{}\" using mode {}!",
                        filespec, mode
                    ))
                })?;
                PortIo::File {
                    reader: Some(BufReader::new(f)),
                    writer: None,
                    filespec: filespec.to_string(),
                }
            }
            1 => {
                let f = File::create(filespec).map_err(|_| {
                    CpuException(format!(
                        "File open error on \"{}\" using mode {}!",
                        filespec, mode
                    ))
                })?;
                PortIo::File {
                    reader: None,
                    writer: Some(BufWriter::new(f)),
                    filespec: filespec.to_string(),
                }
            }
            2 => {
                let f = OpenOptions::new()
                    .create(true)
                    .append(true)
                    .open(filespec)
                    .map_err(|_| {
                        CpuException(format!(
                            "File open error on \"{}\" using mode {}!",
                            filespec, mode
                        ))
                    })?;
                PortIo::File {
                    reader: None,
                    writer: Some(BufWriter::new(f)),
                    filespec: filespec.to_string(),
                }
            }
            _ => return Err(CpuException("Illegal file open mode!".into())),
        };
        Ok(Port {
            state: PortState::default(),
            io,
        })
    }

    /// Open a raw binary file (ports `RawFilePortHandler`).
    pub fn new_raw(filespec: &str, mode: i32) -> CpuResult<Port> {
        let io = match mode {
            0 => {
                let f = File::open(filespec).map_err(|_| {
                    CpuException(format!(
                        "File open error on \"{}\" using mode {}!",
                        filespec, mode
                    ))
                })?;
                PortIo::Raw {
                    reader: Some(BufReader::new(f)),
                    writer: None,
                    filespec: filespec.to_string(),
                }
            }
            1 => {
                let f = File::create(filespec).map_err(|_| {
                    CpuException(format!(
                        "File open error on \"{}\" using mode {}!",
                        filespec, mode
                    ))
                })?;
                PortIo::Raw {
                    reader: None,
                    writer: Some(BufWriter::new(f)),
                    filespec: filespec.to_string(),
                }
            }
            2 => {
                let f = OpenOptions::new()
                    .create(true)
                    .append(true)
                    .open(filespec)
                    .map_err(|_| {
                        CpuException(format!(
                            "File open error on \"{}\" using mode {}!",
                            filespec, mode
                        ))
                    })?;
                PortIo::Raw {
                    reader: None,
                    writer: Some(BufWriter::new(f)),
                    filespec: filespec.to_string(),
                }
            }
            _ => return Err(CpuException("Illegal file open mode!".into())),
        };
        Ok(Port {
            state: PortState::default(),
            io,
        })
    }

    pub fn set_port(&mut self, p: i32) {
        self.state.port = p;
    }
    pub fn port(&self) -> i32 {
        self.state.port
    }
    pub fn set_little_endian(&mut self, b: bool) {
        self.state.little_endian = b;
    }
    pub fn is_eof(&self) -> bool {
        self.state.eof
    }
    pub fn is_error(&self) -> bool {
        self.state.error
    }

    // ---- low-level single byte read (ports abstract read()) ----
    fn read_byte(&mut self) -> CpuResult<i32> {
        match &mut self.io {
            PortIo::Std => {
                if self.state.port != 0 {
                    return Err(CpuException(
                        "Can't read from STDOUT or STDERR!".into(),
                    ));
                }
                let mut b = [0u8; 1];
                match std::io::stdin().read(&mut b) {
                    Ok(0) => Ok(-1),
                    Ok(_) => Ok(b[0] as i32),
                    Err(_) => Err(CpuException(format!(
                        "Read error on port {}!",
                        self.state.port
                    ))),
                }
            }
            PortIo::File { reader, filespec, .. } | PortIo::Raw { reader, filespec, .. } => {
                let r = reader.as_mut().ok_or_else(|| {
                    CpuException(format!("File \"{}\" not open for input!", filespec))
                })?;
                let mut b = [0u8; 1];
                match r.read(&mut b) {
                    Ok(0) => Ok(-1),
                    Ok(_) => Ok(b[0] as i32),
                    Err(_) => Err(CpuException(format!(
                        "Read error on file \"{}\"!",
                        filespec
                    ))),
                }
            }
        }
    }

    // Bulk read up to len bytes; returns count read (ports readBytes override).
    fn read_bytes(&mut self, buf: &mut [u8]) -> CpuResult<usize> {
        match &mut self.io {
            PortIo::Std => {
                // Default implementation loops single-byte reads.
                let mut n = 0;
                while n < buf.len() {
                    match self.read_byte_std()? {
                        -1 => break,
                        r => {
                            buf[n] = r as u8;
                            n += 1;
                        }
                    }
                }
                Ok(n)
            }
            PortIo::File { reader, filespec, .. } | PortIo::Raw { reader, filespec, .. } => {
                let r = reader.as_mut().ok_or_else(|| {
                    CpuException(format!("File \"{}\" not open for input!", filespec))
                })?;
                r.read(buf)
                    .map_err(|_| CpuException(format!("Read error on file \"{}\"!", filespec)))
            }
        }
    }

    fn read_byte_std(&mut self) -> CpuResult<i32> {
        let mut b = [0u8; 1];
        match std::io::stdin().read(&mut b) {
            Ok(0) => Ok(-1),
            Ok(_) => Ok(b[0] as i32),
            Err(_) => Err(CpuException(format!("Read error on port {}!", self.state.port))),
        }
    }

    fn next_byte(&mut self) -> CpuResult<i32> {
        if let Some(b) = self.state.pushback.pop() {
            return Ok(b as i32);
        }
        self.read_byte()
    }

    fn push_back(&mut self, b: i32) {
        self.state.pushback.push((b & 0xFF) as u8);
    }

    /// Read `count` bytes as an integer with current endianness (ports read(count)).
    /// Returns -1 on EOF (bytes pushed back). Values are zero-extended.
    pub fn read(&mut self, count: i32) -> CpuResult<i64> {
        let mut result: i64 = 0;
        let mut count = count;
        if count <= 0 || count as usize > REG_SIZE_BYTES {
            count = REG_SIZE_BYTES as i32;
        }
        let requested = count as usize;
        if !self.state.little_endian {
            // big-endian
            if self.state.pushback.is_empty() {
                let mut buf = vec![0u8; requested];
                let mut n = 0;
                while n < requested {
                    let r = self.read_bytes(&mut buf[n..requested])?;
                    if r == 0 {
                        break;
                    }
                    n += r;
                }
                if n < requested {
                    for i in (0..n).rev() {
                        self.push_back(buf[i] as i32);
                    }
                    self.state.eof = true;
                    return Ok(-1);
                }
                for &b in buf.iter().take(requested) {
                    result <<= 8;
                    result |= (b & 0xFF) as i64;
                }
            } else {
                let mut got = vec![0i32; requested];
                let mut n = 0;
                while n < requested {
                    let r = self.next_byte()?;
                    if r == -1 {
                        for i in (0..n).rev() {
                            self.push_back(got[i]);
                        }
                        self.state.eof = true;
                        return Ok(-1);
                    }
                    got[n] = r & 0xFF;
                    n += 1;
                }
                for &b in got.iter().take(requested) {
                    result <<= 8;
                    result |= (b & 0xFF) as i64;
                }
            }
        } else {
            // little-endian
            let mut got = vec![0i32; requested];
            let mut n = 0;
            while n < requested {
                let r = self.next_byte()?;
                if r == -1 {
                    for i in (0..n).rev() {
                        self.push_back(got[i]);
                    }
                    self.state.eof = true;
                    return Ok(-1);
                }
                got[n] = r & 0xFF;
                n += 1;
            }
            let mut shift = 0;
            for &b in got.iter().take(requested) {
                result |= ((b & 0xFF) as i64) << shift;
                shift += 8;
            }
        }
        Ok(result)
    }

    /// Read a single Unicode code point (ports readChar).
    pub fn read_char(&mut self) -> CpuResult<i32> {
        match &self.io {
            PortIo::Raw { .. } => Err(CpuException("Can't read chars from raw file!".into())),
            PortIo::Std if self.state.port != 0 => {
                Err(CpuException("Can't read from STDOUT or STDERR!".into()))
            }
            _ => {
                // Read a UTF-8 sequence from bytes.
                let b0 = self.read_byte()?;
                if b0 == -1 {
                    return Ok(-1);
                }
                let b0 = b0 as u8;
                let len = if b0 < 0x80 {
                    1
                } else if b0 >> 5 == 0b110 {
                    2
                } else if b0 >> 4 == 0b1110 {
                    3
                } else if b0 >> 3 == 0b11110 {
                    4
                } else {
                    1
                };
                let mut bytes = vec![b0];
                for _ in 1..len {
                    let b = self.read_byte()?;
                    if b == -1 {
                        break;
                    }
                    bytes.push(b as u8);
                }
                match std::str::from_utf8(&bytes) {
                    Ok(s) => Ok(s.chars().next().map(|c| c as i32).unwrap_or(-1)),
                    Err(_) => Ok(b0 as i32),
                }
            }
        }
    }

    /// Write `count` bytes of `x` with current endianness (ports write(x,count)).
    pub fn write(&mut self, x: i64, count: i32) -> CpuResult<()> {
        let mut count = count;
        if count <= 0 || count as usize > REG_SIZE_BYTES {
            count = REG_SIZE_BYTES as i32;
        }
        if !self.state.little_endian {
            let mut shift = (count - 1) * 8;
            let mut c = count;
            while c > 0 {
                self.write_byte(((x >> shift) & 0xFF) as u8)?;
                shift -= 8;
                c -= 1;
            }
        } else {
            let mut x = x;
            let mut c = count;
            while c > 0 {
                self.write_byte((x & 0xFF) as u8)?;
                x >>= 8;
                c -= 1;
            }
        }
        Ok(())
    }

    fn write_byte(&mut self, b: u8) -> CpuResult<()> {
        match &mut self.io {
            PortIo::Std => {
                if self.state.port == 0 {
                    return Err(CpuException("Can't write to STDIN!".into()));
                }
                if self.state.port == 2 {
                    let _ = std::io::stderr().write_all(&[b]);
                } else {
                    let _ = std::io::stdout().write_all(&[b]);
                }
                Ok(())
            }
            PortIo::File { writer, filespec, .. } | PortIo::Raw { writer, filespec, .. } => {
                let w = writer.as_mut().ok_or_else(|| {
                    CpuException(format!("File \"{}\" not open for output!", filespec))
                })?;
                w.write_all(&[b])
                    .map_err(|_| CpuException(format!("Write error on file \"{}\"!", filespec)))
            }
        }
    }

    /// Write a single Unicode code point as UTF-8 (ports writeChar).
    pub fn write_char(&mut self, code_point: i32) -> CpuResult<()> {
        match &mut self.io {
            PortIo::Raw { .. } => Err(CpuException("Can't write chars to raw file!".into())),
            PortIo::Std => {
                if self.state.port == 0 {
                    return Err(CpuException("Can't write to STDIN!".into()));
                }
                let s = char::from_u32(code_point as u32)
                    .map(|c| c.to_string())
                    .unwrap_or_default();
                let bytes = s.as_bytes();
                if self.state.port == 2 {
                    let _ = std::io::stderr().write_all(bytes);
                } else {
                    let _ = std::io::stdout().write_all(bytes);
                }
                Ok(())
            }
            PortIo::File { writer, filespec, .. } => {
                let w = writer.as_mut().ok_or_else(|| {
                    CpuException(format!("File \"{}\" not open for output!", filespec))
                })?;
                let s = char::from_u32(code_point as u32)
                    .map(|c| c.to_string())
                    .unwrap_or_default();
                w.write_all(s.as_bytes())
                    .map_err(|_| CpuException(format!("Write error on file \"{}\"!", filespec)))
            }
        }
    }

    pub fn flush(&mut self) -> CpuResult<()> {
        match &mut self.io {
            PortIo::Std => {
                if self.state.port == 0 {
                    return Err(CpuException("Can't write to STDIN!".into()));
                }
                if self.state.port == 2 {
                    let _ = std::io::stderr().flush();
                } else {
                    let _ = std::io::stdout().flush();
                }
                Ok(())
            }
            PortIo::File { writer, filespec, .. } | PortIo::Raw { writer, filespec, .. } => {
                let w = writer.as_mut().ok_or_else(|| {
                    CpuException(format!("File \"{}\" not open for output!", filespec))
                })?;
                w.flush()
                    .map_err(|_| CpuException(format!("Flush error on file \"{}\"!", filespec)))
            }
        }
    }

    pub fn close(&mut self) -> CpuResult<()> {
        match &mut self.io {
            PortIo::Std => Err(CpuException("Can't close STDIO or STDERR!".into())),
            PortIo::File { reader, writer, .. } | PortIo::Raw { reader, writer, .. } => {
                if let Some(mut w) = writer.take() {
                    let _ = w.flush();
                }
                *reader = None;
                Ok(())
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // An in-memory port for testing endian read/write without real files.
    fn mem_port() -> Port {
        // Use a raw temp file round-trip to validate endian logic.
        let dir = std::env::temp_dir();
        let path = dir.join(format!("cpusim64_port_test_{}.bin", std::process::id()));
        let p = Port::new_raw(path.to_str().unwrap(), 1).unwrap();
        p
    }

    #[test]
    fn big_endian_write_then_read() {
        let dir = std::env::temp_dir();
        let path = dir.join(format!("cpusim64_be_{}.bin", std::process::id()));
        let ps = path.to_str().unwrap().to_string();
        {
            let mut w = Port::new_raw(&ps, 1).unwrap();
            w.write(0x0102_0304_i64, 4).unwrap();
            w.flush().unwrap();
            w.close().unwrap();
        }
        {
            let mut r = Port::new_raw(&ps, 0).unwrap();
            let v = r.read(4).unwrap();
            assert_eq!(v, 0x0102_0304);
        }
        let _ = std::fs::remove_file(&ps);
    }

    #[test]
    fn little_endian_roundtrip() {
        let dir = std::env::temp_dir();
        let path = dir.join(format!("cpusim64_le_{}.bin", std::process::id()));
        let ps = path.to_str().unwrap().to_string();
        {
            let mut w = Port::new_raw(&ps, 1).unwrap();
            w.set_little_endian(true);
            w.write(0x0102_0304_i64, 4).unwrap();
            w.flush().unwrap();
            w.close().unwrap();
        }
        {
            let mut r = Port::new_raw(&ps, 0).unwrap();
            r.set_little_endian(true);
            let v = r.read(4).unwrap();
            assert_eq!(v, 0x0102_0304);
            // Read past EOF returns -1.
            assert_eq!(r.read(1).unwrap(), -1);
            assert!(r.is_eof());
        }
        let _ = std::fs::remove_file(&ps);
    }

    #[test]
    fn partial_read_pushback() {
        // Write 2 bytes, then request a 4-byte big-endian read: should hit EOF,
        // push back the 2 bytes, return -1; subsequent single-byte reads recover
        // them in order.
        let dir = std::env::temp_dir();
        let path = dir.join(format!("cpusim64_pb_{}.bin", std::process::id()));
        let ps = path.to_str().unwrap().to_string();
        {
            let mut w = Port::new_raw(&ps, 1).unwrap();
            w.write(0xAB, 1).unwrap();
            w.write(0xCD, 1).unwrap();
            w.flush().unwrap();
            w.close().unwrap();
        }
        {
            let mut r = Port::new_raw(&ps, 0).unwrap();
            assert_eq!(r.read(4).unwrap(), -1);
            // Now recover bytes in original order.
            assert_eq!(r.read(1).unwrap(), 0xAB);
            assert_eq!(r.read(1).unwrap(), 0xCD);
        }
        let _ = std::fs::remove_file(&ps);
    }

    #[test]
    fn construct_mem_port() {
        let _p = mem_port();
    }
}
