/*
 * VBParser.g4 - Visual Basic .NET parser, derived from vb.g4 (Visual Basic
 * Language Specification 11, sections 4 to 11; see README.md and the NOTICE
 * file).  It describes the language accepted by the compiler's hand-written
 * recursive-descent parser (src/parser.c), and tests/antlr_check.sh checks
 * that the two accept and reject the same files.
 *
 * Changes from vb.g4:
 *  - parser rules (lower-case names) over the tokens of VBLexer.g4;
 *  - Expression is one left-recursive rule with VB operator precedence
 *    (spec 11.12.1): ^, unary - +, * /, \, Mod, + -, &, << >>, relational
 *    and Is IsNot Like TypeOf, Not, And AndAlso, Or OrElse, Xor;
 *  - StatementTerminator is 'terms' (one or more line ends or ':');
 *    implicit line continuation is applied by the token filter of
 *    tests/antlr/VBCheck.java, as by src/lexer.c;
 *  - "For ... Next j, i" sharing a Next is written out (forShared);
 *  - the alternatives that differ in their modifiers are split
 *    (MustOverride members have no body);
 *  - not supported, so not in the grammar: XML literals and XML member
 *    access, Await/Async/Iterator/Yield, Declare, On Error/Resume/Error,
 *    GoSub, Let/Set assignment, date literals and the Mid statement
 *    (Mid(...) = x parses as an assignment to an invocation);
 *  - EXT: additions after version 11: interpolated strings, '?.' and
 *    '?(', NameOf, multi-line lambdas in any expression position.
 */
parser grammar VBParser;

options { tokenVocab = VBLexer; }

// ---------------------------------------------------------------- files (6)

start
    : terms? ( optionStatement terms )* ( importsStatement terms )* ( namespaceMember terms )* EOF
    ;

terms
    : ( NEWLINE | COLON )+
    ;

optionStatement
    : OPTION ( EXPLICIT | STRICT | INFER ) ( ON | OFF )?
    | OPTION COMPARE ( BINARY | TEXT )
    ;

importsStatement
    : IMPORTS importsClause ( COMMA importsClause )*
    ;

importsClause
    : ( identifier EQ )? nonArrayTypeName
    ;

namespaceMember
    : attributes? ( namespaceDeclaration | anyModifier* typeDeclaration )
    | attributes
    ;

namespaceDeclaration
    : NAMESPACE nonArrayTypeName terms ( namespaceMember terms )* END NAMESPACE
    ;

typeDeclaration
    : moduleDeclaration
    | nestedType
    ;

nestedType
    : classDeclaration
    | structureDeclaration
    | interfaceDeclaration
    | enumDeclaration
    | delegateDeclaration
    ;

// ---------------------------------------------------------------- types (7)

classDeclaration
    : CLASS identifier typeParameterList? terms
      ( INHERITS nonArrayTypeName terms )?
      ( IMPLEMENTS nonArrayTypeName ( COMMA nonArrayTypeName )* terms )*
      ( classMember terms )*
      END CLASS
    ;

structureDeclaration
    : STRUCTURE identifier typeParameterList? terms
      ( IMPLEMENTS nonArrayTypeName ( COMMA nonArrayTypeName )* terms )*
      ( classMember terms )*
      END STRUCTURE
    ;

moduleDeclaration
    : MODULE identifier terms ( moduleMember terms )* END MODULE
    ;

interfaceDeclaration
    : INTERFACE identifier typeParameterList? terms
      ( INHERITS nonArrayTypeName ( COMMA nonArrayTypeName )* terms )*
      ( interfaceMember terms )*
      END INTERFACE
    ;

enumDeclaration
    : ENUM identifier ( AS nonArrayTypeName )? terms ( enumMember terms )+ END ENUM
    ;

enumMember
    : attributes? identifier ( EQ expression )?
    ;

