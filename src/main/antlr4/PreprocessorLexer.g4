lexer grammar PreprocessorLexer;

// ---- Default mode rules first ----
// Comments
BLOCK_COMMENT : '/*' .*? '*/' NL? -> channel(HIDDEN) ;
LINE_COMMENT  : [ \t]* '//' ~[\r\n]* -> channel(HIDDEN) ;

// backslash at end of physical line (plus optional spaces)
LINE_CONT :  [ \t]* '\\' [ \t]* ;     // backslash at end of physical line (plus optional spaces)

// Whitespace and newlines
WS : [ \t]+ -> channel(HIDDEN) ;
NL : ('\r'? '\n') ;

// Use a mode for processing arguments to preprocessor directives
PP_INCLUDE  : '#' [iI][nN][cC][lL][uU][dD][eE]       -> pushMode(DIRECTIVE_ARGS) ;
PP_DEFINE   : '#' [dD][eE][fF][iI][nN][eE]           -> pushMode(DIRECTIVE_ARGS) ;
PP_UNDEF    : '#' [uU][nN][dD][eE][fF]               -> pushMode(DIRECTIVE_ARGS) ;
PP_CALL     : '#' [cC][aA][lL][lL]                   -> pushMode(DIRECTIVE_ARGS) ;
PP_DEF_FUNC : '#' [dD][eE][fF]([iI][nN][eE])? '_' [fF][uU][nN][cC]([tT][iI][oO][nN])? -> pushMode(DIRECTIVE_ARGS) ;
PP_END_FUNC : '#' [eE][nN][dD] '_' [fF][uU][nN][cC]([tT][iI][oO][nN])? ;
PP_DEF_MACRO: '#' [dD][eE][fF]([iI][nN][eE])? '_' [mM][aA][cC][rR][oO] -> pushMode(DIRECTIVE_ARGS) ;
PP_END_MACRO: '#' [eE][nN][dD] '_' [mM][aA][cC][rR][oO] ;
PP_MACRO    : '#' [mM][aA][cC][rR][oO]               -> pushMode(DIRECTIVE_ARGS) ;
PP_GLOBAL   : '#' [gG][lL][oO][bB][aA][lL] ;
PP_SVAR     : '#' [sS][vV][aA][rR]                   -> pushMode(DIRECTIVE_ARGS) ;
PP_VAR      : '#' [vV][aA][rR]                       -> pushMode(DIRECTIVE_ARGS) ;
PP_FVAR     : '#' [fF][vV][aA][rR]                   -> pushMode(DIRECTIVE_ARGS) ;
PP_RETURN   : '#' [rR][eE][tT][uU][rR][nN]           -> pushMode(DIRECTIVE_ARGS) ;
PP_FRETURN  : '#' [fF][rR][eE][tT][uU][rR][nN]       -> pushMode(DIRECTIVE_ARGS) ;
PP_IF       : '#' [iI][fF]				             -> pushMode(DIRECTIVE_ARGS) ;
PP_IFDEF    : '#' [iI][fF][dD][eE][fF]	     		-> pushMode(DIRECTIVE_ARGS) ;
PP_IFNDEF   : '#' [iI][fF][nN][dD][eE][fF]    		-> pushMode(DIRECTIVE_ARGS) ;
PP_ELSEIF   : '#' [eE][lL][sS][eE] '_' [iI][fF]     -> pushMode(DIRECTIVE_ARGS) ;
PP_ELSE     : '#' [eE][lL][sS][eE] ;
PP_ENDIF    : '#' [eE][nN][dD] '_'? [iI][fF] ;
PP_FOR		: '#' [fF][oO][rR]						 -> pushMode(DIRECTIVE_ARGS) ;
PP_ENDFOR	: '#' [eE][nN][dD] '_' [fF][oO][rR] ;
PP_WHILE	: '#' [wW][hH][iI][lL][eE]				 -> pushMode(DIRECTIVE_ARGS) ;
PP_ENDWHILE	: '#' [eE][nN][dD] '_' [wW][hH][iI][lL][eE] ;
PP_DOWHILE	: '#' [dD][oO] '_' [wW][hH][iI][lL][eE] ;
PP_ENDDOWHILE: '#' [eE][nN][dD] '_' [dD][oO] '_' [wW][hH][iI][lL][eE] -> pushMode(DIRECTIVE_ARGS) ;
PP_BREAK    : '#' [bB][rR][eE][aA][kK]               -> pushMode(DIRECTIVE_ARGS) ;
PP_CONTINUE : '#' [cC][oO][nN][tT][iI][nN][uU][eE]   -> pushMode(DIRECTIVE_ARGS) ;
PP_IFCONDSR : '#' ([iI][fF]) '_' [cC][oO][nN][dD] '_' [sS][rR]	 -> pushMode(DIRECTIVE_ARGS) ;
PP_IFCOND   : '#' ([iI][fF]) '_' [cC][oO][nN][dD]	 -> pushMode(DIRECTIVE_ARGS) ;
PP_ELSEIFCOND: '#' [eE][lL][sS][eE] '_' [iI][fF] '_' [cC][oO][nN][dD] -> pushMode(DIRECTIVE_ARGS) ;
PP_ELSECOND : '#' [eE][lL][sS][eE] '_' [cC][oO][nN][dD] ;
PP_ENDCOND  : '#' [eE][nN][dD] '_' [cC][oO][nN][dD] ('_' [sR][rR])? ;
PP_SYNC     : '#' [sS][yY][nN][cC] -> pushMode(DIRECTIVE_ARGS) ;
PP_ENDSYNC  : '#' [eE][nN][dD] '_' [sS][yY][nN][cC] ;

