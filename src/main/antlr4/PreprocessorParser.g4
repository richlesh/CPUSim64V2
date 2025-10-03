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
  : ( directive | codeLine )* EOF
  ;

// Any line that does not start with '#' is passed through as code
codeLine
  : IDENT NL
  | CODE_TEXT? NL
  ;

/* ----- Directives ----- */

directive
  : includeDir
  | defineDir
  | undefDir
  | callDir
  | defFuncDir
  | svarDir
  | varDir
  | fvarDir
  | returnDir
  | freturnDir
  | ifBlock
  ;

/* #include <path> | #include "path" */
includeDir
  : PP_INCLUDE ( ANGLE_PATH | STRING ) NL
  ;

/* #define NAME [literal] */
defineDir
  : PP_DEFINE IDENT ( literal )? NL
  ;

/* #undef NAME */
undefDir
  : PP_UNDEF IDENT NL
  ;

/* #call funcName '(' argList? ')' */
callDir
  : PP_CALL IDENT LPAREN ( argList )? RPAREN NL
  ;

argList
  : callArg ( COMMA callArg )*
  ;

callArg
  : literal
  | IDENT
  | REG_R
  | REG_F
  ;

/* #def_func name '(' paramList? ')' ... #end_func */
defFuncDir
  : PP_DEF_FUNC IDENT LPAREN ( paramList )? RPAREN NL
      ( directive | codeLine )*
    PP_END_FUNC NL
  ;

paramList
  : IDENT ( COMMA IDENT )*
  ;

/* #svar a, b, c      (stack variables / aliases) */
svarDir
  : PP_SVAR identList NL
  ;

/* #var a, b          (integer register aliases) */
varDir
  : PP_VAR identList NL
  ;

/* #fvar a, b         (float register aliases) */
fvarDir
  : PP_FVAR identList NL
  ;

identList
  : IDENT ( COMMA IDENT )*
  ;

/* #return name */
returnDir
  : PP_RETURN primary NL
  ;

/* #freturn name */
freturnDir
  : PP_FRETURN primary NL
  ;

/* ----- Conditional blocks (explicit arms) ----- */

ifBlock
  : PP_IF expr NL block (elseifClause)* (elseClause)? PP_ENDIF NL
  ;

elseifClause
  : PP_ELSEIF expr NL block
  ;

elseClause
  : PP_ELSE NL block
  ;

block
  : (directive | codeLine)*
  ;

/* Simple expressions for #if/#elseif:
   IDENT | literal | primary cmpOp primary
*/
expr
  : primary ( cmpOp primary )?
  ;

primary
  : IDENT
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
  ;