delegateDeclaration
    : DELEGATE SUB identifier methodSignature
    | DELEGATE FUNCTION identifier methodSignature functionAs?
    ;

typeName
    : nonArrayTypeName QUESTION? arrayTypeModifier*
    ;

nonArrayTypeName
    : primitiveTypeName
    | ( GLOBAL DOT identifierOrKeyword | identifier ) typeArguments? ( DOT identifierOrKeyword typeArguments? )*
    ;

typeArguments
    : LPAREN OF typeArgument ( COMMA typeArgument )* RPAREN
    ;

typeArgument
    : typeName
    |                               // an open type argument: GetType(List(Of ))
    ;

arrayTypeModifier
    : LPAREN COMMA* RPAREN
    ;

primitiveTypeName
    : BOOLEAN | BYTE | CHAR | DATE | DECIMAL | DOUBLE | INTEGER | LONG | OBJECT | SBYTE
    | SHORT | SINGLE | STRING | UINTEGER | ULONG | USHORT
    ;

typeParameterList
    : LPAREN OF typeParameter ( COMMA typeParameter )* RPAREN
    ;

typeParameter
    : ( IN | OUT )? identifier ( AS ( constraint | LBRACE constraint ( COMMA constraint )* RBRACE ) )?
    ;

constraint
    : NEW | STRUCTURE | CLASS | typeName
    ;

// ---------------------------------------------------------------- attributes (5)

attributes
    : ( LT attribute ( COMMA attribute )* GT )+
    ;

attribute
    : ( ( ASSEMBLY | MODULE ) COLON )? nonArrayTypeName ( LPAREN argumentList RPAREN )?
    ;

// ---------------------------------------------------------------- type members (9)

anyModifier
    : modifier | MUSTOVERRIDE
    ;

modifier
    : PUBLIC | PRIVATE | PROTECTED | FRIEND | SHADOWS | SHARED | OVERRIDABLE | NOTOVERRIDABLE
    | OVERRIDES | OVERLOADS | PARTIAL | MUSTINHERIT | NOTINHERITABLE | READONLY | WRITEONLY
    | WITHEVENTS | DIM | STATIC | DEFAULT | WIDENING | NARROWING
    ;

mustOverride
    : modifier* MUSTOVERRIDE anyModifier*
    ;

classMember
    : attributes? classMemberDeclaration
    | attributes
    ;

moduleMember
    : attributes? moduleMemberDeclaration
    | attributes
    ;

interfaceMember
    : attributes? interfaceMemberDeclaration
    | attributes
    ;

classMemberDeclaration
    : moduleMemberDeclaration
    | anyModifier* operatorDeclaration
    ;

moduleMemberDeclaration
    : anyModifier* nestedType
    | modifier* methodDeclaration
    | mustOverride methodHeader
    | anyModifier* constructorDeclaration
    | modifier* propertyDeclaration
    | mustOverride propertyHeader
    | anyModifier* eventDeclaration
    | anyModifier* CUSTOM eventDeclaration
    | anyModifier* customEventDeclaration
    | anyModifier* CONST variableDeclarators
    | anyModifier+ variableDeclarators
    ;

interfaceMemberDeclaration
    : anyModifier* nestedType
    | anyModifier* methodHeader
    | anyModifier* propertyHeader
    | anyModifier* eventDeclaration
    ;

methodDeclaration
    : SUB identifier methodSignature handlesOrImplements? terms block END SUB
    | FUNCTION identifier methodSignature functionAs? handlesOrImplements? terms block END FUNCTION
    ;

methodHeader
    : SUB identifier methodSignature handlesOrImplements?
    | FUNCTION identifier methodSignature functionAs? handlesOrImplements?
    ;

methodSignature
    : typeParameterList? ( LPAREN parameterList? RPAREN )?
    ;

functionAs
    : AS attributes? typeName
    ;

handlesOrImplements
    : HANDLES handlesTarget ( COMMA handlesTarget )*
    | implementsClause
    ;

