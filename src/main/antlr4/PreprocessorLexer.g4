lexer grammar PreprocessorLexer;

// ---- Default mode rules first ----
// Comments
BLOCK_COMMENT : '/*' ( . | '\r' | '\n' )*? '*/' NL? -> channel(HIDDEN) ;
LINE_COMMENT  : [ \t]* '//' ~[\r\n]* -> channel(HIDDEN) ;

// Whitespace and newlines
WS : [ \t\f]+ -> channel(HIDDEN) ;
NL : ('\r'? '\n')+ ;

PP_INCLUDE  : '#' [iI][nN][cC][lL][uU][dD][eE]       -> pushMode(DIRECTIVE_ARGS) ;
PP_DEFINE   : '#' [dD][eE][fF][iI][nN][eE]           -> pushMode(DIRECTIVE_ARGS) ;
PP_UNDEF    : '#' [uU][nN][dD][eE][fF]               -> pushMode(DIRECTIVE_ARGS) ;
PP_CALL     : '#' [cC][aA][lL][lL]                   -> pushMode(DIRECTIVE_ARGS) ;
PP_DEF_FUNC : '#' [dD][eE][fF] '_'? [fF][uU][nN][cC] -> pushMode(DIRECTIVE_ARGS) ;
PP_END_FUNC : '#' [eE][nN][dD] '_'? [fF][uU][nN][cC] ;
PP_SVAR     : '#' [sS][vV][aA][rR]                   -> pushMode(DIRECTIVE_ARGS) ;
PP_VAR      : '#' [vV][aA][rR]                       -> pushMode(DIRECTIVE_ARGS) ;
PP_FVAR     : '#' [fF][vV][aA][rR]                   -> pushMode(DIRECTIVE_ARGS) ;
PP_RETURN   : '#' [rR][eE][tT][uU][rR][nN]           -> pushMode(DIRECTIVE_ARGS) ;
PP_FRETURN  : '#' [fF][rR][eE][tT][uU][rR][nN]       -> pushMode(DIRECTIVE_ARGS) ;
PP_IF       : '#' [iI][fF]                           -> pushMode(DIRECTIVE_ARGS) ;
PP_ELSEIF   : '#' [eE][lL][sS][eE] '_'? [iI][fF]     -> pushMode(DIRECTIVE_ARGS) ;
PP_ELSE     : '#' [eE][lL][sS][eE] ;
PP_ENDIF    : '#' [eE][nN][dD] '_'? [iI][fF] ;
REG_R : [rR] [0-9]+ ;
REG_F : [fF] [0-9]+ ;
IDENT : [A-Za-z_.$] [A-Za-z0-9_.$]* ;
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
fragment ESC : '\\' [btnfr"'\\] | '\\' 'u' HEX HEX HEX HEX ;
fragment HEX : [0-9A-Fa-f] ;
EQEQ : '==' ; NEQ : '!=' ; LE : '<=' ; GE : '>=' ; LT : '<' ; GT : '>' ;
COMMA: ',' ; LPAREN:'(' ; RPAREN:')' ;

// helper: next chars are NOT the start of a comment
fragment NOT_COMMENT_START
  : { _input.LA(1) != '/' || (_input.LA(1) == '/' && _input.LA(2) != '/' && _input.LA(2) != '*') }?
  ;
// stop CODE_TEXT at newline OR before '//' or '/*'
CODE_TEXT
  : [ \t]* ( ~[#\r\n] ( NOT_COMMENT_START ~[\r\n] )* )?
    { setText(getText().replaceFirst("[ \\t]+$", "")); }
  ;

// ---- Then the mode block ----
mode DIRECTIVE_ARGS;

WS_D : [ \t\f]+ -> channel(HIDDEN) ;
NL_D : ('\r'? '\n')+ -> type(NL), popMode ;
ANGLE_PATH_D : '<' (~[>\r\n])+ '>' -> type(ANGLE_PATH) ;
STRING_D	: '"' ( ESC | ~["\\\r\n] )* '"' -> type(STRING) ;
CHAR_D		: '\'' ( ESC | ~['\\\r\n] ) '\'' -> type(CHAR) ;
FLOAT_D		: '-'? [0-9]+ '.' [0-9]+ ([eE] [+-]? [0-9]+)? -> type(FLOAT) ;
INT_D        : '-'? ([0-9]+ | ('0' [xX] [0-9a-fA-F]+)) -> type(INT) ;
IDENT_D      : [A-Za-z_.$] [A-Za-z0-9_.$]* -> type(IDENT) ;
REG_R_D      : [rR] [0-9]+ -> type(REG_R) ;
REG_F_D      : [fF] [0-9]+ -> type(REG_F) ;
EQEQ_D       : '==' -> type(EQEQ) ;
NEQ_D        : '!=' -> type(NEQ) ;
LE_D         : '<=' -> type(LE) ;
GE_D         : '>=' -> type(GE) ;
LT_D         : '<' -> type(LT) ;
GT_D         : '>' -> type(GT) ;
COMMA_D      : ',' -> type(COMMA) ;
LPAREN_D     : '(' -> type(LPAREN) ;
RPAREN_D     : ')' -> type(RPAREN) ;
LINE_COMMENT_D : '//' ~[\r\n]* -> channel(HIDDEN) ;
