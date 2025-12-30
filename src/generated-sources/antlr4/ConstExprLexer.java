// Generated from ConstExpr.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64;
import org.antlr.v4.runtime.Lexer;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.Token;
import org.antlr.v4.runtime.TokenStream;
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.atn.*;
import org.antlr.v4.runtime.dfa.DFA;
import org.antlr.v4.runtime.misc.*;

@SuppressWarnings({"all", "warnings", "unchecked", "unused", "cast", "CheckReturnValue", "this-escape"})
public class ConstExprLexer extends Lexer {
	static { RuntimeMetaData.checkVersion("4.13.2", RuntimeMetaData.VERSION); }

	protected static final DFA[] _decisionToDFA;
	protected static final PredictionContextCache _sharedContextCache =
		new PredictionContextCache();
	public static final int
		T__0=1, T__1=2, T__2=3, T__3=4, T__4=5, T__5=6, CHAR=7, STRING=8, HEXINT=9, 
		INT=10, FLOAT=11, WS=12, OTHER=13;
	public static String[] channelNames = {
		"DEFAULT_TOKEN_CHANNEL", "HIDDEN"
	};

	public static String[] modeNames = {
		"DEFAULT_MODE"
	};

	private static String[] makeRuleNames() {
		return new String[] {
			"T__0", "T__1", "T__2", "T__3", "T__4", "T__5", "CHAR", "STRING", "HEXINT", 
			"INT", "FLOAT", "WS", "ESC", "DIGITS", "HEX", "OTHER"
		};
	}
	public static final String[] ruleNames = makeRuleNames();

	private static String[] makeLiteralNames() {
		return new String[] {
			null, "'+'", "'-'", "'*'", "'/'", "'('", "')'"
		};
	}
	private static final String[] _LITERAL_NAMES = makeLiteralNames();
	private static String[] makeSymbolicNames() {
		return new String[] {
			null, null, null, null, null, null, null, "CHAR", "STRING", "HEXINT", 
			"INT", "FLOAT", "WS", "OTHER"
		};
	}
	private static final String[] _SYMBOLIC_NAMES = makeSymbolicNames();
	public static final Vocabulary VOCABULARY = new VocabularyImpl(_LITERAL_NAMES, _SYMBOLIC_NAMES);

	/**
	 * @deprecated Use {@link #VOCABULARY} instead.
	 */
	@Deprecated
	public static final String[] tokenNames;
	static {
		tokenNames = new String[_SYMBOLIC_NAMES.length];
		for (int i = 0; i < tokenNames.length; i++) {
			tokenNames[i] = VOCABULARY.getLiteralName(i);
			if (tokenNames[i] == null) {
				tokenNames[i] = VOCABULARY.getSymbolicName(i);
			}

			if (tokenNames[i] == null) {
				tokenNames[i] = "<INVALID>";
			}
		}
	}

	@Override
	@Deprecated
	public String[] getTokenNames() {
		return tokenNames;
	}

	@Override

	public Vocabulary getVocabulary() {
		return VOCABULARY;
	}


	public ConstExprLexer(CharStream input) {
		super(input);
		_interp = new LexerATNSimulator(this,_ATN,_decisionToDFA,_sharedContextCache);
	}

	@Override
	public String getGrammarFileName() { return "ConstExpr.g4"; }

	@Override
	public String[] getRuleNames() { return ruleNames; }

	@Override
	public String getSerializedATN() { return _serializedATN; }

	@Override
	public String[] getChannelNames() { return channelNames; }

	@Override
	public String[] getModeNames() { return modeNames; }

	@Override
	public ATN getATN() { return _ATN; }

