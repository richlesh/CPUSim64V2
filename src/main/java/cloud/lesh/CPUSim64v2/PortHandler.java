package cloud.lesh.CPUSim64v2;

public abstract class PortHandler implements Cloneable {
	protected Simulator cpu;
	private static final int REG_SIZE_BYTES = 8;
	private int port;
	private boolean littleEndian = false;		// True if big-endian
	private boolean error = false;
	private boolean eof = false;

	public PortHandler(Simulator cpu) { this.cpu = cpu; }
	// returns -1 on EOF
	public abstract int read() throws Simulator.CPUException;
	public abstract int readChar() throws Simulator.CPUException;
	public abstract void write(byte x) throws Simulator.CPUException;
	// Writes Unicode codepoint
	public abstract void writeChar(int x) throws Simulator.CPUException;
	public abstract void flush() throws Simulator.CPUException;
	public abstract void close() throws Simulator.CPUException;
	public boolean isError() { return error; }
	public boolean isEOF() { return eof; }
	public void setEndian(boolean b) { littleEndian = b; }

	public final void setPort(int i){port=i;}
	public final void setPort(long i){port=(int)i;}
	public final int port(){return port;}

	public PortHandler duplicate(Simulator cpu) {
		PortHandler newPH = null;
		try {
			newPH = (PortHandler) this.clone();
			newPH.cpu = cpu;
		} catch (CloneNotSupportedException ex) {
		}
		return newPH;
	}

	public long read(int count) throws Simulator.CPUException
	{
//System.out.println("read("+count+")");
		long result=0;
		if (count<=0 || count>REG_SIZE_BYTES) count=REG_SIZE_BYTES;
		if (!littleEndian) {		// big-endian
			while (count-- > 0) {
				result <<= 8;
				int r = read();
				if (r == -1) { eof = true; return -1; }
				result |= r & 0xFF;
//System.out.println(result);
			}
		} else {				// little-endian
			int shiftAmount = 0;
			while (count-- > 0) {
				result |= (read() & 0xFFL) << shiftAmount;
				shiftAmount += 8;
			}
		}
		switch (count) {
			case 1:
				result = Simulator.signExtend(result, 8);
				break;
			case 2:
				result = Simulator.signExtend(result, 16);
				break;
			case 4:
				result = Simulator.signExtend(result, 32);
				break;
			default:
				break;
		}
		return result;
	}

	public short readShort() throws Simulator.CPUException {return (short)read(2);}
	public int readInt() throws Simulator.CPUException {return (int)read(4);}
	public long readLong() throws Simulator.CPUException {return read(8);}
	
	public double readDouble() throws Simulator.CPUException {return Double.longBitsToDouble(readLong());}
	
	public void write(long x, int count) throws Simulator.CPUException
	{
//System.out.println("write("+x+","+count+")");
		if (count <= 0 || count > REG_SIZE_BYTES)
			count = REG_SIZE_BYTES;
		if (!littleEndian) {		// big-endian
			int shiftAmount=(count-1)*8;
			while (count-- > 0) {
				write((byte)((x>>shiftAmount)&0xff));
				shiftAmount-=8;
			}
		} else {				// little-endian
			while (count-- > 0) {
				write((byte)(x & 0xff));
				x >>= 8;
			}
		}
	}

	public void write(short x) throws Simulator.CPUException {write(x,2);}
	public void write(int x) throws Simulator.CPUException {write(x,4);}
	public void write(long x) throws Simulator.CPUException {write(x,8);}
	
	public void write(double x) throws Simulator.CPUException {write(Double.doubleToRawLongBits(x),8);}
}

