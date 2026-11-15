/*
 * VBLexer.g4 - Visual Basic .NET lexer, derived from vb.g4 (Visual Basic
 * Language Specification 11, section 2 "Lexical grammar"; see README.md and
 * the NOTICE file).  Used only by tests/antlr_check.sh to check the
 * compiler's hand-written lexer and parser (src/lexer.c, src/parser.c).
 *
 * Changes from vb.g4:
 *  - real lexer rules with caseInsensitive instead of documentation rules;
 *    placeholder literals ('<Unicode classes ...>') replaced by \p{..} sets;
 *  - one token per reserved keyword (rule Keyword), generated from
 *    src/keywords.inc; REM starts a comment;
 *  - contextual keywords (Strict, From, Where, ...) are tokens here and are
 *    accepted as identifiers by the parser rule 'identifier';
 *  - type characters '!' and '&' after an identifier, and implicit line
 *    continuation (vb.g4 attaches LineTerminator? to ',' '(' '{' '.' '='
 *    ':=' ')' '}' and the binary operators), are applied by the test
 *    driver's token filter (tests/antlr/VBCheck.java), exactly as
 *    src/lexer.c join_lines() does, because ANTLR grammars in grammars-v4
 *    style have no actions or predicates;
 *  - preprocessing directive lines (section 3) are skipped; conditional
 *    compilation is not evaluated, so the check corpus does not use #If;
 *  - date literals are not part of the language (Date is not supported);
 *  - EXT: additions after version 11: interpolated strings ($"..."),
 *    '?.', binary literals (&B), digit separators ('_'), multi-line
 *    strings and comments after a line continuation.
 */
lexer grammar VBLexer;

options { caseInsensitive = true; }

// ---------------------------------------------------------------- trivia

LINE_CONTINUATION
    : [ \t]+ '_' [ \t]* ( SingleQuote ~[\r\n\u2028\u2029]* )? LineTerminator -> skip
    ;

COMMENT
    : ( SingleQuote | 'REM' ) ( [ \t\u00A0] ~[\r\n\u2028\u2029]* )? -> skip
    ;

DIRECTIVE
    : '#' [ \t]* [a-z] ~[\r\n\u2028\u2029]* -> skip
    ;

NEWLINE
    : LineTerminator
    ;

WS
    : [ \t\u00A0\u1680\u2000-\u200A\u202F\u205F\u3000\uFEFF]+ -> skip
    ;

// ---------------------------------------------------------------- keywords

