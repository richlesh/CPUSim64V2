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
		CHAR=1, STRING=2, HEXINT=3, INT=4, FLOAT=5, PLUS=6, MINUS=7, MULTIPLY=8, 
		DIVIDE=9, LPAREN=10, RPAREN=11, WS=12, OTHER=13;
	public static String[] channelNames = {
		"DEFAULT_TOKEN_CHANNEL", "HIDDEN"
	};

	public static String[] modeNames = {
		"DEFAULT_MODE"
	};

	private static String[] makeRuleNames() {
		return new String[] {
			"CHAR", "STRING", "HEXINT", "INT", "FLOAT", "PLUS", "MINUS", "MULTIPLY", 
			"DIVIDE", "LPAREN", "RPAREN", "WS", "ESC", "DIGITS", "HEX", "OTHER"
		};
	}
	public static final String[] ruleNames = makeRuleNames();

	private static String[] makeLiteralNames() {
		return new String[] {
			null, null, null, null, null, null, "'+'", "'-'", "'*'", "'/'", "'('", 
			"')'"
		};
	}
	private static final String[] _LITERAL_NAMES = makeLiteralNames();
	private static String[] makeSymbolicNames() {
		return new String[] {
			null, "CHAR", "STRING", "HEXINT", "INT", "FLOAT", "PLUS", "MINUS", "MULTIPLY", 
			"DIVIDE", "LPAREN", "RPAREN", "WS", "OTHER"
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
		"\u0004\u0000\r|\u0006\uffff\uffff\u0002\u0000\u0007\u0000\u0002\u0001"+
		"\u0007\u0001\u0002\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0002\u0004"+
		"\u0007\u0004\u0002\u0005\u0007\u0005\u0002\u0006\u0007\u0006\u0002\u0007"+
		"\u0007\u0007\u0002\b\u0007\b\u0002\t\u0007\t\u0002\n\u0007\n\u0002\u000b"+
		"\u0007\u000b\u0002\f\u0007\f\u0002\r\u0007\r\u0002\u000e\u0007\u000e\u0002"+
		"\u000f\u0007\u000f\u0001\u0000\u0001\u0000\u0001\u0000\u0003\u0000%\b"+
		"\u0000\u0001\u0000\u0001\u0000\u0001\u0001\u0001\u0001\u0001\u0001\u0005"+
		"\u0001,\b\u0001\n\u0001\f\u0001/\t\u0001\u0001\u0001\u0001\u0001\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0004\u00026\b\u0002\u000b\u0002\f\u0002"+
		"7\u0001\u0003\u0004\u0003;\b\u0003\u000b\u0003\f\u0003<\u0001\u0004\u0004"+
		"\u0004@\b\u0004\u000b\u0004\f\u0004A\u0001\u0004\u0001\u0004\u0005\u0004"+
		"F\b\u0004\n\u0004\f\u0004I\t\u0004\u0001\u0004\u0001\u0004\u0003\u0004"+
		"M\b\u0004\u0001\u0004\u0004\u0004P\b\u0004\u000b\u0004\f\u0004Q\u0003"+
		"\u0004T\b\u0004\u0001\u0005\u0001\u0005\u0001\u0006\u0001\u0006\u0001"+
		"\u0007\u0001\u0007\u0001\b\u0001\b\u0001\t\u0001\t\u0001\n\u0001\n\u0001"+
		"\u000b\u0004\u000bc\b\u000b\u000b\u000b\f\u000bd\u0001\u000b\u0001\u000b"+
		"\u0001\f\u0001\f\u0001\f\u0001\f\u0001\f\u0001\f\u0004\fo\b\f\u000b\f"+
		"\f\fp\u0001\f\u0001\f\u0003\fu\b\f\u0001\r\u0001\r\u0001\u000e\u0001\u000e"+
		"\u0001\u000f\u0001\u000f\u0000\u0000\u0010\u0001\u0001\u0003\u0002\u0005"+
		"\u0003\u0007\u0004\t\u0005\u000b\u0006\r\u0007\u000f\b\u0011\t\u0013\n"+
		"\u0015\u000b\u0017\f\u0019\u0000\u001b\u0000\u001d\u0000\u001f\r\u0001"+
		"\u0000\n\u0004\u0000\n\n\r\r\'\'\\\\\u0004\u0000\n\n\r\r\"\"\\\\\u0002"+
		"\u0000XXxx\u0002\u0000EEee\u0002\u0000++--\u0003\u0000\t\n\r\r  \t\u0000"+
		"\"\"\'\'00\\\\bbffnnrrtt\u0002\u0000UUuu\u0001\u000009\u0003\u000009A"+
		"Faf\u0085\u0000\u0001\u0001\u0000\u0000\u0000\u0000\u0003\u0001\u0000"+
		"\u0000\u0000\u0000\u0005\u0001\u0000\u0000\u0000\u0000\u0007\u0001\u0000"+
		"\u0000\u0000\u0000\t\u0001\u0000\u0000\u0000\u0000\u000b\u0001\u0000\u0000"+
		"\u0000\u0000\r\u0001\u0000\u0000\u0000\u0000\u000f\u0001\u0000\u0000\u0000"+
		"\u0000\u0011\u0001\u0000\u0000\u0000\u0000\u0013\u0001\u0000\u0000\u0000"+
		"\u0000\u0015\u0001\u0000\u0000\u0000\u0000\u0017\u0001\u0000\u0000\u0000"+
		"\u0000\u001f\u0001\u0000\u0000\u0000\u0001!\u0001\u0000\u0000\u0000\u0003"+
		"(\u0001\u0000\u0000\u0000\u00052\u0001\u0000\u0000\u0000\u0007:\u0001"+
		"\u0000\u0000\u0000\t?\u0001\u0000\u0000\u0000\u000bU\u0001\u0000\u0000"+
		"\u0000\rW\u0001\u0000\u0000\u0000\u000fY\u0001\u0000\u0000\u0000\u0011"+
		"[\u0001\u0000\u0000\u0000\u0013]\u0001\u0000\u0000\u0000\u0015_\u0001"+
		"\u0000\u0000\u0000\u0017b\u0001\u0000\u0000\u0000\u0019t\u0001\u0000\u0000"+
		"\u0000\u001bv\u0001\u0000\u0000\u0000\u001dx\u0001\u0000\u0000\u0000\u001f"+
		"z\u0001\u0000\u0000\u0000!$\u0005\'\u0000\u0000\"%\u0003\u0019\f\u0000"+
		"#%\b\u0000\u0000\u0000$\"\u0001\u0000\u0000\u0000$#\u0001\u0000\u0000"+
		"\u0000%&\u0001\u0000\u0000\u0000&\'\u0005\'\u0000\u0000\'\u0002\u0001"+
		"\u0000\u0000\u0000(-\u0005\"\u0000\u0000),\u0003\u0019\f\u0000*,\b\u0001"+
		"\u0000\u0000+)\u0001\u0000\u0000\u0000+*\u0001\u0000\u0000\u0000,/\u0001"+
		"\u0000\u0000\u0000-+\u0001\u0000\u0000\u0000-.\u0001\u0000\u0000\u0000"+
		".0\u0001\u0000\u0000\u0000/-\u0001\u0000\u0000\u000001\u0005\"\u0000\u0000"+
		"1\u0004\u0001\u0000\u0000\u000023\u00050\u0000\u000035\u0007\u0002\u0000"+
		"\u000046\u0003\u001d\u000e\u000054\u0001\u0000\u0000\u000067\u0001\u0000"+
		"\u0000\u000075\u0001\u0000\u0000\u000078\u0001\u0000\u0000\u00008\u0006"+
		"\u0001\u0000\u0000\u00009;\u0003\u001b\r\u0000:9\u0001\u0000\u0000\u0000"+
		";<\u0001\u0000\u0000\u0000<:\u0001\u0000\u0000\u0000<=\u0001\u0000\u0000"+
		"\u0000=\b\u0001\u0000\u0000\u0000>@\u0003\u001b\r\u0000?>\u0001\u0000"+
		"\u0000\u0000@A\u0001\u0000\u0000\u0000A?\u0001\u0000\u0000\u0000AB\u0001"+
		"\u0000\u0000\u0000BC\u0001\u0000\u0000\u0000CG\u0005.\u0000\u0000DF\u0003"+
		"\u001b\r\u0000ED\u0001\u0000\u0000\u0000FI\u0001\u0000\u0000\u0000GE\u0001"+
		"\u0000\u0000\u0000GH\u0001\u0000\u0000\u0000HS\u0001\u0000\u0000\u0000"+
		"IG\u0001\u0000\u0000\u0000JL\u0007\u0003\u0000\u0000KM\u0007\u0004\u0000"+
		"\u0000LK\u0001\u0000\u0000\u0000LM\u0001\u0000\u0000\u0000MO\u0001\u0000"+
		"\u0000\u0000NP\u0003\u001b\r\u0000ON\u0001\u0000\u0000\u0000PQ\u0001\u0000"+
		"\u0000\u0000QO\u0001\u0000\u0000\u0000QR\u0001\u0000\u0000\u0000RT\u0001"+
		"\u0000\u0000\u0000SJ\u0001\u0000\u0000\u0000ST\u0001\u0000\u0000\u0000"+
		"T\n\u0001\u0000\u0000\u0000UV\u0005+\u0000\u0000V\f\u0001\u0000\u0000"+
		"\u0000WX\u0005-\u0000\u0000X\u000e\u0001\u0000\u0000\u0000YZ\u0005*\u0000"+
		"\u0000Z\u0010\u0001\u0000\u0000\u0000[\\\u0005/\u0000\u0000\\\u0012\u0001"+
		"\u0000\u0000\u0000]^\u0005(\u0000\u0000^\u0014\u0001\u0000\u0000\u0000"+
		"_`\u0005)\u0000\u0000`\u0016\u0001\u0000\u0000\u0000ac\u0007\u0005\u0000"+
		"\u0000ba\u0001\u0000\u0000\u0000cd\u0001\u0000\u0000\u0000db\u0001\u0000"+
		"\u0000\u0000de\u0001\u0000\u0000\u0000ef\u0001\u0000\u0000\u0000fg\u0006"+
		"\u000b\u0000\u0000g\u0018\u0001\u0000\u0000\u0000hi\u0005\\\u0000\u0000"+
		"iu\u0007\u0006\u0000\u0000jk\u0005\\\u0000\u0000kl\u0007\u0007\u0000\u0000"+
		"ln\u0005{\u0000\u0000mo\u0003\u001d\u000e\u0000nm\u0001\u0000\u0000\u0000"+
		"op\u0001\u0000\u0000\u0000pn\u0001\u0000\u0000\u0000pq\u0001\u0000\u0000"+
		"\u0000qr\u0001\u0000\u0000\u0000rs\u0005}\u0000\u0000su\u0001\u0000\u0000"+
		"\u0000th\u0001\u0000\u0000\u0000tj\u0001\u0000\u0000\u0000u\u001a\u0001"+
		"\u0000\u0000\u0000vw\u0007\b\u0000\u0000w\u001c\u0001\u0000\u0000\u0000"+
		"xy\u0007\t\u0000\u0000y\u001e\u0001\u0000\u0000\u0000z{\t\u0000\u0000"+
		"\u0000{ \u0001\u0000\u0000\u0000\u000e\u0000$+-7<AGLQSdpt\u0001\u0000"+
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