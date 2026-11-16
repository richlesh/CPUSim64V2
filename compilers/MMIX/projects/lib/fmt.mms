% fmt.mms - doubles as text.  Listed after the program in Options, with
% std.mms.  A double lives in an ordinary register (FADD, FMUL ...).
%
%   Fmt:PutFixed  $0 = a double, $1 = digits after the point (< 19):
%                 writes it in decimal, rounded (halves up) as the
%                 simulator's %.Nf does; NaN and Infinity as in Java
%   Fmt:Parse     $0 = a string -> $0 = the double: optional sign,
%                 digits, optional point and digits, optional e and an
%                 exponent (a mantissa of up to 18 digits)
%
% Data: Data_Segment+#10300 .. +#1032F.
        PREFIX Fmt:
Home    IS    @
        LOC   :Data_Segment+#10300
Base    GREG  @
NaNs    BYTE  "NaN",0
Infs    BYTE  "Infinity",0
NInfs   BYTE  "-Infinity",0
        LOC   Home
Two60   GREG  #43B0000000000000         % 2^60 as a double
Mask60  GREG  #0FFFFFFFFFFFFFFF
Half59  GREG  #0800000000000000
K7FF    GREG  #7FF
Sign    GREG  #8000000000000000
Ten     GREG  #4024000000000000         % 10.0
One     GREG  #3FF0000000000000         % 1.0

PutFixed GET  $2,:rJ
        SRU   $3,$0,63          % the sign
        SLU   $0,$0,1
        SRU   $0,$0,1           % the absolute value
        SRU   $4,$0,52
        CMP   $4,$4,K7FF
        BNZ   $4,1F
        LDA   $9,NaNs           % all exponent bits set: NaN or Inf
        SLU   $5,$0,12
        BNZ   $5,8F
        LDA   $9,Infs
        BZ    $3,8F
        LDA   $9,NInfs
8H      PUSHJ $8,:Std:Puts
        JMP   9F
1H      BZ    $3,2F
        SET   $9,'-'
        PUSHJ $8,:Std:PutChar
2H      FIX   $3,:ROUND_OFF,$0   % the integer part
        FLOT  $4,$3
        FSUB  $4,$0,$4          % the fraction: exact
        FMUL  $4,$4,Two60
        FIX   $4,:ROUND_OFF,$4   % the fraction * 2^60
        SET   $5,0              % the digits as an integer
        SET   $6,$1
3H      BZ    $6,4F
        MULU  $4,$4,10
        SRU   $7,$4,60          % the next digit
        AND   $4,$4,Mask60
        MULU  $5,$5,10
        ADDU  $5,$5,$7
        SUB   $6,$6,1
        JMP   3B
4H      CMPU  $6,$4,Half59      % round up if the rest is 1/2 or more
        BN    $6,6F
        ADD   $5,$5,1
        SET   $6,1              % 10^digits: did the digits overflow?
        SET   $7,$1
5H      BZ    $7,7F
        MULU  $6,$6,10
        SUB   $7,$7,1
        JMP   5B
7H      CMP   $7,$5,$6
        BNZ   $7,6F
        SET   $5,0              % yes: carry into the integer part
        ADD   $3,$3,1
6H      SET   $9,$3
        PUSHJ $8,:Std:PutDec
        BZ    $1,9F
        SET   $9,'.'
        PUSHJ $8,:Std:PutChar
        SET   $9,$5
        SET   $10,$1
        PUSHJ $8,:Std:PutDecZ
9H      PUT   :rJ,$2
        POP   0,0

Parse   SET   $1,0              % the mantissa as an integer
        SET   $2,0              % the digits after the point
        SET   $3,0              % 1 if negative
        SET   $4,0              % 1 after the point
        SET   $9,0              % the exponent
        SET   $10,0             % 1 if the exponent is negative
        LDBU  $5,$0,0
        CMP   $6,$5,'-'
        BNZ   $6,1F
        SET   $3,1
        ADD   $0,$0,1
        JMP   2F
1H      CMP   $6,$5,'+'
        BNZ   $6,2F
        ADD   $0,$0,1
2H      LDBU  $5,$0,0
        SUB   $6,$5,'0'
        CMPU  $7,$6,10
        BN    $7,3F             % a digit
        CMP   $7,$5,'.'
        BNZ   $7,5F
        BNZ   $4,5F
        SET   $4,1
        ADD   $0,$0,1
        JMP   2B
3H      MULU  $1,$1,10
        ADDU  $1,$1,$6
        BZ    $4,4F
        ADD   $2,$2,1
4H      ADD   $0,$0,1
        JMP   2B
5H      CMP   $7,$5,'e'         % an exponent?
        BZ    $7,6F
        CMP   $7,$5,'E'
        BNZ   $7,9F
6H      ADD   $0,$0,1
        LDBU  $5,$0,0
        CMP   $7,$5,'-'
        BNZ   $7,7F
        SET   $10,1
        ADD   $0,$0,1
        JMP   8F
7H      CMP   $7,$5,'+'
        BNZ   $7,8F
        ADD   $0,$0,1
8H      LDBU  $5,$0,0
        SUB   $6,$5,'0'
        CMPU  $7,$6,10
        BNN   $7,9F
        MULU  $9,$9,10
        ADDU  $9,$9,$6
        ADD   $0,$0,1
        JMP   8B
9H      BZ    $10,0F
        NEG   $9,$9
0H      SUB   $9,$9,$2          % the power of 10 to apply
        FLOT  $0,$1
        SET   $4,One
        BNN   $9,2F
        NEG   $9,$9             % divide by 10^(-power)
1H      BZ    $9,3F
        FMUL  $4,$4,Ten
        SUB   $9,$9,1
        JMP   1B
3H      FDIV  $0,$0,$4
        JMP   5F
2H      BZ    $9,4F             % multiply by 10^power
        FMUL  $4,$4,Ten
        SUB   $9,$9,1
        JMP   2B
4H      FMUL  $0,$0,$4
5H      BZ    $3,6F
        XOR   $0,$0,Sign
6H      POP   1,0
        PREFIX :
