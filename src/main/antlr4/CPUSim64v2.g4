grammar CPUSim64v2;

/* =======================
   PARSER
   ======================= */

program
  : (line)* EOF
  ;

line
  : (labelDef)? (instruction | directive)? NL
  ;

labelDef
  : IDENT ':'
  ;

/* ----- Directives (kept simple; tighten as needed) ----- */
directive
  : dataDirective							# Data_Directive
  | ORG (INTLIT | HEXLIT)					# ORG_Directive
  | LINE FILENAMELIT ','? INTLIT			# LINE_Directive
  | LINE_BEGIN FILENAMELIT ','? INTLIT		# LINE_BEGIN_Directive
  | LINE_END								# LINE_END_Directive
  | BLOCK_BEGIN (IDENT | BLOCK_IDENT)		# BLOCK_BEGIN_Directive
  | BLOCK_END (IDENT | BLOCK_IDENT)?		# BLOCK_END_Directive
  ;

dataDirective
  : DCI (INTLIT | HEXLIT)					// integers
  | DCF FLOATLIT							// floats
  | DCA (INTLIT | HEXLIT)					// Declared array
  | DCB (byteList)							// 8-bit bytes(s)
  | DCC (byteList)							// 16-bit chars(s)
  | DCS STRINGLIT							// strings
  | DCW (intList | floatList | charList)	// array of any of the above
  ;

intList : kLiteral (',' kLiteral)* ;
floatList : FLOATLIT (',' FLOATLIT)* ;
charList : CHARLIT (',' CHARLIT)* ;
byteList : bLiteral (',' bLiteral)* ;

/* =======================
   INSTRUCTIONS (STRICT)
   ======================= */

instruction
  : instrNOP
  | instrDEBUG
  | instrCLEAR
  | instrMOVE
  | instrLOAD
  | instrSTORE
  | instrPOP
  | instrPUSH
  | instrJUMP
  | instrCALL
  | instrRETURN
  | instrINTERRUPT
  | instrSTOP
  | instrNEG
  | instrADD
  | instrSUB
  | instrMULT
  | instrDIV
  | instrRECIP
  | instrCOMPL
  | instrAND
  | instrOR
  | instrXOR
  | instrTEST
  | instrCMP
  | instrLSHIFT
  | instrRSHIFT
  | instrARSHIFT
  | instrLROTATE
  | instrRROTATE
  | instrIN
  | instrOUT
  | instrPACK
  | instrPACK64
  | instrUNPACK
  | instrUNPACK64
  | instrCAS
  | instrENDIAN
  | instrSAVE
  | instrRESTORE
  ;

/* ----- 0 NOP/DEBUG ----- */
instrNOP
  : NOP                               /* N */
  ;

instrDEBUG
  : DEBUG
  | DEBUG y1to4                        			/* Y / YY / YYY / YYYY */
  | DEBUG (aOperand | aLiteral) ',' cLiteral    /* AC (dump start,count) */
  ;

/* ----- 1 CLEAR ----- */
instrCLEAR
  : CLEAR                              /* N (clear all) */
  | CLEAR x1to4                        /* X / XX / XXX / XXXX */
  ;

/* ----- 2 MOVE ----- */
instrMOVE
  : MOVE yOperand ',' yOperand                         			/* YY */
  | MOVE yOperand ',' cLiteral                         			/* YC */
  | MOVE aOperand ',' aOperand ('+' | ',' | '[') rOperand ']'?  /* AAR  (A1 <- A2 + R) */
  | MOVE aOperand ',' aOperand ('+' | ',' | '[') aLiteral ']'?  /* AAC  (A1 <- A2 + C) */
  | MOVE aOperand ',' aLiteral ('+' | ',' | '[') aOperand ']'?  /* ACA  (A1 <- C + A2) */
  | MOVE zCond ',' yOperand ',' qOperand ',' qOperand  /* ZYQQ conditional */
  ;

