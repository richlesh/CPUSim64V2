package cloud.lesh.CPUSim64v2;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.file.Files;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class StdInterruptHandler extends InterruptHandler
{
	private Simulator cpu;

	public static final int iINT_MIN=1;
	public static final int iINT_MAX=2;
	public static final int iFLOAT_MIN=3;
	public static final int iFLOAT_LOWEST=4;
	public static final int iFLOAT_MAX=5;
	public static final int iNEGATIVE_INFINITY=6;
	public static final int iPOSITIVE_INFINITY=7;
	public static final int iNAN=8;

	public static final int iCYCLES=10;
	public static final int iCLOCK=11;
	public static final int iSAVE=12;
	public static final int iSAVE_FP=13;
	public static final int iRESTORE=14;
	public static final int iRESTORE_FP=15;
	public static final int iPrintCPUState=16;

	public static final int iALLOC=20;
	public static final int iREALLOC=21;
	public static final int iFREE=22;
	public static final int iARGS=23;
	public static final int iARGC=24;
	public static final int iMEMMOVE=25;
	public static final int iMEMCLEAR=26;

	public static final int iEXIT=30;
	public static final int iSYSTEM=31;
	public static final int iFORK=32;
	public static final int iWAIT=33;
	public static final int iWAIT_PID=34;
	public static final int iTHREAD=35;
	public static final int iJOIN_THREAD=36;
	public static final int iSLEEP=37;
	public static final int iGET_PID=38;
	public static final int iGET_NUM_CORES=39;

	public static final int iPI=100;
	public static final int iE=101;		
	public static final int iABS_FP=102;
	public static final int iABS=103;
	public static final int iCEIL=104;
	public static final int iFLOOR=105;
	public static final int iROUND=106;
	public static final int iSQRT=107;
	public static final int iEXP=108;
	public static final int iLOG=109;
	public static final int iPOW=110;
	public static final int iREMAINDER=111;
	public static final int iMAX_FP=112;
	public static final int iMIN_FP=113;
	public static final int iMAX=114;
	public static final int iMIN=115;
	public static final int iRANDOM=116;
	public static final int iRAND=117;
	public static final int iTO_DEGREES=118;
	public static final int iTO_RADIANS=119;
	public static final int iSIN=120;
	public static final int iCOS=121;
	public static final int iTAN=122;
	public static final int iASIN=123;
	public static final int iACOS=124;
	public static final int iATAN=125;
	public static final int iATAN2=126;
	
	public static final int iPUT_NL=200;
	public static final int iPUT_INT=201;
	public static final int iPUT_DEC=202;
	public static final int iPUT_HEX=203;
	public static final int iPUT_FP=204;
	public static final int iPUTS=205;
	public static final int iGET_INT=206;
	public static final int iGET_DEC=207;
	public static final int iGET_HEX=208;
	public static final int iGET_FP=209;
	public static final int iGETS=210;
	public static final int iPRINTF=211;
	public static final int iCOND_PRINTF=212;
	
	public static final int iOPEN_FILE_READ=220;
	public static final int iOPEN_FILE_WRITE=221;
	public static final int iOPEN_FILE_APPEND=222;
	public static final int iOPEN_RAW_FILE_READ=223;
	public static final int iOPEN_RAW_FILE_WRITE=224;
	public static final int iOPEN_RAW_FILE_APPEND=225;
	public static final int iCLOSE_FILE=226;
	public static final int iFLUSH=227;
	public static final int iDELETE_FILE=228;
	public static final int iMAKE_DIR=229;
	public static final int iDELETE_DIR=230;
	public static final int iIS_DIR=231;
	public static final int iIS_FILE=232;
	public static final int iFILE_EXISTS=233;
	public static final int iFILES=234;
	public static final int iTEMP_DIR=235;
	public static final int iTEMP_FILE=236;
	
	public static final int iFMT_DEC=301;
	public static final int iFMT_HEX=302;
	public static final int iFMT_FLOAT=303;
	public static final int iPARSE_INT=304;
	public static final int iPARSE_DEC=305;
	public static final int iPARSE_HEX=306;
	public static final int iPARSE_FLOAT=307;
	public static final int iSPRINTF=308;
	public static final int iTO_LOWER=309;
	public static final int iTO_UPPER=310;
	public static final int iTO_LOWER_STR=311;
	public static final int iTO_UPPER_STR=312;
    public static final int iSUBSTRING=313;
    public static final int iPREFIX=314;
    public static final int iSUFFIX=315;
    public static final int iCHAR_SEARCH=316;
    public static final int iLAST_CHAR_SEARCH=317;
    public static final int iSUBSTRING_SEARCH=318;
    public static final int iLAST_SUBSTRING_SEARCH=319;
	
	public static final int iMATCHES=350;
	public static final int iREPLACE_FIRST=351;
	public static final int iREPLACE_ALL=352;
	public static final int iSPLIT=353;
	public static final int iJOIN=354;

	private long argv_ptr = 0;

	public StdInterruptHandler(Simulator c) { cpu=c; }

	@Override
	public boolean dispatch(int id) throws Simulator.CPUException
	{
		boolean result = true;
		boolean b;
		String s = "";
		int i;			// int/long for 32/64-bit
		long v;
		double f;
		int argc;
		PortHandler ph;
		File filespec;
		
//System.out.println("Interrupt: "+id);
		switch (id) {
			case iINT_MIN:							// returns largest negative integer
				cpu.setR(0,Long.MIN_VALUE);
				break;
			case iINT_MAX:							// returns largest positive integer
				cpu.setR(0,Long.MAX_VALUE);
				break;
			case iFLOAT_MIN:						// returns largest negative integer
				cpu.setFP(0,-Double.MAX_VALUE);
				break;
		    case iFLOAT_LOWEST:                     // returns smallest FP value greater than 0
		        cpu.setFP(0, Double.MIN_VALUE);
		        break;
			case iFLOAT_MAX:						// returns largest FP value
				cpu.setFP(0,Double.MAX_VALUE);
				break;
			case iNEGATIVE_INFINITY:				// returns negative infinity value
				cpu.setFP(0,Double.NEGATIVE_INFINITY);
				break;
			case iPOSITIVE_INFINITY:				// returns positive infinity value
				cpu.setFP(0,Double.POSITIVE_INFINITY);
				break;
			case iNAN:								// returns Not A Number value
				cpu.setFP(0,Double.NaN);
				break;
			case iCYCLES:							// returns CPU time in cycles in r0
				cpu.setR(0,cpu.getCycles());
				break;
			case iCLOCK:							// returns current time in ns in r0
				cpu.setR(0,cpu.getClock());
				break;
			case iSAVE:								// saves r0-r28 on the stack
				for (i=0;i<cpu.GPR_COUNT;++i) cpu.push(cpu.getR(i));
				break;
			case iSAVE_FP:							// saves f0-f31 on the stack
				for (i=0;i<cpu.FPR_COUNT;++i) cpu.fpush(cpu.getFP(i));
				break;
			case iRESTORE:							// restores r0-r28 from the stack
				for (i=cpu.GPR_COUNT-1;i>=0;--i) cpu.setR(i,cpu.pop());
				break;
			case iRESTORE_FP:						// restores f0-f31 from the stack
				for (i=cpu.FPR_COUNT-1;i>=0;--i) cpu.setFP(i,cpu.fpop());
				break;
			case iPrintCPUState:
				cpu.printCPUState();
				break;
			case iALLOC:							// returns pointer in r0 to allocated array of r0 words
													// returns 0 on failure
													// Block structure prev:next:size:data....
				cpu.setR(0,cpu.alloc((int)cpu.getR(0)));
				break;
			case iREALLOC:							// array in r0 is reallocated to size of r1 words
													// Data is copied from r0 then r0 is freed
													// returns new array in r0 or 0 on failure
				cpu.setR(0,cpu.realloc((int)cpu.getR(0),(int)cpu.getR(1)));
				break;
			case iFREE:								// frees allocation in r0
				cpu.free((int)cpu.getR(0));
				break;
			case iARGC:								// returns the number of command line arguments
				argc=cpu.getCommandLineCount();
				cpu.setR(0,argc);
				break;
			case iARGS:								// returns address of ARGV[r0] in r0 or null
				argc=cpu.getCommandLineCount();
				if (argv_ptr == 0 && argc>0) {
					argv_ptr = cpu.alloc(argc);
					for (i=0;i<argc;++i){
						String arg = cpu.getCommandLineArg(i);
						cpu.store(argv_ptr+i,cpu.allocString(arg));
					}
				}
				v = cpu.getR(0);
				v = (argv_ptr>0 && v>=0 && v<argc) ? 
					cpu.load(argv_ptr+v) : 0;
				cpu.setR(0,v);
				break;
			case iMEMMOVE:
				{
					long dest = cpu.getR(0);
					long src = cpu.getR(1);
					long count = cpu.getR(2);
					cpu.memmove(dest, src, count);
				}
				break;
			case iMEMCLEAR:
				{
					long dest = cpu.getR(0);
					long count = cpu.getR(1);
					cpu.memclear((int)dest, (int)count);
				}
				break;
			case iEXIT:								// exits process with return code in r0
				cpu.exit((int)cpu.getR(0));
				break;
			case iSYSTEM:
				s = cpu.convertString(cpu.getR(0));
				try {
					Runtime run = Runtime.getRuntime();
					Process pr = run.exec(s);
					pr.waitFor();
					BufferedReader buf = new BufferedReader(new InputStreamReader(pr.getInputStream()));
					while ((s=buf.readLine())!=null) {
						System.out.println(s);
					}
					cpu.setR(0, pr.exitValue());
				}
				catch (Exception ex) {
					throw cpu.new CPUException(ex.getMessage());
				}
				break;
			case iFORK:
				cpu.setR(0, cpu.fork());
				break;
			case iWAIT:
				cpu.waitAll();
				break;
			case iWAIT_PID:
				{
					Simulator childCPU = Simulator.getChildCPU((int)cpu.getR(0));
					if (childCPU != null) {
	//					CPU.ChildProcess p = childCPU.getProcess();
	//					if (p != null)
	//						p.join();
					}
				}
				break;
			case iTHREAD:		// r0: function, r1: data block
				cpu.setR(0, cpu.thread(cpu.getR(0), cpu.getR(1)));
				break;
			case iJOIN_THREAD:
				{
//					Simulator childCPU = CPU.getChildCPU((int)cpu.getR(0));
//					if (childCPU != null) {
//						CPU.ChildThread t = childCPU.getThread();
//						if (t != null) {
//							try {
//								t.join();
//							} catch (InterruptedException e) {
//							}
//						}
//					}
				}
				break;
			case iSLEEP:
				try {
					Thread.sleep(cpu.getR(0));
				} catch (InterruptedException e) {
				}
				break;
			case iGET_PID:
				cpu.setR(0, cpu.getPID());
				break;
			case iGET_NUM_CORES:
				cpu.setR(0, Runtime.getRuntime().availableProcessors());
				break;
// Math
			case iPI:								// returns PI in f0
				cpu.setFP(0,Math.PI);
				break;
			case iE:								// returns e in f0
				cpu.setFP(0,Math.E);
				break;
			case iABS_FP:							// abs of f0 in f0
				cpu.setFP(0,Math.abs(cpu.getFP(0)));
				break;
			case iABS:								// abs of r0 in r0
				cpu.setR(0,Math.abs(cpu.getR(0)));
				break;
			case iCEIL:								// ceil of f0 in f0
				cpu.setFP(0,Math.ceil(cpu.getFP(0)));
				break;
			case iFLOOR:							// floor of f0 in f0
				cpu.setFP(0,Math.floor(cpu.getFP(0)));
				break;
			case iROUND:							// round of f0 in f0
				cpu.setFP(0,Math.round(cpu.getFP(0)));
				break;
			case iSQRT:								// sqrt of f0 in f0
				cpu.setFP(0,Math.sqrt(cpu.getFP(0)));
				break;
			case iEXP:								// exp of f0 in f0
				cpu.setFP(0,Math.exp(cpu.getFP(0)));
				break;
			case iLOG:								// ln of f0 in f0
				cpu.setFP(0,Math.log(cpu.getFP(0)));
				break;
			case iPOW:								// pow of f0^f1 in f0
				cpu.setFP(0,Math.pow(cpu.getFP(0),cpu.getFP(1)));
				break;
			case iREMAINDER:						// remainder of f0/f1 in f0
				cpu.setFP(0,Math.IEEEremainder(cpu.getFP(0),cpu.getFP(1)));
				break;
			case iMAX_FP:							// max of f0 or f1 in f0
				cpu.setFP(0,Math.max(cpu.getFP(0),cpu.getFP(1)));
				break;
			case iMIN_FP:							// min of f0 or f1 in f0
				cpu.setFP(0,Math.min(cpu.getFP(0),cpu.getFP(1)));
				break;
			case iMAX:								// max of r0 or r1 in r0
				cpu.setR(0,Math.max(cpu.getR(0),cpu.getR(1)));
				break;
			case iMIN:								// min of r0 or r1 in r0
				cpu.setR(0,Math.min(cpu.getR(0),cpu.getR(1)));
				break;
			case iRANDOM:							// random [0,1) in f0
				cpu.setFP(0,Math.random());
				break;
			case iRAND:								// random [r1,r2] in r0
				cpu.setR(0,(long)Math.floor(Math.random()*(cpu.getR(1)-cpu.getR(0)+1))+cpu.getR(0));
//Utils.debug("iRAND("+cpu.getR(1)+","+cpu.getR(2)+")="+cpu.getR(0));
				break;
			case iTO_DEGREES:						// toDegrees of f0 in f0
				cpu.setFP(0,Math.toDegrees(cpu.getFP(0)));
				break;
			case iTO_RADIANS:						// toRadians of f0 in f0
				cpu.setFP(0,Math.toRadians(cpu.getFP(0)));
				break;
			case iSIN:								// sin of f0 in f0
				cpu.setFP(0,Math.sin(cpu.getFP(0)));
				break;
			case iCOS:								// cos of f0 in f0
				cpu.setFP(0,Math.cos(cpu.getFP(0)));
				break;
			case iTAN:								// tan of f0 in f0
				cpu.setFP(0,Math.tan(cpu.getFP(0)));
				break;
			case iASIN:								// asin of f0 in f0
				cpu.setFP(0,Math.asin(cpu.getFP(0)));
				break;
			case iACOS:								// acos of f0 in f0
				cpu.setFP(0,Math.acos(cpu.getFP(0)));
				break;
			case iATAN:								// atan of f0 in f0
				cpu.setFP(0,Math.atan(cpu.getFP(0)));
				break;
			case iATAN2:							// atan2 of f0/f1 in f0
				cpu.setFP(0,Math.atan2(cpu.getFP(0),cpu.getFP(1)));
				break;
// IO
			case iPUT_NL:							// Send newline to port r0
				ph=cpu.getPortHandler((int)cpu.getR(0));
				if (ph!=null) {
					ph.setPort(cpu.getR(0));
					ph.writeChar('\n');		
				}
				break;
			case iPUT_INT:							// Print Int in r1 to port r0 as base in r2
				s=Long.toString(cpu.getR(1),(int)cpu.getR(2));
				ph=cpu.getPortHandler((int)cpu.getR(0));
				if (ph!=null)
				{
					ph.setPort(cpu.getR(0));
					int len=s.length();
					synchronized (ph) {
						for (int pos=0;pos<len;++pos)
							ph.writeChar(s.charAt(pos));
					}
				}
				break;
			case iPUT_DEC:							// Print Int in r1 to port r0 as decimal
				s=Long.toString(cpu.getR(1));
				ph=cpu.getPortHandler((int)cpu.getR(0));
				if (ph!=null) {
					ph.setPort(cpu.getR(0));
					int len=s.length();
					synchronized (ph) {
						for (int pos=0;pos<len;++pos)
							ph.writeChar(s.charAt(pos));
					}
				}
				break;
			case iPUT_HEX:							// Print Int in r1 to port r0 as hex using r2 digits
				s = String.format("%0" + cpu.getR(2) + "X", cpu.getR(1));
				ph=cpu.getPortHandler((int)cpu.getR(0));
				if (ph!=null) {
					ph.setPort(cpu.getR(0));
					int len=s.length();
					synchronized (ph) {
						for (int pos=0;pos<len;++pos)
							ph.writeChar(s.charAt(pos));
					}
				}
				break;
			case iPUT_FP:							// Print FP in f0 to port r0 using r1 sig digits
				s = String.format("%." + cpu.getR(1) + "f", cpu.getFP(0));
				ph=cpu.getPortHandler((int)cpu.getR(0));
				if (ph!=null) {
					ph.setPort(cpu.getR(0));
					int len=s.length();
					synchronized (ph) {
						for (int pos=0;pos<len;++pos)
							ph.writeChar(s.charAt(pos));
					}
				}
				break;
			case iPUTS:								// Print String pointed to by r1 to port r0
				ph=cpu.getPortHandler((int)cpu.getR(0));
				if (ph!=null)
				{
					ph.setPort(cpu.getR(0));
					int pos = (int)cpu.getR(1);
					char c;
					synchronized (ph) {
						while ((c=(char)cpu.load(pos))!=0){
							ph.writeChar(c);
							++pos;
						}
					}
				}
				break;
			case iGET_INT:							// Get characters from port r0 and convert to an integer using base in r1
				i = (int)cpu.getR(1);				// get base
				cpu.setR(1,cpu.alloc(65));
				dispatch(iGETS);
				try {
					s = cpu.convertString(cpu.getR(1));
					v = Integer.parseInt(s, i);
				}
				catch (Exception e) {throw cpu.new CPUException("Can't convert \""+s+"\" to an integer base " + i + "!");}
				cpu.free((int)cpu.getR(1));
				cpu.setR(0,v);
				break;
			case iGET_DEC:							// Get characters from port r0 and convert to a decimal integer in r0
				cpu.setR(1,10);
				dispatch(iGET_INT);
				break;
			case iGET_HEX:							// Get characters from port r0 and convert to a hexdecimal integer in r0
				cpu.setR(1,16);
				dispatch(iGET_INT);
				break;
			case iGET_FP:							// Get characters from port r0 and convert to a floating point number in f0
				cpu.setR(1,cpu.alloc(65));
				dispatch(iGETS);
				try {
					f = Double.parseDouble(s=cpu.convertString(cpu.getR(1)));
				}
				catch (Exception e) {throw cpu.new CPUException("Can't convert \""+s+"\" to a floating point!");}
				cpu.free((int)cpu.getR(1));
				cpu.setFP(0,f);
				break;
			case iGETS:								// Get characters from port r0 and put into an alloc buffer at r1
													// r1 will change if buffer is reallocated
				ph=cpu.getPortHandler((int)cpu.getR(0));
				if (ph!=null) {
					ph.setPort(cpu.getR(0));
					long pos = 0;
					long bufSize = cpu.load(cpu.getR(1)-1)-1;
					int c;
					while ((c=ph.read())!=-1 && c!='\n' && c!='\r') {
						cpu.store(cpu.getR(1)+(pos++),c);
						if (pos>bufSize) {
							cpu.setR(1,cpu.realloc((int)cpu.getR(1),(int)(bufSize+32+1)));
							bufSize = cpu.load(cpu.getR(1)-1)-1;
						}
					}
		            if (c == '\r') c=ph.read();  // should be a '\n'
					cpu.store(cpu.getR(1)+pos,0);	// null byte
//Utils.debug("iGETS: \""+cpu.convertString(cpu.getR(1))+"\"");
				}
				break;
			case iPRINTF:
				v = cpu.load(cpu.getR(Simulator.R_SP) + 3);		// get port
				ph=cpu.getPortHandler((int)v);
				if (ph!=null) {
					ph.setPort(v);
					s = sprintf(4);
					int len=s.length();
					synchronized (ph) {
						for (int pos=0;pos<len;++pos)
							ph.writeChar(s.charAt(pos));
					}
				}
				break;
			case iCOND_PRINTF:
				b = cpu.load(cpu.getR(Simulator.R_SP) + 3) != 0;		// get condition
				if (b) {
					v = cpu.load(cpu.getR(Simulator.R_SP) + 4);		// get port
					ph=cpu.getPortHandler((int)v);
					if (ph!=null) {
						ph.setPort(v);
						s = sprintf(5);
						int len=s.length();
						synchronized (ph) {
							for (int pos=0;pos<len;++pos)
								ph.writeChar(s.charAt(pos));
						}
					}
				}
				break;
			case iOPEN_FILE_READ:					// Open file on port r0 using filespec pointed to by r1 for reading
				i=(int)cpu.getR(0);
//System.out.println("Opening port "+i+" for reading!");
				ph=cpu.getPortHandler(i);
				if (ph==null) {
					cpu.setPortHandler(i,new FilePortHandler(cpu, cpu.convertString(cpu.getR(1)),0));
				}
				else
					throw cpu.new CPUException("Port "+i+" already mapped!");
				break;
			case iOPEN_FILE_WRITE:					// Open file on port r0 using filespec pointed to by r1 for write
				i=(int)cpu.getR(0);
//System.out.println("Opening port "+i+" for writing!");
				ph=cpu.getPortHandler(i);
				if (ph==null) {
					cpu.setPortHandler(i,new FilePortHandler(cpu, cpu.convertString(cpu.getR(1)),1));
				} else
					throw cpu.new CPUException("Port "+i+" already mapped!");
				break;
			case iOPEN_FILE_APPEND:					// Open file on port r0 using filespec pointed to by r1 for append
				i=(int)cpu.getR(0);
//System.out.println("Opening port "+i+" for appending!");
				ph=cpu.getPortHandler(i);
				if (ph==null) {
					cpu.setPortHandler(i,new FilePortHandler(cpu, cpu.convertString(cpu.getR(1)),2));
				} else
					throw cpu.new CPUException("Port "+i+" already mapped!");
				break;
			case iOPEN_RAW_FILE_READ:					// Open file on port r0 using filespec pointed to by r1 for reading
				i=(int)cpu.getR(0);
//System.out.println("Opening port "+i+" for reading!");
				ph=cpu.getPortHandler(i);
				if (ph==null) {
					cpu.setPortHandler(i,new RawFilePortHandler(cpu, cpu.convertString(cpu.getR(1)),0));
				} else
					throw cpu.new CPUException("Port "+i+" already mapped!");
				break;
			case iOPEN_RAW_FILE_WRITE:					// Open file on port r0 using filespec pointed to by r1 for write
				i=(int)cpu.getR(0);
//System.out.println("Opening port "+i+" for writing!");
				ph=cpu.getPortHandler(i);
				if (ph==null) {
					cpu.setPortHandler(i,new RawFilePortHandler(cpu, cpu.convertString(cpu.getR(1)),1));
				} else
					throw cpu.new CPUException("Port "+i+" already mapped!");
				break;
			case iOPEN_RAW_FILE_APPEND:					// Open file on port r0 using filespec pointed to by r1 for append
				i=(int)cpu.getR(0);
//System.out.println("Opening port "+i+" for appending!");
				ph=cpu.getPortHandler(i);
				if (ph==null) {
					cpu.setPortHandler(i,new RawFilePortHandler(cpu, cpu.convertString(cpu.getR(1)),2));
				} else
					throw cpu.new CPUException("Port "+i+" already mapped!");
				break;
			case iCLOSE_FILE:						// Close file on port r0
				ph=cpu.getPortHandler((int)cpu.getR(0));
//System.out.println("Closing port "+cpu.getR(0)+"!");
				if (ph!=null) {
					ph.setPort((int)cpu.getR(0));
					ph.close();
					cpu.setPortHandler((int)cpu.getR(0),null);
				} else
					throw cpu.new CPUException("Port "+cpu.getR(0)+" not mapped!");
				break;
			case iFLUSH:							// Flush file on port r0
				ph=cpu.getPortHandler((int)cpu.getR(0));
				if (ph!=null) {
					ph.setPort(cpu.getR(0));
					ph.flush();
				} else
					throw cpu.new CPUException("Port "+cpu.getR(0)+" not mapped!");
				break;
			case  iDELETE_FILE:
				s = cpu.convertString(cpu.getR(0));
				filespec = new File(s);
				b = false;
				if (filespec.isFile()) {
					b = filespec.delete();
				}
				cpu.setR(0, b ? -1 : 0);
				break;
			case  iMAKE_DIR:
				s = cpu.convertString(cpu.getR(0));
				filespec = new File(s);
				b = filespec.mkdir();
				cpu.setR(0, b ? -1 : 0);
				break;
			case  iDELETE_DIR:
				s = cpu.convertString(cpu.getR(0));
				filespec = new File(s);
				b = false;
				if (filespec.isDirectory()) {
					b = filespec.delete();
				}
				cpu.setR(0, b ? -1 : 0);
				break;
			case  iIS_DIR:
				s = cpu.convertString(cpu.getR(0));
				filespec = new File(s);
				cpu.setR(0, filespec.isDirectory() ? -1 : 0);
				break;
			case  iIS_FILE:
				s = cpu.convertString(cpu.getR(0));
				filespec = new File(s);
				cpu.setR(0, filespec.isFile() ? -1 : 0);
				break;
			case  iFILE_EXISTS:
				s = cpu.convertString(cpu.getR(0));
				filespec = new File(s);
				cpu.setR(0, filespec.exists() ? -1 : 0);
				break;
			case  iFILES:
				s = cpu.convertString(cpu.getR(0));
				if (!s.endsWith("/")) s += "/";
				filespec = new File(s);
				{
					String[] filelist = filespec.list();
					long resultArray = cpu.alloc(filelist.length + 1);
					for (i = 0; i < filelist.length; ++i) {
						cpu.store(resultArray + i, cpu.allocString(s + filelist[i]));
					}
					cpu.store(resultArray + filelist.length, 0);
					cpu.setR(0, resultArray);
				}
				break;
			case  iTEMP_DIR:
				try {
					filespec = Files.createTempDirectory("").toFile();
					cpu.setR(0, cpu.allocString(filespec.getAbsolutePath()));
				} catch (IOException ex) {
					cpu.setR(0, 0);
				}
				break;
			case  iTEMP_FILE:
				s = cpu.convertString(cpu.getR(1));
				try {
					filespec = File.createTempFile(cpu.convertString(cpu.getR(0)), s);
					cpu.setR(0, cpu.allocString(filespec.getAbsolutePath()));
				} catch (IOException ex) {
					cpu.setR(0, 0);
				}
				break;
// Strings and Formatting
			case iFMT_DEC:
				s = Long.toString(cpu.getR(0));
				cpu.setR(0,cpu.allocString(s));
				break;
			case iFMT_HEX:				// format Int in r0 as hex using r1 digits
				s = String.format("%0" + cpu.getR(1) + "X", cpu.getR(0));
				cpu.setR(0,cpu.allocString(s));
				break;
			case iFMT_FLOAT:			// format f0 using r0 sig digits
				s = String.format("%." + cpu.getR(0) + "f", cpu.getFP(0));
				cpu.setR(0,cpu.allocString(s));
				break;
			case iPARSE_INT:
				s = cpu.convertString(cpu.getR(0));
				cpu.setR(0,Long.parseLong(s));
				break;
			case iPARSE_DEC:
				s = cpu.convertString(cpu.getR(0));
				cpu.setR(0,Long.parseLong(s,10));
				break;
			case iPARSE_HEX:
				s = cpu.convertString(cpu.getR(0));
				cpu.setR(0,Long.parseLong(s,16));
				break;
			case iPARSE_FLOAT:
				s = cpu.convertString(cpu.getR(0));
				cpu.setFP(0,Double.parseDouble(s));
				break;
			case iSPRINTF:
				s = sprintf(3);
				cpu.setR(0,cpu.allocString(s));
				break;
			case iTO_LOWER:
				v = cpu.getR(0);
				v = Character.toLowerCase((int)v);
				cpu.setR(0,v);
				break;
			case iTO_UPPER:
				v = cpu.getR(0);
				v = Character.toUpperCase((int)v);
				cpu.setR(0,v);
				break;
			case iTO_LOWER_STR:
				s = cpu.convertString(cpu.getR(0));
				s = s.toLowerCase();
				cpu.setR(0,cpu.allocString(s));
				break;
			case iTO_UPPER_STR:
				s = cpu.convertString(cpu.getR(0));
				s = s.toUpperCase();
				cpu.setR(0,cpu.allocString(s));
				break;
			case iSUBSTRING:    // r0 string, r1 start, r2 length
				{
					s = cpu.convertString((int)cpu.getR(0));
					String sub = s.substring((int)cpu.getR(1), (int)(cpu.getR(1) + cpu.getR(2)));
					cpu.setR(0, cpu.allocString(sub));
				}
				break;
			case iPREFIX:    // r0 String, r1 length
				{
					s = cpu.convertString((int)cpu.getR(0));
					String sub = s.substring(0, (int)cpu.getR(1));
					cpu.setR(0, cpu.allocString(sub));
				}
				break;
			case iSUFFIX:    // r0 String, r1 length
				{
					s = cpu.convertString((int)cpu.getR(0));
					String sub = s.substring(s.length() - (int)cpu.getR(1), s.length());
					cpu.setR(0, cpu.allocString(sub));
				}
				break;
		    case iCHAR_SEARCH:      // r0 string, r1 search char, r2 first pos or 0
				{
					s = cpu.convertString((int)cpu.getR(0));
					int c = (int)cpu.getR(1);
					i = (int)cpu.getR(2);
					v = s.indexOf(c, i);
					cpu.setR(0,v);
				}
				break;
			case iLAST_CHAR_SEARCH:  // r0 string, r1 search char, r2 last pos or -1
				{
					s = cpu.convertString((int)cpu.getR(0));
					int c = (int)cpu.getR(1);
					i = (int)cpu.getR(2);
					v = s.lastIndexOf(c, i);
					cpu.setR(0,v);
				}
				break;
			case iSUBSTRING_SEARCH:     // r0 string, r1 search string, r2 first pos or 0
				{
					s = cpu.convertString((int)cpu.getR(0));
					String substr = cpu.convertString((int)cpu.getR(1));
					i = (int)cpu.getR(2);
					v = s.indexOf(substr, i);
					cpu.setR(0,v);
				}
				break;
			case iLAST_SUBSTRING_SEARCH:  // r0 string, r1 search str, r2 last pos or -1
				{
					s = cpu.convertString((int)cpu.getR(0));
					String substr = cpu.convertString((int)cpu.getR(1));
					i = (int)cpu.getR(2);
					v = s.lastIndexOf(substr, i);
					cpu.setR(0,v);
				}
				break;
			case iMATCHES:		// matches portion unlike Java matches() which matches whole string as in "^regex$"
				{
					s = cpu.convertString(cpu.getR(0));
					String regex = cpu.convertString(cpu.getR(1));
					Pattern p = Pattern.compile(regex);
					Matcher m = p.matcher(s);
					cpu.setR(0,m.find()?-1:0);
				}
				break;
			case iREPLACE_FIRST	:
				{
					s = cpu.convertString(cpu.getR(0));
					String regex = cpu.convertString(cpu.getR(1));
					String repl = cpu.convertString(cpu.getR(2));
					cpu.setR(0,cpu.allocString(s.replaceFirst(regex, repl)));
				}
				break;
			case iREPLACE_ALL:
				{
					s = cpu.convertString(cpu.getR(0));
					String regex = cpu.convertString(cpu.getR(1));
					String repl = cpu.convertString(cpu.getR(2));
					cpu.setR(0,cpu.allocString(s.replaceAll(regex, repl)));
				}
				break;
			case iSPLIT:		// String r0, regex r1, returns null terminated array in r0
				{
					s = cpu.convertString(cpu.getR(0));
					String regex = cpu.convertString(cpu.getR(1));
					int limit = (int)cpu.getR(2);
					String[] splits;
					if (limit == -1) {
						splits = s.split(regex);
					} else {
						splits = s.split(regex, limit);
					}
					long resultArray = cpu.alloc(splits.length + 1);
					for (i = 0; i < splits.length; ++i) {
						cpu.store(resultArray + i, cpu.allocString(splits[i]));
					}
					cpu.store(resultArray + splits.length, 0L);
					cpu.setR(0,resultArray);
				}
				break;
			case iJOIN:			// null termintated list of strings in r0, delimiter string in r1
				{
					String delim = cpu.convertString(cpu.getR(1));
					int a = (int)cpu.getR(0);
					boolean first = true;
					v = cpu.load(a);
					s = "";
					while (v != 0) {
						if (!first)
							s += delim;
						else
							first = false;
						s += cpu.convertString(v);
						v = cpu.load(++a);
					}
					cpu.setR(0, cpu.allocString(s));
				}
				break;
			default:
				result=(superHandler!=null)?superHandler.dispatch(id):false;
				break;
		}
		return result;
	}
	
	public String sprintf(int msgOffset) throws Simulator.CPUException {
		String s;
		String fmt = cpu.convertString(cpu.load(cpu.getR(Simulator.R_SP) + msgOffset));
		Vector<Object> args = new Vector<>();
		Pattern rx = Pattern.compile("%[-+0 ]?\\d*.?\\d*([cdxefgs])", Pattern.CASE_INSENSITIVE);
		Matcher m = rx.matcher(fmt);
		int numArgs = 0;
		while (m.find()) {
			switch (m.group(1).toLowerCase()) {
			case "c":
				args.add((int)cpu.load(cpu.getR(Simulator.R_SP) + numArgs + msgOffset + 1));
				break;
			case "s":
				args.add(cpu.convertString(cpu.load(cpu.getR(Simulator.R_SP) + numArgs + msgOffset + 1)));
				break;
			case "e":
			case "f":
			case "g":
				args.add(Double.longBitsToDouble(cpu.load(cpu.getR(Simulator.R_SP) + numArgs + msgOffset + 1)));
				break;
			default:
				args.add(cpu.load(cpu.getR(Simulator.R_SP) + numArgs + msgOffset + 1));
				break;
			}
			++numArgs;
		}
		s = String.format(fmt, args.toArray());
		return s;
	}
}
