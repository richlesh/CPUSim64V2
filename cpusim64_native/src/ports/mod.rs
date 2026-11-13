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
use std::io::{BufReader, BufWriter, Read, Seek, SeekFrom, Write};
use std::sync::{Mutex, OnceLock};

pub const REG_SIZE_BYTES: usize = 8;

/// Process-global console lock. All thread/child writes to STDOUT/STDERR are
/// serialized through this so that a single `printf`/`put_str` line is emitted
/// atomically rather than interleaving byte-by-byte with other threads.
///
/// Java serializes on the shared `System.out`/`System.err` `PrintStream`
/// monitors; we take one lock covering both streams so their relative ordering
/// is also preserved (mirroring the flush-on-port-switch behavior).
fn console_lock() -> &'static Mutex<()> {
    static LOCK: OnceLock<Mutex<()>> = OnceLock::new();
    LOCK.get_or_init(|| Mutex::new(()))
}

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
    /// Random-access file (ports `RandomAccessPortHandler`): read, write and
    /// seek on one port; byte and UTF-8 character I/O may be mixed.
    Random(RandomFile),
}

const RA_BUF_SIZE: usize = 8192;

/// A random-access file with a logical position and small read / write
/// buffers that are invisible to the program.
pub struct RandomFile {
    file: Option<File>,
    filespec: String,
    writable: bool,
    /// Logical position of the next byte.
    pos: u64,
    /// Read buffer holds file bytes [rstart, rstart + rbuf.len()), next at rpos.
    rbuf: Vec<u8>,
    rstart: u64,
    rpos: usize,
    /// Write buffer holds bytes for [wstart, wstart + wbuf.len()).
    wbuf: Vec<u8>,
    wstart: u64,
}

impl RandomFile {
    /// Opens (creating if needed, never truncating) for read/write; falls
    /// back to read-only access for an existing file that is not writable.
    fn open(filespec: &str) -> CpuResult<RandomFile> {
        let err = || CpuException(format!("File open error on \"{}\" for random access!", filespec));
        let path = std::path::Path::new(filespec);
        if path.is_dir() {
            return Err(err());
        }
        let (file, writable) = match OpenOptions::new().read(true).write(true).create(true).open(filespec) {
            Ok(f) => (f, true),
            Err(_) => {
                if !path.is_file() {
                    return Err(err());
                }
                (File::open(filespec).map_err(|_| err())?, false)
            }
        };
        Ok(RandomFile {
            file: Some(file),
            filespec: filespec.to_string(),
            writable,
            pos: 0,
            rbuf: Vec::new(),
            rstart: 0,
            rpos: 0,
            wbuf: Vec::new(),
            wstart: 0,
        })
    }

    fn file(&mut self) -> CpuResult<&mut File> {
        let spec = self.filespec.clone();
        self.file
            .as_mut()
            .ok_or_else(|| CpuException(format!("File \"{}\" is not open!", spec)))
    }

    fn flush_writes(&mut self) -> CpuResult<()> {
        if self.wbuf.is_empty() {
            return Ok(());
        }
        let start = self.wstart;
        let data = std::mem::take(&mut self.wbuf);
        let spec = self.filespec.clone();
        let f = self.file()?;
        f.seek(SeekFrom::Start(start))
            .and_then(|_| f.write_all(&data))
            .map_err(|_| CpuException(format!("Write error on file \"{}\"!", spec)))
    }

    fn drop_read_buffer(&mut self) {
        self.rbuf.clear();
        self.rpos = 0;
    }

    fn length(&mut self) -> CpuResult<u64> {
        let spec = self.filespec.clone();
        let mut len = self
            .file()?
            .metadata()
            .map_err(|_| CpuException(format!("Error reading length of file \"{}\"!", spec)))?
            .len();
        let wend = self.wstart + self.wbuf.len() as u64;
        if !self.wbuf.is_empty() && wend > len {
            len = wend;
        }
        Ok(len)
    }

