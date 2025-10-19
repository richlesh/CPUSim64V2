// Generated from PreprocessorParser.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64v2;
import org.antlr.v4.runtime.atn.*;
import org.antlr.v4.runtime.dfa.DFA;
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.misc.*;
import org.antlr.v4.runtime.tree.*;
import java.util.List;
import java.util.Iterator;
import java.util.ArrayList;

@SuppressWarnings({"all", "warnings", "unchecked", "unused", "cast", "CheckReturnValue", "this-escape"})
public class PreprocessorParser extends Parser {
	static { RuntimeMetaData.checkVersion("4.13.2", RuntimeMetaData.VERSION); }

	protected static final DFA[] _decisionToDFA;
	protected static final PredictionContextCache _sharedContextCache =
		new PredictionContextCache();
	public static final int
		BLOCK_COMMENT=1, LINE_COMMENT=2, WS=3, NL=4, PP_INCLUDE=5, PP_DEFINE=6, 
		PP_UNDEF=7, PP_CALL=8, PP_DEF_FUNC=9, PP_END_FUNC=10, PP_DEF_MACRO=11, 
		PP_END_MACRO=12, PP_MACRO=13, PP_SVAR=14, PP_VAR=15, PP_FVAR=16, PP_RETURN=17, 
		PP_FRETURN=18, PP_IF=19, PP_ELSEIF=20, PP_ELSE=21, PP_ENDIF=22, PP_FOR=23, 
		PP_ENDFOR=24, PP_WHILE=25, PP_ENDWHILE=26, PP_DOWHILE=27, PP_ENDDOWHILE=28, 
		PP_IFCOND=29, PP_ELSEIFCOND=30, PP_ELSECOND=31, PP_ENDCOND=32, REG_R=33, 
		REG_F=34, PLACEHOLDER=35, IDENT=36, INT=37, FLOAT=38, CHAR=39, STRING=40, 
		ANGLE_PATH=41, EQEQ=42, NEQ=43, LE=44, GE=45, LT=46, GT=47, COMMA=48, 
		LPAREN=49, RPAREN=50, DOLLAR=51, LCURLY=52, RCURLY=53, ELLIPSIS=54, CODE_TEXT=55, 
		LINE_COMMENT_D=56, WS_D=57;
	public static final int
		RULE_preproc = 0, RULE_codeLine = 1, RULE_directive = 2, RULE_includeDir = 3, 
		RULE_defineDir = 4, RULE_undefDir = 5, RULE_callDir = 6, RULE_macroDir = 7, 
		RULE_argList = 8, RULE_callArg = 9, RULE_defFuncDir = 10, RULE_defMacroDir = 11, 
		RULE_paramList = 12, RULE_codeLineOrDirective = 13, RULE_svarDir = 14, 
		RULE_varDir = 15, RULE_fvarDir = 16, RULE_identList = 17, RULE_returnDir = 18, 
		RULE_freturnDir = 19, RULE_ifBlock = 20, RULE_elseifClause = 21, RULE_elseClause = 22, 
		RULE_forBlock = 23, RULE_whileBlock = 24, RULE_doWhileBlock = 25, RULE_ifCondBlock = 26, 
		RULE_elseifCondClause = 27, RULE_elseCondClause = 28, RULE_block = 29, 
		RULE_expr = 30, RULE_primary = 31, RULE_cmpOp = 32, RULE_literal = 33;
	private static String[] makeRuleNames() {
		return new String[] {
			"preproc", "codeLine", "directive", "includeDir", "defineDir", "undefDir", 
			"callDir", "macroDir", "argList", "callArg", "defFuncDir", "defMacroDir", 
			"paramList", "codeLineOrDirective", "svarDir", "varDir", "fvarDir", "identList", 
			"returnDir", "freturnDir", "ifBlock", "elseifClause", "elseClause", "forBlock", 
			"whileBlock", "doWhileBlock", "ifCondBlock", "elseifCondClause", "elseCondClause", 
			"block", "expr", "primary", "cmpOp", "literal"
		};
	}
	public static final String[] ruleNames = makeRuleNames();

