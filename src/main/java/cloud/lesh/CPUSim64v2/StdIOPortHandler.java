package cloud.lesh.CPUSim64v2;

import java.io.InputStreamReader;

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
		try {
			return reader.read();
		}
		catch (Exception e) {
			throw cpu.new CPUException("Read error on port " + port() + "!");
		}
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
	public void writeChar(int x) throws Simulator.CPUException {
		if (port() == 0) throw cpu.new CPUException("Can't write to STDIN!");
		try {
			if (port() != lastPort) {lastPort=port();System.out.flush();System.err.flush();}
	//System.err.printf("{%d}",(int)x);
			if (port() == 2) {
				if (x > 0xffff) {
					StringBuilder sb = new StringBuilder();
					sb.appendCodePoint(x);
					System.err.print(sb.toString());
				}else System.err.print((char)x);
			} else {
				if (x > 0xffff) {
					StringBuilder sb = new StringBuilder();
					sb.appendCodePoint(x);
					System.out.print(sb.toString());
				}else System.out.print((char)x);
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