    fn read_byte(&mut self) -> CpuResult<i32> {
        if self.rpos >= self.rbuf.len() {
            self.flush_writes()?;
            let pos = self.pos;
            let spec = self.filespec.clone();
            let mut buf = vec![0u8; RA_BUF_SIZE];
            let f = self.file()?;
            let n = f
                .seek(SeekFrom::Start(pos))
                .and_then(|_| f.read(&mut buf))
                .map_err(|_| CpuException(format!("Read error on file \"{}\"!", spec)))?;
            buf.truncate(n);
            self.rbuf = buf;
            self.rstart = pos;
            self.rpos = 0;
            if n == 0 {
                return Ok(-1);
            }
        }
        let b = self.rbuf[self.rpos];
        self.rpos += 1;
        self.pos += 1;
        Ok(b as i32)
    }

    /// Steps back over the byte just read (used by the UTF-8 decoder).
    fn unread(&mut self) {
        if self.rpos > 0 {
            self.rpos -= 1;
        } else {
            self.drop_read_buffer();
        }
        self.pos -= 1;
    }

    fn write_byte(&mut self, b: u8) -> CpuResult<()> {
        if self.file.is_none() {
            return Err(CpuException(format!("File \"{}\" is not open!", self.filespec)));
        }
        if !self.writable {
            return Err(CpuException(format!("File \"{}\" is read-only!", self.filespec)));
        }
        self.drop_read_buffer();
        if !self.wbuf.is_empty()
            && (self.wstart + self.wbuf.len() as u64 != self.pos || self.wbuf.len() >= RA_BUF_SIZE)
        {
            self.flush_writes()?;
        }
        if self.wbuf.is_empty() {
            self.wstart = self.pos;
        }
        self.wbuf.push(b);
        self.pos += 1;
        Ok(())
    }

    /// pos >= 0: from the beginning (clamped to the end); pos < 0: from the
    /// end.  Returns the new position or -1.
    fn seek(&mut self, p: i64) -> CpuResult<i64> {
        self.flush_writes()?;
        let len = self.length()? as i64;
        let target = if p >= 0 { p.min(len) } else { len + p };
        if target < 0 {
            return Ok(-1);
        }
        let t = target as u64;
        if !self.rbuf.is_empty() && t >= self.rstart && t < self.rstart + self.rbuf.len() as u64 {
            self.rpos = (t - self.rstart) as usize;
        } else {
            self.drop_read_buffer();
        }
        self.pos = t;
        Ok(target)
    }

    fn close(&mut self) -> CpuResult<()> {
        let r = self.flush_writes();
        self.file = None;
        r
    }
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

    /// Open a random-access file (ports `RandomAccessPortHandler`).
    pub fn new_random(filespec: &str) -> CpuResult<Port> {
        Ok(Port {
            state: PortState::default(),
            io: PortIo::Random(RandomFile::open(filespec)?),
        })
    }

    /// iSEEK: new position, or -1 (also -1 for ports that cannot seek).
    pub fn seek(&mut self, pos: i64) -> CpuResult<i64> {
        match &mut self.io {
            PortIo::Random(rf) => {
                let r = rf.seek(pos)?;
                if r >= 0 {
                    // pushed-back bytes and EOF belong to the old position
                    self.state.pushback.clear();
                    self.state.eof = false;
                }
                Ok(r)
            }
            _ => Ok(-1),
        }
    }

    /// iTELL: current position, or -1 for ports that cannot seek.
    pub fn tell(&mut self) -> CpuResult<i64> {
        match &self.io {
            PortIo::Random(rf) => {
                if rf.file.is_none() {
                    return Err(CpuException(format!("File \"{}\" is not open!", rf.filespec)));
                }
                Ok(rf.pos as i64 - self.state.pushback.len() as i64)
            }
            _ => Ok(-1),
        }
    }

