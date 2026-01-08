grammar ConstExpr;

line
    : part* EOF
    ;

part
    : expr		                    		# PartExpr
    | CHAR									# PartChar
    | STRING								# PartString
    | OTHER                      			# PartOther
    ;

expr
    : <assoc=right> MINUS expr              # unaryExpr
    | expr op=(MULTIPLY | DIVIDE) expr		# mulExpr
    | expr op=(PLUS | MINUS) expr     		# addExpr
    | LPAREN expr RPAREN             		# parensExpr
    | primary                             	# primaryExpr
    ;

primary
    : INT | HEXINT | FLOAT
    ;

// ===== LEXER RULES =====
// Order matters! More specific rules must come before OTHER

CHAR
  : '\'' ( ESC | ~['\\\r\n] ) '\''
  ;

STRING
  : '"' ( ESC | ~["\\\r\n] )* '"'
  ;

HEXINT
  : '0' [xX] HEX+
  ;

INT
  : DIGITS
  ;

FLOAT
    : DIGITS+ '.' DIGITS* ([eE] [+-]? DIGITS+)? ;

// Operators - MUST be defined before OTHER
PLUS     : '+' ;
MINUS    : '-' ;
MULTIPLY : '*' ;
DIVIDE   : '/' ;
LPAREN   : '(' ;
RPAREN   : ')' ;

WS : [ \t\r\n]+ -> channel(HIDDEN) ;

fragment ESC
  : '\\' [0btnfr"'\\]
  | '\\' [uU] '{' HEX+ '}'
  ;

fragment DIGITS : [0-9]+ ;

fragment HEX : [0-9A-Fa-f];

// Anything else: letters, punctuation, etc.
// Make it "as small as possible" so expressions can still be recognized.
OTHER
    : .
    ;