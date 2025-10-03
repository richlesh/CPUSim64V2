package cloud.lesh.CPUSim64v2;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class IncludeLoader implements PreprocessorVisitor.IncludeLoader {
	private final Path baseDir;
	private final PreprocessorVisitor visitor = null;

	public IncludeLoader(Path baseDir) {
		this.baseDir = baseDir;
	}

	public String load(String filename, boolean isSystemPath, PreprocessorVisitor visitor) {
		if (!isSystemPath) {
			try {
				Path file = baseDir.resolve(filename).normalize();
				return Files.readString(file);
			} catch (IOException e) {
				throw new IllegalArgumentException(visitor.getLocation() + ": Failed to include local file " + filename, e);
			}
		}
		// Try to load from classpath
		String resourceName = filename.startsWith("/") ? filename.substring(1) : filename;
		try (InputStream in = getClass().getClassLoader().getResourceAsStream(resourceName)) {
			if (in == null) {
				throw new IllegalArgumentException(visitor.getLocation() + ": System include not found on classpath: " + filename);
			}
			return new String(in.readAllBytes(), StandardCharsets.UTF_8);
		} catch (IOException e) {
			throw new IllegalArgumentException(visitor.getLocation() + ": Failed to read system include " + filename, e);
		}
	}
}