ADDHANDLER        : 'AddHandler';
ADDRESSOF         : 'AddressOf';
ALIAS             : 'Alias';
AND               : 'And';
ANDALSO           : 'AndAlso';
AS                : 'As';
BOOLEAN           : 'Boolean';
BYREF             : 'ByRef';
BYTE              : 'Byte';
BYVAL             : 'ByVal';
CALL              : 'Call';
CASE              : 'Case';
CATCH             : 'Catch';
CBOOL             : 'CBool';
CBYTE             : 'CByte';
CCHAR             : 'CChar';
CDATE             : 'CDate';
CDBL              : 'CDbl';
CDEC              : 'CDec';
CHAR              : 'Char';
CINT              : 'CInt';
CLASS             : 'Class';
CLNG              : 'CLng';
COBJ              : 'CObj';
CONST             : 'Const';
CONTINUE          : 'Continue';
CSBYTE            : 'CSByte';
CSHORT            : 'CShort';
CSNG              : 'CSng';
CSTR              : 'CStr';
CTYPE             : 'CType';
CUINT             : 'CUInt';
CULNG             : 'CULng';
CUSHORT           : 'CUShort';
DATE              : 'Date';
DECIMAL           : 'Decimal';
DECLARE           : 'Declare';
DEFAULT           : 'Default';
DELEGATE          : 'Delegate';
DIM               : 'Dim';
DIRECTCAST        : 'DirectCast';
DO                : 'Do';
DOUBLE            : 'Double';
EACH              : 'Each';
ELSE              : 'Else';
ELSEIF            : 'ElseIf';
END               : 'End';
ENDIF             : 'EndIf';
ENUM              : 'Enum';
ERASE             : 'Erase';
ERROR             : 'Error';
EVENT             : 'Event';
EXIT              : 'Exit';
FALSE             : 'False';
FINALLY           : 'Finally';
FOR               : 'For';
FRIEND            : 'Friend';
FUNCTION          : 'Function';
GET               : 'Get';
GETTYPE           : 'GetType';
GETXMLNAMESPACE   : 'GetXmlNamespace';
GLOBAL            : 'Global';
GOSUB             : 'GoSub';
GOTO              : 'GoTo';
HANDLES           : 'Handles';
IF                : 'If';
IMPLEMENTS        : 'Implements';
IMPORTS           : 'Imports';
IN                : 'In';
INHERITS          : 'Inherits';
INTEGER           : 'Integer';
INTERFACE         : 'Interface';
IS                : 'Is';
ISNOT             : 'IsNot';
LET               : 'Let';
LIB               : 'Lib';
LIKE              : 'Like';
LONG              : 'Long';
LOOP              : 'Loop';
ME                : 'Me';
MOD               : 'Mod';
MODULE            : 'Module';
MUSTINHERIT       : 'MustInherit';
MUSTOVERRIDE      : 'MustOverride';
MYBASE            : 'MyBase';
MYCLASS           : 'MyClass';
NAMESPACE         : 'Namespace';
NARROWING         : 'Narrowing';
NEW               : 'New';
NEXT              : 'Next';
NOT               : 'Not';
NOTHING           : 'Nothing';
NOTINHERITABLE    : 'NotInheritable';
NOTOVERRIDABLE    : 'NotOverridable';
OBJECT            : 'Object';
OF                : 'Of';
ON                : 'On';
OPERATOR          : 'Operator';
OPTION            : 'Option';
OPTIONAL          : 'Optional';
OR                : 'Or';
ORELSE            : 'OrElse';
OVERLOADS         : 'Overloads';
OVERRIDABLE       : 'Overridable';
OVERRIDES         : 'Overrides';
PARAMARRAY        : 'ParamArray';
PARTIAL           : 'Partial';
PRIVATE           : 'Private';
PROPERTY          : 'Property';
PROTECTED         : 'Protected';
PUBLIC            : 'Public';
RAISEEVENT        : 'RaiseEvent';
READONLY          : 'ReadOnly';
REDIM             : 'ReDim';
REMOVEHANDLER     : 'RemoveHandler';
RESUME            : 'Resume';
RETURN            : 'Return';
SBYTE             : 'SByte';
SELECT            : 'Select';
SET               : 'Set';
SHADOWS           : 'Shadows';
SHARED            : 'Shared';
SHORT             : 'Short';
SINGLE            : 'Single';
STATIC            : 'Static';
STEP              : 'Step';
STOP              : 'Stop';
STRING            : 'String';
STRUCTURE         : 'Structure';
SUB               : 'Sub';
SYNCLOCK          : 'SyncLock';
THEN              : 'Then';
THROW             : 'Throw';
TO                : 'To';
TRUE              : 'True';
TRY               : 'Try';
TRYCAST           : 'TryCast';
TYPEOF            : 'TypeOf';
UINTEGER          : 'UInteger';
ULONG             : 'ULong';
USHORT            : 'UShort';
USING             : 'Using';
VARIANT           : 'Variant';
WEND              : 'Wend';
WHEN              : 'When';
WHILE             : 'While';
WIDENING          : 'Widening';
WITH              : 'With';
WITHEVENTS        : 'WithEvents';
WRITEONLY         : 'WriteOnly';
XOR               : 'Xor';

// contextual keywords (identifiers in other places)

AGGREGATE         : 'Aggregate';
ANSI              : 'Ansi';
ASCENDING         : 'Ascending';
ASSEMBLY          : 'Assembly';
ASYNC             : 'Async';
AUTO              : 'Auto';
AWAIT             : 'Await';
BINARY            : 'Binary';
BY                : 'By';
COMPARE           : 'Compare';
CUSTOM            : 'Custom';
DESCENDING        : 'Descending';
DISTINCT          : 'Distinct';
EQUALS            : 'Equals';
EXPLICIT          : 'Explicit';
FROM              : 'From';
GROUP             : 'Group';
INFER             : 'Infer';
INTO              : 'Into';
ISFALSE           : 'IsFalse';
ISTRUE            : 'IsTrue';
ITERATOR          : 'Iterator';
JOIN              : 'Join';
KEY               : 'Key';
MID               : 'Mid';
NAMEOF            : 'NameOf';
OFF               : 'Off';
ORDER             : 'Order';
OUT               : 'Out';
PRESERVE          : 'Preserve';
SKIP_KW           : 'Skip';
STRICT            : 'Strict';
TAKE              : 'Take';
TEXT              : 'Text';
UNICODE           : 'Unicode';
UNTIL             : 'Until';
WHERE             : 'Where';
YIELD             : 'Yield';
REGION            : 'Region';