/* ----- 3 LOAD (memory -> reg/float) ----- */
instrLOAD
  : LOAD yOperand ',' memC             /* YC:  Y <- [C] */
  | LOAD yOperand ',' memA             /* YA:  Y <- [A] */
  | LOAD yOperand ',' memAplusC        /* YAC: Y <- [A+C] */
  | LOAD yOperand ',' memCplusA        /* YCA: Y <- [C+A] */
  | LOAD yOperand ',' memCplusC        /* YCC: Y <- [C+C] */
  | LOAD yOperand ',' memAplusR        /* YAR: Y <- [A+R] */
  ;

/* ----- 4 STORE (reg/float/const -> memory) ----- */
instrSTORE
  : STORE qOperand ',' memC
  | STORE qOperand ',' memA
  | STORE qOperand ',' memAplusC
  | STORE qOperand ',' memCplusA
  | STORE qOperand ',' memCplusC
  | STORE qOperand ',' memAplusR
  ;

/* ----- 5 POP / 6 PUSH ----- */
instrPOP
  : POP                                /* N: SP=SP+1; discard [SP] */
  | POP yOperand                       /* Y: SP=SP+1; Y <- [SP]   */
  ;

instrPUSH
  : PUSH yOperand                      /* Y -> [SP]; SP=SP-1 */
  | PUSH cLiteral                      /* C -> [SP]; SP=SP-1 */
  ;

/* ----- 7 JUMP / 8 CALL ----- */
branchModes
  : '@'? aOperand
  | '@'? cLiteral
  | zCond ',' '@'? aOperand
  | zCond ',' '@'? cLiteral
  | zCond ',' '@'? aOperand '+' cLiteral
  | zCond ',' '@'? cLiteral '+' aOperand
  | zCond ',' '@'? cLiteral '+' cLiteral
  | zCond ',' '@'? aOperand '+' rOperand
  ;

instrJUMP : JUMP branchModes ;

instrCALL : CALL branchModes ;

instrRETURN
  : RETURN                             /* N (the 3 internal pops are implicit) */
  ;

/* ----- 10 INTERRUPT / 11 STOP ----- */
instrINTERRUPT
  : INTERRUPT rOperand                 /* R  (or C, but spec lists R; OS map chooses) */
  | INTERRUPT cLiteral
  ;

instrSTOP
  : STOP
  ;

/* ----- 12 NEG ----- */
instrNEG
  : NEG xOperand
  ;

/* Arithmetic modes for ADD/SUB/MULT/DIV */
arithmeticModes
  : aOperand ',' rOperand              /* AR   */
  | fOperand ',' xOperand              /* FX   */
  | yOperand ',' cLiteral              /* YC   */
  | aOperand ',' aOperand ',' rOperand /* AAR  */
  | aOperand ',' aOperand ',' cLiteral /* AAC  */
  | aOperand ',' cLiteral ',' aOperand /* ACA  */
  | fOperand ',' fOperand ',' xOperand /* FFX  */
  | fOperand ',' fOperand ',' cLiteral /* FFC  */
  | fOperand ',' cLiteral ',' fOperand /* FCF  */
  ;

/* ----- 13 ADD ----- */

instrADD : ADD arithmeticModes ;

/* ----- 14 SUB ----- */
instrSUB : SUB arithmeticModes ;

/* ----- 15 MULT ----- */
instrMULT : MULT arithmeticModes ;

/* ----- 16 DIV & RECIP ----- */
instrDIV
  : DIV arithmeticModes
  | DIV rOperand ',' rOperand ',' rOperand ',' rOperand /* RRRR: q,r = a/b */
  | DIV rOperand ',' rOperand ',' rOperand ',' cLiteral /* RRRC */
  ;

instrRECIP
  : RECIP fOperand                      /* F <- 1/F */
  ;

/* ----- 17..22 bitwise/test/cmp ----- */
instrCOMPL : COMPL rOperand ;           /* R <- ~R */

logicModes
  : rOperand ',' rOperand
  | rOperand ',' cLiteral
  | rOperand ',' rOperand ',' rOperand
  | rOperand ',' rOperand ',' cLiteral
  ;

