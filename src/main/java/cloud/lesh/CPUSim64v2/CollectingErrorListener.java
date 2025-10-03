package cloud.lesh.CPUSim64v2;

import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.misc.Interval;

import java.util.List;

public final class CollectingErrorListener extends BaseErrorListener {
	private List<String> errors;
	HasLocation locator;
	public CollectingErrorListener(List<String> errors, HasLocation locator) {
		this.errors = errors;
		this.locator = locator;
	}

	@Override
	public void syntaxError(Recognizer<?, ?> recognizer,
							Object offendingSymbol,
							int line,
							int charPositionInLine,
							String msg,
							RecognitionException e) {

		// Try to extract a helpful offending text (works for both lexer & parser)
		String offendingText = "";
		if (e instanceof LexerNoViableAltException lexNoAlt) {
			// Lexer error: pull the single character (or span) that caused it
			CharStream cs = ((Lexer) recognizer).getInputStream();
			int start = lexNoAlt.getStartIndex();
			int stop  = Math.min(start, cs.size() - 1);
			offendingText = cs.getText(Interval.of(start, stop));
		} else if (offendingSymbol instanceof Token tok) {
			// Parser error: token text is available
			offendingText = tok.getText();
		}

		String formatted;
		if (locator == null ) {
			formatted = String.format("Preprocessed line %d:%d %s%s",
					line, charPositionInLine, msg,
					offendingText.isEmpty() ? "" : " (offending: " + escape(offendingText) + ")");
		} else {
			formatted = String.format("%s:%d %s%s",
					locator.getLocation(), charPositionInLine, msg,
					offendingText.isEmpty() ? "" : " (offending: " + escape(offendingText) + ")");
		}
		errors.add(formatted);
	}

	private static String escape(String s) {
		return s.replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t");
	}
}