	public static final String _serializedATN =
		"\u0004\u0000\r\u0085\u0006\uffff\uffff\u0002\u0000\u0007\u0000\u0002\u0001"+
		"\u0007\u0001\u0002\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0002\u0004"+
		"\u0007\u0004\u0002\u0005\u0007\u0005\u0002\u0006\u0007\u0006\u0002\u0007"+
		"\u0007\u0007\u0002\b\u0007\b\u0002\t\u0007\t\u0002\n\u0007\n\u0002\u000b"+
		"\u0007\u000b\u0002\f\u0007\f\u0002\r\u0007\r\u0002\u000e\u0007\u000e\u0002"+
		"\u000f\u0007\u000f\u0001\u0000\u0001\u0000\u0001\u0001\u0001\u0001\u0001"+
		"\u0002\u0001\u0002\u0001\u0003\u0001\u0003\u0001\u0004\u0001\u0004\u0001"+
		"\u0005\u0001\u0005\u0001\u0006\u0001\u0006\u0001\u0006\u0003\u00061\b"+
		"\u0006\u0001\u0006\u0001\u0006\u0001\u0007\u0001\u0007\u0001\u0007\u0005"+
		"\u00078\b\u0007\n\u0007\f\u0007;\t\u0007\u0001\u0007\u0001\u0007\u0001"+
		"\b\u0003\b@\b\b\u0001\b\u0001\b\u0001\b\u0004\bE\b\b\u000b\b\f\bF\u0001"+
		"\t\u0003\tJ\b\t\u0001\t\u0001\t\u0001\n\u0003\nO\b\n\u0001\n\u0004\nR"+
		"\b\n\u000b\n\f\nS\u0001\n\u0001\n\u0005\nX\b\n\n\n\f\n[\t\n\u0001\n\u0001"+
		"\n\u0003\n_\b\n\u0001\n\u0004\nb\b\n\u000b\n\f\nc\u0003\nf\b\n\u0001\u000b"+
		"\u0004\u000bi\b\u000b\u000b\u000b\f\u000bj\u0001\u000b\u0001\u000b\u0001"+
		"\f\u0001\f\u0001\f\u0001\f\u0001\f\u0001\f\u0004\fu\b\f\u000b\f\f\fv\u0001"+
		"\f\u0001\f\u0003\f{\b\f\u0001\r\u0004\r~\b\r\u000b\r\f\r\u007f\u0001\u000e"+
		"\u0001\u000e\u0001\u000f\u0001\u000f\u0000\u0000\u0010\u0001\u0001\u0003"+
		"\u0002\u0005\u0003\u0007\u0004\t\u0005\u000b\u0006\r\u0007\u000f\b\u0011"+
		"\t\u0013\n\u0015\u000b\u0017\f\u0019\u0000\u001b\u0000\u001d\u0000\u001f"+
		"\r\u0001\u0000\n\u0004\u0000\n\n\r\r\'\'\\\\\u0004\u0000\n\n\r\r\"\"\\"+
		"\\\u0002\u0000XXxx\u0002\u0000EEee\u0002\u0000++--\u0003\u0000\t\n\r\r"+
		"  \t\u0000\"\"\'\'00\\\\bbffnnrrtt\u0002\u0000UUuu\u0001\u000009\u0003"+
		"\u000009AFaf\u0091\u0000\u0001\u0001\u0000\u0000\u0000\u0000\u0003\u0001"+
		"\u0000\u0000\u0000\u0000\u0005\u0001\u0000\u0000\u0000\u0000\u0007\u0001"+
		"\u0000\u0000\u0000\u0000\t\u0001\u0000\u0000\u0000\u0000\u000b\u0001\u0000"+
		"\u0000\u0000\u0000\r\u0001\u0000\u0000\u0000\u0000\u000f\u0001\u0000\u0000"+
		"\u0000\u0000\u0011\u0001\u0000\u0000\u0000\u0000\u0013\u0001\u0000\u0000"+
		"\u0000\u0000\u0015\u0001\u0000\u0000\u0000\u0000\u0017\u0001\u0000\u0000"+
		"\u0000\u0000\u001f\u0001\u0000\u0000\u0000\u0001!\u0001\u0000\u0000\u0000"+
		"\u0003#\u0001\u0000\u0000\u0000\u0005%\u0001\u0000\u0000\u0000\u0007\'"+
		"\u0001\u0000\u0000\u0000\t)\u0001\u0000\u0000\u0000\u000b+\u0001\u0000"+
		"\u0000\u0000\r-\u0001\u0000\u0000\u0000\u000f4\u0001\u0000\u0000\u0000"+
		"\u0011?\u0001\u0000\u0000\u0000\u0013I\u0001\u0000\u0000\u0000\u0015N"+
		"\u0001\u0000\u0000\u0000\u0017h\u0001\u0000\u0000\u0000\u0019z\u0001\u0000"+
		"\u0000\u0000\u001b}\u0001\u0000\u0000\u0000\u001d\u0081\u0001\u0000\u0000"+
		"\u0000\u001f\u0083\u0001\u0000\u0000\u0000!\"\u0005+\u0000\u0000\"\u0002"+
		"\u0001\u0000\u0000\u0000#$\u0005-\u0000\u0000$\u0004\u0001\u0000\u0000"+
		"\u0000%&\u0005*\u0000\u0000&\u0006\u0001\u0000\u0000\u0000\'(\u0005/\u0000"+
		"\u0000(\b\u0001\u0000\u0000\u0000)*\u0005(\u0000\u0000*\n\u0001\u0000"+
		"\u0000\u0000+,\u0005)\u0000\u0000,\f\u0001\u0000\u0000\u0000-0\u0005\'"+
		"\u0000\u0000.1\u0003\u0019\f\u0000/1\b\u0000\u0000\u00000.\u0001\u0000"+
		"\u0000\u00000/\u0001\u0000\u0000\u000012\u0001\u0000\u0000\u000023\u0005"+
		"\'\u0000\u00003\u000e\u0001\u0000\u0000\u000049\u0005\"\u0000\u000058"+
		"\u0003\u0019\f\u000068\b\u0001\u0000\u000075\u0001\u0000\u0000\u00007"+
		"6\u0001\u0000\u0000\u00008;\u0001\u0000\u0000\u000097\u0001\u0000\u0000"+
		"\u00009:\u0001\u0000\u0000\u0000:<\u0001\u0000\u0000\u0000;9\u0001\u0000"+
		"\u0000\u0000<=\u0005\"\u0000\u0000=\u0010\u0001\u0000\u0000\u0000>@\u0005"+
		"-\u0000\u0000?>\u0001\u0000\u0000\u0000?@\u0001\u0000\u0000\u0000@A\u0001"+
		"\u0000\u0000\u0000AB\u00050\u0000\u0000BD\u0007\u0002\u0000\u0000CE\u0003"+
		"\u001d\u000e\u0000DC\u0001\u0000\u0000\u0000EF\u0001\u0000\u0000\u0000"+
		"FD\u0001\u0000\u0000\u0000FG\u0001\u0000\u0000\u0000G\u0012\u0001\u0000"+
		"\u0000\u0000HJ\u0005-\u0000\u0000IH\u0001\u0000\u0000\u0000IJ\u0001\u0000"+
		"\u0000\u0000JK\u0001\u0000\u0000\u0000KL\u0003\u001b\r\u0000L\u0014\u0001"+
		"\u0000\u0000\u0000MO\u0005-\u0000\u0000NM\u0001\u0000\u0000\u0000NO\u0001"+
		"\u0000\u0000\u0000OQ\u0001\u0000\u0000\u0000PR\u0003\u001b\r\u0000QP\u0001"+
		"\u0000\u0000\u0000RS\u0001\u0000\u0000\u0000SQ\u0001\u0000\u0000\u0000"+
		"ST\u0001\u0000\u0000\u0000TU\u0001\u0000\u0000\u0000UY\u0005.\u0000\u0000"+
		"VX\u0003\u001b\r\u0000WV\u0001\u0000\u0000\u0000X[\u0001\u0000\u0000\u0000"+
		"YW\u0001\u0000\u0000\u0000YZ\u0001\u0000\u0000\u0000Ze\u0001\u0000\u0000"+
		"\u0000[Y\u0001\u0000\u0000\u0000\\^\u0007\u0003\u0000\u0000]_\u0007\u0004"+
		"\u0000\u0000^]\u0001\u0000\u0000\u0000^_\u0001\u0000\u0000\u0000_a\u0001"+
		"\u0000\u0000\u0000`b\u0003\u001b\r\u0000a`\u0001\u0000\u0000\u0000bc\u0001"+
		"\u0000\u0000\u0000ca\u0001\u0000\u0000\u0000cd\u0001\u0000\u0000\u0000"+
		"df\u0001\u0000\u0000\u0000e\\\u0001\u0000\u0000\u0000ef\u0001\u0000\u0000"+
		"\u0000f\u0016\u0001\u0000\u0000\u0000gi\u0007\u0005\u0000\u0000hg\u0001"+
		"\u0000\u0000\u0000ij\u0001\u0000\u0000\u0000jh\u0001\u0000\u0000\u0000"+
		"jk\u0001\u0000\u0000\u0000kl\u0001\u0000\u0000\u0000lm\u0006\u000b\u0000"+
		"\u0000m\u0018\u0001\u0000\u0000\u0000no\u0005\\\u0000\u0000o{\u0007\u0006"+
		"\u0000\u0000pq\u0005\\\u0000\u0000qr\u0007\u0007\u0000\u0000rt\u0005{"+
		"\u0000\u0000su\u0003\u001d\u000e\u0000ts\u0001\u0000\u0000\u0000uv\u0001"+
		"\u0000\u0000\u0000vt\u0001\u0000\u0000\u0000vw\u0001\u0000\u0000\u0000"+
		"wx\u0001\u0000\u0000\u0000xy\u0005}\u0000\u0000y{\u0001\u0000\u0000\u0000"+
		"zn\u0001\u0000\u0000\u0000zp\u0001\u0000\u0000\u0000{\u001a\u0001\u0000"+
		"\u0000\u0000|~\u0007\b\u0000\u0000}|\u0001\u0000\u0000\u0000~\u007f\u0001"+
		"\u0000\u0000\u0000\u007f}\u0001\u0000\u0000\u0000\u007f\u0080\u0001\u0000"+
		"\u0000\u0000\u0080\u001c\u0001\u0000\u0000\u0000\u0081\u0082\u0007\t\u0000"+
		"\u0000\u0082\u001e\u0001\u0000\u0000\u0000\u0083\u0084\t\u0000\u0000\u0000"+
		"\u0084 \u0001\u0000\u0000\u0000\u0011\u0000079?FINSY^cejvz\u007f\u0001"+
		"\u0000\u0001\u0000";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}