// ---------------------------------------------------------------- literals

INTEGER_LITERAL
    : ( Digits | '&H' HexDigits | '&O' OctDigits | '&B' BinDigits ) IntegralSuffix?
    ;

FLOAT_LITERAL
    : ( Digits '.' Digits Exponent? | '.' Digits Exponent? | Digits Exponent ) FloatSuffix?
    | Digits FloatSuffix
    ;

STRING_LITERAL
    : DQ ( ~["\u201C\u201D] | DQ DQ )* DQ
    ;

CHAR_LITERAL
    : DQ ( ~["\u201C\u201D] | DQ DQ ) DQ 'C'
    ;

// EXT: interpolated strings
INTERPOLATED_STRING
    : '$' DQ ( ~["\u201C\u201D{}] | DQ DQ | '{{' | '}}' | '{' InterpHole '}' )* DQ
    ;

// ---------------------------------------------------------------- identifiers

IDENTIFIER
    : IdentifierName TypeCharacter?
    | '[' IdentifierName ']' TypeCharacter?
    ;

// ---------------------------------------------------------------- punctuation

LPAREN      : '(';
RPAREN      : ')';
LBRACE      : '{';
RBRACE      : '}';
BANG        : '!';
HASH        : '#';
COMMA       : ',';
DOT         : '.';
QDOT        : '?.';     // EXT
QUESTION    : '?';
COLONEQ     : ':=';
COLON       : ':';
AMPEQ       : '&=';
AMP         : '&';
STAREQ      : '*=';
STAR        : '*';
PLUSEQ      : '+=';
PLUS        : '+';
MINUSEQ     : '-=';
MINUS       : '-';
SLASHEQ     : '/=';
SLASH       : '/';
BACKSLASHEQ : '\\=';
BACKSLASH   : '\\';
CARETEQ     : '^=';
CARET       : '^';
LE          : '<=';
NE          : '<>';
SHLEQ       : '<<=';
SHL         : '<<';
LT          : '<';
EQ          : '=';
GE          : '>=';
SHREQ       : '>>=';
SHR         : '>>';
GT          : '>';

// ---------------------------------------------------------------- fragments

fragment LineTerminator : '\r' '\n'? | '\n' | '\u2028' | '\u2029' ;
fragment SingleQuote    : ['\u2018\u2019] ;
fragment DQ             : ["\u201C\u201D] ;
fragment Digits         : [0-9] ( '_'* [0-9] )* ;
fragment HexDigits      : [0-9a-f] ( '_'* [0-9a-f] )* ;
fragment OctDigits      : [0-7] ( '_'* [0-7] )* ;
fragment BinDigits      : [01] ( '_'* [01] )* ;
fragment Exponent       : 'E' [+-]? [0-9]+ ;
fragment IntegralSuffix : 'S' | 'US' | 'I' | 'UI' | 'L' | 'UL' | '%' | '&' ;
fragment FloatSuffix    : 'F' | 'R' | 'D' | '!' | '#' | '@' ;
fragment TypeCharacter  : [%@#$] ;
fragment IdentifierName : ( AlphaCharacter | UnderscoreCharacter IdentifierCharacter ) IdentifierCharacter* ;
fragment IdentifierCharacter
    : UnderscoreCharacter | AlphaCharacter | NumericCharacter | CombiningCharacter | FormattingCharacter ;
fragment AlphaCharacter      : [\p{Lu}\p{Ll}\p{Lt}\p{Lm}\p{Lo}\p{Nl}] ;
fragment NumericCharacter    : [\p{Nd}] ;
fragment CombiningCharacter  : [\p{Mn}\p{Mc}] ;
fragment FormattingCharacter : [\p{Cf}] ;
fragment UnderscoreCharacter : [\p{Pc}] ;
fragment InterpHole
    : ( ~["\u201C\u201D{}()] | STRING_LITERAL | '(' InterpHole ')' | '{' InterpHole '}' )*
    ;