instrAND : AND logicModes ;

instrOR : OR logicModes ;

instrXOR : XOR logicModes ;

instrTEST
  : TEST xOperand
  ;

instrCMP
  : CMP aOperand ',' aOperand     /* AA  */
  | CMP aOperand ',' cLiteral     /* AC  */
  | CMP fOperand ',' fOperand     /* FF  */
  ;

/* ----- 23..27 shifts/rotates ----- */
instrLSHIFT : LSHIFT logicModes ;

instrRSHIFT : RSHIFT logicModes ;

instrARSHIFT : ARSHIFT logicModes ;

instrLROTATE : LROTATE logicModes ;

instrRROTATE : RROTATE logicModes ;

/* ----- 28..29 IN/OUT ----- */
instrIN
  : IN  xOperand ',' zPort ',' zPort         /* XZZ */
  | IN  xOperand ',' rOperand ',' rOperand   /* XRR */
  | IN  xOperand ',' rOperand ',' zPort      /* XRZ */
  | IN  xOperand ',' zPort ',' rOperand      /* XZR */
  ;

instrOUT
  : OUT qOperand ',' zPort ',' zPort         /* ZZQ */
  | OUT qOperand ',' rOperand ',' rOperand   /* RRQ */
  | OUT qOperand ',' rOperand ',' zPort      /* RZQ */
  | OUT qOperand ',' zPort ',' rOperand      /* ZRQ */
  ;

/* ----- 30..31 PACK/UNPACK (arity-only strictness) ----- */
instrPACK
  : PACK rOperand ',' rOperand
  | PACK rOperand ',' rOperand ',' rOperand
  | PACK rOperand ',' rOperand ',' rOperand ',' rOperand
  ;

instrPACK64
  : PACK64 rOperand ',' rOperand
  | PACK64 rOperand ',' rOperand ',' rOperand
  | PACK64 rOperand ',' rOperand ',' rOperand ',' rOperand
  ;

instrUNPACK
  : UNPACK rOperand ',' rOperand
  | UNPACK rOperand ',' rOperand ',' rOperand
  | UNPACK rOperand ',' rOperand ',' rOperand ',' rOperand
  ;

instrUNPACK64
  : UNPACK64 rOperand ',' rOperand
  | UNPACK64 rOperand ',' rOperand ',' rOperand
  | UNPACK64 rOperand ',' rOperand ',' rOperand ',' rOperand
  ;

instrCAS
  : CAS rOperand ',' rOperand ',' aOperand ',' oOperand  /* RRAO */
  | CAS cLiteral ',' cLiteral ',' aOperand ',' oOperand  /* CCAO */
  | CAS rOperand ',' cLiteral ',' aOperand ',' oOperand  /* RCAO */
  | CAS cLiteral ',' rOperand ',' aOperand ',' oOperand  /* CRAO */
  ;

instrENDIAN
  : ENDIAN rOperand ',' rOperand
  | ENDIAN rOperand ',' zPort
  | ENDIAN zPort ',' rOperand
  | ENDIAN zPort ',' zPort
  ;

instrSAVE
  : SAVE xOperand ',' xOperand ;

instrRESTORE
  : RESTORE xOperand ',' xOperand ;

/* =======================
   OPERAND CATEGORIES
   ======================= */

operand
  : rOperand | fOperand | aOperand
  | cLiteral | kLiteral | eLiteral | pLiteral
  | zCond | zPort
  | memRef
  | IDENT          // symbol/label/const
  ;

rOperand : REG_R ;                             // R: integer register
fOperand : REG_F ;                             // F: float register
aOperand : REG_R | SF | SP | PC ;              // A: address-capable regs
xOperand : rOperand | fOperand ;               // X: R or F
yOperand : aOperand | fOperand ;               // Y: A or F
oOperand : rOperand | cLiteral ;               // O: R or C
qOperand : aOperand | fOperand | cLiteral ;    // Q: A or F or C

