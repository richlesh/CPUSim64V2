//! Simulator-side port handler registry and the IN/OUT glue (ports the port
//! bits of `Simulator`: `getPortHandler`/`setPortHandler` and the `opIN`/`opOUT`
//! byte-vs-word dispatch).

use super::{CpuException, CpuResult, Simulator};
use crate::ports::Port;

impl Simulator {
    pub fn get_port_handler_mut(&mut self, port: i32) -> Option<&mut Port> {
        if let Some(ph) = self.ports.get_mut(&port) {
            ph.set_port(port);
            Some(ph)
        } else {
            None
        }
    }

    pub fn has_port(&self, port: i32) -> bool {
        self.ports.contains_key(&port)
    }

    pub fn set_port_handler(&mut self, port: i32, ph: Option<Port>) {
        match ph {
            Some(p) => {
                self.ports.insert(port, p);
            }
            None => {
                self.ports.remove(&port);
            }
        }
    }

    /// IN: read `bytes` from `port` (0 => one code point, else multi-byte int).
    pub fn port_in(&mut self, port: i32, bytes: i32) -> CpuResult<i64> {
        let ph = self
            .get_port_handler_mut(port)
            .ok_or_else(|| CpuException(format!("IN: no handler registered for port {}", port)))?;
        if bytes == 0 {
            Ok(ph.read_char()? as i64)
        } else {
            ph.read(bytes)
        }
    }

    /// OUT: write `bytes` of `val` to `port` (0 => one code point).
    pub fn port_out(&mut self, port: i32, val: i64, bytes: i32) -> CpuResult<()> {
        let ph = self
            .get_port_handler_mut(port)
            .ok_or_else(|| CpuException(format!("OUT: no handler registered for port {}", port)))?;
        if bytes == 0 {
            ph.write_char(val as i32)
        } else {
            ph.write(val, bytes)
        }
    }

    pub fn port_set_little_endian(&mut self, port: i32, little: bool) {
        if let Some(ph) = self.ports.get_mut(&port) {
            ph.set_little_endian(little);
        }
    }
}
