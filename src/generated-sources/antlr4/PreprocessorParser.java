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
		PP_FRETURN=18, PP_IF=19, PP_ELSEIF=20, PP_ELSE=21, PP_ENDIF=22, REG_R=23, 
		REG_F=24, PLACEHOLDER=25, IDENT=26, INT=27, FLOAT=28, CHAR=29, STRING=30, 
		ANGLE_PATH=31, EQEQ=32, NEQ=33, LE=34, GE=35, LT=36, GT=37, COMMA=38, 
		LPAREN=39, RPAREN=40, DOLLAR=41, LCURLY=42, RCURLY=43, CODE_TEXT=44, LINE_COMMENT_D=45, 
		WS_D=46;
	public static final int
		RULE_preproc = 0, RULE_codeLine = 1, RULE_directive = 2, RULE_includeDir = 3, 
		RULE_defineDir = 4, RULE_undefDir = 5, RULE_callDir = 6, RULE_macroDir = 7, 
		RULE_argList = 8, RULE_callArg = 9, RULE_defFuncDir = 10, RULE_defMacroDir = 11, 
		RULE_paramList = 12, RULE_codeLineOrDirective = 13, RULE_svarDir = 14, 
		RULE_varDir = 15, RULE_fvarDir = 16, RULE_identList = 17, RULE_returnDir = 18, 
		RULE_freturnDir = 19, RULE_ifBlock = 20, RULE_elseifClause = 21, RULE_elseClause = 22, 
		RULE_block = 23, RULE_expr = 24, RULE_primary = 25, RULE_cmpOp = 26, RULE_literal = 27;
	private static String[] makeRuleNames() {
		return new String[] {
			"preproc", "codeLine", "directive", "includeDir", "defineDir", "undefDir", 
			"callDir", "macroDir", "argList", "callArg", "defFuncDir", "defMacroDir", 
			"paramList", "codeLineOrDirective", "svarDir", "varDir", "fvarDir", "identList", 
			"returnDir", "freturnDir", "ifBlock", "elseifClause", "elseClause", "block", 
			"expr", "primary", "cmpOp", "literal"
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
			"PP_FRETURN", "PP_IF", "PP_ELSEIF", "PP_ELSE", "PP_ENDIF", "REG_R", "REG_F", 
			"PLACEHOLDER", "IDENT", "INT", "FLOAT", "CHAR", "STRING", "ANGLE_PATH", 
			"EQEQ", "NEQ", "LE", "GE", "LT", "GT", "COMMA", "LPAREN", "RPAREN", "DOLLAR", 
			"LCURLY", "RCURLY", "CODE_TEXT", "LINE_COMMENT_D", "WS_D"
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
			setState(60);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 17592254196720L) != 0)) {
				{
				setState(58);
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
					{
					setState(56);
					directive();
					}
					break;
				case NL:
				case IDENT:
				case CODE_TEXT:
					{
					setState(57);
					codeLine();
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
				setState(62);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(63);
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
			setState(71);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case IDENT:
				enterOuterAlt(_localctx, 1);
				{
				setState(65);
				match(IDENT);
				setState(66);
				match(NL);
				}
				break;
			case NL:
			case CODE_TEXT:
				enterOuterAlt(_localctx, 2);
				{
				setState(68);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if (_la==CODE_TEXT) {
					{
					setState(67);
					match(CODE_TEXT);
					}
				}

				setState(70);
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
			setState(86);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case PP_INCLUDE:
				enterOuterAlt(_localctx, 1);
				{
				setState(73);
				includeDir();
				}
				break;
			case PP_DEFINE:
				enterOuterAlt(_localctx, 2);
				{
				setState(74);
				defineDir();
				}
				break;
			case PP_UNDEF:
				enterOuterAlt(_localctx, 3);
				{
				setState(75);
				undefDir();
				}
				break;
			case PP_CALL:
				enterOuterAlt(_localctx, 4);
				{
				setState(76);
				callDir();
				}
				break;
			case PP_DEF_FUNC:
				enterOuterAlt(_localctx, 5);
				{
				setState(77);
				defFuncDir();
				}
				break;
			case PP_DEF_MACRO:
				enterOuterAlt(_localctx, 6);
				{
				setState(78);
				defMacroDir();
				}
				break;
			case PP_MACRO:
				enterOuterAlt(_localctx, 7);
				{
				setState(79);
				macroDir();
				}
				break;
			case PP_SVAR:
				enterOuterAlt(_localctx, 8);
				{
				setState(80);
				svarDir();
				}
				break;
			case PP_VAR:
				enterOuterAlt(_localctx, 9);
				{
				setState(81);
				varDir();
				}
				break;
			case PP_FVAR:
				enterOuterAlt(_localctx, 10);
				{
				setState(82);
				fvarDir();
				}
				break;
			case PP_RETURN:
				enterOuterAlt(_localctx, 11);
				{
				setState(83);
				returnDir();
				}
				break;
			case PP_FRETURN:
				enterOuterAlt(_localctx, 12);
				{
				setState(84);
				freturnDir();
				}
				break;
			case PP_IF:
				enterOuterAlt(_localctx, 13);
				{
				setState(85);
				ifBlock();
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
			setState(88);
			match(PP_INCLUDE);
			setState(89);
			_la = _input.LA(1);
			if ( !(_la==STRING || _la==ANGLE_PATH) ) {
			_errHandler.recoverInline(this);
			}
			else {
				if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
				_errHandler.reportMatch(this);
				consume();
			}
			setState(90);
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
			setState(92);
			match(PP_DEFINE);
			setState(93);
			match(IDENT);
			setState(95);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 2013265920L) != 0)) {
				{
				setState(94);
				literal();
				}
			}

			setState(97);
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
			setState(99);
			match(PP_UNDEF);
			setState(100);
			match(IDENT);
			setState(101);
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
			setState(103);
			match(PP_CALL);
			setState(104);
			match(IDENT);
			setState(105);
			match(LPAREN);
			setState(107);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 2139095040L) != 0)) {
				{
				setState(106);
				argList();
				}
			}

			setState(109);
			match(RPAREN);
			setState(110);
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
			setState(112);
			match(PP_MACRO);
			setState(113);
			match(IDENT);
			setState(114);
			match(LPAREN);
			setState(116);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 2139095040L) != 0)) {
				{
				setState(115);
				argList();
				}
			}

			setState(118);
			match(RPAREN);
			setState(119);
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
			setState(121);
			callArg();
			setState(126);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==COMMA) {
				{
				{
				setState(122);
				match(COMMA);
				setState(123);
				callArg();
				}
				}
				setState(128);
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
			setState(134);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case INT:
			case FLOAT:
			case CHAR:
			case STRING:
				enterOuterAlt(_localctx, 1);
				{
				setState(129);
				literal();
				}
				break;
			case PLACEHOLDER:
				enterOuterAlt(_localctx, 2);
				{
				setState(130);
				match(PLACEHOLDER);
				}
				break;
			case IDENT:
				enterOuterAlt(_localctx, 3);
				{
				setState(131);
				match(IDENT);
				}
				break;
			case REG_R:
				enterOuterAlt(_localctx, 4);
				{
				setState(132);
				match(REG_R);
				}
				break;
			case REG_F:
				enterOuterAlt(_localctx, 5);
				{
				setState(133);
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
			setState(136);
			match(PP_DEF_FUNC);
			setState(137);
			match(IDENT);
			setState(138);
			match(LPAREN);
			setState(140);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==IDENT) {
				{
				setState(139);
				paramList();
				}
			}

			setState(142);
			match(RPAREN);
			setState(143);
			match(NL);
			setState(147);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 17592254196720L) != 0)) {
				{
				{
				setState(144);
				codeLineOrDirective();
				}
				}
				setState(149);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(150);
			match(PP_END_FUNC);
			setState(151);
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
			setState(153);
			match(PP_DEF_MACRO);
			setState(154);
			match(IDENT);
			setState(155);
			match(LPAREN);
			setState(157);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==IDENT) {
				{
				setState(156);
				paramList();
				}
			}

			setState(159);
			match(RPAREN);
			setState(160);
			match(NL);
			setState(164);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 17592254196720L) != 0)) {
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
			match(PP_END_MACRO);
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
	public static class ParamListContext extends ParserRuleContext {
		public List<TerminalNode> IDENT() { return getTokens(PreprocessorParser.IDENT); }
		public TerminalNode IDENT(int i) {
			return getToken(PreprocessorParser.IDENT, i);
		}
		public List<TerminalNode> COMMA() { return getTokens(PreprocessorParser.COMMA); }
		public TerminalNode COMMA(int i) {
			return getToken(PreprocessorParser.COMMA, i);
		}
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
			enterOuterAlt(_localctx, 1);
			{
			setState(170);
			match(IDENT);
			setState(175);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==COMMA) {
				{
				{
				setState(171);
				match(COMMA);
				setState(172);
				match(IDENT);
				}
				}
				setState(177);
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
			setState(180);
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
				enterOuterAlt(_localctx, 1);
				{
				setState(178);
				directive();
				}
				break;
			case NL:
			case IDENT:
			case CODE_TEXT:
				enterOuterAlt(_localctx, 2);
				{
				setState(179);
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
			setState(182);
			match(PP_SVAR);
			setState(183);
			identList();
			setState(184);
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
			setState(186);
			match(PP_VAR);
			setState(187);
			identList();
			setState(188);
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
			setState(190);
			match(PP_FVAR);
			setState(191);
			identList();
			setState(192);
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
			setState(194);
			match(IDENT);
			setState(199);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==COMMA) {
				{
				{
				setState(195);
				match(COMMA);
				setState(196);
				match(IDENT);
				}
				}
				setState(201);
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
			setState(202);
			match(PP_RETURN);
			setState(203);
			primary();
			setState(204);
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
			setState(206);
			match(PP_FRETURN);
			setState(207);
			primary();
			setState(208);
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
			setState(210);
			match(PP_IF);
			setState(211);
			expr();
			setState(212);
			match(NL);
			setState(213);
			block();
			setState(217);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==PP_ELSEIF) {
				{
				{
				setState(214);
				elseifClause();
				}
				}
				setState(219);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(221);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==PP_ELSE) {
				{
				setState(220);
				elseClause();
				}
			}

			setState(223);
			match(PP_ENDIF);
			setState(224);
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
			setState(226);
			match(PP_ELSEIF);
			setState(227);
			expr();
			setState(228);
			match(NL);
			setState(229);
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
			setState(231);
			match(PP_ELSE);
			setState(232);
			match(NL);
			setState(233);
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
		enterRule(_localctx, 46, RULE_block);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(239);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 17592254196720L) != 0)) {
				{
				setState(237);
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
					{
					setState(235);
					directive();
					}
					break;
				case NL:
				case IDENT:
				case CODE_TEXT:
					{
					setState(236);
					codeLine();
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
				setState(241);
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
		enterRule(_localctx, 48, RULE_expr);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(242);
			primary();
			setState(246);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 270582939648L) != 0)) {
				{
				setState(243);
				cmpOp();
				setState(244);
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
		enterRule(_localctx, 50, RULE_primary);
		try {
			setState(250);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case IDENT:
				enterOuterAlt(_localctx, 1);
				{
				setState(248);
				match(IDENT);
				}
				break;
			case INT:
			case FLOAT:
			case CHAR:
			case STRING:
				enterOuterAlt(_localctx, 2);
				{
				setState(249);
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
		enterRule(_localctx, 52, RULE_cmpOp);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(252);
			_la = _input.LA(1);
			if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 270582939648L) != 0)) ) {
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
		enterRule(_localctx, 54, RULE_literal);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(254);
			_la = _input.LA(1);
			if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 2013265920L) != 0)) ) {
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
		"\u0004\u0001.\u0101\u0002\u0000\u0007\u0000\u0002\u0001\u0007\u0001\u0002"+
		"\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0002\u0004\u0007\u0004\u0002"+
		"\u0005\u0007\u0005\u0002\u0006\u0007\u0006\u0002\u0007\u0007\u0007\u0002"+
		"\b\u0007\b\u0002\t\u0007\t\u0002\n\u0007\n\u0002\u000b\u0007\u000b\u0002"+
		"\f\u0007\f\u0002\r\u0007\r\u0002\u000e\u0007\u000e\u0002\u000f\u0007\u000f"+
		"\u0002\u0010\u0007\u0010\u0002\u0011\u0007\u0011\u0002\u0012\u0007\u0012"+
		"\u0002\u0013\u0007\u0013\u0002\u0014\u0007\u0014\u0002\u0015\u0007\u0015"+
		"\u0002\u0016\u0007\u0016\u0002\u0017\u0007\u0017\u0002\u0018\u0007\u0018"+
		"\u0002\u0019\u0007\u0019\u0002\u001a\u0007\u001a\u0002\u001b\u0007\u001b"+
		"\u0001\u0000\u0001\u0000\u0005\u0000;\b\u0000\n\u0000\f\u0000>\t\u0000"+
		"\u0001\u0000\u0001\u0000\u0001\u0001\u0001\u0001\u0001\u0001\u0003\u0001"+
		"E\b\u0001\u0001\u0001\u0003\u0001H\b\u0001\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0003\u0002W\b"+
		"\u0002\u0001\u0003\u0001\u0003\u0001\u0003\u0001\u0003\u0001\u0004\u0001"+
		"\u0004\u0001\u0004\u0003\u0004`\b\u0004\u0001\u0004\u0001\u0004\u0001"+
		"\u0005\u0001\u0005\u0001\u0005\u0001\u0005\u0001\u0006\u0001\u0006\u0001"+
		"\u0006\u0001\u0006\u0003\u0006l\b\u0006\u0001\u0006\u0001\u0006\u0001"+
		"\u0006\u0001\u0007\u0001\u0007\u0001\u0007\u0001\u0007\u0003\u0007u\b"+
		"\u0007\u0001\u0007\u0001\u0007\u0001\u0007\u0001\b\u0001\b\u0001\b\u0005"+
		"\b}\b\b\n\b\f\b\u0080\t\b\u0001\t\u0001\t\u0001\t\u0001\t\u0001\t\u0003"+
		"\t\u0087\b\t\u0001\n\u0001\n\u0001\n\u0001\n\u0003\n\u008d\b\n\u0001\n"+
		"\u0001\n\u0001\n\u0005\n\u0092\b\n\n\n\f\n\u0095\t\n\u0001\n\u0001\n\u0001"+
		"\n\u0001\u000b\u0001\u000b\u0001\u000b\u0001\u000b\u0003\u000b\u009e\b"+
		"\u000b\u0001\u000b\u0001\u000b\u0001\u000b\u0005\u000b\u00a3\b\u000b\n"+
		"\u000b\f\u000b\u00a6\t\u000b\u0001\u000b\u0001\u000b\u0001\u000b\u0001"+
		"\f\u0001\f\u0001\f\u0005\f\u00ae\b\f\n\f\f\f\u00b1\t\f\u0001\r\u0001\r"+
		"\u0003\r\u00b5\b\r\u0001\u000e\u0001\u000e\u0001\u000e\u0001\u000e\u0001"+
		"\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001\u0010\u0001\u0010\u0001"+
		"\u0010\u0001\u0010\u0001\u0011\u0001\u0011\u0001\u0011\u0005\u0011\u00c6"+
		"\b\u0011\n\u0011\f\u0011\u00c9\t\u0011\u0001\u0012\u0001\u0012\u0001\u0012"+
		"\u0001\u0012\u0001\u0013\u0001\u0013\u0001\u0013\u0001\u0013\u0001\u0014"+
		"\u0001\u0014\u0001\u0014\u0001\u0014\u0001\u0014\u0005\u0014\u00d8\b\u0014"+
		"\n\u0014\f\u0014\u00db\t\u0014\u0001\u0014\u0003\u0014\u00de\b\u0014\u0001"+
		"\u0014\u0001\u0014\u0001\u0014\u0001\u0015\u0001\u0015\u0001\u0015\u0001"+
		"\u0015\u0001\u0015\u0001\u0016\u0001\u0016\u0001\u0016\u0001\u0016\u0001"+
		"\u0017\u0001\u0017\u0005\u0017\u00ee\b\u0017\n\u0017\f\u0017\u00f1\t\u0017"+
		"\u0001\u0018\u0001\u0018\u0001\u0018\u0001\u0018\u0003\u0018\u00f7\b\u0018"+
		"\u0001\u0019\u0001\u0019\u0003\u0019\u00fb\b\u0019\u0001\u001a\u0001\u001a"+
		"\u0001\u001b\u0001\u001b\u0001\u001b\u0000\u0000\u001c\u0000\u0002\u0004"+
		"\u0006\b\n\f\u000e\u0010\u0012\u0014\u0016\u0018\u001a\u001c\u001e \""+
		"$&(*,.0246\u0000\u0003\u0001\u0000\u001e\u001f\u0001\u0000 %\u0001\u0000"+
		"\u001b\u001e\u0109\u0000<\u0001\u0000\u0000\u0000\u0002G\u0001\u0000\u0000"+
		"\u0000\u0004V\u0001\u0000\u0000\u0000\u0006X\u0001\u0000\u0000\u0000\b"+
		"\\\u0001\u0000\u0000\u0000\nc\u0001\u0000\u0000\u0000\fg\u0001\u0000\u0000"+
		"\u0000\u000ep\u0001\u0000\u0000\u0000\u0010y\u0001\u0000\u0000\u0000\u0012"+
		"\u0086\u0001\u0000\u0000\u0000\u0014\u0088\u0001\u0000\u0000\u0000\u0016"+
		"\u0099\u0001\u0000\u0000\u0000\u0018\u00aa\u0001\u0000\u0000\u0000\u001a"+
		"\u00b4\u0001\u0000\u0000\u0000\u001c\u00b6\u0001\u0000\u0000\u0000\u001e"+
		"\u00ba\u0001\u0000\u0000\u0000 \u00be\u0001\u0000\u0000\u0000\"\u00c2"+
		"\u0001\u0000\u0000\u0000$\u00ca\u0001\u0000\u0000\u0000&\u00ce\u0001\u0000"+
		"\u0000\u0000(\u00d2\u0001\u0000\u0000\u0000*\u00e2\u0001\u0000\u0000\u0000"+
		",\u00e7\u0001\u0000\u0000\u0000.\u00ef\u0001\u0000\u0000\u00000\u00f2"+
		"\u0001\u0000\u0000\u00002\u00fa\u0001\u0000\u0000\u00004\u00fc\u0001\u0000"+
		"\u0000\u00006\u00fe\u0001\u0000\u0000\u00008;\u0003\u0004\u0002\u0000"+
		"9;\u0003\u0002\u0001\u0000:8\u0001\u0000\u0000\u0000:9\u0001\u0000\u0000"+
		"\u0000;>\u0001\u0000\u0000\u0000<:\u0001\u0000\u0000\u0000<=\u0001\u0000"+
		"\u0000\u0000=?\u0001\u0000\u0000\u0000><\u0001\u0000\u0000\u0000?@\u0005"+
		"\u0000\u0000\u0001@\u0001\u0001\u0000\u0000\u0000AB\u0005\u001a\u0000"+
		"\u0000BH\u0005\u0004\u0000\u0000CE\u0005,\u0000\u0000DC\u0001\u0000\u0000"+
		"\u0000DE\u0001\u0000\u0000\u0000EF\u0001\u0000\u0000\u0000FH\u0005\u0004"+
		"\u0000\u0000GA\u0001\u0000\u0000\u0000GD\u0001\u0000\u0000\u0000H\u0003"+
		"\u0001\u0000\u0000\u0000IW\u0003\u0006\u0003\u0000JW\u0003\b\u0004\u0000"+
		"KW\u0003\n\u0005\u0000LW\u0003\f\u0006\u0000MW\u0003\u0014\n\u0000NW\u0003"+
		"\u0016\u000b\u0000OW\u0003\u000e\u0007\u0000PW\u0003\u001c\u000e\u0000"+
		"QW\u0003\u001e\u000f\u0000RW\u0003 \u0010\u0000SW\u0003$\u0012\u0000T"+
		"W\u0003&\u0013\u0000UW\u0003(\u0014\u0000VI\u0001\u0000\u0000\u0000VJ"+
		"\u0001\u0000\u0000\u0000VK\u0001\u0000\u0000\u0000VL\u0001\u0000\u0000"+
		"\u0000VM\u0001\u0000\u0000\u0000VN\u0001\u0000\u0000\u0000VO\u0001\u0000"+
		"\u0000\u0000VP\u0001\u0000\u0000\u0000VQ\u0001\u0000\u0000\u0000VR\u0001"+
		"\u0000\u0000\u0000VS\u0001\u0000\u0000\u0000VT\u0001\u0000\u0000\u0000"+
		"VU\u0001\u0000\u0000\u0000W\u0005\u0001\u0000\u0000\u0000XY\u0005\u0005"+
		"\u0000\u0000YZ\u0007\u0000\u0000\u0000Z[\u0005\u0004\u0000\u0000[\u0007"+
		"\u0001\u0000\u0000\u0000\\]\u0005\u0006\u0000\u0000]_\u0005\u001a\u0000"+
		"\u0000^`\u00036\u001b\u0000_^\u0001\u0000\u0000\u0000_`\u0001\u0000\u0000"+
		"\u0000`a\u0001\u0000\u0000\u0000ab\u0005\u0004\u0000\u0000b\t\u0001\u0000"+
		"\u0000\u0000cd\u0005\u0007\u0000\u0000de\u0005\u001a\u0000\u0000ef\u0005"+
		"\u0004\u0000\u0000f\u000b\u0001\u0000\u0000\u0000gh\u0005\b\u0000\u0000"+
		"hi\u0005\u001a\u0000\u0000ik\u0005\'\u0000\u0000jl\u0003\u0010\b\u0000"+
		"kj\u0001\u0000\u0000\u0000kl\u0001\u0000\u0000\u0000lm\u0001\u0000\u0000"+
		"\u0000mn\u0005(\u0000\u0000no\u0005\u0004\u0000\u0000o\r\u0001\u0000\u0000"+
		"\u0000pq\u0005\r\u0000\u0000qr\u0005\u001a\u0000\u0000rt\u0005\'\u0000"+
		"\u0000su\u0003\u0010\b\u0000ts\u0001\u0000\u0000\u0000tu\u0001\u0000\u0000"+
		"\u0000uv\u0001\u0000\u0000\u0000vw\u0005(\u0000\u0000wx\u0005\u0004\u0000"+
		"\u0000x\u000f\u0001\u0000\u0000\u0000y~\u0003\u0012\t\u0000z{\u0005&\u0000"+
		"\u0000{}\u0003\u0012\t\u0000|z\u0001\u0000\u0000\u0000}\u0080\u0001\u0000"+
		"\u0000\u0000~|\u0001\u0000\u0000\u0000~\u007f\u0001\u0000\u0000\u0000"+
		"\u007f\u0011\u0001\u0000\u0000\u0000\u0080~\u0001\u0000\u0000\u0000\u0081"+
		"\u0087\u00036\u001b\u0000\u0082\u0087\u0005\u0019\u0000\u0000\u0083\u0087"+
		"\u0005\u001a\u0000\u0000\u0084\u0087\u0005\u0017\u0000\u0000\u0085\u0087"+
		"\u0005\u0018\u0000\u0000\u0086\u0081\u0001\u0000\u0000\u0000\u0086\u0082"+
		"\u0001\u0000\u0000\u0000\u0086\u0083\u0001\u0000\u0000\u0000\u0086\u0084"+
		"\u0001\u0000\u0000\u0000\u0086\u0085\u0001\u0000\u0000\u0000\u0087\u0013"+
		"\u0001\u0000\u0000\u0000\u0088\u0089\u0005\t\u0000\u0000\u0089\u008a\u0005"+
		"\u001a\u0000\u0000\u008a\u008c\u0005\'\u0000\u0000\u008b\u008d\u0003\u0018"+
		"\f\u0000\u008c\u008b\u0001\u0000\u0000\u0000\u008c\u008d\u0001\u0000\u0000"+
		"\u0000\u008d\u008e\u0001\u0000\u0000\u0000\u008e\u008f\u0005(\u0000\u0000"+
		"\u008f\u0093\u0005\u0004\u0000\u0000\u0090\u0092\u0003\u001a\r\u0000\u0091"+
		"\u0090\u0001\u0000\u0000\u0000\u0092\u0095\u0001\u0000\u0000\u0000\u0093"+
		"\u0091\u0001\u0000\u0000\u0000\u0093\u0094\u0001\u0000\u0000\u0000\u0094"+
		"\u0096\u0001\u0000\u0000\u0000\u0095\u0093\u0001\u0000\u0000\u0000\u0096"+
		"\u0097\u0005\n\u0000\u0000\u0097\u0098\u0005\u0004\u0000\u0000\u0098\u0015"+
		"\u0001\u0000\u0000\u0000\u0099\u009a\u0005\u000b\u0000\u0000\u009a\u009b"+
		"\u0005\u001a\u0000\u0000\u009b\u009d\u0005\'\u0000\u0000\u009c\u009e\u0003"+
		"\u0018\f\u0000\u009d\u009c\u0001\u0000\u0000\u0000\u009d\u009e\u0001\u0000"+
		"\u0000\u0000\u009e\u009f\u0001\u0000\u0000\u0000\u009f\u00a0\u0005(\u0000"+
		"\u0000\u00a0\u00a4\u0005\u0004\u0000\u0000\u00a1\u00a3\u0003\u001a\r\u0000"+
		"\u00a2\u00a1\u0001\u0000\u0000\u0000\u00a3\u00a6\u0001\u0000\u0000\u0000"+
		"\u00a4\u00a2\u0001\u0000\u0000\u0000\u00a4\u00a5\u0001\u0000\u0000\u0000"+
		"\u00a5\u00a7\u0001\u0000\u0000\u0000\u00a6\u00a4\u0001\u0000\u0000\u0000"+
		"\u00a7\u00a8\u0005\f\u0000\u0000\u00a8\u00a9\u0005\u0004\u0000\u0000\u00a9"+
		"\u0017\u0001\u0000\u0000\u0000\u00aa\u00af\u0005\u001a\u0000\u0000\u00ab"+
		"\u00ac\u0005&\u0000\u0000\u00ac\u00ae\u0005\u001a\u0000\u0000\u00ad\u00ab"+
		"\u0001\u0000\u0000\u0000\u00ae\u00b1\u0001\u0000\u0000\u0000\u00af\u00ad"+
		"\u0001\u0000\u0000\u0000\u00af\u00b0\u0001\u0000\u0000\u0000\u00b0\u0019"+
		"\u0001\u0000\u0000\u0000\u00b1\u00af\u0001\u0000\u0000\u0000\u00b2\u00b5"+
		"\u0003\u0004\u0002\u0000\u00b3\u00b5\u0003\u0002\u0001\u0000\u00b4\u00b2"+
		"\u0001\u0000\u0000\u0000\u00b4\u00b3\u0001\u0000\u0000\u0000\u00b5\u001b"+
		"\u0001\u0000\u0000\u0000\u00b6\u00b7\u0005\u000e\u0000\u0000\u00b7\u00b8"+
		"\u0003\"\u0011\u0000\u00b8\u00b9\u0005\u0004\u0000\u0000\u00b9\u001d\u0001"+
		"\u0000\u0000\u0000\u00ba\u00bb\u0005\u000f\u0000\u0000\u00bb\u00bc\u0003"+
		"\"\u0011\u0000\u00bc\u00bd\u0005\u0004\u0000\u0000\u00bd\u001f\u0001\u0000"+
		"\u0000\u0000\u00be\u00bf\u0005\u0010\u0000\u0000\u00bf\u00c0\u0003\"\u0011"+
		"\u0000\u00c0\u00c1\u0005\u0004\u0000\u0000\u00c1!\u0001\u0000\u0000\u0000"+
		"\u00c2\u00c7\u0005\u001a\u0000\u0000\u00c3\u00c4\u0005&\u0000\u0000\u00c4"+
		"\u00c6\u0005\u001a\u0000\u0000\u00c5\u00c3\u0001\u0000\u0000\u0000\u00c6"+
		"\u00c9\u0001\u0000\u0000\u0000\u00c7\u00c5\u0001\u0000\u0000\u0000\u00c7"+
		"\u00c8\u0001\u0000\u0000\u0000\u00c8#\u0001\u0000\u0000\u0000\u00c9\u00c7"+
		"\u0001\u0000\u0000\u0000\u00ca\u00cb\u0005\u0011\u0000\u0000\u00cb\u00cc"+
		"\u00032\u0019\u0000\u00cc\u00cd\u0005\u0004\u0000\u0000\u00cd%\u0001\u0000"+
		"\u0000\u0000\u00ce\u00cf\u0005\u0012\u0000\u0000\u00cf\u00d0\u00032\u0019"+
		"\u0000\u00d0\u00d1\u0005\u0004\u0000\u0000\u00d1\'\u0001\u0000\u0000\u0000"+
		"\u00d2\u00d3\u0005\u0013\u0000\u0000\u00d3\u00d4\u00030\u0018\u0000\u00d4"+
		"\u00d5\u0005\u0004\u0000\u0000\u00d5\u00d9\u0003.\u0017\u0000\u00d6\u00d8"+
		"\u0003*\u0015\u0000\u00d7\u00d6\u0001\u0000\u0000\u0000\u00d8\u00db\u0001"+
		"\u0000\u0000\u0000\u00d9\u00d7\u0001\u0000\u0000\u0000\u00d9\u00da\u0001"+
		"\u0000\u0000\u0000\u00da\u00dd\u0001\u0000\u0000\u0000\u00db\u00d9\u0001"+
		"\u0000\u0000\u0000\u00dc\u00de\u0003,\u0016\u0000\u00dd\u00dc\u0001\u0000"+
		"\u0000\u0000\u00dd\u00de\u0001\u0000\u0000\u0000\u00de\u00df\u0001\u0000"+
		"\u0000\u0000\u00df\u00e0\u0005\u0016\u0000\u0000\u00e0\u00e1\u0005\u0004"+
		"\u0000\u0000\u00e1)\u0001\u0000\u0000\u0000\u00e2\u00e3\u0005\u0014\u0000"+
		"\u0000\u00e3\u00e4\u00030\u0018\u0000\u00e4\u00e5\u0005\u0004\u0000\u0000"+
		"\u00e5\u00e6\u0003.\u0017\u0000\u00e6+\u0001\u0000\u0000\u0000\u00e7\u00e8"+
		"\u0005\u0015\u0000\u0000\u00e8\u00e9\u0005\u0004\u0000\u0000\u00e9\u00ea"+
		"\u0003.\u0017\u0000\u00ea-\u0001\u0000\u0000\u0000\u00eb\u00ee\u0003\u0004"+
		"\u0002\u0000\u00ec\u00ee\u0003\u0002\u0001\u0000\u00ed\u00eb\u0001\u0000"+
		"\u0000\u0000\u00ed\u00ec\u0001\u0000\u0000\u0000\u00ee\u00f1\u0001\u0000"+
		"\u0000\u0000\u00ef\u00ed\u0001\u0000\u0000\u0000\u00ef\u00f0\u0001\u0000"+
		"\u0000\u0000\u00f0/\u0001\u0000\u0000\u0000\u00f1\u00ef\u0001\u0000\u0000"+
		"\u0000\u00f2\u00f6\u00032\u0019\u0000\u00f3\u00f4\u00034\u001a\u0000\u00f4"+
		"\u00f5\u00032\u0019\u0000\u00f5\u00f7\u0001\u0000\u0000\u0000\u00f6\u00f3"+
		"\u0001\u0000\u0000\u0000\u00f6\u00f7\u0001\u0000\u0000\u0000\u00f71\u0001"+
		"\u0000\u0000\u0000\u00f8\u00fb\u0005\u001a\u0000\u0000\u00f9\u00fb\u0003"+
		"6\u001b\u0000\u00fa\u00f8\u0001\u0000\u0000\u0000\u00fa\u00f9\u0001\u0000"+
		"\u0000\u0000\u00fb3\u0001\u0000\u0000\u0000\u00fc\u00fd\u0007\u0001\u0000"+
		"\u0000\u00fd5\u0001\u0000\u0000\u0000\u00fe\u00ff\u0007\u0002\u0000\u0000"+
		"\u00ff7\u0001\u0000\u0000\u0000\u0017:<DGV_kt~\u0086\u008c\u0093\u009d"+
		"\u00a4\u00af\u00b4\u00c7\u00d9\u00dd\u00ed\u00ef\u00f6\u00fa";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}