	private static String[] makeLiteralNames() {
		return new String[] {
		};
	}
	private static final String[] _LITERAL_NAMES = makeLiteralNames();
	private static String[] makeSymbolicNames() {
		return new String[] {
			null, "BLOCK_COMMENT", "LINE_COMMENT", "WS", "NL", "PP_INCLUDE", "PP_DEFINE", 
			"PP_UNDEF", "PP_CALL", "PP_DEF_FUNC", "PP_END_FUNC", "PP_DEF_MACRO", 
			"PP_END_MACRO", "PP_MACRO", "PP_SVAR", "PP_VAR", "PP_FVAR", "PP_RETURN", 
			"PP_FRETURN", "PP_IF", "PP_ELSEIF", "PP_ELSE", "PP_ENDIF", "PP_FOR", 
			"PP_ENDFOR", "PP_WHILE", "PP_ENDWHILE", "PP_DOWHILE", "PP_ENDDOWHILE", 
			"PP_IFCOND", "PP_ELSEIFCOND", "PP_ELSECOND", "PP_ENDCOND", "REG_R", "REG_F", 
			"PLACEHOLDER", "IDENT", "INT", "FLOAT", "CHAR", "STRING", "ANGLE_PATH", 
			"EQEQ", "NEQ", "LE", "GE", "LT", "GT", "COMMA", "LPAREN", "RPAREN", "DOLLAR", 
			"LCURLY", "RCURLY", "ELLIPSIS", "CODE_TEXT", "LINE_COMMENT_D", "WS_D"
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

	@Override
	public String getGrammarFileName() { return "PreprocessorParser.g4"; }

	@Override
	public String[] getRuleNames() { return ruleNames; }

	@Override
	public String getSerializedATN() { return _serializedATN; }

	@Override
	public ATN getATN() { return _ATN; }

	public PreprocessorParser(TokenStream input) {
		super(input);
		_interp = new ParserATNSimulator(this,_ATN,_decisionToDFA,_sharedContextCache);
	}

	@SuppressWarnings("CheckReturnValue")
	public static class PreprocContext extends ParserRuleContext {
		public TerminalNode EOF() { return getToken(PreprocessorParser.EOF, 0); }
		public List<DirectiveContext> directive() {
			return getRuleContexts(DirectiveContext.class);
		}
		public DirectiveContext directive(int i) {
			return getRuleContext(DirectiveContext.class,i);
		}
		public List<CodeLineContext> codeLine() {
			return getRuleContexts(CodeLineContext.class);
		}
		public CodeLineContext codeLine(int i) {
			return getRuleContext(CodeLineContext.class,i);
		}
		public PreprocContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_preproc; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterPreproc(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitPreproc(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitPreproc(this);
			else return visitor.visitChildren(this);
		}
	}

	public final PreprocContext preproc() throws RecognitionException {
		PreprocContext _localctx = new PreprocContext(_ctx, getState());
		enterRule(_localctx, 0, RULE_preproc);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(72);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 36028866452515824L) != 0)) {
				{
				setState(70);
				_errHandler.sync(this);
				switch (_input.LA(1)) {
				case PP_INCLUDE:
				case PP_DEFINE:
				case PP_UNDEF:
				case PP_CALL:
				case PP_DEF_FUNC:
				case PP_DEF_MACRO:
				case PP_MACRO:
				case PP_SVAR:
				case PP_VAR:
				case PP_FVAR:
				case PP_RETURN:
				case PP_FRETURN:
				case PP_IF:
				case PP_FOR:
				case PP_WHILE:
				case PP_DOWHILE:
				case PP_IFCOND:
					{
					setState(68);
					directive();
					}
					break;
				case NL:
				case IDENT:
				case CODE_TEXT:
					{
					setState(69);
					codeLine();
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
				setState(74);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(75);
			match(EOF);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class CodeLineContext extends ParserRuleContext {
		public TerminalNode IDENT() { return getToken(PreprocessorParser.IDENT, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public TerminalNode CODE_TEXT() { return getToken(PreprocessorParser.CODE_TEXT, 0); }
		public CodeLineContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_codeLine; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterCodeLine(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitCodeLine(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitCodeLine(this);
			else return visitor.visitChildren(this);
		}
	}

	public final CodeLineContext codeLine() throws RecognitionException {
		CodeLineContext _localctx = new CodeLineContext(_ctx, getState());
		enterRule(_localctx, 2, RULE_codeLine);
		int _la;
		try {
			setState(83);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case IDENT:
				enterOuterAlt(_localctx, 1);
				{
				setState(77);
				match(IDENT);
				setState(78);
				match(NL);
				}
				break;
			case NL:
			case CODE_TEXT:
				enterOuterAlt(_localctx, 2);
				{
				setState(80);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if (_la==CODE_TEXT) {
					{
					setState(79);
					match(CODE_TEXT);
					}
				}

				setState(82);
				match(NL);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class DirectiveContext extends ParserRuleContext {
		public IncludeDirContext includeDir() {
			return getRuleContext(IncludeDirContext.class,0);
		}
		public DefineDirContext defineDir() {
			return getRuleContext(DefineDirContext.class,0);
		}
		public UndefDirContext undefDir() {
			return getRuleContext(UndefDirContext.class,0);
		}
		public CallDirContext callDir() {
			return getRuleContext(CallDirContext.class,0);
		}
		public DefFuncDirContext defFuncDir() {
			return getRuleContext(DefFuncDirContext.class,0);
		}
		public DefMacroDirContext defMacroDir() {
			return getRuleContext(DefMacroDirContext.class,0);
		}
		public MacroDirContext macroDir() {
			return getRuleContext(MacroDirContext.class,0);
		}
		public SvarDirContext svarDir() {
			return getRuleContext(SvarDirContext.class,0);
		}
		public VarDirContext varDir() {
			return getRuleContext(VarDirContext.class,0);
		}
		public FvarDirContext fvarDir() {
			return getRuleContext(FvarDirContext.class,0);
		}
		public ReturnDirContext returnDir() {
			return getRuleContext(ReturnDirContext.class,0);
		}
		public FreturnDirContext freturnDir() {
			return getRuleContext(FreturnDirContext.class,0);
		}
		public IfBlockContext ifBlock() {
			return getRuleContext(IfBlockContext.class,0);
		}
		public ForBlockContext forBlock() {
			return getRuleContext(ForBlockContext.class,0);
		}
		public WhileBlockContext whileBlock() {
			return getRuleContext(WhileBlockContext.class,0);
		}
		public DoWhileBlockContext doWhileBlock() {
			return getRuleContext(DoWhileBlockContext.class,0);
		}
		public IfCondBlockContext ifCondBlock() {
			return getRuleContext(IfCondBlockContext.class,0);
		}
		public DirectiveContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_directive; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterDirective(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitDirective(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitDirective(this);
			else return visitor.visitChildren(this);
		}
	}

	public final DirectiveContext directive() throws RecognitionException {
		DirectiveContext _localctx = new DirectiveContext(_ctx, getState());
		enterRule(_localctx, 4, RULE_directive);
		try {
			setState(102);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case PP_INCLUDE:
				enterOuterAlt(_localctx, 1);
				{
				setState(85);
				includeDir();
				}
				break;
			case PP_DEFINE:
				enterOuterAlt(_localctx, 2);
				{
				setState(86);
				defineDir();
				}
				break;
			case PP_UNDEF:
				enterOuterAlt(_localctx, 3);
				{
				setState(87);
				undefDir();
				}
				break;
			case PP_CALL:
				enterOuterAlt(_localctx, 4);
				{
				setState(88);
				callDir();
				}
				break;
			case PP_DEF_FUNC:
				enterOuterAlt(_localctx, 5);
				{
				setState(89);
				defFuncDir();
				}
				break;
			case PP_DEF_MACRO:
				enterOuterAlt(_localctx, 6);
				{
				setState(90);
				defMacroDir();
				}
				break;
			case PP_MACRO:
				enterOuterAlt(_localctx, 7);
				{
				setState(91);
				macroDir();
				}
				break;
			case PP_SVAR:
				enterOuterAlt(_localctx, 8);
				{
				setState(92);
				svarDir();
				}
				break;
			case PP_VAR:
				enterOuterAlt(_localctx, 9);
				{
				setState(93);
				varDir();
				}
				break;
			case PP_FVAR:
				enterOuterAlt(_localctx, 10);
				{
				setState(94);
				fvarDir();
				}
				break;
			case PP_RETURN:
				enterOuterAlt(_localctx, 11);
				{
				setState(95);
				returnDir();
				}
				break;
			case PP_FRETURN:
				enterOuterAlt(_localctx, 12);
				{
				setState(96);
				freturnDir();
				}
				break;
			case PP_IF:
				enterOuterAlt(_localctx, 13);
				{
				setState(97);
				ifBlock();
				}
				break;
			case PP_FOR:
				enterOuterAlt(_localctx, 14);
				{
				setState(98);
				forBlock();
				}
				break;
			case PP_WHILE:
				enterOuterAlt(_localctx, 15);
				{
				setState(99);
				whileBlock();
				}
				break;
			case PP_DOWHILE:
				enterOuterAlt(_localctx, 16);
				{
				setState(100);
				doWhileBlock();
				}
				break;
			case PP_IFCOND:
				enterOuterAlt(_localctx, 17);
				{
				setState(101);
				ifCondBlock();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class IncludeDirContext extends ParserRuleContext {
		public TerminalNode PP_INCLUDE() { return getToken(PreprocessorParser.PP_INCLUDE, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public TerminalNode ANGLE_PATH() { return getToken(PreprocessorParser.ANGLE_PATH, 0); }
		public TerminalNode STRING() { return getToken(PreprocessorParser.STRING, 0); }
		public IncludeDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_includeDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterIncludeDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitIncludeDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitIncludeDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final IncludeDirContext includeDir() throws RecognitionException {
		IncludeDirContext _localctx = new IncludeDirContext(_ctx, getState());
		enterRule(_localctx, 6, RULE_includeDir);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(104);
			match(PP_INCLUDE);
			setState(105);
			_la = _input.LA(1);
			if ( !(_la==STRING || _la==ANGLE_PATH) ) {
			_errHandler.recoverInline(this);
			}
			else {
				if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
				_errHandler.reportMatch(this);
				consume();
			}
			setState(106);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class DefineDirContext extends ParserRuleContext {
		public TerminalNode PP_DEFINE() { return getToken(PreprocessorParser.PP_DEFINE, 0); }
		public TerminalNode IDENT() { return getToken(PreprocessorParser.IDENT, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public LiteralContext literal() {
			return getRuleContext(LiteralContext.class,0);
		}
		public DefineDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_defineDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterDefineDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitDefineDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitDefineDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final DefineDirContext defineDir() throws RecognitionException {
		DefineDirContext _localctx = new DefineDirContext(_ctx, getState());
		enterRule(_localctx, 8, RULE_defineDir);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(108);
			match(PP_DEFINE);
			setState(109);
			match(IDENT);
			setState(111);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 2061584302080L) != 0)) {
				{
				setState(110);
				literal();
				}
			}

			setState(113);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class UndefDirContext extends ParserRuleContext {
		public TerminalNode PP_UNDEF() { return getToken(PreprocessorParser.PP_UNDEF, 0); }
		public TerminalNode IDENT() { return getToken(PreprocessorParser.IDENT, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public UndefDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_undefDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterUndefDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitUndefDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitUndefDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final UndefDirContext undefDir() throws RecognitionException {
		UndefDirContext _localctx = new UndefDirContext(_ctx, getState());
		enterRule(_localctx, 10, RULE_undefDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(115);
			match(PP_UNDEF);
			setState(116);
			match(IDENT);
			setState(117);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class CallDirContext extends ParserRuleContext {
		public TerminalNode PP_CALL() { return getToken(PreprocessorParser.PP_CALL, 0); }
		public TerminalNode IDENT() { return getToken(PreprocessorParser.IDENT, 0); }
		public TerminalNode LPAREN() { return getToken(PreprocessorParser.LPAREN, 0); }
		public TerminalNode RPAREN() { return getToken(PreprocessorParser.RPAREN, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public ArgListContext argList() {
			return getRuleContext(ArgListContext.class,0);
		}
		public CallDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_callDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterCallDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitCallDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitCallDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final CallDirContext callDir() throws RecognitionException {
		CallDirContext _localctx = new CallDirContext(_ctx, getState());
		enterRule(_localctx, 12, RULE_callDir);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(119);
			match(PP_CALL);
			setState(120);
			match(IDENT);
			setState(121);
			match(LPAREN);
			setState(123);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 279267363520512L) != 0)) {
				{
				setState(122);
				argList();
				}
			}

			setState(125);
			match(RPAREN);
			setState(126);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class MacroDirContext extends ParserRuleContext {
		public TerminalNode PP_MACRO() { return getToken(PreprocessorParser.PP_MACRO, 0); }
		public TerminalNode IDENT() { return getToken(PreprocessorParser.IDENT, 0); }
		public TerminalNode LPAREN() { return getToken(PreprocessorParser.LPAREN, 0); }
		public TerminalNode RPAREN() { return getToken(PreprocessorParser.RPAREN, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public ArgListContext argList() {
			return getRuleContext(ArgListContext.class,0);
		}
		public MacroDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_macroDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterMacroDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitMacroDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitMacroDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final MacroDirContext macroDir() throws RecognitionException {
		MacroDirContext _localctx = new MacroDirContext(_ctx, getState());
		enterRule(_localctx, 14, RULE_macroDir);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(128);
			match(PP_MACRO);
			setState(129);
			match(IDENT);
			setState(130);
			match(LPAREN);
			setState(132);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 279267363520512L) != 0)) {
				{
				setState(131);
				argList();
				}
			}

			setState(134);
			match(RPAREN);
			setState(135);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ArgListContext extends ParserRuleContext {
		public List<CallArgContext> callArg() {
			return getRuleContexts(CallArgContext.class);
		}
		public CallArgContext callArg(int i) {
			return getRuleContext(CallArgContext.class,i);
		}
		public List<TerminalNode> COMMA() { return getTokens(PreprocessorParser.COMMA); }
		public TerminalNode COMMA(int i) {
			return getToken(PreprocessorParser.COMMA, i);
		}
		public ArgListContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_argList; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterArgList(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitArgList(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitArgList(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ArgListContext argList() throws RecognitionException {
		ArgListContext _localctx = new ArgListContext(_ctx, getState());
		enterRule(_localctx, 16, RULE_argList);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(137);
			callArg();
			setState(142);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==COMMA) {
				{
				{
				setState(138);
				match(COMMA);
				setState(139);
				callArg();
				}
				}
				setState(144);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class CallArgContext extends ParserRuleContext {
		public LiteralContext literal() {
			return getRuleContext(LiteralContext.class,0);
		}
		public CmpOpContext cmpOp() {
			return getRuleContext(CmpOpContext.class,0);
		}
		public TerminalNode PLACEHOLDER() { return getToken(PreprocessorParser.PLACEHOLDER, 0); }
		public TerminalNode IDENT() { return getToken(PreprocessorParser.IDENT, 0); }
		public TerminalNode REG_R() { return getToken(PreprocessorParser.REG_R, 0); }
		public TerminalNode REG_F() { return getToken(PreprocessorParser.REG_F, 0); }
		public CallArgContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_callArg; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterCallArg(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitCallArg(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitCallArg(this);
			else return visitor.visitChildren(this);
		}
	}

	public final CallArgContext callArg() throws RecognitionException {
		CallArgContext _localctx = new CallArgContext(_ctx, getState());
		enterRule(_localctx, 18, RULE_callArg);
		try {
			setState(151);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case INT:
			case FLOAT:
			case CHAR:
			case STRING:
				enterOuterAlt(_localctx, 1);
				{
				setState(145);
				literal();
				}
				break;
			case EQEQ:
			case NEQ:
			case LE:
			case GE:
			case LT:
			case GT:
				enterOuterAlt(_localctx, 2);
				{
				setState(146);
				cmpOp();
				}
				break;
			case PLACEHOLDER:
				enterOuterAlt(_localctx, 3);
				{
				setState(147);
				match(PLACEHOLDER);
				}
				break;
			case IDENT:
				enterOuterAlt(_localctx, 4);
				{
				setState(148);
				match(IDENT);
				}
				break;
			case REG_R:
				enterOuterAlt(_localctx, 5);
				{
				setState(149);
				match(REG_R);
				}
				break;
			case REG_F:
				enterOuterAlt(_localctx, 6);
				{
				setState(150);
				match(REG_F);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class DefFuncDirContext extends ParserRuleContext {
		public TerminalNode PP_DEF_FUNC() { return getToken(PreprocessorParser.PP_DEF_FUNC, 0); }
		public TerminalNode IDENT() { return getToken(PreprocessorParser.IDENT, 0); }
		public TerminalNode LPAREN() { return getToken(PreprocessorParser.LPAREN, 0); }
		public TerminalNode RPAREN() { return getToken(PreprocessorParser.RPAREN, 0); }
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
		}
		public TerminalNode PP_END_FUNC() { return getToken(PreprocessorParser.PP_END_FUNC, 0); }
		public ParamListContext paramList() {
			return getRuleContext(ParamListContext.class,0);
		}
		public List<CodeLineOrDirectiveContext> codeLineOrDirective() {
			return getRuleContexts(CodeLineOrDirectiveContext.class);
		}
		public CodeLineOrDirectiveContext codeLineOrDirective(int i) {
			return getRuleContext(CodeLineOrDirectiveContext.class,i);
		}
		public DefFuncDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_defFuncDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterDefFuncDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitDefFuncDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitDefFuncDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final DefFuncDirContext defFuncDir() throws RecognitionException {
		DefFuncDirContext _localctx = new DefFuncDirContext(_ctx, getState());
		enterRule(_localctx, 20, RULE_defFuncDir);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(153);
			match(PP_DEF_FUNC);
			setState(154);
			match(IDENT);
			setState(155);
			match(LPAREN);
			setState(157);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,10,_ctx) ) {
			case 1:
				{
				setState(156);
				paramList();
				}
				break;
			}
			setState(159);
			match(RPAREN);
			setState(160);
			match(NL);
			setState(164);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 36028866452515824L) != 0)) {
				{
				{
				setState(161);
				codeLineOrDirective();
				}
				}
				setState(166);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(167);
			match(PP_END_FUNC);
			setState(168);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class DefMacroDirContext extends ParserRuleContext {
		public TerminalNode PP_DEF_MACRO() { return getToken(PreprocessorParser.PP_DEF_MACRO, 0); }
		public TerminalNode IDENT() { return getToken(PreprocessorParser.IDENT, 0); }
		public TerminalNode LPAREN() { return getToken(PreprocessorParser.LPAREN, 0); }
		public TerminalNode RPAREN() { return getToken(PreprocessorParser.RPAREN, 0); }
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
		}
		public TerminalNode PP_END_MACRO() { return getToken(PreprocessorParser.PP_END_MACRO, 0); }
		public ParamListContext paramList() {
			return getRuleContext(ParamListContext.class,0);
		}
		public List<CodeLineOrDirectiveContext> codeLineOrDirective() {
			return getRuleContexts(CodeLineOrDirectiveContext.class);
		}
		public CodeLineOrDirectiveContext codeLineOrDirective(int i) {
			return getRuleContext(CodeLineOrDirectiveContext.class,i);
		}
		public DefMacroDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_defMacroDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterDefMacroDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitDefMacroDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitDefMacroDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final DefMacroDirContext defMacroDir() throws RecognitionException {
		DefMacroDirContext _localctx = new DefMacroDirContext(_ctx, getState());
		enterRule(_localctx, 22, RULE_defMacroDir);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(170);
			match(PP_DEF_MACRO);
			setState(171);
			match(IDENT);
			setState(172);
			match(LPAREN);
			setState(174);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,12,_ctx) ) {
			case 1:
				{
				setState(173);
				paramList();
				}
				break;
			}
			setState(176);
			match(RPAREN);
			setState(177);
			match(NL);
			setState(181);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 36028866452515824L) != 0)) {
				{
				{
				setState(178);
				codeLineOrDirective();
				}
				}
				setState(183);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(184);
			match(PP_END_MACRO);
			setState(185);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ParamListContext extends ParserRuleContext {
		public List<TerminalNode> IDENT() { return getTokens(PreprocessorParser.IDENT); }
		public TerminalNode IDENT(int i) {
			return getToken(PreprocessorParser.IDENT, i);
		}
		public List<TerminalNode> COMMA() { return getTokens(PreprocessorParser.COMMA); }
		public TerminalNode COMMA(int i) {
			return getToken(PreprocessorParser.COMMA, i);
		}
		public TerminalNode ELLIPSIS() { return getToken(PreprocessorParser.ELLIPSIS, 0); }
		public ParamListContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_paramList; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterParamList(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitParamList(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitParamList(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ParamListContext paramList() throws RecognitionException {
		ParamListContext _localctx = new ParamListContext(_ctx, getState());
		enterRule(_localctx, 24, RULE_paramList);
		int _la;
		try {
			int _alt;
			enterOuterAlt(_localctx, 1);
			{
			setState(188);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==IDENT) {
				{
				setState(187);
				match(IDENT);
				}
			}

			setState(194);
			_errHandler.sync(this);
			_alt = getInterpreter().adaptivePredict(_input,15,_ctx);
			while ( _alt!=2 && _alt!=org.antlr.v4.runtime.atn.ATN.INVALID_ALT_NUMBER ) {
				if ( _alt==1 ) {
					{
					{
					setState(190);
					match(COMMA);
					setState(191);
					match(IDENT);
					}
					} 
				}
				setState(196);
				_errHandler.sync(this);
				_alt = getInterpreter().adaptivePredict(_input,15,_ctx);
			}
			setState(201);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==COMMA || _la==ELLIPSIS) {
				{
				setState(198);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if (_la==COMMA) {
					{
					setState(197);
					match(COMMA);
					}
				}

				setState(200);
				match(ELLIPSIS);
				}
			}

			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class CodeLineOrDirectiveContext extends ParserRuleContext {
		public DirectiveContext directive() {
			return getRuleContext(DirectiveContext.class,0);
		}
		public CodeLineContext codeLine() {
			return getRuleContext(CodeLineContext.class,0);
		}
		public CodeLineOrDirectiveContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_codeLineOrDirective; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterCodeLineOrDirective(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitCodeLineOrDirective(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitCodeLineOrDirective(this);
			else return visitor.visitChildren(this);
		}
	}

	public final CodeLineOrDirectiveContext codeLineOrDirective() throws RecognitionException {
		CodeLineOrDirectiveContext _localctx = new CodeLineOrDirectiveContext(_ctx, getState());
		enterRule(_localctx, 26, RULE_codeLineOrDirective);
		try {
			setState(205);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case PP_INCLUDE:
			case PP_DEFINE:
			case PP_UNDEF:
			case PP_CALL:
			case PP_DEF_FUNC:
			case PP_DEF_MACRO:
			case PP_MACRO:
			case PP_SVAR:
			case PP_VAR:
			case PP_FVAR:
			case PP_RETURN:
			case PP_FRETURN:
			case PP_IF:
			case PP_FOR:
			case PP_WHILE:
			case PP_DOWHILE:
			case PP_IFCOND:
				enterOuterAlt(_localctx, 1);
				{
				setState(203);
				directive();
				}
				break;
			case NL:
			case IDENT:
			case CODE_TEXT:
				enterOuterAlt(_localctx, 2);
				{
				setState(204);
				codeLine();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class SvarDirContext extends ParserRuleContext {
		public TerminalNode PP_SVAR() { return getToken(PreprocessorParser.PP_SVAR, 0); }
		public IdentListContext identList() {
			return getRuleContext(IdentListContext.class,0);
		}
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public SvarDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_svarDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterSvarDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitSvarDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitSvarDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final SvarDirContext svarDir() throws RecognitionException {
		SvarDirContext _localctx = new SvarDirContext(_ctx, getState());
		enterRule(_localctx, 28, RULE_svarDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(207);
			match(PP_SVAR);
			setState(208);
			identList();
			setState(209);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class VarDirContext extends ParserRuleContext {
		public TerminalNode PP_VAR() { return getToken(PreprocessorParser.PP_VAR, 0); }
		public IdentListContext identList() {
			return getRuleContext(IdentListContext.class,0);
		}
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public VarDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_varDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterVarDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitVarDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitVarDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final VarDirContext varDir() throws RecognitionException {
		VarDirContext _localctx = new VarDirContext(_ctx, getState());
		enterRule(_localctx, 30, RULE_varDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(211);
			match(PP_VAR);
			setState(212);
			identList();
			setState(213);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class FvarDirContext extends ParserRuleContext {
		public TerminalNode PP_FVAR() { return getToken(PreprocessorParser.PP_FVAR, 0); }
		public IdentListContext identList() {
			return getRuleContext(IdentListContext.class,0);
		}
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public FvarDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_fvarDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterFvarDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitFvarDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitFvarDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final FvarDirContext fvarDir() throws RecognitionException {
		FvarDirContext _localctx = new FvarDirContext(_ctx, getState());
		enterRule(_localctx, 32, RULE_fvarDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(215);
			match(PP_FVAR);
			setState(216);
			identList();
			setState(217);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class IdentListContext extends ParserRuleContext {
		public List<TerminalNode> IDENT() { return getTokens(PreprocessorParser.IDENT); }
		public TerminalNode IDENT(int i) {
			return getToken(PreprocessorParser.IDENT, i);
		}
		public List<TerminalNode> COMMA() { return getTokens(PreprocessorParser.COMMA); }
		public TerminalNode COMMA(int i) {
			return getToken(PreprocessorParser.COMMA, i);
		}
		public IdentListContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_identList; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterIdentList(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitIdentList(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitIdentList(this);
			else return visitor.visitChildren(this);
		}
	}

	public final IdentListContext identList() throws RecognitionException {
		IdentListContext _localctx = new IdentListContext(_ctx, getState());
		enterRule(_localctx, 34, RULE_identList);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(219);
			match(IDENT);
			setState(224);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==COMMA) {
				{
				{
				setState(220);
				match(COMMA);
				setState(221);
				match(IDENT);
				}
				}
				setState(226);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ReturnDirContext extends ParserRuleContext {
		public TerminalNode PP_RETURN() { return getToken(PreprocessorParser.PP_RETURN, 0); }
		public PrimaryContext primary() {
			return getRuleContext(PrimaryContext.class,0);
		}
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public ReturnDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_returnDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterReturnDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitReturnDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitReturnDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ReturnDirContext returnDir() throws RecognitionException {
		ReturnDirContext _localctx = new ReturnDirContext(_ctx, getState());
		enterRule(_localctx, 36, RULE_returnDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(227);
			match(PP_RETURN);
			setState(228);
			primary();
			setState(229);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class FreturnDirContext extends ParserRuleContext {
		public TerminalNode PP_FRETURN() { return getToken(PreprocessorParser.PP_FRETURN, 0); }
		public PrimaryContext primary() {
			return getRuleContext(PrimaryContext.class,0);
		}
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public FreturnDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_freturnDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterFreturnDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitFreturnDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitFreturnDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final FreturnDirContext freturnDir() throws RecognitionException {
		FreturnDirContext _localctx = new FreturnDirContext(_ctx, getState());
		enterRule(_localctx, 38, RULE_freturnDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(231);
			match(PP_FRETURN);
			setState(232);
			primary();
			setState(233);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class IfBlockContext extends ParserRuleContext {
		public TerminalNode PP_IF() { return getToken(PreprocessorParser.PP_IF, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
		}
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public TerminalNode PP_ENDIF() { return getToken(PreprocessorParser.PP_ENDIF, 0); }
		public List<ElseifClauseContext> elseifClause() {
			return getRuleContexts(ElseifClauseContext.class);
		}
		public ElseifClauseContext elseifClause(int i) {
			return getRuleContext(ElseifClauseContext.class,i);
		}
		public ElseClauseContext elseClause() {
			return getRuleContext(ElseClauseContext.class,0);
		}
		public IfBlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_ifBlock; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterIfBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitIfBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitIfBlock(this);
			else return visitor.visitChildren(this);
		}
	}

	public final IfBlockContext ifBlock() throws RecognitionException {
		IfBlockContext _localctx = new IfBlockContext(_ctx, getState());
		enterRule(_localctx, 40, RULE_ifBlock);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(235);
			match(PP_IF);
			setState(236);
			expr();
			setState(237);
			match(NL);
			setState(238);
			block();
			setState(242);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==PP_ELSEIF) {
				{
				{
				setState(239);
				elseifClause();
				}
				}
				setState(244);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(246);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==PP_ELSE) {
				{
				setState(245);
				elseClause();
				}
			}

			setState(248);
			match(PP_ENDIF);
			setState(249);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ElseifClauseContext extends ParserRuleContext {
		public TerminalNode PP_ELSEIF() { return getToken(PreprocessorParser.PP_ELSEIF, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public ElseifClauseContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_elseifClause; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterElseifClause(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitElseifClause(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitElseifClause(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ElseifClauseContext elseifClause() throws RecognitionException {
		ElseifClauseContext _localctx = new ElseifClauseContext(_ctx, getState());
		enterRule(_localctx, 42, RULE_elseifClause);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(251);
			match(PP_ELSEIF);
			setState(252);
			expr();
			setState(253);
			match(NL);
			setState(254);
			block();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ElseClauseContext extends ParserRuleContext {
		public TerminalNode PP_ELSE() { return getToken(PreprocessorParser.PP_ELSE, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public ElseClauseContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_elseClause; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterElseClause(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitElseClause(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitElseClause(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ElseClauseContext elseClause() throws RecognitionException {
		ElseClauseContext _localctx = new ElseClauseContext(_ctx, getState());
		enterRule(_localctx, 44, RULE_elseClause);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(256);
			match(PP_ELSE);
			setState(257);
			match(NL);
			setState(258);
			block();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ForBlockContext extends ParserRuleContext {
		public PrimaryContext init;
		public ExprContext cond;
		public PrimaryContext incr;
		public TerminalNode PP_FOR() { return getToken(PreprocessorParser.PP_FOR, 0); }
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
		}
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public TerminalNode PP_ENDFOR() { return getToken(PreprocessorParser.PP_ENDFOR, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public List<TerminalNode> COMMA() { return getTokens(PreprocessorParser.COMMA); }
		public TerminalNode COMMA(int i) {
			return getToken(PreprocessorParser.COMMA, i);
		}
		public List<PrimaryContext> primary() {
			return getRuleContexts(PrimaryContext.class);
		}
		public PrimaryContext primary(int i) {
			return getRuleContext(PrimaryContext.class,i);
		}
		public ForBlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_forBlock; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterForBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitForBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitForBlock(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ForBlockContext forBlock() throws RecognitionException {
		ForBlockContext _localctx = new ForBlockContext(_ctx, getState());
		enterRule(_localctx, 46, RULE_forBlock);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(260);
			match(PP_FOR);
			setState(262);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,22,_ctx) ) {
			case 1:
				{
				setState(261);
				((ForBlockContext)_localctx).init = primary();
				}
				break;
			}
			setState(265);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==COMMA) {
				{
				setState(264);
				match(COMMA);
				}
			}

			setState(267);
			((ForBlockContext)_localctx).cond = expr();
			setState(269);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==COMMA) {
				{
				setState(268);
				match(COMMA);
				}
			}

			setState(272);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 2130303778816L) != 0)) {
				{
				setState(271);
				((ForBlockContext)_localctx).incr = primary();
				}
			}

			setState(274);
			match(NL);
			setState(275);
			block();
			setState(276);
			match(PP_ENDFOR);
			setState(277);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class WhileBlockContext extends ParserRuleContext {
		public ExprContext cond;
		public TerminalNode PP_WHILE() { return getToken(PreprocessorParser.PP_WHILE, 0); }
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
		}
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public TerminalNode PP_ENDWHILE() { return getToken(PreprocessorParser.PP_ENDWHILE, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public WhileBlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_whileBlock; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterWhileBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitWhileBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitWhileBlock(this);
			else return visitor.visitChildren(this);
		}
	}

	public final WhileBlockContext whileBlock() throws RecognitionException {
		WhileBlockContext _localctx = new WhileBlockContext(_ctx, getState());
		enterRule(_localctx, 48, RULE_whileBlock);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(279);
			match(PP_WHILE);
			setState(280);
			((WhileBlockContext)_localctx).cond = expr();
			setState(281);
			match(NL);
			setState(282);
			block();
			setState(283);
			match(PP_ENDWHILE);
			setState(284);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class DoWhileBlockContext extends ParserRuleContext {
		public ExprContext cond;
		public TerminalNode PP_DOWHILE() { return getToken(PreprocessorParser.PP_DOWHILE, 0); }
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
		}
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public TerminalNode PP_ENDDOWHILE() { return getToken(PreprocessorParser.PP_ENDDOWHILE, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public DoWhileBlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_doWhileBlock; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterDoWhileBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitDoWhileBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitDoWhileBlock(this);
			else return visitor.visitChildren(this);
		}
	}

	public final DoWhileBlockContext doWhileBlock() throws RecognitionException {
		DoWhileBlockContext _localctx = new DoWhileBlockContext(_ctx, getState());
		enterRule(_localctx, 50, RULE_doWhileBlock);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(286);
			match(PP_DOWHILE);
			setState(287);
			match(NL);
			setState(288);
			block();
			setState(289);
			match(PP_ENDDOWHILE);
			setState(290);
			((DoWhileBlockContext)_localctx).cond = expr();
			setState(291);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class IfCondBlockContext extends ParserRuleContext {
		public ExprContext cond;
		public TerminalNode PP_IFCOND() { return getToken(PreprocessorParser.PP_IFCOND, 0); }
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
		}
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public TerminalNode PP_ENDCOND() { return getToken(PreprocessorParser.PP_ENDCOND, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public List<ElseifCondClauseContext> elseifCondClause() {
			return getRuleContexts(ElseifCondClauseContext.class);
		}
		public ElseifCondClauseContext elseifCondClause(int i) {
			return getRuleContext(ElseifCondClauseContext.class,i);
		}
		public ElseCondClauseContext elseCondClause() {
			return getRuleContext(ElseCondClauseContext.class,0);
		}
		public IfCondBlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_ifCondBlock; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterIfCondBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitIfCondBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitIfCondBlock(this);
			else return visitor.visitChildren(this);
		}
	}

	public final IfCondBlockContext ifCondBlock() throws RecognitionException {
		IfCondBlockContext _localctx = new IfCondBlockContext(_ctx, getState());
		enterRule(_localctx, 52, RULE_ifCondBlock);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(293);
			match(PP_IFCOND);
			setState(294);
			((IfCondBlockContext)_localctx).cond = expr();
			setState(295);
			match(NL);
			setState(296);
			block();
			setState(300);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==PP_ELSEIFCOND) {
				{
				{
				setState(297);
				elseifCondClause();
				}
				}
				setState(302);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(304);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==PP_ELSECOND) {
				{
				setState(303);
				elseCondClause();
				}
			}

			setState(306);
			match(PP_ENDCOND);
			setState(307);
			match(NL);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ElseifCondClauseContext extends ParserRuleContext {
		public ExprContext cond;
		public TerminalNode PP_ELSEIFCOND() { return getToken(PreprocessorParser.PP_ELSEIFCOND, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public ElseifCondClauseContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_elseifCondClause; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterElseifCondClause(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitElseifCondClause(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitElseifCondClause(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ElseifCondClauseContext elseifCondClause() throws RecognitionException {
		ElseifCondClauseContext _localctx = new ElseifCondClauseContext(_ctx, getState());
		enterRule(_localctx, 54, RULE_elseifCondClause);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(309);
			match(PP_ELSEIFCOND);
			setState(310);
			((ElseifCondClauseContext)_localctx).cond = expr();
			setState(311);
			match(NL);
			setState(312);
			block();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ElseCondClauseContext extends ParserRuleContext {
		public TerminalNode PP_ELSECOND() { return getToken(PreprocessorParser.PP_ELSECOND, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public ElseCondClauseContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_elseCondClause; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterElseCondClause(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitElseCondClause(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitElseCondClause(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ElseCondClauseContext elseCondClause() throws RecognitionException {
		ElseCondClauseContext _localctx = new ElseCondClauseContext(_ctx, getState());
		enterRule(_localctx, 56, RULE_elseCondClause);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(314);
			match(PP_ELSECOND);
			setState(315);
			match(NL);
			setState(316);
			block();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class BlockContext extends ParserRuleContext {
		public List<DirectiveContext> directive() {
			return getRuleContexts(DirectiveContext.class);
		}
		public DirectiveContext directive(int i) {
			return getRuleContext(DirectiveContext.class,i);
		}
		public List<CodeLineContext> codeLine() {
			return getRuleContexts(CodeLineContext.class);
		}
		public CodeLineContext codeLine(int i) {
			return getRuleContext(CodeLineContext.class,i);
		}
		public BlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_block; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitBlock(this);
			else return visitor.visitChildren(this);
		}
	}

	public final BlockContext block() throws RecognitionException {
		BlockContext _localctx = new BlockContext(_ctx, getState());
		enterRule(_localctx, 58, RULE_block);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(322);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 36028866452515824L) != 0)) {
				{
				setState(320);
				_errHandler.sync(this);
				switch (_input.LA(1)) {
				case PP_INCLUDE:
				case PP_DEFINE:
				case PP_UNDEF:
				case PP_CALL:
				case PP_DEF_FUNC:
				case PP_DEF_MACRO:
				case PP_MACRO:
				case PP_SVAR:
				case PP_VAR:
				case PP_FVAR:
				case PP_RETURN:
				case PP_FRETURN:
				case PP_IF:
				case PP_FOR:
				case PP_WHILE:
				case PP_DOWHILE:
				case PP_IFCOND:
					{
					setState(318);
					directive();
					}
					break;
				case NL:
				case IDENT:
				case CODE_TEXT:
					{
					setState(319);
					codeLine();
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
				setState(324);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ExprContext extends ParserRuleContext {
		public List<PrimaryContext> primary() {
			return getRuleContexts(PrimaryContext.class);
		}
		public PrimaryContext primary(int i) {
			return getRuleContext(PrimaryContext.class,i);
		}
		public CmpOpContext cmpOp() {
			return getRuleContext(CmpOpContext.class,0);
		}
		public ExprContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_expr; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterExpr(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitExpr(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitExpr(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ExprContext expr() throws RecognitionException {
		ExprContext _localctx = new ExprContext(_ctx, getState());
		enterRule(_localctx, 60, RULE_expr);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(325);
			primary();
			setState(329);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 277076930199552L) != 0)) {
				{
				setState(326);
				cmpOp();
				setState(327);
				primary();
				}
			}

			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class PrimaryContext extends ParserRuleContext {
		public TerminalNode IDENT() { return getToken(PreprocessorParser.IDENT, 0); }
		public LiteralContext literal() {
			return getRuleContext(LiteralContext.class,0);
		}
		public PrimaryContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_primary; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterPrimary(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitPrimary(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitPrimary(this);
			else return visitor.visitChildren(this);
		}
	}

	public final PrimaryContext primary() throws RecognitionException {
		PrimaryContext _localctx = new PrimaryContext(_ctx, getState());
		enterRule(_localctx, 62, RULE_primary);
		try {
			setState(333);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case IDENT:
				enterOuterAlt(_localctx, 1);
				{
				setState(331);
				match(IDENT);
				}
				break;
			case INT:
			case FLOAT:
			case CHAR:
			case STRING:
				enterOuterAlt(_localctx, 2);
				{
				setState(332);
				literal();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class CmpOpContext extends ParserRuleContext {
		public TerminalNode EQEQ() { return getToken(PreprocessorParser.EQEQ, 0); }
		public TerminalNode NEQ() { return getToken(PreprocessorParser.NEQ, 0); }
		public TerminalNode LT() { return getToken(PreprocessorParser.LT, 0); }
		public TerminalNode LE() { return getToken(PreprocessorParser.LE, 0); }
		public TerminalNode GT() { return getToken(PreprocessorParser.GT, 0); }
		public TerminalNode GE() { return getToken(PreprocessorParser.GE, 0); }
		public CmpOpContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_cmpOp; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterCmpOp(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitCmpOp(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitCmpOp(this);
			else return visitor.visitChildren(this);
		}
	}

	public final CmpOpContext cmpOp() throws RecognitionException {
		CmpOpContext _localctx = new CmpOpContext(_ctx, getState());
		enterRule(_localctx, 64, RULE_cmpOp);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(335);
			_la = _input.LA(1);
			if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 277076930199552L) != 0)) ) {
			_errHandler.recoverInline(this);
			}
			else {
				if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
				_errHandler.reportMatch(this);
				consume();
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class LiteralContext extends ParserRuleContext {
		public TerminalNode INT() { return getToken(PreprocessorParser.INT, 0); }
		public TerminalNode FLOAT() { return getToken(PreprocessorParser.FLOAT, 0); }
		public TerminalNode CHAR() { return getToken(PreprocessorParser.CHAR, 0); }
		public TerminalNode STRING() { return getToken(PreprocessorParser.STRING, 0); }
		public LiteralContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_literal; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterLiteral(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitLiteral(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitLiteral(this);
			else return visitor.visitChildren(this);
		}
	}

	public final LiteralContext literal() throws RecognitionException {
		LiteralContext _localctx = new LiteralContext(_ctx, getState());
		enterRule(_localctx, 66, RULE_literal);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(337);
			_la = _input.LA(1);
			if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 2061584302080L) != 0)) ) {
			_errHandler.recoverInline(this);
			}
			else {
				if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
				_errHandler.reportMatch(this);
				consume();
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	public static final String _serializedATN =
		"\u0004\u00019\u0154\u0002\u0000\u0007\u0000\u0002\u0001\u0007\u0001\u0002"+
		"\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0002\u0004\u0007\u0004\u0002"+
		"\u0005\u0007\u0005\u0002\u0006\u0007\u0006\u0002\u0007\u0007\u0007\u0002"+
		"\b\u0007\b\u0002\t\u0007\t\u0002\n\u0007\n\u0002\u000b\u0007\u000b\u0002"+
		"\f\u0007\f\u0002\r\u0007\r\u0002\u000e\u0007\u000e\u0002\u000f\u0007\u000f"+
		"\u0002\u0010\u0007\u0010\u0002\u0011\u0007\u0011\u0002\u0012\u0007\u0012"+
		"\u0002\u0013\u0007\u0013\u0002\u0014\u0007\u0014\u0002\u0015\u0007\u0015"+
		"\u0002\u0016\u0007\u0016\u0002\u0017\u0007\u0017\u0002\u0018\u0007\u0018"+
		"\u0002\u0019\u0007\u0019\u0002\u001a\u0007\u001a\u0002\u001b\u0007\u001b"+
		"\u0002\u001c\u0007\u001c\u0002\u001d\u0007\u001d\u0002\u001e\u0007\u001e"+
		"\u0002\u001f\u0007\u001f\u0002 \u0007 \u0002!\u0007!\u0001\u0000\u0001"+
		"\u0000\u0005\u0000G\b\u0000\n\u0000\f\u0000J\t\u0000\u0001\u0000\u0001"+
		"\u0000\u0001\u0001\u0001\u0001\u0001\u0001\u0003\u0001Q\b\u0001\u0001"+
		"\u0001\u0003\u0001T\b\u0001\u0001\u0002\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0003\u0002g\b\u0002\u0001\u0003\u0001\u0003\u0001"+
		"\u0003\u0001\u0003\u0001\u0004\u0001\u0004\u0001\u0004\u0003\u0004p\b"+
		"\u0004\u0001\u0004\u0001\u0004\u0001\u0005\u0001\u0005\u0001\u0005\u0001"+
		"\u0005\u0001\u0006\u0001\u0006\u0001\u0006\u0001\u0006\u0003\u0006|\b"+
		"\u0006\u0001\u0006\u0001\u0006\u0001\u0006\u0001\u0007\u0001\u0007\u0001"+
		"\u0007\u0001\u0007\u0003\u0007\u0085\b\u0007\u0001\u0007\u0001\u0007\u0001"+
		"\u0007\u0001\b\u0001\b\u0001\b\u0005\b\u008d\b\b\n\b\f\b\u0090\t\b\u0001"+
		"\t\u0001\t\u0001\t\u0001\t\u0001\t\u0001\t\u0003\t\u0098\b\t\u0001\n\u0001"+
		"\n\u0001\n\u0001\n\u0003\n\u009e\b\n\u0001\n\u0001\n\u0001\n\u0005\n\u00a3"+
		"\b\n\n\n\f\n\u00a6\t\n\u0001\n\u0001\n\u0001\n\u0001\u000b\u0001\u000b"+
		"\u0001\u000b\u0001\u000b\u0003\u000b\u00af\b\u000b\u0001\u000b\u0001\u000b"+
		"\u0001\u000b\u0005\u000b\u00b4\b\u000b\n\u000b\f\u000b\u00b7\t\u000b\u0001"+
		"\u000b\u0001\u000b\u0001\u000b\u0001\f\u0003\f\u00bd\b\f\u0001\f\u0001"+
		"\f\u0005\f\u00c1\b\f\n\f\f\f\u00c4\t\f\u0001\f\u0003\f\u00c7\b\f\u0001"+
		"\f\u0003\f\u00ca\b\f\u0001\r\u0001\r\u0003\r\u00ce\b\r\u0001\u000e\u0001"+
		"\u000e\u0001\u000e\u0001\u000e\u0001\u000f\u0001\u000f\u0001\u000f\u0001"+
		"\u000f\u0001\u0010\u0001\u0010\u0001\u0010\u0001\u0010\u0001\u0011\u0001"+
		"\u0011\u0001\u0011\u0005\u0011\u00df\b\u0011\n\u0011\f\u0011\u00e2\t\u0011"+
		"\u0001\u0012\u0001\u0012\u0001\u0012\u0001\u0012\u0001\u0013\u0001\u0013"+
		"\u0001\u0013\u0001\u0013\u0001\u0014\u0001\u0014\u0001\u0014\u0001\u0014"+
		"\u0001\u0014\u0005\u0014\u00f1\b\u0014\n\u0014\f\u0014\u00f4\t\u0014\u0001"+
		"\u0014\u0003\u0014\u00f7\b\u0014\u0001\u0014\u0001\u0014\u0001\u0014\u0001"+
		"\u0015\u0001\u0015\u0001\u0015\u0001\u0015\u0001\u0015\u0001\u0016\u0001"+
		"\u0016\u0001\u0016\u0001\u0016\u0001\u0017\u0001\u0017\u0003\u0017\u0107"+
		"\b\u0017\u0001\u0017\u0003\u0017\u010a\b\u0017\u0001\u0017\u0001\u0017"+
		"\u0003\u0017\u010e\b\u0017\u0001\u0017\u0003\u0017\u0111\b\u0017\u0001"+
		"\u0017\u0001\u0017\u0001\u0017\u0001\u0017\u0001\u0017\u0001\u0018\u0001"+
		"\u0018\u0001\u0018\u0001\u0018\u0001\u0018\u0001\u0018\u0001\u0018\u0001"+
		"\u0019\u0001\u0019\u0001\u0019\u0001\u0019\u0001\u0019\u0001\u0019\u0001"+
		"\u0019\u0001\u001a\u0001\u001a\u0001\u001a\u0001\u001a\u0001\u001a\u0005"+
		"\u001a\u012b\b\u001a\n\u001a\f\u001a\u012e\t\u001a\u0001\u001a\u0003\u001a"+
		"\u0131\b\u001a\u0001\u001a\u0001\u001a\u0001\u001a\u0001\u001b\u0001\u001b"+
		"\u0001\u001b\u0001\u001b\u0001\u001b\u0001\u001c\u0001\u001c\u0001\u001c"+
		"\u0001\u001c\u0001\u001d\u0001\u001d\u0005\u001d\u0141\b\u001d\n\u001d"+
		"\f\u001d\u0144\t\u001d\u0001\u001e\u0001\u001e\u0001\u001e\u0001\u001e"+
		"\u0003\u001e\u014a\b\u001e\u0001\u001f\u0001\u001f\u0003\u001f\u014e\b"+
		"\u001f\u0001 \u0001 \u0001!\u0001!\u0001!\u0000\u0000\"\u0000\u0002\u0004"+
		"\u0006\b\n\f\u000e\u0010\u0012\u0014\u0016\u0018\u001a\u001c\u001e \""+
		"$&(*,.02468:<>@B\u0000\u0003\u0001\u0000()\u0001\u0000*/\u0001\u0000%"+
		"(\u0164\u0000H\u0001\u0000\u0000\u0000\u0002S\u0001\u0000\u0000\u0000"+
		"\u0004f\u0001\u0000\u0000\u0000\u0006h\u0001\u0000\u0000\u0000\bl\u0001"+
		"\u0000\u0000\u0000\ns\u0001\u0000\u0000\u0000\fw\u0001\u0000\u0000\u0000"+
		"\u000e\u0080\u0001\u0000\u0000\u0000\u0010\u0089\u0001\u0000\u0000\u0000"+
		"\u0012\u0097\u0001\u0000\u0000\u0000\u0014\u0099\u0001\u0000\u0000\u0000"+
		"\u0016\u00aa\u0001\u0000\u0000\u0000\u0018\u00bc\u0001\u0000\u0000\u0000"+
		"\u001a\u00cd\u0001\u0000\u0000\u0000\u001c\u00cf\u0001\u0000\u0000\u0000"+
		"\u001e\u00d3\u0001\u0000\u0000\u0000 \u00d7\u0001\u0000\u0000\u0000\""+
		"\u00db\u0001\u0000\u0000\u0000$\u00e3\u0001\u0000\u0000\u0000&\u00e7\u0001"+
		"\u0000\u0000\u0000(\u00eb\u0001\u0000\u0000\u0000*\u00fb\u0001\u0000\u0000"+
		"\u0000,\u0100\u0001\u0000\u0000\u0000.\u0104\u0001\u0000\u0000\u00000"+
		"\u0117\u0001\u0000\u0000\u00002\u011e\u0001\u0000\u0000\u00004\u0125\u0001"+
		"\u0000\u0000\u00006\u0135\u0001\u0000\u0000\u00008\u013a\u0001\u0000\u0000"+
		"\u0000:\u0142\u0001\u0000\u0000\u0000<\u0145\u0001\u0000\u0000\u0000>"+
		"\u014d\u0001\u0000\u0000\u0000@\u014f\u0001\u0000\u0000\u0000B\u0151\u0001"+
		"\u0000\u0000\u0000DG\u0003\u0004\u0002\u0000EG\u0003\u0002\u0001\u0000"+
		"FD\u0001\u0000\u0000\u0000FE\u0001\u0000\u0000\u0000GJ\u0001\u0000\u0000"+
		"\u0000HF\u0001\u0000\u0000\u0000HI\u0001\u0000\u0000\u0000IK\u0001\u0000"+
		"\u0000\u0000JH\u0001\u0000\u0000\u0000KL\u0005\u0000\u0000\u0001L\u0001"+
		"\u0001\u0000\u0000\u0000MN\u0005$\u0000\u0000NT\u0005\u0004\u0000\u0000"+
		"OQ\u00057\u0000\u0000PO\u0001\u0000\u0000\u0000PQ\u0001\u0000\u0000\u0000"+
		"QR\u0001\u0000\u0000\u0000RT\u0005\u0004\u0000\u0000SM\u0001\u0000\u0000"+
		"\u0000SP\u0001\u0000\u0000\u0000T\u0003\u0001\u0000\u0000\u0000Ug\u0003"+
		"\u0006\u0003\u0000Vg\u0003\b\u0004\u0000Wg\u0003\n\u0005\u0000Xg\u0003"+
		"\f\u0006\u0000Yg\u0003\u0014\n\u0000Zg\u0003\u0016\u000b\u0000[g\u0003"+
		"\u000e\u0007\u0000\\g\u0003\u001c\u000e\u0000]g\u0003\u001e\u000f\u0000"+
		"^g\u0003 \u0010\u0000_g\u0003$\u0012\u0000`g\u0003&\u0013\u0000ag\u0003"+
		"(\u0014\u0000bg\u0003.\u0017\u0000cg\u00030\u0018\u0000dg\u00032\u0019"+
		"\u0000eg\u00034\u001a\u0000fU\u0001\u0000\u0000\u0000fV\u0001\u0000\u0000"+
		"\u0000fW\u0001\u0000\u0000\u0000fX\u0001\u0000\u0000\u0000fY\u0001\u0000"+
		"\u0000\u0000fZ\u0001\u0000\u0000\u0000f[\u0001\u0000\u0000\u0000f\\\u0001"+
		"\u0000\u0000\u0000f]\u0001\u0000\u0000\u0000f^\u0001\u0000\u0000\u0000"+
		"f_\u0001\u0000\u0000\u0000f`\u0001\u0000\u0000\u0000fa\u0001\u0000\u0000"+
		"\u0000fb\u0001\u0000\u0000\u0000fc\u0001\u0000\u0000\u0000fd\u0001\u0000"+
		"\u0000\u0000fe\u0001\u0000\u0000\u0000g\u0005\u0001\u0000\u0000\u0000"+
		"hi\u0005\u0005\u0000\u0000ij\u0007\u0000\u0000\u0000jk\u0005\u0004\u0000"+
		"\u0000k\u0007\u0001\u0000\u0000\u0000lm\u0005\u0006\u0000\u0000mo\u0005"+
		"$\u0000\u0000np\u0003B!\u0000on\u0001\u0000\u0000\u0000op\u0001\u0000"+
		"\u0000\u0000pq\u0001\u0000\u0000\u0000qr\u0005\u0004\u0000\u0000r\t\u0001"+
		"\u0000\u0000\u0000st\u0005\u0007\u0000\u0000tu\u0005$\u0000\u0000uv\u0005"+
		"\u0004\u0000\u0000v\u000b\u0001\u0000\u0000\u0000wx\u0005\b\u0000\u0000"+
		"xy\u0005$\u0000\u0000y{\u00051\u0000\u0000z|\u0003\u0010\b\u0000{z\u0001"+
		"\u0000\u0000\u0000{|\u0001\u0000\u0000\u0000|}\u0001\u0000\u0000\u0000"+
		"}~\u00052\u0000\u0000~\u007f\u0005\u0004\u0000\u0000\u007f\r\u0001\u0000"+
		"\u0000\u0000\u0080\u0081\u0005\r\u0000\u0000\u0081\u0082\u0005$\u0000"+
		"\u0000\u0082\u0084\u00051\u0000\u0000\u0083\u0085\u0003\u0010\b\u0000"+
		"\u0084\u0083\u0001\u0000\u0000\u0000\u0084\u0085\u0001\u0000\u0000\u0000"+
		"\u0085\u0086\u0001\u0000\u0000\u0000\u0086\u0087\u00052\u0000\u0000\u0087"+
		"\u0088\u0005\u0004\u0000\u0000\u0088\u000f\u0001\u0000\u0000\u0000\u0089"+
		"\u008e\u0003\u0012\t\u0000\u008a\u008b\u00050\u0000\u0000\u008b\u008d"+
		"\u0003\u0012\t\u0000\u008c\u008a\u0001\u0000\u0000\u0000\u008d\u0090\u0001"+
		"\u0000\u0000\u0000\u008e\u008c\u0001\u0000\u0000\u0000\u008e\u008f\u0001"+
		"\u0000\u0000\u0000\u008f\u0011\u0001\u0000\u0000\u0000\u0090\u008e\u0001"+
		"\u0000\u0000\u0000\u0091\u0098\u0003B!\u0000\u0092\u0098\u0003@ \u0000"+
		"\u0093\u0098\u0005#\u0000\u0000\u0094\u0098\u0005$\u0000\u0000\u0095\u0098"+
		"\u0005!\u0000\u0000\u0096\u0098\u0005\"\u0000\u0000\u0097\u0091\u0001"+
		"\u0000\u0000\u0000\u0097\u0092\u0001\u0000\u0000\u0000\u0097\u0093\u0001"+
		"\u0000\u0000\u0000\u0097\u0094\u0001\u0000\u0000\u0000\u0097\u0095\u0001"+
		"\u0000\u0000\u0000\u0097\u0096\u0001\u0000\u0000\u0000\u0098\u0013\u0001"+
		"\u0000\u0000\u0000\u0099\u009a\u0005\t\u0000\u0000\u009a\u009b\u0005$"+
		"\u0000\u0000\u009b\u009d\u00051\u0000\u0000\u009c\u009e\u0003\u0018\f"+
		"\u0000\u009d\u009c\u0001\u0000\u0000\u0000\u009d\u009e\u0001\u0000\u0000"+
		"\u0000\u009e\u009f\u0001\u0000\u0000\u0000\u009f\u00a0\u00052\u0000\u0000"+
		"\u00a0\u00a4\u0005\u0004\u0000\u0000\u00a1\u00a3\u0003\u001a\r\u0000\u00a2"+
		"\u00a1\u0001\u0000\u0000\u0000\u00a3\u00a6\u0001\u0000\u0000\u0000\u00a4"+
		"\u00a2\u0001\u0000\u0000\u0000\u00a4\u00a5\u0001\u0000\u0000\u0000\u00a5"+
		"\u00a7\u0001\u0000\u0000\u0000\u00a6\u00a4\u0001\u0000\u0000\u0000\u00a7"+
		"\u00a8\u0005\n\u0000\u0000\u00a8\u00a9\u0005\u0004\u0000\u0000\u00a9\u0015"+
		"\u0001\u0000\u0000\u0000\u00aa\u00ab\u0005\u000b\u0000\u0000\u00ab\u00ac"+
		"\u0005$\u0000\u0000\u00ac\u00ae\u00051\u0000\u0000\u00ad\u00af\u0003\u0018"+
		"\f\u0000\u00ae\u00ad\u0001\u0000\u0000\u0000\u00ae\u00af\u0001\u0000\u0000"+
		"\u0000\u00af\u00b0\u0001\u0000\u0000\u0000\u00b0\u00b1\u00052\u0000\u0000"+
		"\u00b1\u00b5\u0005\u0004\u0000\u0000\u00b2\u00b4\u0003\u001a\r\u0000\u00b3"+
		"\u00b2\u0001\u0000\u0000\u0000\u00b4\u00b7\u0001\u0000\u0000\u0000\u00b5"+
		"\u00b3\u0001\u0000\u0000\u0000\u00b5\u00b6\u0001\u0000\u0000\u0000\u00b6"+
		"\u00b8\u0001\u0000\u0000\u0000\u00b7\u00b5\u0001\u0000\u0000\u0000\u00b8"+
		"\u00b9\u0005\f\u0000\u0000\u00b9\u00ba\u0005\u0004\u0000\u0000\u00ba\u0017"+
		"\u0001\u0000\u0000\u0000\u00bb\u00bd\u0005$\u0000\u0000\u00bc\u00bb\u0001"+
		"\u0000\u0000\u0000\u00bc\u00bd\u0001\u0000\u0000\u0000\u00bd\u00c2\u0001"+
		"\u0000\u0000\u0000\u00be\u00bf\u00050\u0000\u0000\u00bf\u00c1\u0005$\u0000"+
		"\u0000\u00c0\u00be\u0001\u0000\u0000\u0000\u00c1\u00c4\u0001\u0000\u0000"+
		"\u0000\u00c2\u00c0\u0001\u0000\u0000\u0000\u00c2\u00c3\u0001\u0000\u0000"+
		"\u0000\u00c3\u00c9\u0001\u0000\u0000\u0000\u00c4\u00c2\u0001\u0000\u0000"+
		"\u0000\u00c5\u00c7\u00050\u0000\u0000\u00c6\u00c5\u0001\u0000\u0000\u0000"+
		"\u00c6\u00c7\u0001\u0000\u0000\u0000\u00c7\u00c8\u0001\u0000\u0000\u0000"+
		"\u00c8\u00ca\u00056\u0000\u0000\u00c9\u00c6\u0001\u0000\u0000\u0000\u00c9"+
		"\u00ca\u0001\u0000\u0000\u0000\u00ca\u0019\u0001\u0000\u0000\u0000\u00cb"+
		"\u00ce\u0003\u0004\u0002\u0000\u00cc\u00ce\u0003\u0002\u0001\u0000\u00cd"+
		"\u00cb\u0001\u0000\u0000\u0000\u00cd\u00cc\u0001\u0000\u0000\u0000\u00ce"+
		"\u001b\u0001\u0000\u0000\u0000\u00cf\u00d0\u0005\u000e\u0000\u0000\u00d0"+
		"\u00d1\u0003\"\u0011\u0000\u00d1\u00d2\u0005\u0004\u0000\u0000\u00d2\u001d"+
		"\u0001\u0000\u0000\u0000\u00d3\u00d4\u0005\u000f\u0000\u0000\u00d4\u00d5"+
		"\u0003\"\u0011\u0000\u00d5\u00d6\u0005\u0004\u0000\u0000\u00d6\u001f\u0001"+
		"\u0000\u0000\u0000\u00d7\u00d8\u0005\u0010\u0000\u0000\u00d8\u00d9\u0003"+
		"\"\u0011\u0000\u00d9\u00da\u0005\u0004\u0000\u0000\u00da!\u0001\u0000"+
		"\u0000\u0000\u00db\u00e0\u0005$\u0000\u0000\u00dc\u00dd\u00050\u0000\u0000"+
		"\u00dd\u00df\u0005$\u0000\u0000\u00de\u00dc\u0001\u0000\u0000\u0000\u00df"+
		"\u00e2\u0001\u0000\u0000\u0000\u00e0\u00de\u0001\u0000\u0000\u0000\u00e0"+
		"\u00e1\u0001\u0000\u0000\u0000\u00e1#\u0001\u0000\u0000\u0000\u00e2\u00e0"+
		"\u0001\u0000\u0000\u0000\u00e3\u00e4\u0005\u0011\u0000\u0000\u00e4\u00e5"+
		"\u0003>\u001f\u0000\u00e5\u00e6\u0005\u0004\u0000\u0000\u00e6%\u0001\u0000"+
		"\u0000\u0000\u00e7\u00e8\u0005\u0012\u0000\u0000\u00e8\u00e9\u0003>\u001f"+
		"\u0000\u00e9\u00ea\u0005\u0004\u0000\u0000\u00ea\'\u0001\u0000\u0000\u0000"+
		"\u00eb\u00ec\u0005\u0013\u0000\u0000\u00ec\u00ed\u0003<\u001e\u0000\u00ed"+
		"\u00ee\u0005\u0004\u0000\u0000\u00ee\u00f2\u0003:\u001d\u0000\u00ef\u00f1"+
		"\u0003*\u0015\u0000\u00f0\u00ef\u0001\u0000\u0000\u0000\u00f1\u00f4\u0001"+
		"\u0000\u0000\u0000\u00f2\u00f0\u0001\u0000\u0000\u0000\u00f2\u00f3\u0001"+
		"\u0000\u0000\u0000\u00f3\u00f6\u0001\u0000\u0000\u0000\u00f4\u00f2\u0001"+
		"\u0000\u0000\u0000\u00f5\u00f7\u0003,\u0016\u0000\u00f6\u00f5\u0001\u0000"+
		"\u0000\u0000\u00f6\u00f7\u0001\u0000\u0000\u0000\u00f7\u00f8\u0001\u0000"+
		"\u0000\u0000\u00f8\u00f9\u0005\u0016\u0000\u0000\u00f9\u00fa\u0005\u0004"+
		"\u0000\u0000\u00fa)\u0001\u0000\u0000\u0000\u00fb\u00fc\u0005\u0014\u0000"+
		"\u0000\u00fc\u00fd\u0003<\u001e\u0000\u00fd\u00fe\u0005\u0004\u0000\u0000"+
		"\u00fe\u00ff\u0003:\u001d\u0000\u00ff+\u0001\u0000\u0000\u0000\u0100\u0101"+
		"\u0005\u0015\u0000\u0000\u0101\u0102\u0005\u0004\u0000\u0000\u0102\u0103"+
		"\u0003:\u001d\u0000\u0103-\u0001\u0000\u0000\u0000\u0104\u0106\u0005\u0017"+
		"\u0000\u0000\u0105\u0107\u0003>\u001f\u0000\u0106\u0105\u0001\u0000\u0000"+
		"\u0000\u0106\u0107\u0001\u0000\u0000\u0000\u0107\u0109\u0001\u0000\u0000"+
		"\u0000\u0108\u010a\u00050\u0000\u0000\u0109\u0108\u0001\u0000\u0000\u0000"+
		"\u0109\u010a\u0001\u0000\u0000\u0000\u010a\u010b\u0001\u0000\u0000\u0000"+
		"\u010b\u010d\u0003<\u001e\u0000\u010c\u010e\u00050\u0000\u0000\u010d\u010c"+
		"\u0001\u0000\u0000\u0000\u010d\u010e\u0001\u0000\u0000\u0000\u010e\u0110"+
		"\u0001\u0000\u0000\u0000\u010f\u0111\u0003>\u001f\u0000\u0110\u010f\u0001"+
		"\u0000\u0000\u0000\u0110\u0111\u0001\u0000\u0000\u0000\u0111\u0112\u0001"+
		"\u0000\u0000\u0000\u0112\u0113\u0005\u0004\u0000\u0000\u0113\u0114\u0003"+
		":\u001d\u0000\u0114\u0115\u0005\u0018\u0000\u0000\u0115\u0116\u0005\u0004"+
		"\u0000\u0000\u0116/\u0001\u0000\u0000\u0000\u0117\u0118\u0005\u0019\u0000"+
		"\u0000\u0118\u0119\u0003<\u001e\u0000\u0119\u011a\u0005\u0004\u0000\u0000"+
		"\u011a\u011b\u0003:\u001d\u0000\u011b\u011c\u0005\u001a\u0000\u0000\u011c"+
		"\u011d\u0005\u0004\u0000\u0000\u011d1\u0001\u0000\u0000\u0000\u011e\u011f"+
		"\u0005\u001b\u0000\u0000\u011f\u0120\u0005\u0004\u0000\u0000\u0120\u0121"+
		"\u0003:\u001d\u0000\u0121\u0122\u0005\u001c\u0000\u0000\u0122\u0123\u0003"+
		"<\u001e\u0000\u0123\u0124\u0005\u0004\u0000\u0000\u01243\u0001\u0000\u0000"+
		"\u0000\u0125\u0126\u0005\u001d\u0000\u0000\u0126\u0127\u0003<\u001e\u0000"+
		"\u0127\u0128\u0005\u0004\u0000\u0000\u0128\u012c\u0003:\u001d\u0000\u0129"+
		"\u012b\u00036\u001b\u0000\u012a\u0129\u0001\u0000\u0000\u0000\u012b\u012e"+
		"\u0001\u0000\u0000\u0000\u012c\u012a\u0001\u0000\u0000\u0000\u012c\u012d"+
		"\u0001\u0000\u0000\u0000\u012d\u0130\u0001\u0000\u0000\u0000\u012e\u012c"+
		"\u0001\u0000\u0000\u0000\u012f\u0131\u00038\u001c\u0000\u0130\u012f\u0001"+
		"\u0000\u0000\u0000\u0130\u0131\u0001\u0000\u0000\u0000\u0131\u0132\u0001"+
		"\u0000\u0000\u0000\u0132\u0133\u0005 \u0000\u0000\u0133\u0134\u0005\u0004"+
		"\u0000\u0000\u01345\u0001\u0000\u0000\u0000\u0135\u0136\u0005\u001e\u0000"+
		"\u0000\u0136\u0137\u0003<\u001e\u0000\u0137\u0138\u0005\u0004\u0000\u0000"+
		"\u0138\u0139\u0003:\u001d\u0000\u01397\u0001\u0000\u0000\u0000\u013a\u013b"+
		"\u0005\u001f\u0000\u0000\u013b\u013c\u0005\u0004\u0000\u0000\u013c\u013d"+
		"\u0003:\u001d\u0000\u013d9\u0001\u0000\u0000\u0000\u013e\u0141\u0003\u0004"+
		"\u0002\u0000\u013f\u0141\u0003\u0002\u0001\u0000\u0140\u013e\u0001\u0000"+
		"\u0000\u0000\u0140\u013f\u0001\u0000\u0000\u0000\u0141\u0144\u0001\u0000"+
		"\u0000\u0000\u0142\u0140\u0001\u0000\u0000\u0000\u0142\u0143\u0001\u0000"+
		"\u0000\u0000\u0143;\u0001\u0000\u0000\u0000\u0144\u0142\u0001\u0000\u0000"+
		"\u0000\u0145\u0149\u0003>\u001f\u0000\u0146\u0147\u0003@ \u0000\u0147"+
		"\u0148\u0003>\u001f\u0000\u0148\u014a\u0001\u0000\u0000\u0000\u0149\u0146"+
		"\u0001\u0000\u0000\u0000\u0149\u014a\u0001\u0000\u0000\u0000\u014a=\u0001"+
		"\u0000\u0000\u0000\u014b\u014e\u0005$\u0000\u0000\u014c\u014e\u0003B!"+
		"\u0000\u014d\u014b\u0001\u0000\u0000\u0000\u014d\u014c\u0001\u0000\u0000"+
		"\u0000\u014e?\u0001\u0000\u0000\u0000\u014f\u0150\u0007\u0001\u0000\u0000"+
		"\u0150A\u0001\u0000\u0000\u0000\u0151\u0152\u0007\u0002\u0000\u0000\u0152"+
		"C\u0001\u0000\u0000\u0000 FHPSfo{\u0084\u008e\u0097\u009d\u00a4\u00ae"+
		"\u00b5\u00bc\u00c2\u00c6\u00c9\u00cd\u00e0\u00f2\u00f6\u0106\u0109\u010d"+
		"\u0110\u012c\u0130\u0140\u0142\u0149\u014d";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}