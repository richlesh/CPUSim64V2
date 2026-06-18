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
		CHAR=1, STRING=2, HEXINT=3, INT=4, FLOAT=5, IDENT=6, PLUS=7, MINUS=8, 
		MULTIPLY=9, DIVIDE=10, LPAREN=11, RPAREN=12, WS=13, OTHER=14;
	public static String[] channelNames = {
		"DEFAULT_TOKEN_CHANNEL", "HIDDEN"
	};

	public static String[] modeNames = {
		"DEFAULT_MODE"
	};

	private static String[] makeRuleNames() {
		return new String[] {
			"CHAR", "STRING", "HEXINT", "INT", "FLOAT", "IDENT", "PLUS", "MINUS", 
			"MULTIPLY", "DIVIDE", "LPAREN", "RPAREN", "WS", "ESC", "DIGITS", "HEX", 
			"OTHER"
		};
	}
	public static final String[] ruleNames = makeRuleNames();

	private static String[] makeLiteralNames() {
		return new String[] {
			null, null, null, null, null, null, null, "'+'", "'-'", "'*'", "'/'", 
			"'('", "')'"
		};
	}
	private static final String[] _LITERAL_NAMES = makeLiteralNames();
	private static String[] makeSymbolicNames() {
		return new String[] {
			null, "CHAR", "STRING", "HEXINT", "INT", "FLOAT", "IDENT", "PLUS", "MINUS", 
			"MULTIPLY", "DIVIDE", "LPAREN", "RPAREN", "WS", "OTHER"
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
		"\u0004\u0000\u000e\u0085\u0006\uffff\uffff\u0002\u0000\u0007\u0000\u0002"+
		"\u0001\u0007\u0001\u0002\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0002"+
		"\u0004\u0007\u0004\u0002\u0005\u0007\u0005\u0002\u0006\u0007\u0006\u0002"+
		"\u0007\u0007\u0007\u0002\b\u0007\b\u0002\t\u0007\t\u0002\n\u0007\n\u0002"+
		"\u000b\u0007\u000b\u0002\f\u0007\f\u0002\r\u0007\r\u0002\u000e\u0007\u000e"+
		"\u0002\u000f\u0007\u000f\u0002\u0010\u0007\u0010\u0001\u0000\u0001\u0000"+
		"\u0001\u0000\u0003\u0000\'\b\u0000\u0001\u0000\u0001\u0000\u0001\u0001"+
		"\u0001\u0001\u0001\u0001\u0005\u0001.\b\u0001\n\u0001\f\u00011\t\u0001"+
		"\u0001\u0001\u0001\u0001\u0001\u0002\u0001\u0002\u0001\u0002\u0004\u0002"+
		"8\b\u0002\u000b\u0002\f\u00029\u0001\u0003\u0004\u0003=\b\u0003\u000b"+
		"\u0003\f\u0003>\u0001\u0004\u0004\u0004B\b\u0004\u000b\u0004\f\u0004C"+
		"\u0001\u0004\u0001\u0004\u0005\u0004H\b\u0004\n\u0004\f\u0004K\t\u0004"+
		"\u0001\u0004\u0001\u0004\u0003\u0004O\b\u0004\u0001\u0004\u0004\u0004"+
		"R\b\u0004\u000b\u0004\f\u0004S\u0003\u0004V\b\u0004\u0001\u0005\u0001"+
		"\u0005\u0005\u0005Z\b\u0005\n\u0005\f\u0005]\t\u0005\u0001\u0006\u0001"+
		"\u0006\u0001\u0007\u0001\u0007\u0001\b\u0001\b\u0001\t\u0001\t\u0001\n"+
		"\u0001\n\u0001\u000b\u0001\u000b\u0001\f\u0004\fl\b\f\u000b\f\f\fm\u0001"+
		"\f\u0001\f\u0001\r\u0001\r\u0001\r\u0001\r\u0001\r\u0001\r\u0004\rx\b"+
		"\r\u000b\r\f\ry\u0001\r\u0001\r\u0003\r~\b\r\u0001\u000e\u0001\u000e\u0001"+
		"\u000f\u0001\u000f\u0001\u0010\u0001\u0010\u0000\u0000\u0011\u0001\u0001"+
		"\u0003\u0002\u0005\u0003\u0007\u0004\t\u0005\u000b\u0006\r\u0007\u000f"+
		"\b\u0011\t\u0013\n\u0015\u000b\u0017\f\u0019\r\u001b\u0000\u001d\u0000"+
		"\u001f\u0000!\u000e\u0001\u0000\f\u0004\u0000\n\n\r\r\'\'\\\\\u0004\u0000"+
		"\n\n\r\r\"\"\\\\\u0002\u0000XXxx\u0002\u0000EEee\u0002\u0000++--\u0004"+
		"\u0000$$@Z__az\u0005\u0000$$09AZ__az\u0003\u0000\t\n\r\r  \t\u0000\"\""+
		"\'\'00\\\\bbffnnrrtt\u0002\u0000UUuu\u0001\u000009\u0003\u000009AFaf\u008f"+
		"\u0000\u0001\u0001\u0000\u0000\u0000\u0000\u0003\u0001\u0000\u0000\u0000"+
		"\u0000\u0005\u0001\u0000\u0000\u0000\u0000\u0007\u0001\u0000\u0000\u0000"+
		"\u0000\t\u0001\u0000\u0000\u0000\u0000\u000b\u0001\u0000\u0000\u0000\u0000"+
		"\r\u0001\u0000\u0000\u0000\u0000\u000f\u0001\u0000\u0000\u0000\u0000\u0011"+
		"\u0001\u0000\u0000\u0000\u0000\u0013\u0001\u0000\u0000\u0000\u0000\u0015"+
		"\u0001\u0000\u0000\u0000\u0000\u0017\u0001\u0000\u0000\u0000\u0000\u0019"+
		"\u0001\u0000\u0000\u0000\u0000!\u0001\u0000\u0000\u0000\u0001#\u0001\u0000"+
		"\u0000\u0000\u0003*\u0001\u0000\u0000\u0000\u00054\u0001\u0000\u0000\u0000"+
		"\u0007<\u0001\u0000\u0000\u0000\tA\u0001\u0000\u0000\u0000\u000bW\u0001"+
		"\u0000\u0000\u0000\r^\u0001\u0000\u0000\u0000\u000f`\u0001\u0000\u0000"+
		"\u0000\u0011b\u0001\u0000\u0000\u0000\u0013d\u0001\u0000\u0000\u0000\u0015"+
		"f\u0001\u0000\u0000\u0000\u0017h\u0001\u0000\u0000\u0000\u0019k\u0001"+
		"\u0000\u0000\u0000\u001b}\u0001\u0000\u0000\u0000\u001d\u007f\u0001\u0000"+
		"\u0000\u0000\u001f\u0081\u0001\u0000\u0000\u0000!\u0083\u0001\u0000\u0000"+
		"\u0000#&\u0005\'\u0000\u0000$\'\u0003\u001b\r\u0000%\'\b\u0000\u0000\u0000"+
		"&$\u0001\u0000\u0000\u0000&%\u0001\u0000\u0000\u0000\'(\u0001\u0000\u0000"+
		"\u0000()\u0005\'\u0000\u0000)\u0002\u0001\u0000\u0000\u0000*/\u0005\""+
		"\u0000\u0000+.\u0003\u001b\r\u0000,.\b\u0001\u0000\u0000-+\u0001\u0000"+
		"\u0000\u0000-,\u0001\u0000\u0000\u0000.1\u0001\u0000\u0000\u0000/-\u0001"+
		"\u0000\u0000\u0000/0\u0001\u0000\u0000\u000002\u0001\u0000\u0000\u0000"+
		"1/\u0001\u0000\u0000\u000023\u0005\"\u0000\u00003\u0004\u0001\u0000\u0000"+
		"\u000045\u00050\u0000\u000057\u0007\u0002\u0000\u000068\u0003\u001f\u000f"+
		"\u000076\u0001\u0000\u0000\u000089\u0001\u0000\u0000\u000097\u0001\u0000"+
		"\u0000\u00009:\u0001\u0000\u0000\u0000:\u0006\u0001\u0000\u0000\u0000"+
		";=\u0003\u001d\u000e\u0000<;\u0001\u0000\u0000\u0000=>\u0001\u0000\u0000"+
		"\u0000><\u0001\u0000\u0000\u0000>?\u0001\u0000\u0000\u0000?\b\u0001\u0000"+
		"\u0000\u0000@B\u0003\u001d\u000e\u0000A@\u0001\u0000\u0000\u0000BC\u0001"+
		"\u0000\u0000\u0000CA\u0001\u0000\u0000\u0000CD\u0001\u0000\u0000\u0000"+
		"DE\u0001\u0000\u0000\u0000EI\u0005.\u0000\u0000FH\u0003\u001d\u000e\u0000"+
		"GF\u0001\u0000\u0000\u0000HK\u0001\u0000\u0000\u0000IG\u0001\u0000\u0000"+
		"\u0000IJ\u0001\u0000\u0000\u0000JU\u0001\u0000\u0000\u0000KI\u0001\u0000"+
		"\u0000\u0000LN\u0007\u0003\u0000\u0000MO\u0007\u0004\u0000\u0000NM\u0001"+
		"\u0000\u0000\u0000NO\u0001\u0000\u0000\u0000OQ\u0001\u0000\u0000\u0000"+
		"PR\u0003\u001d\u000e\u0000QP\u0001\u0000\u0000\u0000RS\u0001\u0000\u0000"+
		"\u0000SQ\u0001\u0000\u0000\u0000ST\u0001\u0000\u0000\u0000TV\u0001\u0000"+
		"\u0000\u0000UL\u0001\u0000\u0000\u0000UV\u0001\u0000\u0000\u0000V\n\u0001"+
		"\u0000\u0000\u0000W[\u0007\u0005\u0000\u0000XZ\u0007\u0006\u0000\u0000"+
		"YX\u0001\u0000\u0000\u0000Z]\u0001\u0000\u0000\u0000[Y\u0001\u0000\u0000"+
		"\u0000[\\\u0001\u0000\u0000\u0000\\\f\u0001\u0000\u0000\u0000][\u0001"+
		"\u0000\u0000\u0000^_\u0005+\u0000\u0000_\u000e\u0001\u0000\u0000\u0000"+
		"`a\u0005-\u0000\u0000a\u0010\u0001\u0000\u0000\u0000bc\u0005*\u0000\u0000"+
		"c\u0012\u0001\u0000\u0000\u0000de\u0005/\u0000\u0000e\u0014\u0001\u0000"+
		"\u0000\u0000fg\u0005(\u0000\u0000g\u0016\u0001\u0000\u0000\u0000hi\u0005"+
		")\u0000\u0000i\u0018\u0001\u0000\u0000\u0000jl\u0007\u0007\u0000\u0000"+
		"kj\u0001\u0000\u0000\u0000lm\u0001\u0000\u0000\u0000mk\u0001\u0000\u0000"+
		"\u0000mn\u0001\u0000\u0000\u0000no\u0001\u0000\u0000\u0000op\u0006\f\u0000"+
		"\u0000p\u001a\u0001\u0000\u0000\u0000qr\u0005\\\u0000\u0000r~\u0007\b"+
		"\u0000\u0000st\u0005\\\u0000\u0000tu\u0007\t\u0000\u0000uw\u0005{\u0000"+
		"\u0000vx\u0003\u001f\u000f\u0000wv\u0001\u0000\u0000\u0000xy\u0001\u0000"+
		"\u0000\u0000yw\u0001\u0000\u0000\u0000yz\u0001\u0000\u0000\u0000z{\u0001"+
		"\u0000\u0000\u0000{|\u0005}\u0000\u0000|~\u0001\u0000\u0000\u0000}q\u0001"+
		"\u0000\u0000\u0000}s\u0001\u0000\u0000\u0000~\u001c\u0001\u0000\u0000"+
		"\u0000\u007f\u0080\u0007\n\u0000\u0000\u0080\u001e\u0001\u0000\u0000\u0000"+
		"\u0081\u0082\u0007\u000b\u0000\u0000\u0082 \u0001\u0000\u0000\u0000\u0083"+
		"\u0084\t\u0000\u0000\u0000\u0084\"\u0001\u0000\u0000\u0000\u000f\u0000"+
		"&-/9>CINSU[my}\u0001\u0000\u0001\u0000";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}