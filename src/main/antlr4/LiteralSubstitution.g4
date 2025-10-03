grammar LiteralSubstitution;

/*
 * Parser: just a flat stream of "pieces".
 * We keep NEWLINEs explicit so you preserve original line breaks.
 */
file
  : piece* EOF
  ;

piece
  : DIRECTIVE_LINE      // <- keep these lines intact
  | STRING
  | CHARLIT
  | FLOAT
  | NEWLINE
  | OTHER
  ;

/* ------------ Lexer rules ------------ */

/*
 * Keep entire directive lines intact so we don't rewrite their literals.
 * Matches (optionally) a leading label like `foo:` then DCF/DCS/DCW,
 * spaces, then the rest of the line up to but not including the newline.
 *
 * We also guard to only match at column 0 (start of line) so we don't
 * accidentally grab a mid-line "DCS ..." in a comment, etc.
 */
DIRECTIVE_LINE
  : {getCharPositionInLine()==0}? SP* LABEL? SP* '.'[dD][cC][fFsSwW] SP+ ~[\r\n]*
  | {getCharPositionInLine()==0}? SP* '.' [lL][iI][nN][eE] SP+ ~[\r\n]*
  ;

/*
 * String literal:
 * " ... " with escapes for \" and \\ (and general backslash-escape for any char).
 * No newlines inside.
 */
STRING: '"' ( ESC | ~["\\\r\n] )* '"' ;
CHARLIT: '\'' ( ESC | ~['\\\r\n] ) '\'' ;
fragment ESC : '\\' [0btnfr"'\\] | '\\' [uU] HEX HEX HEX HEX ;
fragment HEX : [0-9A-Fa-f] ;

/*
 * Floating-point literals (NO leading sign in the token, so '-' as an operator
 * stays separate and passes through untouched):
 *
 *  Fixed (must have a decimal point):
 *    123.      123.45      .5
 *  Exponential (decimal point optional if exponent present):
 *    123e5     1.2e-3      .5E+1
 */
FLOAT
  : '-'? DIGITS '.' DIGITS* ( [eE] [+\-]? DIGITS )?
  | '-'? '.' DIGITS         ( [eE] [+\-]? DIGITS )?
  | '-'? DIGITS             ( [eE] [+\-]? DIGITS )
  ;

/* Preserve line structure */
NEWLINE
  : '\r'? '\n'
  ;

/* Catch-all: every other single character (spaces, tabs, punctuation, etc.) */
OTHER
  : .
  ;

fragment DIGITS : [0-9]+ ;
fragment SP     : [ \t] ;
fragment LABEL  : [A-Za-z_][A-Za-z0-9_]* ':' ;
