package cloud.lesh.CPUSim64;

import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public class StdIOPortHandler extends PortHandler
{
	// Ports supported are...
	// 0 STDIN, 1 STDOUT, 2 STDERR
	static int lastPort = 0;
	static InputStreamReader reader = new InputStreamReader(System.in);

	public StdIOPortHandler(Simulator cpu) { super(cpu); }

	// returns -1 on EOF
	@Override
	public int read() throws Simulator.CPUException {
		if (port() != 0) throw cpu.new CPUException("Can't read from STDOUT or STDERR!");
		try {
			return System.in.read();
		}
		catch (Exception e) {
			throw cpu.new CPUException("Read error on port " + port() + "!");
		}
	}

	@Override
	public int readChar() throws Simulator.CPUException {
		if (port()!=0) throw cpu.new CPUException("Can't read from STDOUT or STDERR!");
		int codePoint = -1;
		try {
			int ch1;
			if ((ch1 = reader.read()) != -1) { // read one UTF-16 code unit
				char c1 = (char) ch1;
				if (Character.isHighSurrogate(c1)) {
					int ch2 = reader.read();
					if (ch2 != -1) {
						char c2 = (char) ch2;
						if (Character.isLowSurrogate(c2)) {
							codePoint = Character.toCodePoint(c1, c2);
						} else {
							// malformed sequence
							codePoint = c1;
							reader.reset(); // optional: push back
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
			throw cpu.new CPUException("Read error on STDIN!");
		}
		return codePoint;
	}

	@Override
	public void write(byte x) throws Simulator.CPUException {
		if (port() == 0) throw cpu.new CPUException("Can't write to STDIN!");
		try {
			if (port() != lastPort) {lastPort=port();System.out.flush();System.err.flush();}
//System.err.printf("(%d)",x);
			if (port() == 2) System.err.write(x);
			else System.out.write(x);
		}
		catch (Exception e) {
			throw cpu.new CPUException("Write error on port " + port() + "!");
		}
	}

	@Override
	public void writeChar(int codePoint) throws Simulator.CPUException {
		if (port() == 0) throw cpu.new CPUException("Can't write to STDIN!");
		try {
			if (port() != lastPort) {
				lastPort = port();
				System.out.flush();
				System.err.flush();
			}
	//System.err.printf("{%d}",(int)x);
			if (port() == 2) {
				String s = new String(Character.toChars(codePoint));
				byte[] utf8 = s.getBytes(StandardCharsets.UTF_8);
				System.err.write(utf8);
			} else {
				String s = new String(Character.toChars(codePoint));
				byte[] utf8 = s.getBytes(StandardCharsets.UTF_8);
				System.out.write(utf8);
			}
		}
		catch (Exception e) {
			throw cpu.new CPUException("Write error on port " + port() + "!");
		}
	}

	@Override
	public void flush() throws Simulator.CPUException {
		if (port() == 0) throw cpu.new CPUException("Can't write to STDIN!");
		try {
			if (port() == 2) System.err.flush();
			else System.out.flush();
		}
		catch (Exception e) {
			throw cpu.new CPUException("Flush error on port " + port() + "!");
		}
	}
	@Override
	public void close() throws Simulator.CPUException {throw cpu.new CPUException("Can't close STDIO or STDERR!");}
}
