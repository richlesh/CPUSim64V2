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

import java.io.*;
import java.nio.file.Path;
import java.util.List;

import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;

public final class AsmIO {
	private AsmIO() {}

	public static void writeU64BE(Path out, List<Long> words) throws IOException {
		try (FileOutputStream fos = new FileOutputStream(out.toFile());
			 GZIPOutputStream gos = new GZIPOutputStream(fos);
			 DataOutputStream dos = new DataOutputStream(gos)) {

			for (long w : words) {
				dos.writeLong(w);  // always big endian
			}
			dos.flush(); // flush DataOutputStream before GZIP finishes
		}
	}

	public static List<Long> readU64BE(File in) throws IOException {
		List<Long> words = new java.util.ArrayList<>();
		try (FileInputStream fis = new FileInputStream(in);
			 GZIPInputStream gis = new GZIPInputStream(fis);
			 DataInputStream dis = new DataInputStream(gis)) {
			try {
				while (true) {
					long w = dis.readLong();
					words.add(w);
				}
			} catch (EOFException e) {
				// end of file reached
			}
		}
		return words;
	}

	public static long mask(int bits) {
		return bits == 64 ? -1L : ((1L << bits) - 1);
	}

	public static long sex(long val, int bits) { // sign-extend "bits" -> 64
		long m = 1L << (bits - 1);
		return (val ^ m) - m;
	}
}