/* one-to-four helpers */
// one to four X operands separated by commas
x1to4
  : xOperand
    (',' xOperand)?       // up to 2nd
    (',' xOperand)?       // up to 3rd
    (',' xOperand)?       // up to 4th
  ;

// one to four Y operands separated by commas
y1to4
  : yOperand
    (',' yOperand)?
    (',' yOperand)?
    (',' yOperand)?
  ;

/* ---- literals ---- */
bLiteral : INTLIT | HEXLIT | CHARLIT;			// byte literal (8-bit signed; range-check later)
aLiteral : IDENT | INTLIT | HEXLIT ;			// A (32-bit unsigned in spec; range-check later)
cLiteral : IDENT | INTLIT | HEXLIT | CHARLIT;	// C (40-bit signed in spec; range-check later)
kLiteral : INTLIT | HEXLIT ;					// K (64-bit signed)
eLiteral : FLOATLIT ;							// E (64-bit IEEE float)
pLiteral : INTLIT | HEXLIT ;					// P (absolute address)
zPort    : INTLIT | HEXLIT ;					// Z when used as I/O size/port number (0..15) – range-check later

/* ---- conditions (Z when used as cond) ---- */
zCond
  : U | Z | NZ | EQ | NE | N | LT | P | GT | NN | GE | NP | LE | O | NO | PE | PO
  ;

/* ---- memory shapes used by LOAD/STORE ---- */
memRef
  : memC | memA | memAplusC | memCplusA | memCplusC | memAplusR
  ;

memC       : '@'? aLiteral ('[' ']')? ;
memA       : '@'? aOperand ('[' ']')? ;
memAplusC  : '@'? aOperand ('+' | ',' | '[') cLiteral ']'? ;
memCplusA  : '@'? cLiteral ('+' | ',' | '[') aOperand ']'? ;
memCplusC  : '@'? aLiteral ('+' | ',' | '[') cLiteral ']'? ;
memAplusR  : '@'? aOperand ('+' | ',' | '[') rOperand ']'? ;

/* =======================
   LEXER
   ======================= */

/* Mnemonics (case-insensitive via explicit letters) */
NOP      : [nN][oO][pP] ;
DEBUG    : [dD][eE][bB][uU][gG] ;
CLEAR    : [cC][lL][eE][aA][rR] ;
MOVE     : [mM][oO][vV][eE]? ;
LOAD     : [lL][oO][aA][dD]? ;
STORE    : [sS][tT][oO]([rR][eE])? ;
POP      : [pP][oO][pP] ;
PUSH     : [pP][uU][sS][hH] ;
JUMP     : [jJ][uU][mM][pP] ;
CALL     : [cC][aA][lL][lL] ;
RETURN   : [rR][eE][tT]([uU][rR][nN])? ;
INTERRUPT: [iI][nN][tT]([eE][rR][rR][uU][pP][tT])? ;
STOP     : [sS][tT][oO][pP] ;
NEG      : [nN][eE][gG] ;
ADD      : [aA][dD][dD] ;
SUB      : [sS][uU][bB] ;
MULT     : [mM][uU][lL][tT]? ;
DIV      : [dD][iI][vV] ;
RECIP    : [rR][eE][cC][iI][pP] ;
COMPL    : [cC][oO][mM][pP][lL] ;
AND      : [aA][nN][dD] ;
OR       : [oO][rR] ;
XOR      : [xX][oO][rR] ;
TEST     : [tT][eE]?[sS][tT] ;
CMP      : [cC][mM][pP] ;
LSHIFT   : [lL][sS][hH]([iI][fF][tT])? ;
RSHIFT   : [rR][sS][hH]([iI][fF][tT])? ;
ARSHIFT  : [aA][rR][sS][hH]([iI][fF][tT])? ;
LROTATE  : [lL][rR][oO][tT]([aA][tT][eE])? ;
RROTATE  : [rR][rR][oO][tT]([aA][tT][eE])? ;
IN       : [iI][nN] ;
OUT      : [oO][uU][tT] ;
PACK     : [pP][aA][cC][kK] ;
PACK64   : [pP][aA][cC][kK]'64' ;
UNPACK   : [uU][nN][pP][aA][cC][kK] ;
UNPACK64 : [uU][nN][pP][aA][cC][kK]'64' ;
CAS      : [cC][aA][sS] ;
ENDIAN   : [eE][nN][dD][iI][aA][nN] ;
SAVE     : [sS][aA][vV][eE] ;
RESTORE  : [rR][eE][sS][tT]([oO][rR][eE])? ;