handlesTarget
    : ( ME | MYBASE | MYCLASS | identifier ) DOT identifierOrKeyword
    ;

implementsClause
    : IMPLEMENTS implementsTarget ( COMMA implementsTarget )*
    ;

implementsTarget
    : ( GLOBAL DOT identifierOrKeyword | identifier ) typeArguments? ( DOT identifierOrKeyword typeArguments? )+
    ;

parameterList
    : parameter ( COMMA parameter )*
    ;

parameter
    : attributes? ( BYVAL | BYREF | OPTIONAL | PARAMARRAY )* identifier QUESTION? arrayTypeModifier*
      ( AS typeName )? ( EQ expression )?
    ;

constructorDeclaration
    : SUB NEW ( LPAREN parameterList? RPAREN )? terms block END SUB
    ;

propertyHeader                      // auto-implemented, MustOverride or interface property
    : PROPERTY identifier ( LPAREN parameterList? RPAREN )?
      ( AS attributes? newExpression | ( AS attributes? typeName )? ( EQ expression )? ) implementsClause?
    ;

propertyDeclaration
    : propertyHeader
    | PROPERTY identifier ( LPAREN parameterList? RPAREN )? ( AS attributes? typeName )? implementsClause?
      terms ( propertyAccessor terms )+ END PROPERTY
    ;

propertyAccessor
    : attributes? anyModifier* GET terms block END GET
    | attributes? anyModifier* SET ( LPAREN parameterList? RPAREN )? terms block END SET
    ;

eventDeclaration
    : EVENT identifier ( LPAREN parameterList? RPAREN | AS nonArrayTypeName )? implementsClause?
    ;

customEventDeclaration
    : CUSTOM EVENT identifier AS nonArrayTypeName implementsClause? terms ( eventAccessor terms )* END EVENT
    ;

eventAccessor
    : attributes? ADDHANDLER LPAREN parameterList? RPAREN terms block END ADDHANDLER
    | attributes? REMOVEHANDLER LPAREN parameterList? RPAREN terms block END REMOVEHANDLER
    | attributes? RAISEEVENT LPAREN parameterList? RPAREN terms block END RAISEEVENT
    ;

operatorDeclaration
    : OPERATOR overloadableOperator LPAREN parameterList? RPAREN ( AS attributes? typeName )? terms block END OPERATOR
    ;

overloadableOperator
    : PLUS | MINUS | STAR | SLASH | BACKSLASH | AMP | LIKE | MOD | AND | OR | XOR | CARET
    | SHL | SHR | EQ | NE | GT | LT | GE | LE | NOT | CTYPE | ISTRUE | ISFALSE
    ;

variableDeclarators
    : variableDeclarator ( COMMA variableDeclarator )*
    ;

variableDeclarator
    : variableIdentifier ( COMMA variableIdentifier )*
      ( AS newExpression | ( AS typeName )? ( EQ expression )? )
    ;

variableIdentifier
    : identifier QUESTION? ( LPAREN bound ( COMMA bound )* RPAREN )? arrayTypeModifier*
    ;

bound
    : expression ( TO expression )?
    ;

// ---------------------------------------------------------------- statements (10)

block
    : ( terms | labelDeclaration | statement terms )*
    ;

labelDeclaration
    : ( identifier | INTEGER_LITERAL ) COLON
    ;