    /// UTF-8 decode one codepoint from a random-access port, honoring bytes
    /// pushed back by a partial multi-byte read.
    fn read_char_random(&mut self) -> CpuResult<i32> {
        let b0 = self.next_byte()?;
        if b0 < 0 {
            return Ok(-1);
        }
        let (len, mut cp) = if b0 < 0x80 {
            return Ok(b0);
        } else if b0 >> 5 == 0x6 {
            (2, b0 & 0x1F)
        } else if b0 >> 4 == 0xE {
            (3, b0 & 0x0F)
        } else if b0 >> 3 == 0x1E {
            (4, b0 & 0x07)
        } else {
            return Ok(b0);
        };
        for _ in 1..len {
            let from_pushback = !self.state.pushback.is_empty();
            let b = self.next_byte()?;
            if b < 0 || (b & 0xC0) != 0x80 {
                if b >= 0 {
                    if from_pushback {
                        self.push_back(b);
                    } else if let PortIo::Random(rf) = &mut self.io {
                        rf.unread();
                    }
                }
                return Ok(b0);
            }
            cp = (cp << 6) | (b & 0x3F);
        }
        Ok(cp)
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
            PortIo::Random(rf) => rf.read_byte(),
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
            PortIo::Random(rf) => {
                let mut n = 0;
                while n < buf.len() {
                    match rf.read_byte()? {
                        -1 => break,
                        b => {
                            buf[n] = b as u8;
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
        if let PortIo::Random(_) = &self.io {
            return self.read_char_random();
        }
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
                let _guard = console_lock().lock().unwrap();
                if self.state.port == 2 {
                    let _ = std::io::stderr().write_all(&[b]);
                } else {
                    let _ = std::io::stdout().write_all(&[b]);
                }
                Ok(())
            }
            PortIo::Random(rf) => {
                if let Err(e) = rf.write_byte(b) {
                    self.state.error = true;
                    return Err(e);
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
        if let PortIo::Random(_) = &self.io {
            let s = char::from_u32(code_point as u32).ok_or_else(|| {
                CpuException(format!("Invalid codepoint {} written to random-access file!", code_point))
            })?;
            let mut b = [0u8; 4];
            for &x in s.encode_utf8(&mut b).as_bytes() {
                self.write_byte(x)?;
            }
            return Ok(());
        }
        match &mut self.io {
            PortIo::Raw { .. } => Err(CpuException("Can't write chars to raw file!".into())),
            PortIo::Random(_) => unreachable!("handled above"),
            PortIo::Std => {
                if self.state.port == 0 {
                    return Err(CpuException("Can't write to STDIN!".into()));
                }
                let s = char::from_u32(code_point as u32)
                    .map(|c| c.to_string())
                    .unwrap_or_default();
                let bytes = s.as_bytes();
                let _guard = console_lock().lock().unwrap();
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

    /// Write a whole string to this port atomically with respect to other
    /// threads (the per-`printf` output unit). For STDOUT/STDERR the entire
    /// UTF-8 encoding is written under the process-global console lock in a
    /// single `write_all`, so concurrent threads never interleave mid-line. For
    /// file ports (per-thread and uncontended in the validation programs) the
    /// bytes are written directly.
    pub fn write_str(&mut self, s: &str) -> CpuResult<()> {
        match &mut self.io {
            PortIo::Std => {
                if self.state.port == 0 {
                    return Err(CpuException("Can't write to STDIN!".into()));
                }
                let bytes = s.as_bytes();
                let _guard = console_lock().lock().unwrap();
                if self.state.port == 2 {
                    let mut err = std::io::stderr().lock();
                    let _ = err.write_all(bytes);
                    let _ = err.flush();
                } else {
                    let mut out = std::io::stdout().lock();
                    let _ = out.write_all(bytes);
                    let _ = out.flush();
                }
                Ok(())
            }
            PortIo::File { writer, filespec, .. } => {
                let w = writer.as_mut().ok_or_else(|| {
                    CpuException(format!("File \"{}\" not open for output!", filespec))
                })?;
                w.write_all(s.as_bytes())
                    .map_err(|_| CpuException(format!("Write error on file \"{}\"!", filespec)))
            }
            PortIo::Raw { .. } => Err(CpuException("Can't write chars to raw file!".into())),
            PortIo::Random(_) => {
                for b in s.as_bytes().to_vec() {
                    self.write_byte(b)?;
                }
                Ok(())
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
            PortIo::Random(rf) => rf.flush_writes(),
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
            PortIo::Random(rf) => rf.close(),
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

    fn ra_path(tag: &str) -> String {
        let dir = std::env::temp_dir();
        let path = dir.join(format!("cpusim64_ra_{}_{}.bin", tag, std::process::id()));
        let _ = std::fs::remove_file(&path);
        path.to_str().unwrap().to_string()
    }

    #[test]
    fn random_access_write_seek_read_tell() {
        let ps = ra_path("basic");
        let mut p = Port::new_random(&ps).unwrap();
        p.write(0x0102_0304_0506_0708, 8).unwrap();
        p.write(b'A' as i64, 1).unwrap();
        p.write(b'B' as i64, 1).unwrap();
        assert_eq!(p.tell().unwrap(), 10);
        assert_eq!(p.seek(0).unwrap(), 0);
        assert_eq!(p.read(8).unwrap(), 0x0102_0304_0506_0708);
        assert_eq!(p.seek(-1).unwrap(), 9);
        assert_eq!(p.read(1).unwrap(), b'B' as i64);
        assert_eq!(p.read(1).unwrap(), -1);
        assert!(p.is_eof());
        // overwrite in place
        assert_eq!(p.seek(2).unwrap(), 2);
        p.write(0xEE, 1).unwrap();
        assert_eq!(p.tell().unwrap(), 3);
        p.seek(1).unwrap();
        assert_eq!(p.read(1).unwrap(), 0x02);
        assert_eq!(p.read(1).unwrap(), 0xEE);
        assert_eq!(p.read(1).unwrap(), 0x04);
        // end, clamping, before the start
        assert_eq!(p.seek(0x1FF_FFFF_FFFF).unwrap(), 10);
        assert_eq!(p.seek(i64::MAX).unwrap(), 10);
        assert_eq!(p.seek(1000).unwrap(), 10);
        assert_eq!(p.seek(-11).unwrap(), -1);
        assert_eq!(p.tell().unwrap(), 10);
        p.write(b'C' as i64, 1).unwrap();
        p.close().unwrap();
        // reopen: not truncated
        let mut q = Port::new_random(&ps).unwrap();
        assert_eq!(q.seek(i64::MAX).unwrap(), 11);
        q.seek(-1).unwrap();
        assert_eq!(q.read(1).unwrap(), b'C' as i64);
        q.close().unwrap();
        let bytes = std::fs::read(&ps).unwrap();
        assert_eq!(bytes, vec![1, 2, 0xEE, 4, 5, 6, 7, 8, b'A', b'B', b'C']);
        let _ = std::fs::remove_file(&ps);
    }

    #[test]
    fn random_access_chars_and_endian() {
        let ps = ra_path("char");
        let mut p = Port::new_random(&ps).unwrap();
        for cp in ['h' as i32, 0xE9, 0x4E16, 0x1F600] {
            p.write_char(cp).unwrap();
        }
        assert_eq!(p.tell().unwrap(), 10);
        p.seek(0).unwrap();
        assert_eq!(p.read_char().unwrap(), 'h' as i32);
        assert_eq!(p.read_char().unwrap(), 0xE9);
        assert_eq!(p.tell().unwrap(), 3);
        assert_eq!(p.read_char().unwrap(), 0x4E16);
        assert_eq!(p.read_char().unwrap(), 0x1F600);
        assert_eq!(p.read_char().unwrap(), -1);
        p.set_little_endian(true);
        p.seek(i64::MAX).unwrap();
        p.write(0x0A0B, 2).unwrap();
        p.seek(-2).unwrap();
        assert_eq!(p.read(1).unwrap(), 0x0B);
        p.seek(-2).unwrap();
        assert_eq!(p.read(2).unwrap(), 0x0A0B);
        p.close().unwrap();
        let bytes = std::fs::read(&ps).unwrap();
        assert_eq!(bytes.len(), 12);
        assert_eq!(std::str::from_utf8(&bytes[..10]).unwrap(), "h\u{e9}\u{4e16}\u{1f600}");
        let _ = std::fs::remove_file(&ps);
    }

    #[test]
    fn random_access_partial_read_tell_and_errors() {
        let ps = ra_path("partial");
        let mut p = Port::new_random(&ps).unwrap();
        p.write(0xABCD, 2).unwrap();
        p.seek(0).unwrap();
        // a 4-byte read at a 2-byte file fails and pushes the bytes back
        assert_eq!(p.read(4).unwrap(), -1);
        assert_eq!(p.tell().unwrap(), 0);
        assert_eq!(p.read(1).unwrap(), 0xAB);
        assert_eq!(p.tell().unwrap(), 1);
        p.close().unwrap();
        assert!(Port::new_random("/").is_err());
        let mut s = Port::new_std();
        assert_eq!(s.seek(0).unwrap(), -1);
        assert_eq!(s.tell().unwrap(), -1);
        let _ = std::fs::remove_file(&ps);
    }
}
