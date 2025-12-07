package cloud.lesh.CPUSim64v2;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public class FilePortHandler extends PortHandler
{
	private FileOutputStream os;
	private FileInputStream fis;
	private InputStreamReader is;
	private String filespec;

	public FilePortHandler(Simulator cpu, int mode, String filespec) throws Simulator.CPUException
	{
		super(cpu);
//System.out.println("Opening file "+filespec);
		this.filespec = filespec;
		try {
			switch (mode)
			{
				case 0:
					fis = new FileInputStream(filespec);
					is = new InputStreamReader(fis, StandardCharsets.UTF_8);
					break;
				case 1:
					os = new FileOutputStream(filespec);
					break;
				case 2:
					os = new FileOutputStream(filespec, true);
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
			return fis.read();
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
	public void writeChar(int codePoint) throws Simulator.CPUException
	{
		if (os == null) throw cpu.new CPUException("File \"" + filespec + "\" not open for output!");
		try {
			String s = new String(Character.toChars(codePoint));
			byte[] utf8 = s.getBytes(StandardCharsets.UTF_8);
			os.write(utf8);
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
