// Generated from ExpressionFolding.g4 by ANTLR 4.13.2
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
public class ExpressionFoldingLexer extends Lexer {
	static { RuntimeMetaData.checkVersion("4.13.2", RuntimeMetaData.VERSION); }

	protected static final DFA[] _decisionToDFA;
	protected static final PredictionContextCache _sharedContextCache =
		new PredictionContextCache();
	public static final int
		T__0=1, T__1=2, T__2=3, T__3=4, T__4=5, T__5=6, CHARLIT=7, STRINGLIT=8, 
		HEXLIT=9, INTLIT=10, FLOAT=11, WS=12;
	public static String[] channelNames = {
		"DEFAULT_TOKEN_CHANNEL", "HIDDEN"
	};

	public static String[] modeNames = {
		"DEFAULT_MODE"
	};

	private static String[] makeRuleNames() {
		return new String[] {
			"T__0", "T__1", "T__2", "T__3", "T__4", "T__5", "CHARLIT", "STRINGLIT", 
			"HEXLIT", "INTLIT", "FLOAT", "WS", "ESC", "DIGITS", "HEX"
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
			null, null, null, null, null, null, null, "CHARLIT", "STRINGLIT", "HEXLIT", 
			"INTLIT", "FLOAT", "WS"
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


	public ExpressionFoldingLexer(CharStream input) {
		super(input);
		_interp = new LexerATNSimulator(this,_ATN,_decisionToDFA,_sharedContextCache);
	}

	@Override
	public String getGrammarFileName() { return "ExpressionFolding.g4"; }

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
		"\u0004\u0000\f\u009f\u0006\uffff\uffff\u0002\u0000\u0007\u0000\u0002\u0001"+
		"\u0007\u0001\u0002\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0002\u0004"+
		"\u0007\u0004\u0002\u0005\u0007\u0005\u0002\u0006\u0007\u0006\u0002\u0007"+
		"\u0007\u0007\u0002\b\u0007\b\u0002\t\u0007\t\u0002\n\u0007\n\u0002\u000b"+
		"\u0007\u000b\u0002\f\u0007\f\u0002\r\u0007\r\u0002\u000e\u0007\u000e\u0001"+
		"\u0000\u0001\u0000\u0001\u0001\u0001\u0001\u0001\u0002\u0001\u0002\u0001"+
		"\u0003\u0001\u0003\u0001\u0004\u0001\u0004\u0001\u0005\u0001\u0005\u0001"+
		"\u0006\u0001\u0006\u0001\u0006\u0003\u0006/\b\u0006\u0001\u0006\u0001"+
		"\u0006\u0001\u0007\u0001\u0007\u0001\u0007\u0005\u00076\b\u0007\n\u0007"+
		"\f\u00079\t\u0007\u0001\u0007\u0001\u0007\u0001\b\u0003\b>\b\b\u0001\b"+
		"\u0001\b\u0001\b\u0004\bC\b\b\u000b\b\f\bD\u0001\t\u0003\tH\b\t\u0001"+
		"\t\u0001\t\u0001\n\u0004\nM\b\n\u000b\n\f\nN\u0001\n\u0001\n\u0005\nS"+
		"\b\n\n\n\f\nV\t\n\u0001\n\u0001\n\u0003\nZ\b\n\u0001\n\u0004\n]\b\n\u000b"+
		"\n\f\n^\u0003\na\b\n\u0001\n\u0001\n\u0004\ne\b\n\u000b\n\f\nf\u0001\n"+
		"\u0001\n\u0003\nk\b\n\u0001\n\u0004\nn\b\n\u000b\n\f\no\u0003\nr\b\n\u0001"+
		"\n\u0004\nu\b\n\u000b\n\f\nv\u0001\n\u0001\n\u0003\n{\b\n\u0001\n\u0004"+
		"\n~\b\n\u000b\n\f\n\u007f\u0003\n\u0082\b\n\u0001\u000b\u0004\u000b\u0085"+
		"\b\u000b\u000b\u000b\f\u000b\u0086\u0001\u000b\u0001\u000b\u0001\f\u0001"+
		"\f\u0001\f\u0001\f\u0001\f\u0001\f\u0004\f\u0091\b\f\u000b\f\f\f\u0092"+
		"\u0001\f\u0001\f\u0003\f\u0097\b\f\u0001\r\u0004\r\u009a\b\r\u000b\r\f"+
		"\r\u009b\u0001\u000e\u0001\u000e\u0000\u0000\u000f\u0001\u0001\u0003\u0002"+
		"\u0005\u0003\u0007\u0004\t\u0005\u000b\u0006\r\u0007\u000f\b\u0011\t\u0013"+
		"\n\u0015\u000b\u0017\f\u0019\u0000\u001b\u0000\u001d\u0000\u0001\u0000"+
		"\n\u0004\u0000\n\n\r\r\'\'\\\\\u0004\u0000\n\n\r\r\"\"\\\\\u0002\u0000"+
		"XXxx\u0001\u000009\u0002\u0000EEee\u0002\u0000++--\u0003\u0000\t\n\r\r"+
		"  \t\u0000\"\"\'\'00\\\\bbffnnrrtt\u0002\u0000UUuu\u0003\u000009AFaf\u00b3"+
		"\u0000\u0001\u0001\u0000\u0000\u0000\u0000\u0003\u0001\u0000\u0000\u0000"+
		"\u0000\u0005\u0001\u0000\u0000\u0000\u0000\u0007\u0001\u0000\u0000\u0000"+
		"\u0000\t\u0001\u0000\u0000\u0000\u0000\u000b\u0001\u0000\u0000\u0000\u0000"+
		"\r\u0001\u0000\u0000\u0000\u0000\u000f\u0001\u0000\u0000\u0000\u0000\u0011"+
		"\u0001\u0000\u0000\u0000\u0000\u0013\u0001\u0000\u0000\u0000\u0000\u0015"+
		"\u0001\u0000\u0000\u0000\u0000\u0017\u0001\u0000\u0000\u0000\u0001\u001f"+
		"\u0001\u0000\u0000\u0000\u0003!\u0001\u0000\u0000\u0000\u0005#\u0001\u0000"+
		"\u0000\u0000\u0007%\u0001\u0000\u0000\u0000\t\'\u0001\u0000\u0000\u0000"+
		"\u000b)\u0001\u0000\u0000\u0000\r+\u0001\u0000\u0000\u0000\u000f2\u0001"+
		"\u0000\u0000\u0000\u0011=\u0001\u0000\u0000\u0000\u0013G\u0001\u0000\u0000"+
		"\u0000\u0015\u0081\u0001\u0000\u0000\u0000\u0017\u0084\u0001\u0000\u0000"+
		"\u0000\u0019\u0096\u0001\u0000\u0000\u0000\u001b\u0099\u0001\u0000\u0000"+
		"\u0000\u001d\u009d\u0001\u0000\u0000\u0000\u001f \u0005+\u0000\u0000 "+
		"\u0002\u0001\u0000\u0000\u0000!\"\u0005-\u0000\u0000\"\u0004\u0001\u0000"+
		"\u0000\u0000#$\u0005*\u0000\u0000$\u0006\u0001\u0000\u0000\u0000%&\u0005"+
		"/\u0000\u0000&\b\u0001\u0000\u0000\u0000\'(\u0005(\u0000\u0000(\n\u0001"+
		"\u0000\u0000\u0000)*\u0005)\u0000\u0000*\f\u0001\u0000\u0000\u0000+.\u0005"+
		"\'\u0000\u0000,/\u0003\u0019\f\u0000-/\b\u0000\u0000\u0000.,\u0001\u0000"+
		"\u0000\u0000.-\u0001\u0000\u0000\u0000/0\u0001\u0000\u0000\u000001\u0005"+
		"\'\u0000\u00001\u000e\u0001\u0000\u0000\u000027\u0005\"\u0000\u000036"+
		"\u0003\u0019\f\u000046\b\u0001\u0000\u000053\u0001\u0000\u0000\u00005"+
		"4\u0001\u0000\u0000\u000069\u0001\u0000\u0000\u000075\u0001\u0000\u0000"+
		"\u000078\u0001\u0000\u0000\u00008:\u0001\u0000\u0000\u000097\u0001\u0000"+
		"\u0000\u0000:;\u0005\"\u0000\u0000;\u0010\u0001\u0000\u0000\u0000<>\u0005"+
		"-\u0000\u0000=<\u0001\u0000\u0000\u0000=>\u0001\u0000\u0000\u0000>?\u0001"+
		"\u0000\u0000\u0000?@\u00050\u0000\u0000@B\u0007\u0002\u0000\u0000AC\u0003"+
		"\u001d\u000e\u0000BA\u0001\u0000\u0000\u0000CD\u0001\u0000\u0000\u0000"+
		"DB\u0001\u0000\u0000\u0000DE\u0001\u0000\u0000\u0000E\u0012\u0001\u0000"+
		"\u0000\u0000FH\u0005-\u0000\u0000GF\u0001\u0000\u0000\u0000GH\u0001\u0000"+
		"\u0000\u0000HI\u0001\u0000\u0000\u0000IJ\u0003\u001b\r\u0000J\u0014\u0001"+
		"\u0000\u0000\u0000KM\u0007\u0003\u0000\u0000LK\u0001\u0000\u0000\u0000"+
		"MN\u0001\u0000\u0000\u0000NL\u0001\u0000\u0000\u0000NO\u0001\u0000\u0000"+
		"\u0000OP\u0001\u0000\u0000\u0000PT\u0005.\u0000\u0000QS\u0007\u0003\u0000"+
		"\u0000RQ\u0001\u0000\u0000\u0000SV\u0001\u0000\u0000\u0000TR\u0001\u0000"+
		"\u0000\u0000TU\u0001\u0000\u0000\u0000U`\u0001\u0000\u0000\u0000VT\u0001"+
		"\u0000\u0000\u0000WY\u0007\u0004\u0000\u0000XZ\u0007\u0005\u0000\u0000"+
		"YX\u0001\u0000\u0000\u0000YZ\u0001\u0000\u0000\u0000Z\\\u0001\u0000\u0000"+
		"\u0000[]\u0007\u0003\u0000\u0000\\[\u0001\u0000\u0000\u0000]^\u0001\u0000"+
		"\u0000\u0000^\\\u0001\u0000\u0000\u0000^_\u0001\u0000\u0000\u0000_a\u0001"+
		"\u0000\u0000\u0000`W\u0001\u0000\u0000\u0000`a\u0001\u0000\u0000\u0000"+
		"a\u0082\u0001\u0000\u0000\u0000bd\u0005.\u0000\u0000ce\u0007\u0003\u0000"+
		"\u0000dc\u0001\u0000\u0000\u0000ef\u0001\u0000\u0000\u0000fd\u0001\u0000"+
		"\u0000\u0000fg\u0001\u0000\u0000\u0000gq\u0001\u0000\u0000\u0000hj\u0007"+
		"\u0004\u0000\u0000ik\u0007\u0005\u0000\u0000ji\u0001\u0000\u0000\u0000"+
		"jk\u0001\u0000\u0000\u0000km\u0001\u0000\u0000\u0000ln\u0007\u0003\u0000"+
		"\u0000ml\u0001\u0000\u0000\u0000no\u0001\u0000\u0000\u0000om\u0001\u0000"+
		"\u0000\u0000op\u0001\u0000\u0000\u0000pr\u0001\u0000\u0000\u0000qh\u0001"+
		"\u0000\u0000\u0000qr\u0001\u0000\u0000\u0000r\u0082\u0001\u0000\u0000"+
		"\u0000su\u0007\u0003\u0000\u0000ts\u0001\u0000\u0000\u0000uv\u0001\u0000"+
		"\u0000\u0000vt\u0001\u0000\u0000\u0000vw\u0001\u0000\u0000\u0000wx\u0001"+
		"\u0000\u0000\u0000xz\u0007\u0004\u0000\u0000y{\u0007\u0005\u0000\u0000"+
		"zy\u0001\u0000\u0000\u0000z{\u0001\u0000\u0000\u0000{}\u0001\u0000\u0000"+
		"\u0000|~\u0007\u0003\u0000\u0000}|\u0001\u0000\u0000\u0000~\u007f\u0001"+
		"\u0000\u0000\u0000\u007f}\u0001\u0000\u0000\u0000\u007f\u0080\u0001\u0000"+
		"\u0000\u0000\u0080\u0082\u0001\u0000\u0000\u0000\u0081L\u0001\u0000\u0000"+
		"\u0000\u0081b\u0001\u0000\u0000\u0000\u0081t\u0001\u0000\u0000\u0000\u0082"+
		"\u0016\u0001\u0000\u0000\u0000\u0083\u0085\u0007\u0006\u0000\u0000\u0084"+
		"\u0083\u0001\u0000\u0000\u0000\u0085\u0086\u0001\u0000\u0000\u0000\u0086"+
		"\u0084\u0001\u0000\u0000\u0000\u0086\u0087\u0001\u0000\u0000\u0000\u0087"+
		"\u0088\u0001\u0000\u0000\u0000\u0088\u0089\u0006\u000b\u0000\u0000\u0089"+
		"\u0018\u0001\u0000\u0000\u0000\u008a\u008b\u0005\\\u0000\u0000\u008b\u0097"+
		"\u0007\u0007\u0000\u0000\u008c\u008d\u0005\\\u0000\u0000\u008d\u008e\u0007"+
		"\b\u0000\u0000\u008e\u0090\u0005{\u0000\u0000\u008f\u0091\u0003\u001d"+
		"\u000e\u0000\u0090\u008f\u0001\u0000\u0000\u0000\u0091\u0092\u0001\u0000"+
		"\u0000\u0000\u0092\u0090\u0001\u0000\u0000\u0000\u0092\u0093\u0001\u0000"+
		"\u0000\u0000\u0093\u0094\u0001\u0000\u0000\u0000\u0094\u0095\u0005}\u0000"+
		"\u0000\u0095\u0097\u0001\u0000\u0000\u0000\u0096\u008a\u0001\u0000\u0000"+
		"\u0000\u0096\u008c\u0001\u0000\u0000\u0000\u0097\u001a\u0001\u0000\u0000"+
		"\u0000\u0098\u009a\u0007\u0003\u0000\u0000\u0099\u0098\u0001\u0000\u0000"+
		"\u0000\u009a\u009b\u0001\u0000\u0000\u0000\u009b\u0099\u0001\u0000\u0000"+
		"\u0000\u009b\u009c\u0001\u0000\u0000\u0000\u009c\u001c\u0001\u0000\u0000"+
		"\u0000\u009d\u009e\u0007\t\u0000\u0000\u009e\u001e\u0001\u0000\u0000\u0000"+
		"\u0018\u0000.57=DGNTY^`fjoqvz\u007f\u0081\u0086\u0092\u0096\u009b\u0001"+
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