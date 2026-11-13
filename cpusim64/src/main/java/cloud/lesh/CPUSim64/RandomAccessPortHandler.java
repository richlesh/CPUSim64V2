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

import java.io.File;
import java.io.RandomAccessFile;
import java.nio.charset.StandardCharsets;

/**
 * A port on a random-access file (iOPEN_RANDOM_ACCESS_FILE).
 *
 * The same port can be read and written, and positioned with iSEEK / iTELL.
 * Positions are byte offsets.  Byte-sized IN/OUT (1-8 bytes, honoring the
 * port's endian mode) and character IN/OUT (size CHAR: one UTF-8 encoded
 * codepoint) may be mixed freely.
 *
 * The file is created if it does not exist and is never truncated.  If it
 * exists but cannot be written, it is opened read-only and writes report an
 * error.
 *
 * Reads and writes are buffered; the logical position is tracked here so that
 * the buffers are invisible to the program.
 */
public class RandomAccessPortHandler extends PortHandler
{
	private static final int BUF_SIZE = 8192;

	private RandomAccessFile raf;
	private final String filespec;
	private final boolean writable;
	private long pos;						// logical position of the next byte
	// read buffer: bytes [rstart, rstart + rlen) of the file, next at rpos
	private final byte[] rbuf = new byte[BUF_SIZE];
	private long rstart;
	private int rlen;
	private int rpos;
	// write buffer: bytes for [wstart, wstart + wlen) not yet in the file
	private final byte[] wbuf = new byte[BUF_SIZE];
	private long wstart;
	private int wlen;

	public RandomAccessPortHandler(Simulator cpu, String filespec) throws Simulator.CPUException
	{
		super(cpu);
		this.filespec = filespec;
		RandomAccessFile f = null;
		boolean w = true;
		try {
			File file = new File(filespec);
			if (file.isDirectory()) throw new Exception();
			try {
				f = new RandomAccessFile(file, "rw");
			}
			catch (Exception e) {
				// exists but is not writable: read-only access
				if (!file.isFile()) throw e;
				f = new RandomAccessFile(file, "r");
				w = false;
			}
		}
		catch (Exception e) {
			throw cpu.new CPUException("File open error on \"" + filespec + "\" for random access!");
		}
		raf = f;
		writable = w;
	}

	private void checkOpen() throws Simulator.CPUException {
		if (raf == null) throw cpu.new CPUException("File \"" + filespec + "\" is not open!");
	}

	// Writes the pending output to the file.
	private void flushWrites() throws Simulator.CPUException {
		if (wlen == 0) return;
		try {
			raf.seek(wstart);
			raf.write(wbuf, 0, wlen);
		}
		catch (Exception e) {
			setError();
			throw cpu.new CPUException("Write error on file \"" + filespec + "\"!");
		}
		wlen = 0;
	}

	private void dropReadBuffer() {
		rlen = 0;
		rpos = 0;
	}

	private long length() throws Simulator.CPUException {
		try {
			long len = raf.length();
			if (wlen > 0 && wstart + wlen > len) len = wstart + wlen;
			return len;
		}
		catch (Exception e) {
			throw cpu.new CPUException("Error reading length of file \"" + filespec + "\"!");
		}
	}

	@Override
	public int read() throws Simulator.CPUException
	{
		checkOpen();
		if (rpos >= rlen) {
			flushWrites();
			try {
				raf.seek(pos);
				int n = raf.read(rbuf, 0, BUF_SIZE);
				rstart = pos;
				rlen = n < 0 ? 0 : n;
				rpos = 0;
			}
			catch (Exception e) {
				setError();
				throw cpu.new CPUException("Read error on file \"" + filespec + "\"!");
			}
			if (rlen == 0) return -1;
		}
		++pos;
		return rbuf[rpos++] & 0xFF;
	}

	@Override
	protected int readBytes(byte[] buf, int off, int len) throws Simulator.CPUException
	{
		int n = 0;
		while (n < len) {
			if (rpos < rlen) {
				int k = Math.min(len - n, rlen - rpos);
				System.arraycopy(rbuf, rpos, buf, off + n, k);
				rpos += k;
				pos += k;
				n += k;
			} else {
				int b = read();
				if (b < 0) break;
				buf[off + n++] = (byte) b;
			}
		}
		return n;
	}

	@Override
	public int readChar() throws Simulator.CPUException
	{
		int b0 = readByteWithPushback();
		if (b0 < 0) return -1;
		int len, cp;
		if (b0 < 0x80) return b0;
		else if ((b0 >> 5) == 0x6) { len = 2; cp = b0 & 0x1F; }
		else if ((b0 >> 4) == 0xE) { len = 3; cp = b0 & 0x0F; }
		else if ((b0 >> 3) == 0x1E) { len = 4; cp = b0 & 0x07; }
		else return b0;							// invalid lead byte
		for (int i = 1; i < len; ++i) {
			int b = readByteWithPushback();
			if (b < 0 || (b & 0xC0) != 0x80) {
				// truncated or malformed sequence: return the lead byte; a
				// byte that is not a continuation byte is left unread
				if (b >= 0) unread();
				return b0;
			}
			cp = (cp << 6) | (b & 0x3F);
		}
		return cp;
	}

	// Steps back over the byte just read.
	private void unread() {
		if (pushbackPending() == 0 && rpos > 0) {
			--rpos;
			--pos;
		} else {
			// cannot happen for the UTF-8 decoder (it reads one byte at a
			// time after the lead byte), but keep the position correct
			--pos;
			dropReadBuffer();
		}
	}

	@Override
	public void write(byte x) throws Simulator.CPUException
	{
		checkOpen();
		if (!writable) {
			setError();
			throw cpu.new CPUException("File \"" + filespec + "\" is read-only!");
		}
		// a pushed-back byte or read-ahead is no longer valid once we write
		if (pushbackPending() > 0) {
			pos -= pushbackPending();
			resetReadState();
		}
		dropReadBuffer();
		if (wlen > 0 && (wstart + wlen != pos || wlen == BUF_SIZE)) flushWrites();
		if (wlen == 0) wstart = pos;
		wbuf[wlen++] = x;
		++pos;
	}

	@Override
	public void writeChar(int codePoint) throws Simulator.CPUException
	{
		byte[] utf8;
		try {
			utf8 = new String(Character.toChars(codePoint)).getBytes(StandardCharsets.UTF_8);
		}
		catch (Exception e) {
			throw cpu.new CPUException("Invalid codepoint " + codePoint + " written to \"" + filespec + "\"!");
		}
		for (byte b : utf8) write(b);
	}

	@Override
	public long seek(long p) throws Simulator.CPUException
	{
		checkOpen();
		flushWrites();
		long len = length();
		long target = p >= 0 ? Math.min(p, len) : len + p;
		if (target < 0) return -1;
		resetReadState();
		// keep the read buffer if the target lies inside it
		if (rlen > 0 && target >= rstart && target < rstart + rlen) {
			rpos = (int) (target - rstart);
		} else {
			dropReadBuffer();
		}
		pos = target;
		return pos;
	}

	@Override
	public long tell() throws Simulator.CPUException
	{
		checkOpen();
		return pos - pushbackPending();
	}

	@Override
	public void flush() throws Simulator.CPUException
	{
		checkOpen();
		flushWrites();
	}

	@Override
	public void close() throws Simulator.CPUException
	{
		if (raf == null) return;
		try {
			flushWrites();
		}
		finally {
			try {
				raf.close();
			}
			catch (Exception e) {
				throw cpu.new CPUException("Error closing file \"" + filespec + "\"!");
			}
			finally {
				raf = null;
			}
		}
	}
}
