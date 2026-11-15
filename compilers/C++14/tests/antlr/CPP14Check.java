// SPDX-License-Identifier: Apache-2.0
//
// CPP14Check - parses a C++ file with the ANTLR grammar grammar/CPP14Lexer.g4 and
// grammar/CPP14Parser.g4; exit status 0 if the file is accepted, 1 if not.
// Used by tests/antlr_check.sh to compare the grammar with the translator's
// hand-written parser (cppc --parse-only).

import org.antlr.v4.runtime.BaseErrorListener;
import org.antlr.v4.runtime.CharStream;
import org.antlr.v4.runtime.CharStreams;
import org.antlr.v4.runtime.CommonTokenStream;
import org.antlr.v4.runtime.RecognitionException;
import org.antlr.v4.runtime.Recognizer;

public class CPP14Check {

    static int errors = 0;
    static String fileName;

    static class Listener extends BaseErrorListener {
        @Override
        public void syntaxError(Recognizer<?, ?> r, Object sym, int line, int col, String msg, RecognitionException e) {
            if (errors++ == 0) System.out.println(fileName + ":" + line + ":" + (col + 1) + ": " + msg);
        }
    }

    public static void main(String[] args) throws Exception {
        fileName = args[0];
        CharStream cs = CharStreams.fromFileName(fileName);
        CPP14Lexer lexer = new CPP14Lexer(cs);
        lexer.removeErrorListeners();
        lexer.addErrorListener(new Listener());
        CPP14Parser parser = new CPP14Parser(new CommonTokenStream(lexer));
        parser.removeErrorListeners();
        parser.addErrorListener(new Listener());
        parser.translationUnit();
        System.exit(errors == 0 ? 0 : 1);
    }
}