/* Registers */
REG_R : [rR] [0-9]+ ;			// r0..r28 (range-check later)
REG_F : [fF] [0-9]+ ;			// f0..f31 (range-check later)
SF    : [sS][fF] ;
SP    : [sS][pP] ;
PC    : [pP][cC] ;
SR    : [sS][rR] ;

/* Condition-code keywords (Z in tables) */
U   : [uU] ;
Z   : [zZ] ;
NZ  : [nN][zZ] ;
EQ  : [eE][qQ] ;
NE  : [nN][eE] ;
N   : [nN] ;
LT  : [lL][tT] ;
P   : [pP] ;
GT  : [gG][tT] ;
NN  : [nN][nN] ;
GE  : [gG][eE] ;
NP  : [nN][pP] ;
LE  : [lL][eE] ;
O   : [oO] ;
NO  : [nN][oO] ;
PE  : [pP][eE] ;
PO  : [pP][oO] ;

/* Literals */
HEXLIT
  : '-'? '0' [xX] [0-9A-Fa-f]+
  ;

INTLIT
  : '-'? DIGITS
  ;

FLOATLIT
  : '-'? DIGITS '.' DIGITS* ( [eE] [+\-]? DIGITS )?
  | '-'? '.' DIGITS         ( [eE] [+\-]? DIGITS )?
  | '-'? DIGITS             ( [eE] [+\-]? DIGITS )
  ;
fragment DIGITS : [0-9]+ ;

CHARLIT
  : '\'' ( ESC | ~['\\\r\n] ) '\''
  ;

STRINGLIT
  : '"' ( ESC | ~["\\\r\n] )* '"'
  ;

FILENAMELIT
  : '\u00ab' ( ESC | ~["\\\r\n] )* '\u00bb'
  ;

fragment ESC
  : '\\' [0btnfr"'\\]
  | '\\' [uU] HEX HEX HEX HEX
  ;

fragment HEX : [0-9A-Fa-f];

/* Identifiers (labels/symbols) */
IDENT : [\p{L}_$] [\p{L}\p{Nd}_$]* ;
BLOCK_IDENT : [\p{L}_] [\p{L}\p{Nd}_{}]* ;

/* Data directives */
DCI : '.' [dD][cC][iI] ;
DCF : '.' [dD][cC][fF] ;
DCA : '.' [dD][cC][aA] ;
DCB : '.' [dD][cC][bB] ;
DCC : '.' [dD][cC][cC] ;		// Don't use
DCW : '.' [dD][cC][wW] ;
DCS : '.' [dD][cC][sS] ;
ORG : '.' [oO][rR][gG] ;
LINE : '.' [lL][iI][nN][eE] ;
LINE_BEGIN : '.' [lL][iI][nN][eE] '_' [bB][eE][gG][iI][nN] ;
LINE_END : '.' [lL][iI][nN][eE] '_' [eE][nN][dD] ;
BLOCK_BEGIN : '.' [bB][lL][oO][cC][kK] ;
BLOCK_END : '.' [bB][lL][oO][cC][kK] '_' [eE][nN][dD] ;

// Comments
BLOCK_COMMENT : '/*' ( . | '\r' | '\n' )*? '*/' -> channel(HIDDEN) ;
LINE_COMMENT  : '//' ~[\r\n]* -> channel(HIDDEN) ;

// Whitespace and newlines
WS : [ \t\f]+ -> channel(HIDDEN) ;
NL : ('\r'? '\n')+ ;