package cloud.lesh.CPUSim64v2;

import java.io.FileInputStream;
import java.io.FileOutputStream;

public class RawFilePortHandler extends PortHandler
{
	private FileOutputStream os;
	private FileInputStream is;
	private String filespec;

	public RawFilePortHandler(Simulator cpu, String filespec, int mode) throws Simulator.CPUException
	{
		super(cpu);
//System.out.println("Opening file "+filespec);
		this.filespec = filespec;
		try {
		switch (mode)
		{
			case 0:
				is=new FileInputStream(filespec);
				break;
			case 1:
				os=new FileOutputStream(filespec);
				break;
			case 2:
				os=new FileOutputStream(filespec,true);
				break;
			default:
					throw new Exception();
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
		throw cpu.new CPUException("Can't read chars from raw file!");
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
		throw cpu.new CPUException("Can't write chars to raw file!");
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
			if (is!=null) is.close(); 
			if (os!=null) os.close();
			is=null;
			os=null;
		}
		catch (Exception e) {
			throw cpu.new CPUException("Error closing file \"" + filespec + "\"!");
		}
	}
}