statement
    : localDeclaration
    | ifStatement
    | selectStatement
    | whileStatement
    | doStatement
    | forStatement
    | tryStatement
    | WITH expression terms block END WITH
    | SYNCLOCK expression terms block END SYNCLOCK
    | USING ( variableDeclarators | expression ) terms block END USING
    | THROW expression?
    | RETURN expression?
    | GOTO ( identifier | INTEGER_LITERAL )
    | EXIT ( DO | FOR | WHILE | SELECT | SUB | FUNCTION | PROPERTY | TRY )
    | CONTINUE ( DO | FOR | WHILE )
    | STOP
    | END
    | REDIM PRESERVE? redimClause ( COMMA redimClause )*
    | ERASE term ( COMMA term )*
    | RAISEEVENT identifierOrKeyword ( LPAREN argumentList RPAREN )?
    | ( ADDHANDLER | REMOVEHANDLER ) expression COMMA expression
    | CALL term
    | term ( EQ | PLUSEQ | MINUSEQ | STAREQ | SLASHEQ | BACKSLASHEQ | CARETEQ | AMPEQ | SHLEQ | SHREQ ) expression
    | callTerm
    ;

localDeclaration
    : ( DIM | STATIC | CONST )+ variableDeclarators
    ;

redimClause
    : ( nameStart | primary ) postfix* LPAREN bound ( COMMA bound )* RPAREN
    ;

ifStatement
    : IF expression THEN lineStatementsFirst ( ELSE lineStatements )?
    | IF expression THEN ELSE lineStatements
    | IF expression ( THEN NEWLINE | terms ) block elseIfPart* elsePart? END IF
    ;

lineStatementsFirst
    : COLON* statement ( COLON+ statement )* COLON*
    | COLON+
    ;

lineStatements
    : COLON* ( statement ( COLON+ statement )* COLON* )?
    ;

elseIfPart
    : ( ELSEIF | ELSE IF ) expression THEN? terms block
    ;

elsePart
    : ELSE terms block
    ;

selectStatement
    : SELECT CASE? expression terms caseBlock* caseElseBlock? END SELECT
    ;

caseBlock
    : CASE caseClause ( COMMA caseClause )* terms block
    ;

caseElseBlock
    : CASE ELSE terms block
    ;

caseClause
    : IS? ( EQ | NE | LT | GT | LE | GE ) expression
    | expression ( TO expression )?
    ;

whileStatement
    : WHILE expression terms block END WHILE
    ;

doStatement
    : DO ( WHILE | UNTIL ) expression terms block LOOP
    | DO terms block LOOP ( ( WHILE | UNTIL ) expression )?
    ;

forStatement
    : forHeader terms block ( NEXT term? | forShared )
    ;

forShared                           // a nested loop whose Next also closes the loop around it
    : forHeader terms block ( NEXT term COMMA term | forShared COMMA term )
    ;

forHeader
    : FOR loopControlVariable EQ expression TO expression ( STEP expression )?
    | FOR EACH loopControlVariable IN expression
    ;

loopControlVariable
    : identifier QUESTION? AS typeName
    | term
    ;

tryStatement
    : TRY terms block catchPart* ( FINALLY terms block )? END TRY
    ;

catchPart
    : CATCH ( identifier ( AS nonArrayTypeName )? )? ( WHEN expression )? terms block
    ;

// ---------------------------------------------------------------- expressions (11)

expression
    : term
    | expression CARET expression
    | ( MINUS | PLUS ) expression
    | expression ( STAR | SLASH ) expression
    | expression BACKSLASH expression
    | expression MOD expression
    | expression ( PLUS | MINUS ) expression
    | expression AMP expression
    | expression ( SHL | SHR ) expression
    | expression ( EQ | NE | LT | GT | LE | GE | IS | ISNOT | LIKE ) expression
    | NOT expression
    | expression ( AND | ANDALSO ) expression
    | expression ( OR | ORELSE ) expression
    | expression XOR expression
    ;

holeExpression                      // an interpolation hole, checked by VBCheck
    : expression EOF
    ;

term
    : ( nameStart | primary ) postfix*
    ;

callTerm                            // an invocation statement
    : nameStart postfix*
    | primary postfix+
    ;

nameStart
    : identifier typeArguments?
    | ( DOT | BANG ) identifierOrKeyword typeArguments?
    | ( MYBASE | MYCLASS | GLOBAL | primitiveTypeName ) DOT identifierOrKeyword typeArguments?
    ;

