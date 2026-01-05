grammar ConstExpr;

line
    : part* EOF
    ;

part
    : expr                     		# PartExpr
    | CHAR							# PartChar
    | STRING						# PartString
    | OTHER                      	# PartOther
    ;

/*
 * Lowest precedence: + -
 */
expr
    : addExpr
    ;

addExpr
    : addExpr op=('+' | '-') mulExpr
    | mulExpr
    ;

/*
 * Next precedence: * /
 */
mulExpr
    : mulExpr op=('*' | '/') unaryExpr
    | unaryExpr
    ;

/*
 * Unary negation
 */
unaryExpr
    : '-' unaryExpr
    | primary
    ;

/*
 * Literals and grouping
 */
primary
    : INT
    | HEXINT
    | FLOAT
    | '(' addExpr ')'
    ;

CHAR
  : '\'' ( ESC | ~['\\\r\n] ) '\''
  ;

STRING
  : '"' ( ESC | ~["\\\r\n] )* '"'
  ;

HEXINT
  : '-'? '0' [xX] HEX+
  ;

INT
  : DIGITS
  ;

FLOAT
    : DIGITS+ '.' DIGITS* ([eE] [+-]? DIGITS+)? ;

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