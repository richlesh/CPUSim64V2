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
		"\u0004\u0000\r\u007f\u0006\uffff\uffff\u0002\u0000\u0007\u0000\u0002\u0001"+
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
		"\t\u0001\t\u0001\n\u0004\nL\b\n\u000b\n\f\nM\u0001\n\u0001\n\u0005\nR"+
		"\b\n\n\n\f\nU\t\n\u0001\n\u0001\n\u0003\nY\b\n\u0001\n\u0004\n\\\b\n\u000b"+
		"\n\f\n]\u0003\n`\b\n\u0001\u000b\u0004\u000bc\b\u000b\u000b\u000b\f\u000b"+
		"d\u0001\u000b\u0001\u000b\u0001\f\u0001\f\u0001\f\u0001\f\u0001\f\u0001"+
		"\f\u0004\fo\b\f\u000b\f\f\fp\u0001\f\u0001\f\u0003\fu\b\f\u0001\r\u0004"+
		"\rx\b\r\u000b\r\f\ry\u0001\u000e\u0001\u000e\u0001\u000f\u0001\u000f\u0000"+
		"\u0000\u0010\u0001\u0001\u0003\u0002\u0005\u0003\u0007\u0004\t\u0005\u000b"+
		"\u0006\r\u0007\u000f\b\u0011\t\u0013\n\u0015\u000b\u0017\f\u0019\u0000"+
		"\u001b\u0000\u001d\u0000\u001f\r\u0001\u0000\n\u0004\u0000\n\n\r\r\'\'"+
		"\\\\\u0004\u0000\n\n\r\r\"\"\\\\\u0002\u0000XXxx\u0002\u0000EEee\u0002"+
		"\u0000++--\u0003\u0000\t\n\r\r  \t\u0000\"\"\'\'00\\\\bbffnnrrtt\u0002"+
		"\u0000UUuu\u0001\u000009\u0003\u000009AFaf\u0089\u0000\u0001\u0001\u0000"+
		"\u0000\u0000\u0000\u0003\u0001\u0000\u0000\u0000\u0000\u0005\u0001\u0000"+
		"\u0000\u0000\u0000\u0007\u0001\u0000\u0000\u0000\u0000\t\u0001\u0000\u0000"+
		"\u0000\u0000\u000b\u0001\u0000\u0000\u0000\u0000\r\u0001\u0000\u0000\u0000"+
		"\u0000\u000f\u0001\u0000\u0000\u0000\u0000\u0011\u0001\u0000\u0000\u0000"+
		"\u0000\u0013\u0001\u0000\u0000\u0000\u0000\u0015\u0001\u0000\u0000\u0000"+
		"\u0000\u0017\u0001\u0000\u0000\u0000\u0000\u001f\u0001\u0000\u0000\u0000"+
		"\u0001!\u0001\u0000\u0000\u0000\u0003#\u0001\u0000\u0000\u0000\u0005%"+
		"\u0001\u0000\u0000\u0000\u0007\'\u0001\u0000\u0000\u0000\t)\u0001\u0000"+
		"\u0000\u0000\u000b+\u0001\u0000\u0000\u0000\r-\u0001\u0000\u0000\u0000"+
		"\u000f4\u0001\u0000\u0000\u0000\u0011?\u0001\u0000\u0000\u0000\u0013H"+
		"\u0001\u0000\u0000\u0000\u0015K\u0001\u0000\u0000\u0000\u0017b\u0001\u0000"+
		"\u0000\u0000\u0019t\u0001\u0000\u0000\u0000\u001bw\u0001\u0000\u0000\u0000"+
		"\u001d{\u0001\u0000\u0000\u0000\u001f}\u0001\u0000\u0000\u0000!\"\u0005"+
		"+\u0000\u0000\"\u0002\u0001\u0000\u0000\u0000#$\u0005-\u0000\u0000$\u0004"+
		"\u0001\u0000\u0000\u0000%&\u0005*\u0000\u0000&\u0006\u0001\u0000\u0000"+
		"\u0000\'(\u0005/\u0000\u0000(\b\u0001\u0000\u0000\u0000)*\u0005(\u0000"+
		"\u0000*\n\u0001\u0000\u0000\u0000+,\u0005)\u0000\u0000,\f\u0001\u0000"+
		"\u0000\u0000-0\u0005\'\u0000\u0000.1\u0003\u0019\f\u0000/1\b\u0000\u0000"+
		"\u00000.\u0001\u0000\u0000\u00000/\u0001\u0000\u0000\u000012\u0001\u0000"+
		"\u0000\u000023\u0005\'\u0000\u00003\u000e\u0001\u0000\u0000\u000049\u0005"+
		"\"\u0000\u000058\u0003\u0019\f\u000068\b\u0001\u0000\u000075\u0001\u0000"+
		"\u0000\u000076\u0001\u0000\u0000\u00008;\u0001\u0000\u0000\u000097\u0001"+
		"\u0000\u0000\u00009:\u0001\u0000\u0000\u0000:<\u0001\u0000\u0000\u0000"+
		";9\u0001\u0000\u0000\u0000<=\u0005\"\u0000\u0000=\u0010\u0001\u0000\u0000"+
		"\u0000>@\u0005-\u0000\u0000?>\u0001\u0000\u0000\u0000?@\u0001\u0000\u0000"+
		"\u0000@A\u0001\u0000\u0000\u0000AB\u00050\u0000\u0000BD\u0007\u0002\u0000"+
		"\u0000CE\u0003\u001d\u000e\u0000DC\u0001\u0000\u0000\u0000EF\u0001\u0000"+
		"\u0000\u0000FD\u0001\u0000\u0000\u0000FG\u0001\u0000\u0000\u0000G\u0012"+
		"\u0001\u0000\u0000\u0000HI\u0003\u001b\r\u0000I\u0014\u0001\u0000\u0000"+
		"\u0000JL\u0003\u001b\r\u0000KJ\u0001\u0000\u0000\u0000LM\u0001\u0000\u0000"+
		"\u0000MK\u0001\u0000\u0000\u0000MN\u0001\u0000\u0000\u0000NO\u0001\u0000"+
		"\u0000\u0000OS\u0005.\u0000\u0000PR\u0003\u001b\r\u0000QP\u0001\u0000"+
		"\u0000\u0000RU\u0001\u0000\u0000\u0000SQ\u0001\u0000\u0000\u0000ST\u0001"+
		"\u0000\u0000\u0000T_\u0001\u0000\u0000\u0000US\u0001\u0000\u0000\u0000"+
		"VX\u0007\u0003\u0000\u0000WY\u0007\u0004\u0000\u0000XW\u0001\u0000\u0000"+
		"\u0000XY\u0001\u0000\u0000\u0000Y[\u0001\u0000\u0000\u0000Z\\\u0003\u001b"+
		"\r\u0000[Z\u0001\u0000\u0000\u0000\\]\u0001\u0000\u0000\u0000][\u0001"+
		"\u0000\u0000\u0000]^\u0001\u0000\u0000\u0000^`\u0001\u0000\u0000\u0000"+
		"_V\u0001\u0000\u0000\u0000_`\u0001\u0000\u0000\u0000`\u0016\u0001\u0000"+
		"\u0000\u0000ac\u0007\u0005\u0000\u0000ba\u0001\u0000\u0000\u0000cd\u0001"+
		"\u0000\u0000\u0000db\u0001\u0000\u0000\u0000de\u0001\u0000\u0000\u0000"+
		"ef\u0001\u0000\u0000\u0000fg\u0006\u000b\u0000\u0000g\u0018\u0001\u0000"+
		"\u0000\u0000hi\u0005\\\u0000\u0000iu\u0007\u0006\u0000\u0000jk\u0005\\"+
		"\u0000\u0000kl\u0007\u0007\u0000\u0000ln\u0005{\u0000\u0000mo\u0003\u001d"+
		"\u000e\u0000nm\u0001\u0000\u0000\u0000op\u0001\u0000\u0000\u0000pn\u0001"+
		"\u0000\u0000\u0000pq\u0001\u0000\u0000\u0000qr\u0001\u0000\u0000\u0000"+
		"rs\u0005}\u0000\u0000su\u0001\u0000\u0000\u0000th\u0001\u0000\u0000\u0000"+
		"tj\u0001\u0000\u0000\u0000u\u001a\u0001\u0000\u0000\u0000vx\u0007\b\u0000"+
		"\u0000wv\u0001\u0000\u0000\u0000xy\u0001\u0000\u0000\u0000yw\u0001\u0000"+
		"\u0000\u0000yz\u0001\u0000\u0000\u0000z\u001c\u0001\u0000\u0000\u0000"+
		"{|\u0007\t\u0000\u0000|\u001e\u0001\u0000\u0000\u0000}~\t\u0000\u0000"+
		"\u0000~ \u0001\u0000\u0000\u0000\u000f\u0000079?FMSX]_dpty\u0001\u0000"+
		"\u0001\u0000";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}