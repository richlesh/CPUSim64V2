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
		PP_END_MACRO=12, PP_MACRO=13, PP_GLOBAL=14, PP_SVAR=15, PP_VAR=16, PP_FVAR=17, 
		PP_RETURN=18, PP_FRETURN=19, PP_IF=20, PP_IFDEF=21, PP_IFNDEF=22, PP_ELSEIF=23, 
		PP_ELSE=24, PP_ENDIF=25, PP_FOR=26, PP_ENDFOR=27, PP_WHILE=28, PP_ENDWHILE=29, 
		PP_DOWHILE=30, PP_ENDDOWHILE=31, PP_BREAK=32, PP_CONTINUE=33, PP_IFCONDSR=34, 
		PP_IFCOND=35, PP_ELSEIFCOND=36, PP_ELSECOND=37, PP_ENDCOND=38, REG_R=39, 
		REG_F=40, PLACEHOLDER=41, IDENT=42, INT=43, FLOAT=44, CHAR=45, STRING=46, 
		ANGLE_PATH=47, EQEQ=48, NEQ=49, LE=50, GE=51, LT=52, GT=53, COMMA=54, 
		LPAREN=55, RPAREN=56, DOLLAR=57, LCURLY=58, RCURLY=59, ELLIPSIS=60, COLON=61, 
		CODE_TEXT=62, LINE_COMMENT_D=63, WS_D=64;
	public static final int
		RULE_preproc = 0, RULE_codeLine = 1, RULE_directive = 2, RULE_includeDir = 3, 
		RULE_defineDir = 4, RULE_undefDir = 5, RULE_callDir = 6, RULE_macroDir = 7, 
		RULE_argList = 8, RULE_callArg = 9, RULE_defFuncDir = 10, RULE_defMacroDir = 11, 
		RULE_paramList = 12, RULE_codeLineOrDirective = 13, RULE_globalDir = 14, 
		RULE_svarDir = 15, RULE_varDir = 16, RULE_fvarDir = 17, RULE_identList = 18, 
		RULE_returnDir = 19, RULE_freturnDir = 20, RULE_ifBlock = 21, RULE_ifDefBlock = 22, 
		RULE_ifNDefBlock = 23, RULE_elseifClause = 24, RULE_elseClause = 25, RULE_forBlock = 26, 
		RULE_whileBlock = 27, RULE_doWhileBlock = 28, RULE_breakDir = 29, RULE_continueDir = 30, 
		RULE_ifCondBlock = 31, RULE_ifCondSRBlock = 32, RULE_elseifCondClause = 33, 
		RULE_elseCondClause = 34, RULE_block = 35, RULE_expr = 36, RULE_primary = 37, 
		RULE_cmpOp = 38, RULE_literal = 39;
	private static String[] makeRuleNames() {
		return new String[] {
			"preproc", "codeLine", "directive", "includeDir", "defineDir", "undefDir", 
			"callDir", "macroDir", "argList", "callArg", "defFuncDir", "defMacroDir", 
			"paramList", "codeLineOrDirective", "globalDir", "svarDir", "varDir", 
			"fvarDir", "identList", "returnDir", "freturnDir", "ifBlock", "ifDefBlock", 
			"ifNDefBlock", "elseifClause", "elseClause", "forBlock", "whileBlock", 
			"doWhileBlock", "breakDir", "continueDir", "ifCondBlock", "ifCondSRBlock", 
			"elseifCondClause", "elseCondClause", "block", "expr", "primary", "cmpOp", 
			"literal"
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
			"PP_END_MACRO", "PP_MACRO", "PP_GLOBAL", "PP_SVAR", "PP_VAR", "PP_FVAR", 
			"PP_RETURN", "PP_FRETURN", "PP_IF", "PP_IFDEF", "PP_IFNDEF", "PP_ELSEIF", 
			"PP_ELSE", "PP_ENDIF", "PP_FOR", "PP_ENDFOR", "PP_WHILE", "PP_ENDWHILE", 
			"PP_DOWHILE", "PP_ENDDOWHILE", "PP_BREAK", "PP_CONTINUE", "PP_IFCONDSR", 
			"PP_IFCOND", "PP_ELSEIFCOND", "PP_ELSECOND", "PP_ENDCOND", "REG_R", "REG_F", 
			"PLACEHOLDER", "IDENT", "INT", "FLOAT", "CHAR", "STRING", "ANGLE_PATH", 
			"EQEQ", "NEQ", "LE", "GE", "LT", "GT", "COMMA", "LPAREN", "RPAREN", "DOLLAR", 
			"LCURLY", "RCURLY", "ELLIPSIS", "COLON", "CODE_TEXT", "LINE_COMMENT_D", 
			"WS_D"
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
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
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
			setState(85);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 4611690482316078064L) != 0)) {
				{
				setState(83);
				_errHandler.sync(this);
				switch (_input.LA(1)) {
				case PP_INCLUDE:
				case PP_DEFINE:
				case PP_UNDEF:
				case PP_CALL:
				case PP_DEF_FUNC:
				case PP_DEF_MACRO:
				case PP_MACRO:
				case PP_GLOBAL:
				case PP_SVAR:
				case PP_VAR:
				case PP_FVAR:
				case PP_RETURN:
				case PP_FRETURN:
				case PP_IF:
				case PP_IFDEF:
				case PP_IFNDEF:
				case PP_FOR:
				case PP_WHILE:
				case PP_DOWHILE:
				case PP_BREAK:
				case PP_CONTINUE:
				case PP_IFCONDSR:
				case PP_IFCOND:
					{
					setState(80);
					directive();
					}
					break;
				case IDENT:
				case CODE_TEXT:
					{
					setState(81);
					codeLine();
					}
					break;
				case NL:
					{
					setState(82);
					match(NL);
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
				setState(87);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(88);
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
		try {
			setState(94);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case IDENT:
				enterOuterAlt(_localctx, 1);
				{
				setState(90);
				match(IDENT);
				setState(91);
				match(NL);
				}
				break;
			case CODE_TEXT:
				enterOuterAlt(_localctx, 2);
				{
				setState(92);
				match(CODE_TEXT);
				setState(93);
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
		public GlobalDirContext globalDir() {
			return getRuleContext(GlobalDirContext.class,0);
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
		public IfDefBlockContext ifDefBlock() {
			return getRuleContext(IfDefBlockContext.class,0);
		}
		public IfNDefBlockContext ifNDefBlock() {
			return getRuleContext(IfNDefBlockContext.class,0);
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
		public BreakDirContext breakDir() {
			return getRuleContext(BreakDirContext.class,0);
		}
		public ContinueDirContext continueDir() {
			return getRuleContext(ContinueDirContext.class,0);
		}
		public IfCondBlockContext ifCondBlock() {
			return getRuleContext(IfCondBlockContext.class,0);
		}
		public IfCondSRBlockContext ifCondSRBlock() {
			return getRuleContext(IfCondSRBlockContext.class,0);
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
			setState(119);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case PP_INCLUDE:
				enterOuterAlt(_localctx, 1);
				{
				setState(96);
				includeDir();
				}
				break;
			case PP_DEFINE:
				enterOuterAlt(_localctx, 2);
				{
				setState(97);
				defineDir();
				}
				break;
			case PP_UNDEF:
				enterOuterAlt(_localctx, 3);
				{
				setState(98);
				undefDir();
				}
				break;
			case PP_CALL:
				enterOuterAlt(_localctx, 4);
				{
				setState(99);
				callDir();
				}
				break;
			case PP_DEF_FUNC:
				enterOuterAlt(_localctx, 5);
				{
				setState(100);
				defFuncDir();
				}
				break;
			case PP_DEF_MACRO:
				enterOuterAlt(_localctx, 6);
				{
				setState(101);
				defMacroDir();
				}
				break;
			case PP_MACRO:
				enterOuterAlt(_localctx, 7);
				{
				setState(102);
				macroDir();
				}
				break;
			case PP_GLOBAL:
				enterOuterAlt(_localctx, 8);
				{
				setState(103);
				globalDir();
				}
				break;
			case PP_SVAR:
				enterOuterAlt(_localctx, 9);
				{
				setState(104);
				svarDir();
				}
				break;
			case PP_VAR:
				enterOuterAlt(_localctx, 10);
				{
				setState(105);
				varDir();
				}
				break;
			case PP_FVAR:
				enterOuterAlt(_localctx, 11);
				{
				setState(106);
				fvarDir();
				}
				break;
			case PP_RETURN:
				enterOuterAlt(_localctx, 12);
				{
				setState(107);
				returnDir();
				}
				break;
			case PP_FRETURN:
				enterOuterAlt(_localctx, 13);
				{
				setState(108);
				freturnDir();
				}
				break;
			case PP_IF:
				enterOuterAlt(_localctx, 14);
				{
				setState(109);
				ifBlock();
				}
				break;
			case PP_IFDEF:
				enterOuterAlt(_localctx, 15);
				{
				setState(110);
				ifDefBlock();
				}
				break;
			case PP_IFNDEF:
				enterOuterAlt(_localctx, 16);
				{
				setState(111);
				ifNDefBlock();
				}
				break;
			case PP_FOR:
				enterOuterAlt(_localctx, 17);
				{
				setState(112);
				forBlock();
				}
				break;
			case PP_WHILE:
				enterOuterAlt(_localctx, 18);
				{
				setState(113);
				whileBlock();
				}
				break;
			case PP_DOWHILE:
				enterOuterAlt(_localctx, 19);
				{
				setState(114);
				doWhileBlock();
				}
				break;
			case PP_BREAK:
				enterOuterAlt(_localctx, 20);
				{
				setState(115);
				breakDir();
				}
				break;
			case PP_CONTINUE:
				enterOuterAlt(_localctx, 21);
				{
				setState(116);
				continueDir();
				}
				break;
			case PP_IFCOND:
				enterOuterAlt(_localctx, 22);
				{
				setState(117);
				ifCondBlock();
				}
				break;
			case PP_IFCONDSR:
				enterOuterAlt(_localctx, 23);
				{
				setState(118);
				ifCondSRBlock();
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
			setState(121);
			match(PP_INCLUDE);
			setState(122);
			_la = _input.LA(1);
			if ( !(_la==STRING || _la==ANGLE_PATH) ) {
			_errHandler.recoverInline(this);
			}
			else {
				if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
				_errHandler.reportMatch(this);
				consume();
			}
			setState(123);
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
			setState(125);
			match(PP_DEFINE);
			setState(126);
			match(IDENT);
			setState(128);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 131941395333120L) != 0)) {
				{
				setState(127);
				literal();
				}
			}

			setState(130);
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
			setState(132);
			match(PP_UNDEF);
			setState(133);
			match(IDENT);
			setState(134);
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
			setState(136);
			match(PP_CALL);
			setState(137);
			match(IDENT);
			setState(138);
			match(LPAREN);
			setState(140);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 17873111265312768L) != 0)) {
				{
				setState(139);
				argList();
				}
			}

			setState(142);
			match(RPAREN);
			setState(143);
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
			setState(145);
			match(PP_MACRO);
			setState(146);
			match(IDENT);
			setState(147);
			match(LPAREN);
			setState(149);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 17873111265312768L) != 0)) {
				{
				setState(148);
				argList();
				}
			}

			setState(151);
			match(RPAREN);
			setState(152);
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
			setState(154);
			callArg();
			setState(159);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==COMMA) {
				{
				{
				setState(155);
				match(COMMA);
				setState(156);
				callArg();
				}
				}
				setState(161);
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
			setState(168);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case INT:
			case FLOAT:
			case CHAR:
			case STRING:
				enterOuterAlt(_localctx, 1);
				{
				setState(162);
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
				setState(163);
				cmpOp();
				}
				break;
			case PLACEHOLDER:
				enterOuterAlt(_localctx, 3);
				{
				setState(164);
				match(PLACEHOLDER);
				}
				break;
			case IDENT:
				enterOuterAlt(_localctx, 4);
				{
				setState(165);
				match(IDENT);
				}
				break;
			case REG_R:
				enterOuterAlt(_localctx, 5);
				{
				setState(166);
				match(REG_R);
				}
				break;
			case REG_F:
				enterOuterAlt(_localctx, 6);
				{
				setState(167);
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
			setState(170);
			match(PP_DEF_FUNC);
			setState(171);
			match(IDENT);
			setState(172);
			match(LPAREN);
			setState(174);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,9,_ctx) ) {
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
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 4611690482316078064L) != 0)) {
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
			match(PP_END_FUNC);
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
			setState(187);
			match(PP_DEF_MACRO);
			setState(188);
			match(IDENT);
			setState(189);
			match(LPAREN);
			setState(191);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,11,_ctx) ) {
			case 1:
				{
				setState(190);
				paramList();
				}
				break;
			}
			setState(193);
			match(RPAREN);
			setState(194);
			match(NL);
			setState(198);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 4611690482316078064L) != 0)) {
				{
				{
				setState(195);
				codeLineOrDirective();
				}
				}
				setState(200);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(201);
			match(PP_END_MACRO);
			setState(202);
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
			setState(205);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==IDENT) {
				{
				setState(204);
				match(IDENT);
				}
			}

			setState(211);
			_errHandler.sync(this);
			_alt = getInterpreter().adaptivePredict(_input,14,_ctx);
			while ( _alt!=2 && _alt!=org.antlr.v4.runtime.atn.ATN.INVALID_ALT_NUMBER ) {
				if ( _alt==1 ) {
					{
					{
					setState(207);
					match(COMMA);
					setState(208);
					match(IDENT);
					}
					} 
				}
				setState(213);
				_errHandler.sync(this);
				_alt = getInterpreter().adaptivePredict(_input,14,_ctx);
			}
			setState(218);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==COMMA || _la==ELLIPSIS) {
				{
				setState(215);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if (_la==COMMA) {
					{
					setState(214);
					match(COMMA);
					}
				}

				setState(217);
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
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
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
			setState(223);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case PP_INCLUDE:
			case PP_DEFINE:
			case PP_UNDEF:
			case PP_CALL:
			case PP_DEF_FUNC:
			case PP_DEF_MACRO:
			case PP_MACRO:
			case PP_GLOBAL:
			case PP_SVAR:
			case PP_VAR:
			case PP_FVAR:
			case PP_RETURN:
			case PP_FRETURN:
			case PP_IF:
			case PP_IFDEF:
			case PP_IFNDEF:
			case PP_FOR:
			case PP_WHILE:
			case PP_DOWHILE:
			case PP_BREAK:
			case PP_CONTINUE:
			case PP_IFCONDSR:
			case PP_IFCOND:
				enterOuterAlt(_localctx, 1);
				{
				setState(220);
				directive();
				}
				break;
			case IDENT:
			case CODE_TEXT:
				enterOuterAlt(_localctx, 2);
				{
				setState(221);
				codeLine();
				}
				break;
			case NL:
				enterOuterAlt(_localctx, 3);
				{
				setState(222);
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
	public static class GlobalDirContext extends ParserRuleContext {
		public TerminalNode PP_GLOBAL() { return getToken(PreprocessorParser.PP_GLOBAL, 0); }
		public TerminalNode CODE_TEXT() { return getToken(PreprocessorParser.CODE_TEXT, 0); }
		public GlobalDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_globalDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterGlobalDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitGlobalDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitGlobalDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final GlobalDirContext globalDir() throws RecognitionException {
		GlobalDirContext _localctx = new GlobalDirContext(_ctx, getState());
		enterRule(_localctx, 28, RULE_globalDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(225);
			match(PP_GLOBAL);
			setState(226);
			match(CODE_TEXT);
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
		enterRule(_localctx, 30, RULE_svarDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(228);
			match(PP_SVAR);
			setState(229);
			identList();
			setState(230);
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
		enterRule(_localctx, 32, RULE_varDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(232);
			match(PP_VAR);
			setState(233);
			identList();
			setState(234);
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
		enterRule(_localctx, 34, RULE_fvarDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(236);
			match(PP_FVAR);
			setState(237);
			identList();
			setState(238);
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
		enterRule(_localctx, 36, RULE_identList);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(240);
			match(IDENT);
			setState(245);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==COMMA) {
				{
				{
				setState(241);
				match(COMMA);
				setState(242);
				match(IDENT);
				}
				}
				setState(247);
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
		enterRule(_localctx, 38, RULE_returnDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(248);
			match(PP_RETURN);
			setState(249);
			primary();
			setState(250);
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
		enterRule(_localctx, 40, RULE_freturnDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(252);
			match(PP_FRETURN);
			setState(253);
			primary();
			setState(254);
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
		enterRule(_localctx, 42, RULE_ifBlock);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(256);
			match(PP_IF);
			setState(257);
			expr();
			setState(258);
			match(NL);
			setState(259);
			block();
			setState(263);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==PP_ELSEIF) {
				{
				{
				setState(260);
				elseifClause();
				}
				}
				setState(265);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(267);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==PP_ELSE) {
				{
				setState(266);
				elseClause();
				}
			}

			setState(269);
			match(PP_ENDIF);
			setState(270);
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
	public static class IfDefBlockContext extends ParserRuleContext {
		public TerminalNode PP_IFDEF() { return getToken(PreprocessorParser.PP_IFDEF, 0); }
		public PrimaryContext primary() {
			return getRuleContext(PrimaryContext.class,0);
		}
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
		}
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public TerminalNode PP_ENDIF() { return getToken(PreprocessorParser.PP_ENDIF, 0); }
		public ElseClauseContext elseClause() {
			return getRuleContext(ElseClauseContext.class,0);
		}
		public IfDefBlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_ifDefBlock; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterIfDefBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitIfDefBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitIfDefBlock(this);
			else return visitor.visitChildren(this);
		}
	}

	public final IfDefBlockContext ifDefBlock() throws RecognitionException {
		IfDefBlockContext _localctx = new IfDefBlockContext(_ctx, getState());
		enterRule(_localctx, 44, RULE_ifDefBlock);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(272);
			match(PP_IFDEF);
			setState(273);
			primary();
			setState(274);
			match(NL);
			setState(275);
			block();
			setState(277);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==PP_ELSE) {
				{
				setState(276);
				elseClause();
				}
			}

			setState(279);
			match(PP_ENDIF);
			setState(280);
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
	public static class IfNDefBlockContext extends ParserRuleContext {
		public TerminalNode PP_IFNDEF() { return getToken(PreprocessorParser.PP_IFNDEF, 0); }
		public PrimaryContext primary() {
			return getRuleContext(PrimaryContext.class,0);
		}
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
		}
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public TerminalNode PP_ENDIF() { return getToken(PreprocessorParser.PP_ENDIF, 0); }
		public ElseClauseContext elseClause() {
			return getRuleContext(ElseClauseContext.class,0);
		}
		public IfNDefBlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_ifNDefBlock; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterIfNDefBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitIfNDefBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitIfNDefBlock(this);
			else return visitor.visitChildren(this);
		}
	}

	public final IfNDefBlockContext ifNDefBlock() throws RecognitionException {
		IfNDefBlockContext _localctx = new IfNDefBlockContext(_ctx, getState());
		enterRule(_localctx, 46, RULE_ifNDefBlock);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(282);
			match(PP_IFNDEF);
			setState(283);
			primary();
			setState(284);
			match(NL);
			setState(285);
			block();
			setState(287);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==PP_ELSE) {
				{
				setState(286);
				elseClause();
				}
			}

			setState(289);
			match(PP_ENDIF);
			setState(290);
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
		enterRule(_localctx, 48, RULE_elseifClause);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(292);
			match(PP_ELSEIF);
			setState(293);
			expr();
			setState(294);
			match(NL);
			setState(295);
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
		enterRule(_localctx, 50, RULE_elseClause);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(297);
			match(PP_ELSE);
			setState(298);
			match(NL);
			setState(299);
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
		enterRule(_localctx, 52, RULE_forBlock);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(301);
			match(PP_FOR);
			setState(303);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,23,_ctx) ) {
			case 1:
				{
				setState(302);
				((ForBlockContext)_localctx).init = primary();
				}
				break;
			}
			setState(306);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==COMMA) {
				{
				setState(305);
				match(COMMA);
				}
			}

			setState(308);
			((ForBlockContext)_localctx).cond = expr();
			setState(310);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==COMMA) {
				{
				setState(309);
				match(COMMA);
				}
			}

			setState(313);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 137988709285888L) != 0)) {
				{
				setState(312);
				((ForBlockContext)_localctx).incr = primary();
				}
			}

			setState(315);
			match(NL);
			setState(316);
			block();
			setState(317);
			match(PP_ENDFOR);
			setState(318);
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
		enterRule(_localctx, 54, RULE_whileBlock);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(320);
			match(PP_WHILE);
			setState(321);
			((WhileBlockContext)_localctx).cond = expr();
			setState(322);
			match(NL);
			setState(323);
			block();
			setState(324);
			match(PP_ENDWHILE);
			setState(325);
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
		enterRule(_localctx, 56, RULE_doWhileBlock);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(327);
			match(PP_DOWHILE);
			setState(328);
			match(NL);
			setState(329);
			block();
			setState(330);
			match(PP_ENDDOWHILE);
			setState(331);
			((DoWhileBlockContext)_localctx).cond = expr();
			setState(332);
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
	public static class BreakDirContext extends ParserRuleContext {
		public TerminalNode PP_BREAK() { return getToken(PreprocessorParser.PP_BREAK, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public BreakDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_breakDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterBreakDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitBreakDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitBreakDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final BreakDirContext breakDir() throws RecognitionException {
		BreakDirContext _localctx = new BreakDirContext(_ctx, getState());
		enterRule(_localctx, 58, RULE_breakDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(334);
			match(PP_BREAK);
			setState(335);
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
	public static class ContinueDirContext extends ParserRuleContext {
		public TerminalNode PP_CONTINUE() { return getToken(PreprocessorParser.PP_CONTINUE, 0); }
		public TerminalNode NL() { return getToken(PreprocessorParser.NL, 0); }
		public ContinueDirContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_continueDir; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterContinueDir(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitContinueDir(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitContinueDir(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ContinueDirContext continueDir() throws RecognitionException {
		ContinueDirContext _localctx = new ContinueDirContext(_ctx, getState());
		enterRule(_localctx, 60, RULE_continueDir);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(337);
			match(PP_CONTINUE);
			setState(338);
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
		enterRule(_localctx, 62, RULE_ifCondBlock);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(340);
			match(PP_IFCOND);
			setState(341);
			((IfCondBlockContext)_localctx).cond = expr();
			setState(342);
			match(NL);
			setState(343);
			block();
			setState(347);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==PP_ELSEIFCOND) {
				{
				{
				setState(344);
				elseifCondClause();
				}
				}
				setState(349);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(351);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==PP_ELSECOND) {
				{
				setState(350);
				elseCondClause();
				}
			}

			setState(353);
			match(PP_ENDCOND);
			setState(354);
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
	public static class IfCondSRBlockContext extends ParserRuleContext {
		public TerminalNode PP_IFCONDSR() { return getToken(PreprocessorParser.PP_IFCONDSR, 0); }
		public TerminalNode IDENT() { return getToken(PreprocessorParser.IDENT, 0); }
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
		}
		public BlockContext block() {
			return getRuleContext(BlockContext.class,0);
		}
		public TerminalNode PP_ENDCOND() { return getToken(PreprocessorParser.PP_ENDCOND, 0); }
		public ElseCondClauseContext elseCondClause() {
			return getRuleContext(ElseCondClauseContext.class,0);
		}
		public IfCondSRBlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_ifCondSRBlock; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).enterIfCondSRBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof PreprocessorParserListener ) ((PreprocessorParserListener)listener).exitIfCondSRBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof PreprocessorParserVisitor ) return ((PreprocessorParserVisitor<? extends T>)visitor).visitIfCondSRBlock(this);
			else return visitor.visitChildren(this);
		}
	}

	public final IfCondSRBlockContext ifCondSRBlock() throws RecognitionException {
		IfCondSRBlockContext _localctx = new IfCondSRBlockContext(_ctx, getState());
		enterRule(_localctx, 64, RULE_ifCondSRBlock);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(356);
			match(PP_IFCONDSR);
			setState(357);
			match(IDENT);
			setState(358);
			match(NL);
			setState(359);
			block();
			setState(361);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==PP_ELSECOND) {
				{
				setState(360);
				elseCondClause();
				}
			}

			setState(363);
			match(PP_ENDCOND);
			setState(364);
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
		enterRule(_localctx, 66, RULE_elseifCondClause);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(366);
			match(PP_ELSEIFCOND);
			setState(367);
			((ElseifCondClauseContext)_localctx).cond = expr();
			setState(368);
			match(NL);
			setState(369);
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
		enterRule(_localctx, 68, RULE_elseCondClause);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(371);
			match(PP_ELSECOND);
			setState(372);
			match(NL);
			setState(373);
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
		public List<TerminalNode> NL() { return getTokens(PreprocessorParser.NL); }
		public TerminalNode NL(int i) {
			return getToken(PreprocessorParser.NL, i);
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
		enterRule(_localctx, 70, RULE_block);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(380);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 4611690482316078064L) != 0)) {
				{
				setState(378);
				_errHandler.sync(this);
				switch (_input.LA(1)) {
				case PP_INCLUDE:
				case PP_DEFINE:
				case PP_UNDEF:
				case PP_CALL:
				case PP_DEF_FUNC:
				case PP_DEF_MACRO:
				case PP_MACRO:
				case PP_GLOBAL:
				case PP_SVAR:
				case PP_VAR:
				case PP_FVAR:
				case PP_RETURN:
				case PP_FRETURN:
				case PP_IF:
				case PP_IFDEF:
				case PP_IFNDEF:
				case PP_FOR:
				case PP_WHILE:
				case PP_DOWHILE:
				case PP_BREAK:
				case PP_CONTINUE:
				case PP_IFCONDSR:
				case PP_IFCOND:
					{
					setState(375);
					directive();
					}
					break;
				case IDENT:
				case CODE_TEXT:
					{
					setState(376);
					codeLine();
					}
					break;
				case NL:
					{
					setState(377);
					match(NL);
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
				setState(382);
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
		public List<TerminalNode> COMMA() { return getTokens(PreprocessorParser.COMMA); }
		public TerminalNode COMMA(int i) {
			return getToken(PreprocessorParser.COMMA, i);
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
		enterRule(_localctx, 72, RULE_expr);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(383);
			primary();
			setState(385);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,32,_ctx) ) {
			case 1:
				{
				setState(384);
				match(COMMA);
				}
				break;
			}
			setState(393);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 17732923532771328L) != 0)) {
				{
				setState(387);
				cmpOp();
				setState(389);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if (_la==COMMA) {
					{
					setState(388);
					match(COMMA);
					}
				}

				setState(391);
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
		public TerminalNode REG_R() { return getToken(PreprocessorParser.REG_R, 0); }
		public TerminalNode REG_F() { return getToken(PreprocessorParser.REG_F, 0); }
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
		enterRule(_localctx, 74, RULE_primary);
		try {
			setState(399);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case IDENT:
				enterOuterAlt(_localctx, 1);
				{
				setState(395);
				match(IDENT);
				}
				break;
			case REG_R:
				enterOuterAlt(_localctx, 2);
				{
				setState(396);
				match(REG_R);
				}
				break;
			case REG_F:
				enterOuterAlt(_localctx, 3);
				{
				setState(397);
				match(REG_F);
				}
				break;
			case INT:
			case FLOAT:
			case CHAR:
			case STRING:
				enterOuterAlt(_localctx, 4);
				{
				setState(398);
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
		enterRule(_localctx, 76, RULE_cmpOp);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(401);
			_la = _input.LA(1);
			if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 17732923532771328L) != 0)) ) {
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
		enterRule(_localctx, 78, RULE_literal);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(403);
			_la = _input.LA(1);
			if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 131941395333120L) != 0)) ) {
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
		"\u0004\u0001@\u0196\u0002\u0000\u0007\u0000\u0002\u0001\u0007\u0001\u0002"+
		"\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0002\u0004\u0007\u0004\u0002"+
		"\u0005\u0007\u0005\u0002\u0006\u0007\u0006\u0002\u0007\u0007\u0007\u0002"+
		"\b\u0007\b\u0002\t\u0007\t\u0002\n\u0007\n\u0002\u000b\u0007\u000b\u0002"+
		"\f\u0007\f\u0002\r\u0007\r\u0002\u000e\u0007\u000e\u0002\u000f\u0007\u000f"+
		"\u0002\u0010\u0007\u0010\u0002\u0011\u0007\u0011\u0002\u0012\u0007\u0012"+
		"\u0002\u0013\u0007\u0013\u0002\u0014\u0007\u0014\u0002\u0015\u0007\u0015"+
		"\u0002\u0016\u0007\u0016\u0002\u0017\u0007\u0017\u0002\u0018\u0007\u0018"+
		"\u0002\u0019\u0007\u0019\u0002\u001a\u0007\u001a\u0002\u001b\u0007\u001b"+
		"\u0002\u001c\u0007\u001c\u0002\u001d\u0007\u001d\u0002\u001e\u0007\u001e"+
		"\u0002\u001f\u0007\u001f\u0002 \u0007 \u0002!\u0007!\u0002\"\u0007\"\u0002"+
		"#\u0007#\u0002$\u0007$\u0002%\u0007%\u0002&\u0007&\u0002\'\u0007\'\u0001"+
		"\u0000\u0001\u0000\u0001\u0000\u0005\u0000T\b\u0000\n\u0000\f\u0000W\t"+
		"\u0000\u0001\u0000\u0001\u0000\u0001\u0001\u0001\u0001\u0001\u0001\u0001"+
		"\u0001\u0003\u0001_\b\u0001\u0001\u0002\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0003\u0002x\b\u0002\u0001\u0003\u0001\u0003\u0001"+
		"\u0003\u0001\u0003\u0001\u0004\u0001\u0004\u0001\u0004\u0003\u0004\u0081"+
		"\b\u0004\u0001\u0004\u0001\u0004\u0001\u0005\u0001\u0005\u0001\u0005\u0001"+
		"\u0005\u0001\u0006\u0001\u0006\u0001\u0006\u0001\u0006\u0003\u0006\u008d"+
		"\b\u0006\u0001\u0006\u0001\u0006\u0001\u0006\u0001\u0007\u0001\u0007\u0001"+
		"\u0007\u0001\u0007\u0003\u0007\u0096\b\u0007\u0001\u0007\u0001\u0007\u0001"+
		"\u0007\u0001\b\u0001\b\u0001\b\u0005\b\u009e\b\b\n\b\f\b\u00a1\t\b\u0001"+
		"\t\u0001\t\u0001\t\u0001\t\u0001\t\u0001\t\u0003\t\u00a9\b\t\u0001\n\u0001"+
		"\n\u0001\n\u0001\n\u0003\n\u00af\b\n\u0001\n\u0001\n\u0001\n\u0005\n\u00b4"+
		"\b\n\n\n\f\n\u00b7\t\n\u0001\n\u0001\n\u0001\n\u0001\u000b\u0001\u000b"+
		"\u0001\u000b\u0001\u000b\u0003\u000b\u00c0\b\u000b\u0001\u000b\u0001\u000b"+
		"\u0001\u000b\u0005\u000b\u00c5\b\u000b\n\u000b\f\u000b\u00c8\t\u000b\u0001"+
		"\u000b\u0001\u000b\u0001\u000b\u0001\f\u0003\f\u00ce\b\f\u0001\f\u0001"+
		"\f\u0005\f\u00d2\b\f\n\f\f\f\u00d5\t\f\u0001\f\u0003\f\u00d8\b\f\u0001"+
		"\f\u0003\f\u00db\b\f\u0001\r\u0001\r\u0001\r\u0003\r\u00e0\b\r\u0001\u000e"+
		"\u0001\u000e\u0001\u000e\u0001\u000f\u0001\u000f\u0001\u000f\u0001\u000f"+
		"\u0001\u0010\u0001\u0010\u0001\u0010\u0001\u0010\u0001\u0011\u0001\u0011"+
		"\u0001\u0011\u0001\u0011\u0001\u0012\u0001\u0012\u0001\u0012\u0005\u0012"+
		"\u00f4\b\u0012\n\u0012\f\u0012\u00f7\t\u0012\u0001\u0013\u0001\u0013\u0001"+
		"\u0013\u0001\u0013\u0001\u0014\u0001\u0014\u0001\u0014\u0001\u0014\u0001"+
		"\u0015\u0001\u0015\u0001\u0015\u0001\u0015\u0001\u0015\u0005\u0015\u0106"+
		"\b\u0015\n\u0015\f\u0015\u0109\t\u0015\u0001\u0015\u0003\u0015\u010c\b"+
		"\u0015\u0001\u0015\u0001\u0015\u0001\u0015\u0001\u0016\u0001\u0016\u0001"+
		"\u0016\u0001\u0016\u0001\u0016\u0003\u0016\u0116\b\u0016\u0001\u0016\u0001"+
		"\u0016\u0001\u0016\u0001\u0017\u0001\u0017\u0001\u0017\u0001\u0017\u0001"+
		"\u0017\u0003\u0017\u0120\b\u0017\u0001\u0017\u0001\u0017\u0001\u0017\u0001"+
		"\u0018\u0001\u0018\u0001\u0018\u0001\u0018\u0001\u0018\u0001\u0019\u0001"+
		"\u0019\u0001\u0019\u0001\u0019\u0001\u001a\u0001\u001a\u0003\u001a\u0130"+
		"\b\u001a\u0001\u001a\u0003\u001a\u0133\b\u001a\u0001\u001a\u0001\u001a"+
		"\u0003\u001a\u0137\b\u001a\u0001\u001a\u0003\u001a\u013a\b\u001a\u0001"+
		"\u001a\u0001\u001a\u0001\u001a\u0001\u001a\u0001\u001a\u0001\u001b\u0001"+
		"\u001b\u0001\u001b\u0001\u001b\u0001\u001b\u0001\u001b\u0001\u001b\u0001"+
		"\u001c\u0001\u001c\u0001\u001c\u0001\u001c\u0001\u001c\u0001\u001c\u0001"+
		"\u001c\u0001\u001d\u0001\u001d\u0001\u001d\u0001\u001e\u0001\u001e\u0001"+
		"\u001e\u0001\u001f\u0001\u001f\u0001\u001f\u0001\u001f\u0001\u001f\u0005"+
		"\u001f\u015a\b\u001f\n\u001f\f\u001f\u015d\t\u001f\u0001\u001f\u0003\u001f"+
		"\u0160\b\u001f\u0001\u001f\u0001\u001f\u0001\u001f\u0001 \u0001 \u0001"+
		" \u0001 \u0001 \u0003 \u016a\b \u0001 \u0001 \u0001 \u0001!\u0001!\u0001"+
		"!\u0001!\u0001!\u0001\"\u0001\"\u0001\"\u0001\"\u0001#\u0001#\u0001#\u0005"+
		"#\u017b\b#\n#\f#\u017e\t#\u0001$\u0001$\u0003$\u0182\b$\u0001$\u0001$"+
		"\u0003$\u0186\b$\u0001$\u0001$\u0003$\u018a\b$\u0001%\u0001%\u0001%\u0001"+
		"%\u0003%\u0190\b%\u0001&\u0001&\u0001\'\u0001\'\u0001\'\u0000\u0000(\u0000"+
		"\u0002\u0004\u0006\b\n\f\u000e\u0010\u0012\u0014\u0016\u0018\u001a\u001c"+
		"\u001e \"$&(*,.02468:<>@BDFHJLN\u0000\u0003\u0001\u0000./\u0001\u0000"+
		"05\u0001\u0000+.\u01af\u0000U\u0001\u0000\u0000\u0000\u0002^\u0001\u0000"+
		"\u0000\u0000\u0004w\u0001\u0000\u0000\u0000\u0006y\u0001\u0000\u0000\u0000"+
		"\b}\u0001\u0000\u0000\u0000\n\u0084\u0001\u0000\u0000\u0000\f\u0088\u0001"+
		"\u0000\u0000\u0000\u000e\u0091\u0001\u0000\u0000\u0000\u0010\u009a\u0001"+
		"\u0000\u0000\u0000\u0012\u00a8\u0001\u0000\u0000\u0000\u0014\u00aa\u0001"+
		"\u0000\u0000\u0000\u0016\u00bb\u0001\u0000\u0000\u0000\u0018\u00cd\u0001"+
		"\u0000\u0000\u0000\u001a\u00df\u0001\u0000\u0000\u0000\u001c\u00e1\u0001"+
		"\u0000\u0000\u0000\u001e\u00e4\u0001\u0000\u0000\u0000 \u00e8\u0001\u0000"+
		"\u0000\u0000\"\u00ec\u0001\u0000\u0000\u0000$\u00f0\u0001\u0000\u0000"+
		"\u0000&\u00f8\u0001\u0000\u0000\u0000(\u00fc\u0001\u0000\u0000\u0000*"+
		"\u0100\u0001\u0000\u0000\u0000,\u0110\u0001\u0000\u0000\u0000.\u011a\u0001"+
		"\u0000\u0000\u00000\u0124\u0001\u0000\u0000\u00002\u0129\u0001\u0000\u0000"+
		"\u00004\u012d\u0001\u0000\u0000\u00006\u0140\u0001\u0000\u0000\u00008"+
		"\u0147\u0001\u0000\u0000\u0000:\u014e\u0001\u0000\u0000\u0000<\u0151\u0001"+
		"\u0000\u0000\u0000>\u0154\u0001\u0000\u0000\u0000@\u0164\u0001\u0000\u0000"+
		"\u0000B\u016e\u0001\u0000\u0000\u0000D\u0173\u0001\u0000\u0000\u0000F"+
		"\u017c\u0001\u0000\u0000\u0000H\u017f\u0001\u0000\u0000\u0000J\u018f\u0001"+
		"\u0000\u0000\u0000L\u0191\u0001\u0000\u0000\u0000N\u0193\u0001\u0000\u0000"+
		"\u0000PT\u0003\u0004\u0002\u0000QT\u0003\u0002\u0001\u0000RT\u0005\u0004"+
		"\u0000\u0000SP\u0001\u0000\u0000\u0000SQ\u0001\u0000\u0000\u0000SR\u0001"+
		"\u0000\u0000\u0000TW\u0001\u0000\u0000\u0000US\u0001\u0000\u0000\u0000"+
		"UV\u0001\u0000\u0000\u0000VX\u0001\u0000\u0000\u0000WU\u0001\u0000\u0000"+
		"\u0000XY\u0005\u0000\u0000\u0001Y\u0001\u0001\u0000\u0000\u0000Z[\u0005"+
		"*\u0000\u0000[_\u0005\u0004\u0000\u0000\\]\u0005>\u0000\u0000]_\u0005"+
		"\u0004\u0000\u0000^Z\u0001\u0000\u0000\u0000^\\\u0001\u0000\u0000\u0000"+
		"_\u0003\u0001\u0000\u0000\u0000`x\u0003\u0006\u0003\u0000ax\u0003\b\u0004"+
		"\u0000bx\u0003\n\u0005\u0000cx\u0003\f\u0006\u0000dx\u0003\u0014\n\u0000"+
		"ex\u0003\u0016\u000b\u0000fx\u0003\u000e\u0007\u0000gx\u0003\u001c\u000e"+
		"\u0000hx\u0003\u001e\u000f\u0000ix\u0003 \u0010\u0000jx\u0003\"\u0011"+
		"\u0000kx\u0003&\u0013\u0000lx\u0003(\u0014\u0000mx\u0003*\u0015\u0000"+
		"nx\u0003,\u0016\u0000ox\u0003.\u0017\u0000px\u00034\u001a\u0000qx\u0003"+
		"6\u001b\u0000rx\u00038\u001c\u0000sx\u0003:\u001d\u0000tx\u0003<\u001e"+
		"\u0000ux\u0003>\u001f\u0000vx\u0003@ \u0000w`\u0001\u0000\u0000\u0000"+
		"wa\u0001\u0000\u0000\u0000wb\u0001\u0000\u0000\u0000wc\u0001\u0000\u0000"+
		"\u0000wd\u0001\u0000\u0000\u0000we\u0001\u0000\u0000\u0000wf\u0001\u0000"+
		"\u0000\u0000wg\u0001\u0000\u0000\u0000wh\u0001\u0000\u0000\u0000wi\u0001"+
		"\u0000\u0000\u0000wj\u0001\u0000\u0000\u0000wk\u0001\u0000\u0000\u0000"+
		"wl\u0001\u0000\u0000\u0000wm\u0001\u0000\u0000\u0000wn\u0001\u0000\u0000"+
		"\u0000wo\u0001\u0000\u0000\u0000wp\u0001\u0000\u0000\u0000wq\u0001\u0000"+
		"\u0000\u0000wr\u0001\u0000\u0000\u0000ws\u0001\u0000\u0000\u0000wt\u0001"+
		"\u0000\u0000\u0000wu\u0001\u0000\u0000\u0000wv\u0001\u0000\u0000\u0000"+
		"x\u0005\u0001\u0000\u0000\u0000yz\u0005\u0005\u0000\u0000z{\u0007\u0000"+
		"\u0000\u0000{|\u0005\u0004\u0000\u0000|\u0007\u0001\u0000\u0000\u0000"+
		"}~\u0005\u0006\u0000\u0000~\u0080\u0005*\u0000\u0000\u007f\u0081\u0003"+
		"N\'\u0000\u0080\u007f\u0001\u0000\u0000\u0000\u0080\u0081\u0001\u0000"+
		"\u0000\u0000\u0081\u0082\u0001\u0000\u0000\u0000\u0082\u0083\u0005\u0004"+
		"\u0000\u0000\u0083\t\u0001\u0000\u0000\u0000\u0084\u0085\u0005\u0007\u0000"+
		"\u0000\u0085\u0086\u0005*\u0000\u0000\u0086\u0087\u0005\u0004\u0000\u0000"+
		"\u0087\u000b\u0001\u0000\u0000\u0000\u0088\u0089\u0005\b\u0000\u0000\u0089"+
		"\u008a\u0005*\u0000\u0000\u008a\u008c\u00057\u0000\u0000\u008b\u008d\u0003"+
		"\u0010\b\u0000\u008c\u008b\u0001\u0000\u0000\u0000\u008c\u008d\u0001\u0000"+
		"\u0000\u0000\u008d\u008e\u0001\u0000\u0000\u0000\u008e\u008f\u00058\u0000"+
		"\u0000\u008f\u0090\u0005\u0004\u0000\u0000\u0090\r\u0001\u0000\u0000\u0000"+
		"\u0091\u0092\u0005\r\u0000\u0000\u0092\u0093\u0005*\u0000\u0000\u0093"+
		"\u0095\u00057\u0000\u0000\u0094\u0096\u0003\u0010\b\u0000\u0095\u0094"+
		"\u0001\u0000\u0000\u0000\u0095\u0096\u0001\u0000\u0000\u0000\u0096\u0097"+
		"\u0001\u0000\u0000\u0000\u0097\u0098\u00058\u0000\u0000\u0098\u0099\u0005"+
		"\u0004\u0000\u0000\u0099\u000f\u0001\u0000\u0000\u0000\u009a\u009f\u0003"+
		"\u0012\t\u0000\u009b\u009c\u00056\u0000\u0000\u009c\u009e\u0003\u0012"+
		"\t\u0000\u009d\u009b\u0001\u0000\u0000\u0000\u009e\u00a1\u0001\u0000\u0000"+
		"\u0000\u009f\u009d\u0001\u0000\u0000\u0000\u009f\u00a0\u0001\u0000\u0000"+
		"\u0000\u00a0\u0011\u0001\u0000\u0000\u0000\u00a1\u009f\u0001\u0000\u0000"+
		"\u0000\u00a2\u00a9\u0003N\'\u0000\u00a3\u00a9\u0003L&\u0000\u00a4\u00a9"+
		"\u0005)\u0000\u0000\u00a5\u00a9\u0005*\u0000\u0000\u00a6\u00a9\u0005\'"+
		"\u0000\u0000\u00a7\u00a9\u0005(\u0000\u0000\u00a8\u00a2\u0001\u0000\u0000"+
		"\u0000\u00a8\u00a3\u0001\u0000\u0000\u0000\u00a8\u00a4\u0001\u0000\u0000"+
		"\u0000\u00a8\u00a5\u0001\u0000\u0000\u0000\u00a8\u00a6\u0001\u0000\u0000"+
		"\u0000\u00a8\u00a7\u0001\u0000\u0000\u0000\u00a9\u0013\u0001\u0000\u0000"+
		"\u0000\u00aa\u00ab\u0005\t\u0000\u0000\u00ab\u00ac\u0005*\u0000\u0000"+
		"\u00ac\u00ae\u00057\u0000\u0000\u00ad\u00af\u0003\u0018\f\u0000\u00ae"+
		"\u00ad\u0001\u0000\u0000\u0000\u00ae\u00af\u0001\u0000\u0000\u0000\u00af"+
		"\u00b0\u0001\u0000\u0000\u0000\u00b0\u00b1\u00058\u0000\u0000\u00b1\u00b5"+
		"\u0005\u0004\u0000\u0000\u00b2\u00b4\u0003\u001a\r\u0000\u00b3\u00b2\u0001"+
		"\u0000\u0000\u0000\u00b4\u00b7\u0001\u0000\u0000\u0000\u00b5\u00b3\u0001"+
		"\u0000\u0000\u0000\u00b5\u00b6\u0001\u0000\u0000\u0000\u00b6\u00b8\u0001"+
		"\u0000\u0000\u0000\u00b7\u00b5\u0001\u0000\u0000\u0000\u00b8\u00b9\u0005"+
		"\n\u0000\u0000\u00b9\u00ba\u0005\u0004\u0000\u0000\u00ba\u0015\u0001\u0000"+
		"\u0000\u0000\u00bb\u00bc\u0005\u000b\u0000\u0000\u00bc\u00bd\u0005*\u0000"+
		"\u0000\u00bd\u00bf\u00057\u0000\u0000\u00be\u00c0\u0003\u0018\f\u0000"+
		"\u00bf\u00be\u0001\u0000\u0000\u0000\u00bf\u00c0\u0001\u0000\u0000\u0000"+
		"\u00c0\u00c1\u0001\u0000\u0000\u0000\u00c1\u00c2\u00058\u0000\u0000\u00c2"+
		"\u00c6\u0005\u0004\u0000\u0000\u00c3\u00c5\u0003\u001a\r\u0000\u00c4\u00c3"+
		"\u0001\u0000\u0000\u0000\u00c5\u00c8\u0001\u0000\u0000\u0000\u00c6\u00c4"+
		"\u0001\u0000\u0000\u0000\u00c6\u00c7\u0001\u0000\u0000\u0000\u00c7\u00c9"+
		"\u0001\u0000\u0000\u0000\u00c8\u00c6\u0001\u0000\u0000\u0000\u00c9\u00ca"+
		"\u0005\f\u0000\u0000\u00ca\u00cb\u0005\u0004\u0000\u0000\u00cb\u0017\u0001"+
		"\u0000\u0000\u0000\u00cc\u00ce\u0005*\u0000\u0000\u00cd\u00cc\u0001\u0000"+
		"\u0000\u0000\u00cd\u00ce\u0001\u0000\u0000\u0000\u00ce\u00d3\u0001\u0000"+
		"\u0000\u0000\u00cf\u00d0\u00056\u0000\u0000\u00d0\u00d2\u0005*\u0000\u0000"+
		"\u00d1\u00cf\u0001\u0000\u0000\u0000\u00d2\u00d5\u0001\u0000\u0000\u0000"+
		"\u00d3\u00d1\u0001\u0000\u0000\u0000\u00d3\u00d4\u0001\u0000\u0000\u0000"+
		"\u00d4\u00da\u0001\u0000\u0000\u0000\u00d5\u00d3\u0001\u0000\u0000\u0000"+
		"\u00d6\u00d8\u00056\u0000\u0000\u00d7\u00d6\u0001\u0000\u0000\u0000\u00d7"+
		"\u00d8\u0001\u0000\u0000\u0000\u00d8\u00d9\u0001\u0000\u0000\u0000\u00d9"+
		"\u00db\u0005<\u0000\u0000\u00da\u00d7\u0001\u0000\u0000\u0000\u00da\u00db"+
		"\u0001\u0000\u0000\u0000\u00db\u0019\u0001\u0000\u0000\u0000\u00dc\u00e0"+
		"\u0003\u0004\u0002\u0000\u00dd\u00e0\u0003\u0002\u0001\u0000\u00de\u00e0"+
		"\u0005\u0004\u0000\u0000\u00df\u00dc\u0001\u0000\u0000\u0000\u00df\u00dd"+
		"\u0001\u0000\u0000\u0000\u00df\u00de\u0001\u0000\u0000\u0000\u00e0\u001b"+
		"\u0001\u0000\u0000\u0000\u00e1\u00e2\u0005\u000e\u0000\u0000\u00e2\u00e3"+
		"\u0005>\u0000\u0000\u00e3\u001d\u0001\u0000\u0000\u0000\u00e4\u00e5\u0005"+
		"\u000f\u0000\u0000\u00e5\u00e6\u0003$\u0012\u0000\u00e6\u00e7\u0005\u0004"+
		"\u0000\u0000\u00e7\u001f\u0001\u0000\u0000\u0000\u00e8\u00e9\u0005\u0010"+
		"\u0000\u0000\u00e9\u00ea\u0003$\u0012\u0000\u00ea\u00eb\u0005\u0004\u0000"+
		"\u0000\u00eb!\u0001\u0000\u0000\u0000\u00ec\u00ed\u0005\u0011\u0000\u0000"+
		"\u00ed\u00ee\u0003$\u0012\u0000\u00ee\u00ef\u0005\u0004\u0000\u0000\u00ef"+
		"#\u0001\u0000\u0000\u0000\u00f0\u00f5\u0005*\u0000\u0000\u00f1\u00f2\u0005"+
		"6\u0000\u0000\u00f2\u00f4\u0005*\u0000\u0000\u00f3\u00f1\u0001\u0000\u0000"+
		"\u0000\u00f4\u00f7\u0001\u0000\u0000\u0000\u00f5\u00f3\u0001\u0000\u0000"+
		"\u0000\u00f5\u00f6\u0001\u0000\u0000\u0000\u00f6%\u0001\u0000\u0000\u0000"+
		"\u00f7\u00f5\u0001\u0000\u0000\u0000\u00f8\u00f9\u0005\u0012\u0000\u0000"+
		"\u00f9\u00fa\u0003J%\u0000\u00fa\u00fb\u0005\u0004\u0000\u0000\u00fb\'"+
		"\u0001\u0000\u0000\u0000\u00fc\u00fd\u0005\u0013\u0000\u0000\u00fd\u00fe"+
		"\u0003J%\u0000\u00fe\u00ff\u0005\u0004\u0000\u0000\u00ff)\u0001\u0000"+
		"\u0000\u0000\u0100\u0101\u0005\u0014\u0000\u0000\u0101\u0102\u0003H$\u0000"+
		"\u0102\u0103\u0005\u0004\u0000\u0000\u0103\u0107\u0003F#\u0000\u0104\u0106"+
		"\u00030\u0018\u0000\u0105\u0104\u0001\u0000\u0000\u0000\u0106\u0109\u0001"+
		"\u0000\u0000\u0000\u0107\u0105\u0001\u0000\u0000\u0000\u0107\u0108\u0001"+
		"\u0000\u0000\u0000\u0108\u010b\u0001\u0000\u0000\u0000\u0109\u0107\u0001"+
		"\u0000\u0000\u0000\u010a\u010c\u00032\u0019\u0000\u010b\u010a\u0001\u0000"+
		"\u0000\u0000\u010b\u010c\u0001\u0000\u0000\u0000\u010c\u010d\u0001\u0000"+
		"\u0000\u0000\u010d\u010e\u0005\u0019\u0000\u0000\u010e\u010f\u0005\u0004"+
		"\u0000\u0000\u010f+\u0001\u0000\u0000\u0000\u0110\u0111\u0005\u0015\u0000"+
		"\u0000\u0111\u0112\u0003J%\u0000\u0112\u0113\u0005\u0004\u0000\u0000\u0113"+
		"\u0115\u0003F#\u0000\u0114\u0116\u00032\u0019\u0000\u0115\u0114\u0001"+
		"\u0000\u0000\u0000\u0115\u0116\u0001\u0000\u0000\u0000\u0116\u0117\u0001"+
		"\u0000\u0000\u0000\u0117\u0118\u0005\u0019\u0000\u0000\u0118\u0119\u0005"+
		"\u0004\u0000\u0000\u0119-\u0001\u0000\u0000\u0000\u011a\u011b\u0005\u0016"+
		"\u0000\u0000\u011b\u011c\u0003J%\u0000\u011c\u011d\u0005\u0004\u0000\u0000"+
		"\u011d\u011f\u0003F#\u0000\u011e\u0120\u00032\u0019\u0000\u011f\u011e"+
		"\u0001\u0000\u0000\u0000\u011f\u0120\u0001\u0000\u0000\u0000\u0120\u0121"+
		"\u0001\u0000\u0000\u0000\u0121\u0122\u0005\u0019\u0000\u0000\u0122\u0123"+
		"\u0005\u0004\u0000\u0000\u0123/\u0001\u0000\u0000\u0000\u0124\u0125\u0005"+
		"\u0017\u0000\u0000\u0125\u0126\u0003H$\u0000\u0126\u0127\u0005\u0004\u0000"+
		"\u0000\u0127\u0128\u0003F#\u0000\u01281\u0001\u0000\u0000\u0000\u0129"+
		"\u012a\u0005\u0018\u0000\u0000\u012a\u012b\u0005\u0004\u0000\u0000\u012b"+
		"\u012c\u0003F#\u0000\u012c3\u0001\u0000\u0000\u0000\u012d\u012f\u0005"+
		"\u001a\u0000\u0000\u012e\u0130\u0003J%\u0000\u012f\u012e\u0001\u0000\u0000"+
		"\u0000\u012f\u0130\u0001\u0000\u0000\u0000\u0130\u0132\u0001\u0000\u0000"+
		"\u0000\u0131\u0133\u00056\u0000\u0000\u0132\u0131\u0001\u0000\u0000\u0000"+
		"\u0132\u0133\u0001\u0000\u0000\u0000\u0133\u0134\u0001\u0000\u0000\u0000"+
		"\u0134\u0136\u0003H$\u0000\u0135\u0137\u00056\u0000\u0000\u0136\u0135"+
		"\u0001\u0000\u0000\u0000\u0136\u0137\u0001\u0000\u0000\u0000\u0137\u0139"+
		"\u0001\u0000\u0000\u0000\u0138\u013a\u0003J%\u0000\u0139\u0138\u0001\u0000"+
		"\u0000\u0000\u0139\u013a\u0001\u0000\u0000\u0000\u013a\u013b\u0001\u0000"+
		"\u0000\u0000\u013b\u013c\u0005\u0004\u0000\u0000\u013c\u013d\u0003F#\u0000"+
		"\u013d\u013e\u0005\u001b\u0000\u0000\u013e\u013f\u0005\u0004\u0000\u0000"+
		"\u013f5\u0001\u0000\u0000\u0000\u0140\u0141\u0005\u001c\u0000\u0000\u0141"+
		"\u0142\u0003H$\u0000\u0142\u0143\u0005\u0004\u0000\u0000\u0143\u0144\u0003"+
		"F#\u0000\u0144\u0145\u0005\u001d\u0000\u0000\u0145\u0146\u0005\u0004\u0000"+
		"\u0000\u01467\u0001\u0000\u0000\u0000\u0147\u0148\u0005\u001e\u0000\u0000"+
		"\u0148\u0149\u0005\u0004\u0000\u0000\u0149\u014a\u0003F#\u0000\u014a\u014b"+
		"\u0005\u001f\u0000\u0000\u014b\u014c\u0003H$\u0000\u014c\u014d\u0005\u0004"+
		"\u0000\u0000\u014d9\u0001\u0000\u0000\u0000\u014e\u014f\u0005 \u0000\u0000"+
		"\u014f\u0150\u0005\u0004\u0000\u0000\u0150;\u0001\u0000\u0000\u0000\u0151"+
		"\u0152\u0005!\u0000\u0000\u0152\u0153\u0005\u0004\u0000\u0000\u0153=\u0001"+
		"\u0000\u0000\u0000\u0154\u0155\u0005#\u0000\u0000\u0155\u0156\u0003H$"+
		"\u0000\u0156\u0157\u0005\u0004\u0000\u0000\u0157\u015b\u0003F#\u0000\u0158"+
		"\u015a\u0003B!\u0000\u0159\u0158\u0001\u0000\u0000\u0000\u015a\u015d\u0001"+
		"\u0000\u0000\u0000\u015b\u0159\u0001\u0000\u0000\u0000\u015b\u015c\u0001"+
		"\u0000\u0000\u0000\u015c\u015f\u0001\u0000\u0000\u0000\u015d\u015b\u0001"+
		"\u0000\u0000\u0000\u015e\u0160\u0003D\"\u0000\u015f\u015e\u0001\u0000"+
		"\u0000\u0000\u015f\u0160\u0001\u0000\u0000\u0000\u0160\u0161\u0001\u0000"+
		"\u0000\u0000\u0161\u0162\u0005&\u0000\u0000\u0162\u0163\u0005\u0004\u0000"+
		"\u0000\u0163?\u0001\u0000\u0000\u0000\u0164\u0165\u0005\"\u0000\u0000"+
		"\u0165\u0166\u0005*\u0000\u0000\u0166\u0167\u0005\u0004\u0000\u0000\u0167"+
		"\u0169\u0003F#\u0000\u0168\u016a\u0003D\"\u0000\u0169\u0168\u0001\u0000"+
		"\u0000\u0000\u0169\u016a\u0001\u0000\u0000\u0000\u016a\u016b\u0001\u0000"+
		"\u0000\u0000\u016b\u016c\u0005&\u0000\u0000\u016c\u016d\u0005\u0004\u0000"+
		"\u0000\u016dA\u0001\u0000\u0000\u0000\u016e\u016f\u0005$\u0000\u0000\u016f"+
		"\u0170\u0003H$\u0000\u0170\u0171\u0005\u0004\u0000\u0000\u0171\u0172\u0003"+
		"F#\u0000\u0172C\u0001\u0000\u0000\u0000\u0173\u0174\u0005%\u0000\u0000"+
		"\u0174\u0175\u0005\u0004\u0000\u0000\u0175\u0176\u0003F#\u0000\u0176E"+
		"\u0001\u0000\u0000\u0000\u0177\u017b\u0003\u0004\u0002\u0000\u0178\u017b"+
		"\u0003\u0002\u0001\u0000\u0179\u017b\u0005\u0004\u0000\u0000\u017a\u0177"+
		"\u0001\u0000\u0000\u0000\u017a\u0178\u0001\u0000\u0000\u0000\u017a\u0179"+
		"\u0001\u0000\u0000\u0000\u017b\u017e\u0001\u0000\u0000\u0000\u017c\u017a"+
		"\u0001\u0000\u0000\u0000\u017c\u017d\u0001\u0000\u0000\u0000\u017dG\u0001"+
		"\u0000\u0000\u0000\u017e\u017c\u0001\u0000\u0000\u0000\u017f\u0181\u0003"+
		"J%\u0000\u0180\u0182\u00056\u0000\u0000\u0181\u0180\u0001\u0000\u0000"+
		"\u0000\u0181\u0182\u0001\u0000\u0000\u0000\u0182\u0189\u0001\u0000\u0000"+
		"\u0000\u0183\u0185\u0003L&\u0000\u0184\u0186\u00056\u0000\u0000\u0185"+
		"\u0184\u0001\u0000\u0000\u0000\u0185\u0186\u0001\u0000\u0000\u0000\u0186"+
		"\u0187\u0001\u0000\u0000\u0000\u0187\u0188\u0003J%\u0000\u0188\u018a\u0001"+
		"\u0000\u0000\u0000\u0189\u0183\u0001\u0000\u0000\u0000\u0189\u018a\u0001"+
		"\u0000\u0000\u0000\u018aI\u0001\u0000\u0000\u0000\u018b\u0190\u0005*\u0000"+
		"\u0000\u018c\u0190\u0005\'\u0000\u0000\u018d\u0190\u0005(\u0000\u0000"+
		"\u018e\u0190\u0003N\'\u0000\u018f\u018b\u0001\u0000\u0000\u0000\u018f"+
		"\u018c\u0001\u0000\u0000\u0000\u018f\u018d\u0001\u0000\u0000\u0000\u018f"+
		"\u018e\u0001\u0000\u0000\u0000\u0190K\u0001\u0000\u0000\u0000\u0191\u0192"+
		"\u0007\u0001\u0000\u0000\u0192M\u0001\u0000\u0000\u0000\u0193\u0194\u0007"+
		"\u0002\u0000\u0000\u0194O\u0001\u0000\u0000\u0000$SU^w\u0080\u008c\u0095"+
		"\u009f\u00a8\u00ae\u00b5\u00bf\u00c6\u00cd\u00d3\u00d7\u00da\u00df\u00f5"+
		"\u0107\u010b\u0115\u011f\u012f\u0132\u0136\u0139\u015b\u015f\u0169\u017a"+
		"\u017c\u0181\u0185\u0189\u018f";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}