postfix
    : ( DOT | QDOT ) identifierOrKeyword typeArguments?
    | BANG identifierOrKeyword
    | QUESTION? LPAREN argumentList RPAREN
    ;

primary
    : INTEGER_LITERAL | FLOAT_LITERAL | STRING_LITERAL | CHAR_LITERAL | INTERPOLATED_STRING
    | TRUE | FALSE | NOTHING | ME
    | LPAREN expression RPAREN
    | collectionInitializer
    | newExpression
    | ( CTYPE | DIRECTCAST | TRYCAST ) LPAREN expression COMMA typeName RPAREN
    | castKeyword LPAREN expression RPAREN
    | TYPEOF expression ( IS | ISNOT ) typeName
    | GETTYPE LPAREN typeName RPAREN
    | ADDRESSOF term
    | IF LPAREN expression COMMA expression ( COMMA expression )? RPAREN
    | NAMEOF LPAREN expression RPAREN
    | lambdaExpression
    | queryExpression
    ;

castKeyword
    : CBOOL | CBYTE | CCHAR | CDATE | CDEC | CDBL | CINT | CLNG | COBJ | CSBYTE | CSHORT
    | CSNG | CSTR | CUINT | CULNG | CUSHORT
    ;

argumentList
    : argument ( COMMA argument )*
    ;

argument
    : identifierOrKeyword COLONEQ expression
    | expression
    |                               // omitted
    ;

collectionInitializer
    : LBRACE ( expression ( COMMA expression )* )? RBRACE
    ;

newExpression
    : NEW WITH LBRACE anonymousFieldInitializer ( COMMA anonymousFieldInitializer )* RBRACE
    | NEW nonArrayTypeName QUESTION? ( arrayTypeModifier | LPAREN bound ( COMMA bound )* RPAREN )
      arrayTypeModifier* collectionInitializer
    | NEW nonArrayTypeName QUESTION? ( LPAREN argumentList RPAREN )?
      ( WITH LBRACE fieldInitializer ( COMMA fieldInitializer )* RBRACE | FROM collectionInitializer )?
    ;

fieldInitializer
    : DOT identifierOrKeyword EQ expression
    ;

anonymousFieldInitializer
    : KEY? ( DOT identifierOrKeyword EQ )? expression
    ;

lambdaExpression                    // vb.g4 makes the parameter list optional; VB requires it
    : FUNCTION LPAREN parameterList? RPAREN expression
    | SUB LPAREN parameterList? RPAREN statement
    | FUNCTION LPAREN parameterList? RPAREN ( AS typeName )? NEWLINE block END FUNCTION
    | SUB LPAREN parameterList? RPAREN NEWLINE block END SUB
    ;

queryExpression
    : FROM collectionRange ( COMMA collectionRange )* queryOperator*
    | AGGREGATE collectionRange ( COMMA collectionRange )* queryOperator* NEWLINE? INTO expressionRangeList
      queryOperator*
    ;

queryOperator
    : NEWLINE? FROM collectionRange ( COMMA collectionRange )*
    | NEWLINE? AGGREGATE collectionRange ( COMMA collectionRange )* queryOperator* NEWLINE? INTO expressionRangeList
    | NEWLINE? SELECT expressionRangeList
    | NEWLINE? LET namedRange ( COMMA namedRange )*
    | NEWLINE? DISTINCT
    | NEWLINE? WHERE expression
    | NEWLINE? ORDER BY expression ( ASCENDING | DESCENDING )? ( COMMA expression ( ASCENDING | DESCENDING )? )*
    | NEWLINE? ( TAKE | SKIP_KW ) WHILE? expression
    | NEWLINE? joinOperator
    | NEWLINE? GROUP expressionRangeList? NEWLINE? BY expressionRangeList NEWLINE? INTO expressionRangeList
    ;

