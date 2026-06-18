// Generated from ConstExpr.g4 by ANTLR 4.13.2
package cloud.lesh.CPUSim64;
import org.antlr.v4.runtime.atn.*;
import org.antlr.v4.runtime.dfa.DFA;
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.misc.*;
import org.antlr.v4.runtime.tree.*;
import java.util.List;
import java.util.Iterator;
import java.util.ArrayList;

@SuppressWarnings({"all", "warnings", "unchecked", "unused", "cast", "CheckReturnValue", "this-escape"})
public class ConstExprParser extends Parser {
	static { RuntimeMetaData.checkVersion("4.13.2", RuntimeMetaData.VERSION); }

	protected static final DFA[] _decisionToDFA;
	protected static final PredictionContextCache _sharedContextCache =
		new PredictionContextCache();
	public static final int
		CHAR=1, STRING=2, HEXINT=3, INT=4, FLOAT=5, IDENT=6, PLUS=7, MINUS=8, 
		MULTIPLY=9, DIVIDE=10, LPAREN=11, RPAREN=12, WS=13, OTHER=14;
	public static final int
		RULE_line = 0, RULE_part = 1, RULE_expr = 2, RULE_primary = 3;
	private static String[] makeRuleNames() {
		return new String[] {
			"line", "part", "expr", "primary"
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

	@Override
	public String getGrammarFileName() { return "ConstExpr.g4"; }

	@Override
	public String[] getRuleNames() { return ruleNames; }

	@Override
	public String getSerializedATN() { return _serializedATN; }

	@Override
	public ATN getATN() { return _ATN; }

	public ConstExprParser(TokenStream input) {
		super(input);
		_interp = new ParserATNSimulator(this,_ATN,_decisionToDFA,_sharedContextCache);
	}

	@SuppressWarnings("CheckReturnValue")
	public static class LineContext extends ParserRuleContext {
		public TerminalNode EOF() { return getToken(ConstExprParser.EOF, 0); }
		public List<PartContext> part() {
			return getRuleContexts(PartContext.class);
		}
		public PartContext part(int i) {
			return getRuleContext(PartContext.class,i);
		}
		public LineContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_line; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterLine(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitLine(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitLine(this);
			else return visitor.visitChildren(this);
		}
	}

	public final LineContext line() throws RecognitionException {
		LineContext _localctx = new LineContext(_ctx, getState());
		enterRule(_localctx, 0, RULE_line);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(11);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 24574L) != 0)) {
				{
				{
				setState(8);
				part();
				}
				}
				setState(13);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(14);
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
	public static class PartContext extends ParserRuleContext {
		public PartContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_part; }
	 
		public PartContext() { }
		public void copyFrom(PartContext ctx) {
			super.copyFrom(ctx);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class PartStringContext extends PartContext {
		public TerminalNode STRING() { return getToken(ConstExprParser.STRING, 0); }
		public PartStringContext(PartContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterPartString(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitPartString(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitPartString(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class PartOtherContext extends PartContext {
		public TerminalNode PLUS() { return getToken(ConstExprParser.PLUS, 0); }
		public TerminalNode MULTIPLY() { return getToken(ConstExprParser.MULTIPLY, 0); }
		public TerminalNode DIVIDE() { return getToken(ConstExprParser.DIVIDE, 0); }
		public TerminalNode LPAREN() { return getToken(ConstExprParser.LPAREN, 0); }
		public TerminalNode RPAREN() { return getToken(ConstExprParser.RPAREN, 0); }
		public TerminalNode OTHER() { return getToken(ConstExprParser.OTHER, 0); }
		public PartOtherContext(PartContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterPartOther(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitPartOther(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitPartOther(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class PartCharContext extends PartContext {
		public TerminalNode CHAR() { return getToken(ConstExprParser.CHAR, 0); }
		public PartCharContext(PartContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterPartChar(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitPartChar(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitPartChar(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class PartIdentContext extends PartContext {
		public TerminalNode IDENT() { return getToken(ConstExprParser.IDENT, 0); }
		public PartIdentContext(PartContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterPartIdent(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitPartIdent(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitPartIdent(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class PartExprContext extends PartContext {
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public PartExprContext(PartContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterPartExpr(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitPartExpr(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitPartExpr(this);
			else return visitor.visitChildren(this);
		}
	}

	public final PartContext part() throws RecognitionException {
		PartContext _localctx = new PartContext(_ctx, getState());
		enterRule(_localctx, 2, RULE_part);
		try {
			setState(26);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,1,_ctx) ) {
			case 1:
				_localctx = new PartExprContext(_localctx);
				enterOuterAlt(_localctx, 1);
				{
				setState(16);
				expr(0);
				}
				break;
			case 2:
				_localctx = new PartCharContext(_localctx);
				enterOuterAlt(_localctx, 2);
				{
				setState(17);
				match(CHAR);
				}
				break;
			case 3:
				_localctx = new PartStringContext(_localctx);
				enterOuterAlt(_localctx, 3);
				{
				setState(18);
				match(STRING);
				}
				break;
			case 4:
				_localctx = new PartIdentContext(_localctx);
				enterOuterAlt(_localctx, 4);
				{
				setState(19);
				match(IDENT);
				}
				break;
			case 5:
				_localctx = new PartOtherContext(_localctx);
				enterOuterAlt(_localctx, 5);
				{
				setState(20);
				match(PLUS);
				}
				break;
			case 6:
				_localctx = new PartOtherContext(_localctx);
				enterOuterAlt(_localctx, 6);
				{
				setState(21);
				match(MULTIPLY);
				}
				break;
			case 7:
				_localctx = new PartOtherContext(_localctx);
				enterOuterAlt(_localctx, 7);
				{
				setState(22);
				match(DIVIDE);
				}
				break;
			case 8:
				_localctx = new PartOtherContext(_localctx);
				enterOuterAlt(_localctx, 8);
				{
				setState(23);
				match(LPAREN);
				}
				break;
			case 9:
				_localctx = new PartOtherContext(_localctx);
				enterOuterAlt(_localctx, 9);
				{
				setState(24);
				match(RPAREN);
				}
				break;
			case 10:
				_localctx = new PartOtherContext(_localctx);
				enterOuterAlt(_localctx, 10);
				{
				setState(25);
				match(OTHER);
				}
				break;
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
		public ExprContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_expr; }
	 
		public ExprContext() { }
		public void copyFrom(ExprContext ctx) {
			super.copyFrom(ctx);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class UnaryExprContext extends ExprContext {
		public TerminalNode MINUS() { return getToken(ConstExprParser.MINUS, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public UnaryExprContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterUnaryExpr(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitUnaryExpr(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitUnaryExpr(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class PrimaryExprContext extends ExprContext {
		public PrimaryContext primary() {
			return getRuleContext(PrimaryContext.class,0);
		}
		public PrimaryExprContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterPrimaryExpr(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitPrimaryExpr(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitPrimaryExpr(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class AddExprContext extends ExprContext {
		public Token op;
		public List<ExprContext> expr() {
			return getRuleContexts(ExprContext.class);
		}
		public ExprContext expr(int i) {
			return getRuleContext(ExprContext.class,i);
		}
		public TerminalNode PLUS() { return getToken(ConstExprParser.PLUS, 0); }
		public TerminalNode MINUS() { return getToken(ConstExprParser.MINUS, 0); }
		public AddExprContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterAddExpr(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitAddExpr(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitAddExpr(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class MulExprContext extends ExprContext {
		public Token op;
		public List<ExprContext> expr() {
			return getRuleContexts(ExprContext.class);
		}
		public ExprContext expr(int i) {
			return getRuleContext(ExprContext.class,i);
		}
		public TerminalNode MULTIPLY() { return getToken(ConstExprParser.MULTIPLY, 0); }
		public TerminalNode DIVIDE() { return getToken(ConstExprParser.DIVIDE, 0); }
		public MulExprContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterMulExpr(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitMulExpr(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitMulExpr(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class ParensExprContext extends ExprContext {
		public TerminalNode LPAREN() { return getToken(ConstExprParser.LPAREN, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public TerminalNode RPAREN() { return getToken(ConstExprParser.RPAREN, 0); }
		public ParensExprContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterParensExpr(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitParensExpr(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitParensExpr(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ExprContext expr() throws RecognitionException {
		return expr(0);
	}

	private ExprContext expr(int _p) throws RecognitionException {
		ParserRuleContext _parentctx = _ctx;
		int _parentState = getState();
		ExprContext _localctx = new ExprContext(_ctx, _parentState);
		ExprContext _prevctx = _localctx;
		int _startState = 4;
		enterRecursionRule(_localctx, 4, RULE_expr, _p);
		int _la;
		try {
			int _alt;
			enterOuterAlt(_localctx, 1);
			{
			setState(36);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case MINUS:
				{
				_localctx = new UnaryExprContext(_localctx);
				_ctx = _localctx;
				_prevctx = _localctx;

				setState(29);
				match(MINUS);
				setState(30);
				expr(5);
				}
				break;
			case LPAREN:
				{
				_localctx = new ParensExprContext(_localctx);
				_ctx = _localctx;
				_prevctx = _localctx;
				setState(31);
				match(LPAREN);
				setState(32);
				expr(0);
				setState(33);
				match(RPAREN);
				}
				break;
			case HEXINT:
			case INT:
			case FLOAT:
				{
				_localctx = new PrimaryExprContext(_localctx);
				_ctx = _localctx;
				_prevctx = _localctx;
				setState(35);
				primary();
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
			_ctx.stop = _input.LT(-1);
			setState(46);
			_errHandler.sync(this);
			_alt = getInterpreter().adaptivePredict(_input,4,_ctx);
			while ( _alt!=2 && _alt!=org.antlr.v4.runtime.atn.ATN.INVALID_ALT_NUMBER ) {
				if ( _alt==1 ) {
					if ( _parseListeners!=null ) triggerExitRuleEvent();
					_prevctx = _localctx;
					{
					setState(44);
					_errHandler.sync(this);
					switch ( getInterpreter().adaptivePredict(_input,3,_ctx) ) {
					case 1:
						{
						_localctx = new MulExprContext(new ExprContext(_parentctx, _parentState));
						pushNewRecursionContext(_localctx, _startState, RULE_expr);
						setState(38);
						if (!(precpred(_ctx, 4))) throw new FailedPredicateException(this, "precpred(_ctx, 4)");
						setState(39);
						((MulExprContext)_localctx).op = _input.LT(1);
						_la = _input.LA(1);
						if ( !(_la==MULTIPLY || _la==DIVIDE) ) {
							((MulExprContext)_localctx).op = (Token)_errHandler.recoverInline(this);
						}
						else {
							if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
							_errHandler.reportMatch(this);
							consume();
						}
						setState(40);
						expr(5);
						}
						break;
					case 2:
						{
						_localctx = new AddExprContext(new ExprContext(_parentctx, _parentState));
						pushNewRecursionContext(_localctx, _startState, RULE_expr);
						setState(41);
						if (!(precpred(_ctx, 3))) throw new FailedPredicateException(this, "precpred(_ctx, 3)");
						setState(42);
						((AddExprContext)_localctx).op = _input.LT(1);
						_la = _input.LA(1);
						if ( !(_la==PLUS || _la==MINUS) ) {
							((AddExprContext)_localctx).op = (Token)_errHandler.recoverInline(this);
						}
						else {
							if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
							_errHandler.reportMatch(this);
							consume();
						}
						setState(43);
						expr(4);
						}
						break;
					}
					} 
				}
				setState(48);
				_errHandler.sync(this);
				_alt = getInterpreter().adaptivePredict(_input,4,_ctx);
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			unrollRecursionContexts(_parentctx);
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class PrimaryContext extends ParserRuleContext {
		public TerminalNode INT() { return getToken(ConstExprParser.INT, 0); }
		public TerminalNode HEXINT() { return getToken(ConstExprParser.HEXINT, 0); }
		public TerminalNode FLOAT() { return getToken(ConstExprParser.FLOAT, 0); }
		public PrimaryContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_primary; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).enterPrimary(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof ConstExprListener ) ((ConstExprListener)listener).exitPrimary(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof ConstExprVisitor ) return ((ConstExprVisitor<? extends T>)visitor).visitPrimary(this);
			else return visitor.visitChildren(this);
		}
	}

	public final PrimaryContext primary() throws RecognitionException {
		PrimaryContext _localctx = new PrimaryContext(_ctx, getState());
		enterRule(_localctx, 6, RULE_primary);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(49);
			_la = _input.LA(1);
			if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 56L) != 0)) ) {
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

	public boolean sempred(RuleContext _localctx, int ruleIndex, int predIndex) {
		switch (ruleIndex) {
		case 2:
			return expr_sempred((ExprContext)_localctx, predIndex);
		}
		return true;
	}
	private boolean expr_sempred(ExprContext _localctx, int predIndex) {
		switch (predIndex) {
		case 0:
			return precpred(_ctx, 4);
		case 1:
			return precpred(_ctx, 3);
		}
		return true;
	}

	public static final String _serializedATN =
		"\u0004\u0001\u000e4\u0002\u0000\u0007\u0000\u0002\u0001\u0007\u0001\u0002"+
		"\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0001\u0000\u0005\u0000\n\b"+
		"\u0000\n\u0000\f\u0000\r\t\u0000\u0001\u0000\u0001\u0000\u0001\u0001\u0001"+
		"\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001\u0001"+
		"\u0001\u0001\u0001\u0001\u0001\u0003\u0001\u001b\b\u0001\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0003\u0002%\b\u0002\u0001\u0002\u0001\u0002\u0001\u0002\u0001"+
		"\u0002\u0001\u0002\u0001\u0002\u0005\u0002-\b\u0002\n\u0002\f\u00020\t"+
		"\u0002\u0001\u0003\u0001\u0003\u0001\u0003\u0000\u0001\u0004\u0004\u0000"+
		"\u0002\u0004\u0006\u0000\u0003\u0001\u0000\t\n\u0001\u0000\u0007\b\u0001"+
		"\u0000\u0003\u0005=\u0000\u000b\u0001\u0000\u0000\u0000\u0002\u001a\u0001"+
		"\u0000\u0000\u0000\u0004$\u0001\u0000\u0000\u0000\u00061\u0001\u0000\u0000"+
		"\u0000\b\n\u0003\u0002\u0001\u0000\t\b\u0001\u0000\u0000\u0000\n\r\u0001"+
		"\u0000\u0000\u0000\u000b\t\u0001\u0000\u0000\u0000\u000b\f\u0001\u0000"+
		"\u0000\u0000\f\u000e\u0001\u0000\u0000\u0000\r\u000b\u0001\u0000\u0000"+
		"\u0000\u000e\u000f\u0005\u0000\u0000\u0001\u000f\u0001\u0001\u0000\u0000"+
		"\u0000\u0010\u001b\u0003\u0004\u0002\u0000\u0011\u001b\u0005\u0001\u0000"+
		"\u0000\u0012\u001b\u0005\u0002\u0000\u0000\u0013\u001b\u0005\u0006\u0000"+
		"\u0000\u0014\u001b\u0005\u0007\u0000\u0000\u0015\u001b\u0005\t\u0000\u0000"+
		"\u0016\u001b\u0005\n\u0000\u0000\u0017\u001b\u0005\u000b\u0000\u0000\u0018"+
		"\u001b\u0005\f\u0000\u0000\u0019\u001b\u0005\u000e\u0000\u0000\u001a\u0010"+
		"\u0001\u0000\u0000\u0000\u001a\u0011\u0001\u0000\u0000\u0000\u001a\u0012"+
		"\u0001\u0000\u0000\u0000\u001a\u0013\u0001\u0000\u0000\u0000\u001a\u0014"+
		"\u0001\u0000\u0000\u0000\u001a\u0015\u0001\u0000\u0000\u0000\u001a\u0016"+
		"\u0001\u0000\u0000\u0000\u001a\u0017\u0001\u0000\u0000\u0000\u001a\u0018"+
		"\u0001\u0000\u0000\u0000\u001a\u0019\u0001\u0000\u0000\u0000\u001b\u0003"+
		"\u0001\u0000\u0000\u0000\u001c\u001d\u0006\u0002\uffff\uffff\u0000\u001d"+
		"\u001e\u0005\b\u0000\u0000\u001e%\u0003\u0004\u0002\u0005\u001f \u0005"+
		"\u000b\u0000\u0000 !\u0003\u0004\u0002\u0000!\"\u0005\f\u0000\u0000\""+
		"%\u0001\u0000\u0000\u0000#%\u0003\u0006\u0003\u0000$\u001c\u0001\u0000"+
		"\u0000\u0000$\u001f\u0001\u0000\u0000\u0000$#\u0001\u0000\u0000\u0000"+
		"%.\u0001\u0000\u0000\u0000&\'\n\u0004\u0000\u0000\'(\u0007\u0000\u0000"+
		"\u0000(-\u0003\u0004\u0002\u0005)*\n\u0003\u0000\u0000*+\u0007\u0001\u0000"+
		"\u0000+-\u0003\u0004\u0002\u0004,&\u0001\u0000\u0000\u0000,)\u0001\u0000"+
		"\u0000\u0000-0\u0001\u0000\u0000\u0000.,\u0001\u0000\u0000\u0000./\u0001"+
		"\u0000\u0000\u0000/\u0005\u0001\u0000\u0000\u00000.\u0001\u0000\u0000"+
		"\u000012\u0007\u0002\u0000\u00002\u0007\u0001\u0000\u0000\u0000\u0005"+
		"\u000b\u001a$,.";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}