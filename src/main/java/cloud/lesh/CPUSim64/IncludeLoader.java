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

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class IncludeLoader implements PreprocessorVisitor.IncludeLoader {
	private final Path baseDir;

	public IncludeLoader(Path baseDir) {
		this.baseDir = baseDir;
	}

	public String load(String filename, boolean isSystemPath) {
		if (!isSystemPath) {
			try {
				Path file = baseDir.resolve(filename).normalize();
				return Files.readString(file);
			} catch (IOException e) {
				throw new IllegalArgumentException("Failed to include local file " + filename, e);
			}
		}
		// Try to load from classpath
		String resourceName = filename.startsWith("/") ? filename.substring(1) : filename;
		try (InputStream in = getClass().getClassLoader().getResourceAsStream(resourceName)) {
			if (in == null) {
				throw new IllegalArgumentException("System include not found on classpath: " + filename);
			}
			return new String(in.readAllBytes(), StandardCharsets.UTF_8);
		} catch (IOException e) {
			throw new IllegalArgumentException("Failed to read system include " + filename, e);
		}
	}
}
