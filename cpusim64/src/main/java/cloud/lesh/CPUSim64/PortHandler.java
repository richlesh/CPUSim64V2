// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright 2001-2026 Richard Lesh
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package cloud.lesh.CPUSim64;

public abstract class PortHandler implements Cloneable {
	protected Simulator cpu;
	private static final int REG_SIZE_BYTES = 8;
	private int port;
	private boolean littleEndian = false;		// True if big-endian
	private boolean error = false;
	private boolean eof = false;
	// Pushback buffer for bytes that were consumed by a multi-byte read that hit
	// EOF before completing. A short (partial) word read must not lose the bytes
	// it already consumed: they are pushed back here so that subsequent single-
	// byte reads (byte mode) can retrieve them in their original order.
	private int[] pushback = new int[REG_SIZE_BYTES];
	private int pushbackCount = 0;

	public PortHandler(Simulator cpu) { this.cpu = cpu; }
	// returns -1 on EOF
	public abstract int read() throws Simulator.CPUException;
	public abstract int readChar() throws Simulator.CPUException;
	public abstract void write(byte x) throws Simulator.CPUException;
	// Writes Unicode codepoint
	public abstract void writeChar(int x) throws Simulator.CPUException;
	public abstract void flush() throws Simulator.CPUException;
	public abstract void close() throws Simulator.CPUException;
	public boolean isError() { return error; }
	public boolean isEOF() { return eof; }
	public void setLittleEndian(boolean b) { littleEndian = b; }

	public final void setPort(int i){port=i;}
	public final void setPort(long i){port=(int)i;}
	public final int port(){return port;}

	// Return the next byte, drawing from the pushback buffer first (LIFO order,
	// which restores the original stream order for pushed-back partial reads),
	// then falling back to the underlying stream. Returns -1 on EOF.
	private int nextByte() throws Simulator.CPUException {
		if (pushbackCount > 0) {
			return pushback[--pushbackCount];
		}
		return read();
	}

	// Push a byte back so the next nextByte() call returns it.
	private void pushBack(int b) {
		pushback[pushbackCount++] = b & 0xFF;
	}

	// Bulk-read up to `len` bytes into buf[off..off+len). Returns the number of
	// bytes actually read, which may be less than `len` only at end-of-stream.
	// The default implementation loops the single-byte read() so that handlers
	// which cannot bulk-read (e.g. interactive STDIN) still work correctly.
	// Stream-backed handlers override this to delegate to the underlying stream's
	// block read for real speed. This primitive does NOT consult the pushback
	// buffer — callers must drain pushback via nextByte() first.
	protected int readBytes(byte[] buf, int off, int len) throws Simulator.CPUException {
		int n = 0;
		while (n < len) {
			int r = read();
			if (r == -1) break;
			buf[off + n++] = (byte) r;
		}
		return n;
	}

	public PortHandler duplicate(Simulator cpu) {
		PortHandler newPH = null;
		try {
			newPH = (PortHandler) this.clone();
			newPH.cpu = cpu;
			// Give the clone its own pushback buffer.
			newPH.pushback = new int[REG_SIZE_BYTES];
			newPH.pushbackCount = 0;
		} catch (CloneNotSupportedException ex) {
		}
		return newPH;
	}

	public long read(int count) throws Simulator.CPUException
	{
		long result = 0;
		if (count <= 0 || count > REG_SIZE_BYTES) count = REG_SIZE_BYTES;
		int requested = count;
		if (!littleEndian) {		// big-endian
			// Fast path: when there are no pushed-back bytes pending, read the
			// whole word in one bulk operation and compose it big-endian. This
			// avoids `count` separate single-byte stream calls.
			if (pushbackCount == 0) {
				byte[] buf = new byte[requested];
				int n = 0;
				// A stream may return a short (non-EOF) read, so keep going until
				// the word is complete or we hit true end-of-stream.
				while (n < requested) {
					int r = readBytes(buf, n, requested - n);
					if (r <= 0) break;
					n += r;
				}
				if (n < requested) {
					// Partial read: restore the bytes we consumed (in reverse so
					// they come back out in original order), flag EOF, return -1.
					for (int i = n - 1; i >= 0; --i) pushBack(buf[i] & 0xFF);
					eof = true;
					return -1;
				}
				for (int i = 0; i < requested; ++i) {
					result <<= 8;
					result |= buf[i] & 0xFF;
				}
			} else {
				// Slow path: pushed-back bytes are pending from a prior partial
				// read; drain them (and any further bytes) one at a time so their
				// original order is preserved.
				int[] got = new int[requested];
				int n = 0;
				while (n < requested) {
					int r = nextByte();
					if (r == -1) {
						for (int i = n - 1; i >= 0; --i) pushBack(got[i]);
						eof = true;
						return -1;
					}
					got[n++] = r & 0xFF;
				}
				for (int i = 0; i < requested; ++i) {
					result <<= 8;
					result |= got[i] & 0xFF;
				}
			}
		} else {				// little-endian
			int[] got = new int[requested];
			int n = 0;
			while (n < requested) {
				int r = nextByte();
				if (r == -1) {
					for (int i = n - 1; i >= 0; --i) pushBack(got[i]);
					eof = true;
					return -1;
				}
				got[n++] = r & 0xFF;
			}
			int shiftAmount = 0;
			for (int i = 0; i < requested; ++i) {
				result |= (got[i] & 0xFFL) << shiftAmount;
				shiftAmount += 8;
			}
		}
		// NOTE: values are returned zero-extended (unsigned), matching the
		// historical behavior. Sign extension is intentionally NOT applied here:
		// a byte read of 0xFF must remain 255, distinct from the -1 EOF sentinel
		// that callers (e.g. hashcode's byte-mode tail loop) test for.
		return result;
	}

	public short readShort() throws Simulator.CPUException {return (short)read(2);}
	public int readInt() throws Simulator.CPUException {return (int)read(4);}
	public long readLong() throws Simulator.CPUException {return read(8);}
	
	public double readDouble() throws Simulator.CPUException {return Double.longBitsToDouble(readLong());}
	
	public void write(long x, int count) throws Simulator.CPUException
	{
//System.out.println("write("+x+","+count+")");
		if (count <= 0 || count > REG_SIZE_BYTES)
			count = REG_SIZE_BYTES;
		if (!littleEndian) {		// big-endian
			int shiftAmount = (count-1)*8;
			while (count-- > 0) {
				write((byte)((x >> shiftAmount) & 0xff));
				shiftAmount -= 8;
			}
		} else {				// little-endian
			while (count-- > 0) {
				write((byte)(x & 0xff));
				x >>= 8;
			}
		}
	}

	public void write(short x) throws Simulator.CPUException {write(x,2);}
	public void write(int x) throws Simulator.CPUException {write(x,4);}
	public void write(long x) throws Simulator.CPUException {write(x,8);}
	
	public void write(double x) throws Simulator.CPUException {write(Double.doubleToRawLongBits(x),8);}
}

