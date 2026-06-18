parser grammar PreprocessorParser;
options { tokenVocab=PreprocessorLexer; }

// ============================================================================
// preprocesor.g4  (preprocessor that runs before the assembler)
// Case-insensitive directive keywords; supports includes/defines/calls/functions
// and conditional compilation (#if / #elseif / #else / #endif).
// ============================================================================

/* =======================
   PARSER
   ======================= */

preproc
  : ( directive | codeLine | NL)* EOF
  ;

// Any line that does not start with '#' is passed through as code
codeLine
  : more+=(LABEL | IDENT | COMP_DIR)
    ( LINE_CONT? (more+=(COMMA | LBRACKET | RBRACKET | PLUS | IDENT | COMP_DIR | PLACEHOLDER | REG_R | REG_F | MEMREF | STRING | CHAR)
    | moreExpr+=constExpr ))*
    NL
  ;

/* ----- Directives ----- */

directive
  : includeDir
  | defineDir
  | undefDir
  | callDir
  | defFuncDir
  | defMacroDir
  | macroDir
  | globalDir
  | svarDir
  | varDir
  | fvarDir
  | returnDir
  | freturnDir
  | ifBlock
  | ifDefBlock
  | ifNDefBlock
  | forBlock
  | whileBlock
  | doWhileBlock
  | breakDir
  | continueDir
  | ifCondBlock
  | ifCondSRBlock
  | syncBlock
  | infoDir
  | errorDir
  ;

infoDir
  : PP_INFO INFO_TEXT? NL+
  ;

errorDir
  : PP_ERROR INFO_TEXT? NL+
  ;

/* #include <path> | #include "path" */
includeDir
  : PP_INCLUDE ( ANGLE_PATH | STRING ) NL+
  ;

/* #define NAME [literal] */
defineDir
  : PP_DEFINE id=IDENT ( lit=literal | symbol=IDENT )? NL+
  ;

/* #undef NAME */
undefDir
  : PP_UNDEF IDENT NL+
  ;

/* #call funcName '(' argList? ')' */
callDir
  : PP_CALL (IDENT | PLACEHOLDER) LPAREN ( argList )? RPAREN NL+
  ;

/* #macro macroName '(' argList? ')' */
macroDir
  : PP_MACRO (IDENT | PLACEHOLDER) LPAREN ( argList )? RPAREN NL+
  ;

argList
  : callArg ( COMMA (LINE_CONT)? callArg )*
  ;

callArg
  : literal
  | cmpOp
  | PLACEHOLDER
  | IDENT
  | REG_R
  | REG_F
  ;

/* #def_func name '(' paramList? ')' ... #end_func */
defFuncDir
  : PP_DEF_FUNC IDENT LPAREN ( paramList )? RPAREN NL+
      block
    PP_END_FUNC NL+
  ;

/* #def_macro name '(' paramList? ')' ... #end_macro */
defMacroDir
  : PP_DEF_MACRO IDENT LPAREN ( paramList )? RPAREN NL+
      block
    PP_END_MACRO NL+
  ;

paramList
  : IDENT? ( COMMA IDENT )* ( COMMA? ELLIPSIS )?
  ;

/* #global IDENT .dc? .* */
globalDir
  : PP_GLOBAL codeLine
  ;

/* #svar a, b, c      (stack variables / aliases) */
svarDir
  : PP_SVAR identList NL+
  ;

/* #var a, b          (integer register aliases) */
varDir
  : PP_VAR identList NL+
  ;

/* #fvar a, b         (float register aliases) */
fvarDir
  : PP_FVAR identList NL+
  ;

identList
  : IDENT ( COMMA (LINE_CONT)? IDENT )*
  ;

/* #return name */
returnDir
  : PP_RETURN primary NL+
  ;

/* #freturn name */
freturnDir
  : PP_FRETURN primary NL+
  ;

/* ----- Preprocessor Conditional blocks (explicit arms) ----- */

ifBlock
  : PP_IF expr NL+ block (elseifClause)* (elseClause)? PP_ENDIF NL+
  ;

ifDefBlock
  : PP_IFDEF primary NL+ block (elseClause)? PP_ENDIF NL+
  ;

ifNDefBlock
  : PP_IFNDEF primary NL+ block (elseClause)? PP_ENDIF NL+
  ;

elseifClause
  : PP_ELSEIF expr NL+ block
  ;

elseClause
  : PP_ELSE NL+ block
  ;

/* ---- Higher level control structures ---- */

forBlock
  : PP_FOR init=primary? COMMA? cond=expr COMMA? incr=primary? NL+ block PP_ENDFOR NL+
  ;

whileBlock
  : PP_WHILE cond=expr NL+ block PP_ENDWHILE NL+
  ;

doWhileBlock
  : PP_DOWHILE NL+ block PP_ENDDOWHILE cond=expr NL+
  ;

breakDir
  : PP_BREAK NL+
  ;

continueDir
  : PP_CONTINUE NL+
  ;
/* ----- assembly conditional blocks (explicit arms) ----- */

ifCondBlock
  : PP_IFCOND cond=expr NL+ block (elseifCondClause)* (elseCondClause)? PP_ENDCOND NL+
  ;

ifCondSRBlock
  : PP_IFCONDSR (IDENT|cmpOp) NL+ block (elseCondClause)? PP_ENDCOND NL+
  ;

elseifCondClause
  : PP_ELSEIFCOND cond=expr NL+ block
  ;

elseCondClause
  : PP_ELSECOND NL+ block
  ;

syncBlock
  : PP_SYNC LPAREN? IDENT (LBRACKET offset=(IDENT|INT) RBRACKET?)? RPAREN? NL+ block PP_ENDSYNC NL+
  ;

block
  : (directive | codeLine | NL)*
  ;

/* Simple expressions for #if/#elseif:
   primary cmpOp primary
*/
expr
  : primary COMMA? ( cmpOp COMMA? primary )?
  ;

primary
  : IDENT
  | REG_R
  | REG_F
  | literal
  ;

cmpOp
  : EQEQ
  | NEQ
  | LT
  | LE
  | GT
  | GE
  ;

/* ----- Literals ----- */
literal
  : INT
  | FLOAT
  | CHAR
  | STRING
  | constExpr
  ;

constExpr
    : constExpr op=(MULTIPLY | DIVIDE) constExpr    # mulExpr
    | constExpr op=(PLUS | MINUS) constExpr         # addExpr
    | MINUS constExpr                          		# unaryExpr
    | LPAREN constExpr RPAREN                  		# parensExpr
    | atom                                			# atomExpr
    ;

atom
    : IDENT | PLACEHOLDER | INT | FLOAT
    ;
