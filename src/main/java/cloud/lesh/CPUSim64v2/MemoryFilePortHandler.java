package cloud.lesh.CPUSim64v2;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;

public class MemoryFilePortHandler extends PortHandler {
	private ByteArrayOutputStream os;
	private ByteArrayInputStream is;

	public MemoryFilePortHandler(Simulator cpu, int mode) throws Simulator.CPUException {
		this(cpu, mode, new byte[0]);
	}

	public MemoryFilePortHandler(Simulator cpu, int mode, byte[] data) throws Simulator.CPUException {
		super(cpu);
		try {
			switch (mode)
			{
				case 0: // Read
					is=new ByteArrayInputStream(data);
					break;
				case 1: // Create
					os=new ByteArrayOutputStream();
					break;
				case 2: // Append
					os=new ByteArrayOutputStream();
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
			return is.read();
		}
		catch (Exception e) {
			throw cpu.new CPUException("Read error on Memory file!");
		}
	}

	@Override
	public int readChar() throws Simulator.CPUException
	{
		if (is == null) throw cpu.new CPUException("Memory file not open for input!");
		try {
			return is.read();
		}
		catch (Exception e) {
			throw cpu.new CPUException("Read error on Memory file!");
		}
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
	public void writeChar(int x) throws Simulator.CPUException
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
			if (is!=null) is.close();
			if (os!=null) os.close();
			is=null;
			os=null;
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
