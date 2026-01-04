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

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public class MemoryFilePortHandler extends PortHandler {
	private ByteArrayOutputStream os;
	private ByteArrayInputStream bis;
	private InputStreamReader is;

	public MemoryFilePortHandler(Simulator cpu, int mode) throws Simulator.CPUException {
		this(cpu, mode, new byte[0]);
	}

	public MemoryFilePortHandler(Simulator cpu, int mode, byte[] data) throws Simulator.CPUException {
		super(cpu);
		try {
			switch (mode)
			{
				case 0: // Read
					bis = new ByteArrayInputStream(data);
					is = new InputStreamReader(bis, StandardCharsets.UTF_8);
					break;
				case 1: // Create
					os = new ByteArrayOutputStream();
					break;
				case 2: // Append
					os = new ByteArrayOutputStream();
					break;
				default:
					throw new Exception();
			}
		}
		catch (Exception e) {
			throw cpu.new CPUException("Memory file open error using mode " + mode + "!");
		}
	}

	@Override
	public int read() throws Simulator.CPUException
	{
		if (is == null) throw cpu.new CPUException("Memory file not open for input!");
		try {
			return bis.read();
		}
		catch (Exception e) {
			throw cpu.new CPUException("Read error on Memory file!");
		}
	}

	@Override
	public int readChar() throws Simulator.CPUException
	{
		int codePoint = -1;
		if (is == null) throw cpu.new CPUException("Memory file not open for input!");
		try {
			int ch1;
			if ((ch1 = is.read()) != -1) { // read one UTF-16 code unit
				char c1 = (char) ch1;
				if (Character.isHighSurrogate(c1)) {
					int ch2 = is.read();
					if (ch2 != -1) {
						char c2 = (char) ch2;
						if (Character.isLowSurrogate(c2)) {
							codePoint = Character.toCodePoint(c1, c2);
						} else {
							// malformed sequence
							codePoint = c1;
							is.reset(); // optional: push back
						}
					} else {
						codePoint = c1; // last char was high surrogate with no pair
					}
				} else {
					codePoint = c1;
				}
			}
		}
		catch (Exception e) {
			throw cpu.new CPUException("Read error on Memory file!");
		}
		return codePoint;
	}

	@Override
	public void write(byte x) throws Simulator.CPUException
	{
		if (os == null) throw cpu.new CPUException("Memory file not open for output!");
		try {
			os.write(x);
		}
		catch (Exception e) {
			throw cpu.new CPUException("Write error on Memory file!");
		}
	}

	@Override
	public void writeChar(int codePoint) throws Simulator.CPUException
	{
		if (os == null) throw cpu.new CPUException("Memory file not open for output!");
		try {
			StringBuilder sb = new StringBuilder();
			sb.appendCodePoint(codePoint);
			os.write(sb.toString().getBytes(StandardCharsets.UTF_8));
		}
		catch (Exception e) {
			throw cpu.new CPUException("Write error on Memory file!");
		}
	}

	@Override
	public void flush() throws Simulator.CPUException
	{
		if (os == null) throw cpu.new CPUException("Memory file not open for output!");
		try {
			os.flush();
		}
		catch (Exception e) {
			throw cpu.new CPUException("Flush error on Memory file!");
		}
	}

	@Override
	public void close() throws Simulator.CPUException
	{
		try {
			if (is != null) is.close();
			if (os != null) os.close();
		}
		catch (Exception e) {
			throw cpu.new CPUException("Error closing Memory file!");
		}
	}

	@Override
	public String toString() {
		return os.toString(StandardCharsets.UTF_8);
	}

	public byte[] toBytes() {
		return os.toByteArray();
	}
}
