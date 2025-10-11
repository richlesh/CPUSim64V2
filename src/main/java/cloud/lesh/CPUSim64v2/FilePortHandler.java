package cloud.lesh.CPUSim64v2;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.FileWriter;

public class FilePortHandler extends PortHandler
{
	private FileWriter os;
	private BufferedReader is;
	private String filespec;

	public FilePortHandler(Simulator cpu, String filespec, int mode) throws Simulator.CPUException
	{
		super(cpu);
//System.out.println("Opening file "+filespec);
		this.filespec = filespec;
		try {
			switch (mode)
			{
				case 0:
					is=new BufferedReader(new FileReader(filespec));
					break;
				case 1:
					os=new FileWriter(filespec);
					break;
				case 2:
					os=new FileWriter(filespec,true);
					break;
				default:
					throw cpu.new CPUException("Illegal file open mode!");
			}
		}
		catch (Exception e) {
			throw cpu.new CPUException("File open error on \"" + filespec + "\" using mode " + mode + "!");
		}
	}
	
	@Override
	public int read() throws Simulator.CPUException
	{
		if (is == null) throw cpu.new CPUException("File \"" + filespec + "\" not open for input!");
		try {
			return is.read();
		}
		catch (Exception e) {
			throw cpu.new CPUException("Read error on file \"" + filespec + "\"!");
		}
 	}
	
	@Override
	public int readChar() throws Simulator.CPUException
	{
		int codePoint = -1;
		if (is == null) throw cpu.new CPUException("File \"" + filespec + "\" not open for input!");
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
			throw cpu.new CPUException("Read error on file \"" + filespec + "\"!");
		}
		return codePoint;
 	}
	
	@Override
	public void write(byte x) throws Simulator.CPUException
	{
		if (os == null) throw cpu.new CPUException("File \"" + filespec + "\" not open for output!");
		try {
			os.write(x);
		}
		catch (Exception e) {
			throw cpu.new CPUException("Write error on file \"" + filespec + "\"!");
		}
	}
	
	@Override
	public void writeChar(int x) throws Simulator.CPUException
	{
		if (os == null) throw cpu.new CPUException("File \"" + filespec + "\" not open for output!");
		try {
			if (x > 0xffff) {
				StringBuilder sb = new StringBuilder();
				sb.appendCodePoint(x);
				os.write(sb.toString());
			} else os.write(x);
		}
		catch (Exception e) {
			throw cpu.new CPUException("Write error on file \"" + filespec + "\"!");
		}
	}
	
	@Override
	public void flush() throws Simulator.CPUException
	{
		if (os == null) throw cpu.new CPUException("File \"" + filespec + "\" not open for output!");
		try {
			os.flush();
		}
		catch (Exception e) {
			throw cpu.new CPUException("Flush error on file \"" + filespec + "\"!");
		}
	}
	
	@Override
	public void close() throws Simulator.CPUException
	{
		try {
			if (is != null) is.close(); 
			if (os != null) os.close();
			is = null;
			os = null;
		}
		catch (Exception e) {
			throw cpu.new CPUException("Error closing file \"" + filespec + "\"!");
		}
	}
}