REG_R : [rR] [0-9]+ ;
REG_F : [fF] [0-9]+ ;
PLACEHOLDER: '$' '{' ([\p{L}_] [\p{L}\p{Nd}_]* | ELLIPSIS)'}' ;
IDENT : [\p{L}_$] [\p{L}\p{Nd}_$]* ;

INT   : '-'? ([0-9]+ | ('0' [xX] [0-9a-fA-F]+)) ;
FLOAT
  : '-'? DIGITS '.' DIGITS* ( [eE] [+\-]? DIGITS )?
  | '-'? '.' DIGITS         ( [eE] [+\-]? DIGITS )?
  | '-'? DIGITS             ( [eE] [+\-]? DIGITS )
  ;
fragment DIGITS : [0-9]+ ;
CHAR  : '\'' ( ESC | ~['\\\r\n] ) '\'' ;
STRING: '"' ( ESC | ~["\\\r\n] )* '"' ;
ANGLE_PATH : '<' (~[>\r\n])+ '>' ;
// Fragments: **no** actions/commands here
fragment ESC : '\\' [0btnfr"'\\] | '\\' [uU] LCURLY (HEX)+ RCURLY ;
fragment HEX : [0-9A-Fa-f] ;
EQEQ : '==' | [eE][qQ] ;
NEQ : '!=' | [nN][eE] ;
LE : '<=' | [lL][eE] ;
GE : '>=' | [gG][eE] ;
LT : '<' | [lL][tT] ;
GT : '>' | [gG][tT] ;
COMMA: ',' ;
LPAREN:'(' ;
RPAREN:')' ;
DOLLAR: '$' ;
LCURLY: '{' ;
RCURLY: '}' ;
ELLIPSIS : '.' '.' '.' ;
COLON: ':' ;

// stop CODE_TEXT at newline OR before '//' or '/*'
// Must contain at least one non-# non-newline char
CODE_TEXT
  : ~[# \t\r\n/\\] ( [ \t]* CODE_CHAR )*
  ;

fragment CODE_CHAR
  : ~[\r\n/\\ \t]
  | '/' ~[/*\r\n]		// Not a comment start
  | '\\' ~[ \t\r\n]		// backslash not followed by newline
  ;

// ---- Then the mode block ----
mode DIRECTIVE_ARGS;

LINE_COMMENT_D : [ \t]* '//' ~[\r\n]* -> channel(HIDDEN) ;
WS_D : [ \t]+ -> channel(HIDDEN) ;
NL_D : ('\r'? '\n')+ -> type(NL), popMode ;
ANGLE_PATH_D : '<' (~[>\r\n])+ '>' -> type(ANGLE_PATH) ;
STRING_D	: '"' ( ESC | ~["\\\r\n] )* '"' -> type(STRING) ;
CHAR_D		: '\'' ( ESC | ~['\\\r\n] ) '\'' -> type(CHAR) ;
FLOAT_D		: '-'? [0-9]+ '.' [0-9]+ ([eE] [+-]? [0-9]+)? -> type(FLOAT) ;
INT_D        : '-'? ([0-9]+ | ('0' [xX] [0-9a-fA-F]+)) -> type(INT) ;
PLACEHOLDER_D: '$' '{' ([\p{L}_] [\p{L}\p{Nd}_]* | ELLIPSIS)'}' -> type(PLACEHOLDER) ;
REG_R_D      : [rR] [0-9]+ -> type(REG_R) ;
REG_F_D      : [fF] [0-9]+ -> type(REG_F) ;
EQEQ_D       : ('==' | [eE][qQ]) -> type(EQEQ) ;
NEQ_D        : ('!=' | [nN][eE]) -> type(NEQ) ;
LE_D         : ('<=' | [lL][eE]) -> type(LE) ;
GE_D         : ('>=' | [gG][eE])-> type(GE) ;
LT_D         : ('<' | [lL][tT]) -> type(LT) ;
GT_D         : ('>' | [gG][tT]) -> type(GT) ;
IDENT_D		 : [\p{L}_$] [\p{L}\p{Nd}_${}]* -> type(IDENT) ;
COMMA_D      : ',' -> type(COMMA) ;
LPAREN_D     : '(' -> type(LPAREN) ;
RPAREN_D     : ')' -> type(RPAREN) ;
DOLLAR_D	 : '$' -> type(DOLLAR) ;
ELLIPSIS_D 	 : '.' '.' '.' -> type(ELLIPSIS) ;
COLON_D		 : ':' -> type(COLON) ;
