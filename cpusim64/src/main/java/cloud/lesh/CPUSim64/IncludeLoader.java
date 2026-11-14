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

/**
 * Loads {@code #include} files. {@code <path>} is a classpath resource (the
 * system and ADT libraries); {@code "path"} is a file, looked up relative to the
 * directory of the file containing the {@code #include} and then relative to
 * the directories of the files that included that one, up to the main source
 * file's directory.
 */
public class IncludeLoader implements PreprocessorVisitor.IncludeLoader {
	private final Path baseDir;
	private final IncludeLoader parent;

	public IncludeLoader(Path baseDir) {
		this(baseDir, null);
	}

	private IncludeLoader(Path baseDir, IncludeLoader parent) {
		this.baseDir = baseDir;
		this.parent = parent;
	}

	/** The local file for {@code filename}: in this directory or, failing that, an including file's directory. */
	private Path localFile(String filename) {
		for (IncludeLoader l = this; l != null; l = l.parent) {
			Path p = l.baseDir.resolve(filename).normalize();
			if (Files.isRegularFile(p)) return p;
		}
		return baseDir.resolve(filename).normalize();
	}

	@Override
	public String resolve(String filename, boolean isSystemPath) {
		if (isSystemPath) return filename.startsWith("/") ? filename.substring(1) : filename;
		return localFile(filename).toAbsolutePath().normalize().toString();
	}

	@Override
	public PreprocessorVisitor.IncludeLoader child(String filename, boolean isSystemPath) {
		if (isSystemPath) return this;
		Path dir = localFile(filename).toAbsolutePath().getParent();
		if (dir == null || dir.equals(baseDir.toAbsolutePath().normalize())) return this;
		return new IncludeLoader(dir, this);
	}

	public String load(String filename, boolean isSystemPath) {
		if (!isSystemPath) {
			try {
				return Files.readString(localFile(filename));
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