joinOperator
    : JOIN collectionRange ( NEWLINE? joinOperator | NEWLINE? GROUP JOIN groupJoinRest )*
      NEWLINE? ON joinCondition ( AND joinCondition )*
    | GROUP JOIN groupJoinRest
    ;

groupJoinRest
    : collectionRange ( NEWLINE? joinOperator | NEWLINE? GROUP JOIN groupJoinRest )*
      NEWLINE? ON joinCondition ( AND joinCondition )* NEWLINE? INTO expressionRangeList
    ;

joinCondition
    : expression EQUALS expression
    ;

collectionRange
    : identifier ( AS typeName )? IN expression
    ;

expressionRangeList
    : expressionRange ( COMMA expressionRange )*
    ;

expressionRange
    : namedRange
    | expression
    ;

namedRange
    : identifier ( AS typeName )? EQ expression
    ;

// ---------------------------------------------------------------- names

identifier
    : IDENTIFIER
    | AGGREGATE | ANSI | ASCENDING | ASSEMBLY | ASYNC | AUTO
    | AWAIT | BINARY | BY | COMPARE | CUSTOM | DESCENDING
    | DISTINCT | EQUALS | EXPLICIT | FROM | GROUP | INFER
    | INTO | ISFALSE | ISTRUE | ITERATOR | JOIN | KEY
    | MID | NAMEOF | OFF | ORDER | OUT | PRESERVE
    | SKIP_KW | STRICT | TAKE | TEXT | UNICODE | UNTIL
    | WHERE | YIELD | REGION
    ;

identifierOrKeyword
    : identifier
    | ADDHANDLER | ADDRESSOF | ALIAS | AND | ANDALSO | AS
    | BOOLEAN | BYREF | BYTE | BYVAL | CALL | CASE
    | CATCH | CBOOL | CBYTE | CCHAR | CDATE | CDBL
    | CDEC | CHAR | CINT | CLASS | CLNG | COBJ
    | CONST | CONTINUE | CSBYTE | CSHORT | CSNG | CSTR
    | CTYPE | CUINT | CULNG | CUSHORT | DATE | DECIMAL
    | DECLARE | DEFAULT | DELEGATE | DIM | DIRECTCAST | DO
    | DOUBLE | EACH | ELSE | ELSEIF | END | ENDIF
    | ENUM | ERASE | ERROR | EVENT | EXIT | FALSE
    | FINALLY | FOR | FRIEND | FUNCTION | GET | GETTYPE
    | GETXMLNAMESPACE | GLOBAL | GOSUB | GOTO | HANDLES | IF
    | IMPLEMENTS | IMPORTS | IN | INHERITS | INTEGER | INTERFACE
    | IS | ISNOT | LET | LIB | LIKE | LONG
    | LOOP | ME | MOD | MODULE | MUSTINHERIT | MUSTOVERRIDE
    | MYBASE | MYCLASS | NAMESPACE | NARROWING | NEW | NEXT
    | NOT | NOTHING | NOTINHERITABLE | NOTOVERRIDABLE | OBJECT | OF
    | ON | OPERATOR | OPTION | OPTIONAL | OR | ORELSE
    | OVERLOADS | OVERRIDABLE | OVERRIDES | PARAMARRAY | PARTIAL | PRIVATE
    | PROPERTY | PROTECTED | PUBLIC | RAISEEVENT | READONLY | REDIM
    | REMOVEHANDLER | RESUME | RETURN | SBYTE | SELECT | SET
    | SHADOWS | SHARED | SHORT | SINGLE | STATIC | STEP
    | STOP | STRING | STRUCTURE | SUB | SYNCLOCK | THEN
    | THROW | TO | TRUE | TRY | TRYCAST | TYPEOF
    | UINTEGER | ULONG | USHORT | USING | VARIANT | WEND
    | WHEN | WHILE | WIDENING | WITH | WITHEVENTS | WRITEONLY
    | XOR
    ;
