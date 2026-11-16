% std.mms - text output.  Listed after the program in Options.
%
%   Std:Puts      $0 = address of a zero-terminated string
%   Std:PutNl     writes a newline
%   Std:PutChar   $0 = a Unicode code point, written as UTF-8
%   Std:PutDec    $0 = a signed integer, written in decimal
%   Std:PutDecW   $0 = a signed integer, $1 = width: right-aligned
%   Std:PutDecZ   $0 = an integer >= 0, $1 = width: filled with zeros
%   Std:PutHex    $0 = a number, $1 = digits (< 30): hexadecimal, a-f
%   Std:PutHexU   the same with the letters A-F
%
% Call as in MMIX: PUSHJ $X,Std:PutDec with the argument in $(X+1).
% Data: Data_Segment+#10000 .. +#100FF (the program uses the data
% segment below +#10000).
        PREFIX Std:
Home    IS    @
        LOC   :Data_Segment+#10000
Base    GREG  @
Buf     OCTA  0,0,0,0           % PutDec: the digits and a 0 at the end
Chr     OCTA  0                 % PutChar: the bytes and a 0
Nl      BYTE  #a,0
        LOC   Home
K800    GREG  #800
K10000  GREG  #10000

Puts    SET   $255,$0
        TRAP  0,:Fputs,:StdOut
        POP   0,0

PutNl   LDA   $255,Nl
        TRAP  0,:Fputs,:StdOut
        POP   0,0

% PutChar: the UTF-8 bytes of the code point go to Chr
PutChar LDA   $1,Chr
        CMPU  $2,$0,#80
        BNN   $2,2F
        STBU  $0,$1,0           % one byte
        SET   $2,1
        JMP   9F
2H      CMPU  $2,$0,K800
        BNN   $2,3F
        SRU   $3,$0,6           % two bytes: 110xxxxx 10xxxxxx
        OR    $3,$3,#C0
        STBU  $3,$1,0
        AND   $3,$0,#3F
        OR    $3,$3,#80
        STBU  $3,$1,1
        SET   $2,2
        JMP   9F
3H      CMPU  $2,$0,K10000
        BNN   $2,4F
        SRU   $3,$0,12          % three bytes: 1110xxxx, 2 more
        OR    $3,$3,#E0
        STBU  $3,$1,0
        SRU   $3,$0,6
        AND   $3,$3,#3F
        OR    $3,$3,#80
        STBU  $3,$1,1
        AND   $3,$0,#3F
        OR    $3,$3,#80
        STBU  $3,$1,2
        SET   $2,3
        JMP   9F
4H      SRU   $3,$0,18          % four bytes: 11110xxx, 3 more
        OR    $3,$3,#F0
        STBU  $3,$1,0
        SRU   $3,$0,12
        AND   $3,$3,#3F
        OR    $3,$3,#80
        STBU  $3,$1,1
        SRU   $3,$0,6
        AND   $3,$3,#3F
        OR    $3,$3,#80
        STBU  $3,$1,2
        AND   $3,$0,#3F
        OR    $3,$3,#80
        STBU  $3,$1,3
        SET   $2,4
9H      SET   $3,0
        STBU  $3,$1,$2          % the 0 at the end
        SET   $255,$1
        TRAP  0,:Fputs,:StdOut
        POP   0,0

% Digits: $0 = a number -> $0 = address of its digits in Buf
Digits  LDA   $1,Buf+31
        SET   $2,0
        STBU  $2,$1,0           % the 0 at the end
        SET   $4,0              % 1 if the number is negative
        BNN   $0,1F
        NEG   $0,$0             % unsigned from here on
        SET   $4,1
1H      DIVU  $0,$0,10
        GET   $3,:rR
        ADD   $3,$3,'0'
        SUB   $1,$1,1
        STBU  $3,$1,0
        BNZ   $0,1B
        BZ    $4,2F
        SUB   $1,$1,1
        SET   $3,'-'
        STBU  $3,$1,0
2H      SET   $0,$1
        POP   1,0

PutDec  GET   $1,:rJ
        SET   $3,$0
        PUSHJ $2,Digits
        PUT   :rJ,$1
        SET   $255,$2
        TRAP  0,:Fputs,:StdOut
        POP   0,0

% PutDecW: as PutDec, with spaces in front up to the width $1 (< 30)
PutDecW SET   $2,#20
        JMP   Pad

% PutDecZ: as PutDecW, with zeros in front (for a number >= 0)
PutDecZ SET   $2,'0'
        JMP   Pad

% Pad: $0 = a number, $1 = a width, $2 = the character to fill with
Pad     GET   $3,:rJ
        SET   $5,$0
        PUSHJ $4,Digits         % $4 = the address of the digits
        PUT   :rJ,$3
        LDA   $5,Buf+31
        SUB   $5,$5,$4          % the number of characters
1H      CMP   $6,$5,$1
        BNN   $6,2F
        SUB   $4,$4,1
        STBU  $2,$4,0
        ADD   $5,$5,1
        JMP   1B
2H      SET   $255,$4
        TRAP  0,:Fputs,:StdOut
        POP   0,0

% Hex: $0 = a number, $1 = its digits, $2 = 87 (a-f) or 55 (A-F)
Hex     LDA   $3,Buf
        ADD   $4,$3,$1
        SET   $5,0
        STBU  $5,$4,0           % the 0 at the end
1H      BZ    $1,9F
        SUB   $1,$1,1
        AND   $5,$0,15          % the last digit
        SRU   $0,$0,4
        CMP   $6,$5,10
        BN    $6,2F
        ADD   $5,$5,$2          % a letter
        JMP   3F
2H      ADD   $5,$5,'0'
3H      ADD   $6,$3,$1
        STBU  $5,$6,0
        JMP   1B
9H      SET   $255,$3
        TRAP  0,:Fputs,:StdOut
        POP   0,0

PutHex  SET   $2,87
        JMP   Hex

PutHexU SET   $2,55
        JMP   Hex
        PREFIX :
