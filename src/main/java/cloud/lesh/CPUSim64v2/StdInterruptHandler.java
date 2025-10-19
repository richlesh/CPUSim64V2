package cloud.lesh.CPUSim64v2;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.file.Files;
import java.text.MessageFormat;
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
	public static final int iMEMMOVE=23;
	public static final int iMEMCLEAR=24;
	public static final int iALLOC_COUNT=25;
	public static final int iFREE_COUNT=26;
	public static final int iALLOC_SIZE=27;
	public static final int iFREE_SIZE=28;

	public static final int iARGC=30;
	public static final int iARGS=31;
	public static final int iEXIT=32;
	public static final int iSYSTEM=33;
	public static final int iGET_PID=34;
	public static final int iGET_NUM_CORES=35;
	public static final int iFORK=36;
	public static final int iWAIT=37;
	public static final int iWAIT_PID=38;
	public static final int iTHREAD=39;
	public static final int iJOIN_THREAD=40;
	public static final int iSLEEP=41;

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
	public static final int iSINH=127;
	public static final int iCOSH=128;
	public static final int iTANH=129;
	public static final int iASINH=130;
	public static final int iACOSH=131;
	public static final int iATANH=132;

	public static final int iPUT_NL=200;
	public static final int iPUT_INT=201;
	public static final int iPUT_DEC=202;
	public static final int iPUT_HEX=203;
	public static final int iPUT_FP=204;
	public static final int iPUTS=205;
	public static final int iPUT_LINE=206;
	public static final int iGET_INT=207;
	public static final int iGET_DEC=208;
	public static final int iGET_HEX=209;
	public static final int iGET_FP=210;
	public static final int iGETS=211;
	public static final int iGET_LINE=212;

	public static final int iPRINTF=213;
	public static final int iCOND_PRINTF=214;

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
	public static final int iFORMAT=309;
	public static final int iTO_LOWER=310;
	public static final int iTO_UPPER=311;
	public static final int iTO_LOWER_STR=312;
	public static final int iTO_UPPER_STR=313;
	public static final int iSTRCMP=314;
	public static final int iSUBSTRING=315;
    public static final int iPREFIX=316;
    public static final int iSUFFIX=317;
    public static final int iCHAR_SEARCH=318;
    public static final int iLAST_CHAR_SEARCH=319;
    public static final int iSUBSTRING_SEARCH=320;
    public static final int iLAST_SUBSTRING_SEARCH=321;
	public static final int iSTRICMP=322;

	public static final int iMATCHES=350;
	public static final int iREPLACE_FIRST=351;
	public static final int iREPLACE_ALL=352;
	public static final int iSPLIT=353;
	public static final int iJOIN=354;
	public static final int iSTRCAT=355;

	private long argv_ptr = 0;

	public StdInterruptHandler(Simulator c) { cpu = c; }

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
				cpu.opSAVE(Simulator.Decoded.decode(cpu.encT0(Opcode.SAVE.code, 2,2,0,0, 0,cpu.GPR_COUNT - 4,0,0)));
				break;
			case iSAVE_FP:							// saves f0-f31 on the stack
				cpu.opSAVE(Simulator.Decoded.decode(cpu.encT0(Opcode.SAVE.code, 3,3,0,0, 0,cpu.FPR_COUNT - 1,0,0)));
				break;
			case iRESTORE:							// restores r0-r28 from the stack
				cpu.opRESTORE(Simulator.Decoded.decode(cpu.encT0(Opcode.RESTORE.code, 2,2,0,0, 0,cpu.GPR_COUNT - 4,0,0)));
				break;
			case iRESTORE_FP:						// restores f0-f31 from the stack
				cpu.opRESTORE(Simulator.Decoded.decode(cpu.encT0(Opcode.RESTORE.code, 3,3,0,0, 0,cpu.FPR_COUNT - 1,0,0)));
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
			case iALLOC_COUNT:						// counts heap allocation blocks
				cpu.setR(0, cpu.countHeapBlocks(true, false));
				break;
			case iFREE_COUNT:						// counts heap free blocks
				cpu.setR(0, cpu.countHeapBlocks(false, true));
				break;
			case iALLOC_SIZE:						// counts heap allocation size
				cpu.setR(0, cpu.countHeapSize(true, false));
				break;
			case iFREE_SIZE:						// counts heap free size
				cpu.setR(0, cpu.countHeapSize(false, true));
				break;
			case iARGC:								// returns the number of command line arguments
				argc = cpu.getCommandLineCount();
				cpu.setR(0,argc);
				break;
			case iARGS:								// returns address of ARGV[r0] in r0 or null
				argc = cpu.getCommandLineCount();
				if (argv_ptr == 0 && argc > 0) {
					argv_ptr = cpu.alloc(argc);
					for (i = 0; i <argc; ++i){
						String arg = cpu.getCommandLineArg(i);
						cpu.memWrite(argv_ptr + i, cpu.allocString(arg));
					}
				}
				v = cpu.getR(0);
				v = (argv_ptr > 0 && v >= 0 && v < argc) ?
						cpu.memRead(argv_ptr + v) : 0;
				cpu.setR(0, v);
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
				cpu.waitPID((int)cpu.getR(0));
				break;
			case iTHREAD:		// r0: function, r1: data block
				cpu.setR(0, cpu.thread(cpu.getR(0), cpu.getR(1)));
				break;
			case iJOIN_THREAD:
				cpu.joinThread((int)cpu.getR(0));
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
				cpu.setFP(0, Math.PI);
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
				cpu.setFP(0, Math.copySign(Math.floor(Math.abs(cpu.getFP(0)) + 0.5), cpu.getFP(0)));
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
			case iSINH:								// sinh of f0 in f0
				cpu.setFP(0,Math.sinh(cpu.getFP(0)));
				break;
			case iCOSH:								// cosh of f0 in f0
				cpu.setFP(0,Math.cosh(cpu.getFP(0)));
				break;
			case iTANH:								// tanh of f0 in f0
				cpu.setFP(0,Math.tanh(cpu.getFP(0)));
				break;
			case iASINH:								// asinh of f0 in f0
				cpu.setFP(0, Utils.asinh(cpu.getFP(0)));
				break;
			case iACOSH:								// acosh of f0 in f0
				cpu.setFP(0, Utils.acosh(cpu.getFP(0)));
				break;
			case iATANH:								// atanh of f0 in f0
				cpu.setFP(0, Utils.atanh(cpu.getFP(0)));
				break;

			case iPUT_NL:							// Send newline to port r0
				ph=cpu.getPortHandler((int)cpu.getR(0));
				if (ph != null) {
					ph.setPort(cpu.getR(0));
					ph.writeChar('\n');		
				}
				break;
			case iPUT_INT:							// Print Int in r1 to port r0 as base in r2
				s = Long.toString(cpu.getR(1), (int)cpu.getR(2));
				ph = cpu.getPortHandler((int)cpu.getR(0));
				if (ph != null) {
					ph.setPort(cpu.getR(0));
					int len = s.length();
					synchronized (ph) {
						for (int pos = 0; pos < len; ++pos)
							ph.writeChar(s.charAt(pos));
					}
				}
				break;
			case iPUT_DEC:							// Print Int in r1 to port r0 as decimal
				s = Long.toString(cpu.getR(1));
				ph = cpu.getPortHandler((int)cpu.getR(0));
				if (ph != null) {
					ph.setPort(cpu.getR(0));
					int len = s.length();
					synchronized (ph) {
						for (int pos = 0; pos < len; ++pos)
							ph.writeChar(s.charAt(pos));
					}
				}
				break;
			case iPUT_HEX:							// Print Int in r1 to port r0 as hex using r2 digits
				s = String.format("%0" + cpu.getR(2) + "X", cpu.getR(1));
				ph = cpu.getPortHandler((int)cpu.getR(0));
				if (ph != null) {
					ph.setPort(cpu.getR(0));
					int len = s.length();
					synchronized (ph) {
						for (int pos = 0; pos < len; ++pos)
							ph.writeChar(s.charAt(pos));
					}
				}
				break;
			case iPUT_FP:							// Print FP in f0 to port r0 using r1 sig digits
				s = String.format("%." + cpu.getR(1) + "f", cpu.getFP(0));
				ph = cpu.getPortHandler((int)cpu.getR(0));
				if (ph != null) {
					ph.setPort(cpu.getR(0));
					int len = s.length();
					synchronized (ph) {
						for (int pos = 0; pos < len; ++pos)
							ph.writeChar(s.charAt(pos));
					}
				}
				break;
			case iPUTS:								// Print String pointed to by r1 to port r0
				ph = cpu.getPortHandler((int)cpu.getR(0));
				if (ph != null) {
					ph.setPort(cpu.getR(0));
					int pos = (int)cpu.getR(1);
					synchronized (ph) {
						s = cpu.convertString(pos);
						s.codePoints().forEach(cp -> {
							ph.writeChar(cp);
						});
					}
				}
				break;
			case iPUT_LINE:
				ph = cpu.getPortHandler((int)cpu.getR(0));
				if (ph != null) {
					ph.setPort(cpu.getR(0));
					int pos = (int)cpu.getR(1);
					synchronized (ph) {
						s = cpu.convertString(pos);
						s.codePoints().forEach(cp -> {
							ph.writeChar(cp);
						});
						ph.writeChar('\n');
					}
				}
				break;
			case iGET_INT:							// Get characters from port r0 and convert to an integer using base in r1
				i = (int)cpu.getR(1);				// get base
				cpu.setR(1, cpu.alloc(65));
				dispatch(iGETS);
				try {
					s = cpu.convertString(cpu.getR(1));
					v = Integer.parseInt(s, i);
				}
				catch (Exception e) {
					throw cpu.new CPUException("Can't convert \"" + s + "\" to an integer base " + i + "!");
				}
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
				ph = cpu.getPortHandler((int)cpu.getR(0));
				if (ph != null) {
					ph.setPort(cpu.getR(0));
					long pos = 0;
					Vector<Integer> codepoints = new Vector<>();
					int cp;
					synchronized (ph) {
						while ((cp = ph.readChar()) != -1) {
							if (Character.isWhitespace(cp)) {
								if (codepoints.isEmpty()) continue;
								else break;
							}
							codepoints.add(cp);
						}
					}
					// Convert to primitive int[] (needed by String constructor)
					int[] cps = codepoints.stream().mapToInt(Integer::intValue).toArray();
					// Construct a String from the code points
					s = new String(cps, 0, cps.length);
					long newAlloc = cpu.allocString(s, cpu.getR(1));
					cpu.setR(1, newAlloc);
					if (cp == -1 && s.isEmpty())
						cpu.setR(0, -1);
					else
						cpu.setR(0, s.length());
				}
				break;
			case iGET_LINE:
				ph = cpu.getPortHandler((int)cpu.getR(0));
				if (ph != null) {
					ph.setPort(cpu.getR(0));
					long pos = 0;
					Vector<Integer> codepoints = new Vector<>();
					int cp;
					synchronized (ph) {
						while ((cp = ph.readChar()) != -1 && cp != '\n') {
							if (cp == '\r') continue;
							codepoints.add(cp);
						}
					}
					// Convert to primitive int[] (needed by String constructor)
					int[] cps = codepoints.stream().mapToInt(Integer::intValue).toArray();
					// Construct a String from the code points
					s = new String(cps, 0, cps.length);
					long newAlloc = cpu.allocString(s, cpu.getR(1));
					cpu.setR(1, newAlloc);
					if (cp == -1 && s.isEmpty())
						cpu.setR(0, -1);
					else
						cpu.setR(0, s.length());
				}
				break;
			case iPRINTF:
				v = cpu.memRead(cpu.getR(Simulator.R_SP) + 3);		// get port
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
				b = cpu.memRead(cpu.getR(Simulator.R_SP) + 3) != 0;		// get condition
				if (b) {
					v = cpu.memRead(cpu.getR(Simulator.R_SP) + 4);		// get port
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
				i = (int)cpu.getR(0);
//System.out.println("Opening port "+i+" for reading!");
				ph = cpu.getPortHandler(i);
				if (ph == null) {
					try {
						cpu.setPortHandler(i, new FilePortHandler(cpu, cpu.convertString(cpu.getR(1)), 0));
						cpu.setR(0, -1);
					} catch (Simulator.CPUException ex) {
						cpu.setR(0, 0);
						break;
					}
				} else
					throw cpu.new CPUException("Port " + i + " already mapped!");
				break;
			case iOPEN_FILE_WRITE:					// Open file on port r0 using filespec pointed to by r1 for write
				i = (int)cpu.getR(0);
//System.out.println("Opening port "+i+" for writing!");
				ph = cpu.getPortHandler(i);
				if (ph == null) {
					try {
						cpu.setPortHandler(i, new FilePortHandler(cpu, cpu.convertString(cpu.getR(1)), 1));
						cpu.setR(0, -1);
					} catch (Simulator.CPUException ex) {
						cpu.setR(0, 0);
						break;
					}
				} else
					throw cpu.new CPUException("Port " + i + " already mapped!");
				break;
			case iOPEN_FILE_APPEND:					// Open file on port r0 using filespec pointed to by r1 for append
				i = (int)cpu.getR(0);
//System.out.println("Opening port "+i+" for appending!");
				ph = cpu.getPortHandler(i);
				if (ph == null) {
					try {
						cpu.setPortHandler(i, new FilePortHandler(cpu, cpu.convertString(cpu.getR(1)), 2));
						cpu.setR(0, -1);
					} catch (Simulator.CPUException ex) {
						cpu.setR(0, 0);
						break;
					}
				} else
					throw cpu.new CPUException("Port " + i + " already mapped!");
				break;
			case iOPEN_RAW_FILE_READ:					// Open file on port r0 using filespec pointed to by r1 for reading
				i = (int)cpu.getR(0);
//System.out.println("Opening port "+i+" for reading!");
				ph = cpu.getPortHandler(i);
				if (ph == null) {
					try {
						cpu.setPortHandler(i, new RawFilePortHandler(cpu, cpu.convertString(cpu.getR(1)),0));
						cpu.setR(0, -1);
					} catch (Simulator.CPUException ex) {
						cpu.setR(0, 0);
						break;
					}
				} else
					throw cpu.new CPUException("Port " + i + " already mapped!");
				break;
			case iOPEN_RAW_FILE_WRITE:					// Open file on port r0 using filespec pointed to by r1 for write
				i = (int)cpu.getR(0);
//System.out.println("Opening port "+i+" for writing!");
				ph = cpu.getPortHandler(i);
				if (ph == null) {
					try {
						cpu.setPortHandler(i,new RawFilePortHandler(cpu, cpu.convertString(cpu.getR(1)),1));
						cpu.setR(0, -1);
					} catch (Simulator.CPUException ex) {
						cpu.setR(0, 0);
						break;
					}
				} else
					throw cpu.new CPUException("Port " + i + " already mapped!");
				break;
			case iOPEN_RAW_FILE_APPEND:					// Open file on port r0 using filespec pointed to by r1 for append
				i = (int)cpu.getR(0);
//System.out.println("Opening port "+i+" for appending!");
				ph = cpu.getPortHandler(i);
				if (ph == null) {
					try {
						cpu.setPortHandler(i,new RawFilePortHandler(cpu, cpu.convertString(cpu.getR(1)),2));
						cpu.setR(0, -1);
					} catch (Simulator.CPUException ex) {
						cpu.setR(0, 0);
						break;
					}
				} else
					throw cpu.new CPUException("Port " + i + " already mapped!");
				break;
			case iCLOSE_FILE:						// Close file on port r0
				ph = cpu.getPortHandler((int)cpu.getR(0));
//System.out.println("Closing port "+cpu.getR(0)+"!");
				if (ph != null) {
					ph.setPort((int)cpu.getR(0));
					ph.close();
					cpu.setPortHandler((int)cpu.getR(0), null);
				} else
					throw cpu.new CPUException("Port " + cpu.getR(0) + " not mapped!");
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
						cpu.memWrite(resultArray + i, cpu.allocString(s + filelist[i]));
					}
					cpu.memWrite(resultArray + filelist.length, 0);
					cpu.setR(0, resultArray);
				}
				break;
			case  iTEMP_DIR:
				try {
					s = cpu.convertString(cpu.getR(0));
					filespec = Files.createTempDirectory(s).toFile();
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
				cpu.setR(0,Long.decode(s));
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
				cpu.setR(0, cpu.allocString(s));
				break;
			case iFORMAT:
				s = format(3);
				cpu.setR(0, cpu.allocString(s));
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
			case iSTRCMP:
				{
					String s1 = cpu.convertString((int) cpu.getR(0));
					String s2 = cpu.convertString((int) cpu.getR(1));
					cpu.setR(0, s1.compareTo(s2));
				}
				break;
			case iSUBSTRING:    // r0 string, r1 start, r2 length
				{
					s = cpu.convertString((int)cpu.getR(0));
					int start = Math.max(0, (int)cpu.getR(1));
					int stop = Math.max(0, (int)(cpu.getR(1) + cpu.getR(2)));
					int len = s.length();
					start = Math.min(len, start);
					stop = Math.min(len, stop);
					String sub = s.substring(start, stop);
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
					i = Math.min(i, s.length());
					i = Math.max(0, i);
					v = s.indexOf(c, i);
					cpu.setR(0,v);
				}
				break;
			case iLAST_CHAR_SEARCH:  // r0 string, r1 search char, r2 last pos or -1
				{
					s = cpu.convertString((int)cpu.getR(0));
					int c = (int)cpu.getR(1);
					i = (int)cpu.getR(2);
					i = Math.min(i, s.length());
					i = Math.max(0, i);
					v = s.lastIndexOf(c, i);
					cpu.setR(0,v);
				}
				break;
			case iSUBSTRING_SEARCH:     // r0 string, r1 search string, r2 first pos or 0
				{
					s = cpu.convertString((int)cpu.getR(0));
					String substr = cpu.convertString((int)cpu.getR(1));
					i = (int)cpu.getR(2);
					i = Math.min(i, s.length());
					i = Math.max(0, i);
					v = s.indexOf(substr, i);
					cpu.setR(0,v);
				}
				break;
			case iLAST_SUBSTRING_SEARCH:  // r0 string, r1 search str, r2 last pos or -1
				{
					s = cpu.convertString((int)cpu.getR(0));
					String substr = cpu.convertString((int)cpu.getR(1));
					i = (int)cpu.getR(2);
					i = Math.min(i, s.length());
					i = Math.max(0, i);
					v = s.lastIndexOf(substr, i);
					cpu.setR(0,v);
				}
				break;
			case iSTRICMP:
			{
				String s1 = cpu.convertString((int) cpu.getR(0));
				String s2 = cpu.convertString((int) cpu.getR(1));
				cpu.setR(0, s1.compareToIgnoreCase(s2));
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
					s = s.replaceFirst(regex, repl);
					cpu.setR(0,cpu.allocString(s));
				}
				break;
			case iREPLACE_ALL:
				{
					s = cpu.convertString(cpu.getR(0));
					String regex = cpu.convertString(cpu.getR(1));
					String repl = cpu.convertString(cpu.getR(2));
					s = s.replaceAll(regex, repl);
					cpu.setR(0,cpu.allocString(s));
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
					for (i = 1; i <= splits.length; ++i) {
						cpu.memWrite(resultArray + i, cpu.allocString(splits[i - 1]));
					}
					cpu.memWrite(resultArray, splits.length);
					cpu.setR(0,resultArray);
				}
				break;
			case iJOIN:			// length prefixed list of strings in r0, delimiter string in r1
				{
					String delim = cpu.convertString(cpu.getR(1));
					int a = (int)cpu.getR(0);
					int len = (int)cpu.memRead(a);
					s = "";
					for (i = 1; i <= len; ++i) {
						if (i != 1)
							s += delim;
						s += cpu.convertString(cpu.memRead(++a));
					}
					cpu.setR(0, cpu.allocString(s));
				}
				break;
			case iSTRCAT:
				{
					String s1 = cpu.convertString(cpu.getR(0));
					String s2 = cpu.convertString(cpu.getR(1));
					s = s1 + s2;
					cpu.setR(0, cpu.allocString(s, cpu.getR(2)));
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
		String fmt = cpu.convertString(cpu.memRead(cpu.getR(Simulator.R_SP) + msgOffset));
		Vector<Object> args = new Vector<>();
		Pattern rx = Pattern.compile("%[-+0 ]?\\d*.?\\d*([cdxefgs])", Pattern.CASE_INSENSITIVE);
		Matcher m = rx.matcher(fmt);
		int numArgs = 0;
		while (m.find()) {
			switch (m.group(1).toLowerCase()) {
				case "c":
					args.add((int)cpu.memRead(cpu.getR(Simulator.R_SP) + numArgs + msgOffset + 1));
					break;
				case "s":
					args.add(cpu.convertString(cpu.memRead(cpu.getR(Simulator.R_SP) + numArgs + msgOffset + 1)));
					break;
				case "e":
				case "f":
				case "g":
					args.add(Double.longBitsToDouble(cpu.memRead(cpu.getR(Simulator.R_SP) + numArgs + msgOffset + 1)));
					break;
				default:
					args.add(cpu.memRead(cpu.getR(Simulator.R_SP) + numArgs + msgOffset + 1));
					break;
			}
			++numArgs;
		}
		s = String.format(fmt, args.toArray());
		return s;
	}

	// Args must all be strings.
	public String format(int msgOffset) throws Simulator.CPUException {
		String s;
		String fmt = cpu.convertString(cpu.memRead(cpu.getR(Simulator.R_SP) + msgOffset));
		Vector<Object> args = new Vector<>();
		Pattern rx = Pattern.compile("\\{([0-9]*)\\}", Pattern.CASE_INSENSITIVE);
		Matcher m = rx.matcher(fmt);
		int numArgs = 0;
		int maxArgs = 0;
		while (m.find()) {
			if (m.group(1).isEmpty()) {
				++numArgs;
			} else {
				int i = Integer.parseInt(m.group(1));
				maxArgs = Math.max(maxArgs, i + 1);
			}
		}
		numArgs = Math.max(numArgs, maxArgs);
		for (int i = 0; i < numArgs; ++i) {
			args.add(cpu.convertString(cpu.memRead(cpu.getR(Simulator.R_SP) + i + msgOffset + 1)));
		}
		s = MessageFormat.format(numberPlaceholders(fmt), args.toArray());
		return s;
	}

	public static String numberPlaceholders(String template) {
		StringBuilder result = new StringBuilder();
		int count = 0;
		for (int i = 0; i < template.length(); i++) {
			char c = template.charAt(i);
			if (c == '{' && i + 1 < template.length() && template.charAt(i + 1) == '}') {
				result.append('{').append(count++).append('}');
				i++; // skip the '}'
			} else {
				result.append(c);
			}
		}
		return result.toString();
	}
}
