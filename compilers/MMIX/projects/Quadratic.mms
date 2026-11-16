% Quadratic.mms - the roots of a quadratic equation.
%
% MMIX version of Quadratic.asm.  Usage: Quadratic A B C
% Solves A*x^2 + B*x + C = 0 for real or complex roots and writes them
% with 6 digits after the point.
%
% Options: lib/std.mms lib/fmt.mms
Two     GREG  #4000000000000000         % 2.0
Four    GREG  #4010000000000000         % 4.0
Sign    GREG  #8000000000000000
        LOC   Data_Segment
        GREG  @
Syn     BYTE  "Syntax: Quadratic A B C   "
        BYTE  "(solves A*x^2 + B*x + C = 0)",0
NotQ    BYTE  "Not a quadratic equation: coefficient A must be "
        BYTE  "non-zero.",0
        GREG  @
Real    BYTE  "Two real roots: x1 = ",0
Comma2  BYTE  ", x2 = ",0
Repeat  BYTE  "One repeated (double) root: x = ",0
Cplx    BYTE  "Two complex roots: x1 = ",0
        GREG  @
Plus    BYTE  " + ",0
Minus   BYTE  " - ",0
ICom    BYTE  "i, x2 = ",0
IEnd    BYTE  "i",0

        LOC   #100
Main    CMP   $2,$0,4           % $0 = argc, $1 = argv
        BZ    $2,1F
        LDA   $5,Syn
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      LDOU  $3,$1,8           % $2 = A, $3 = B, $4 = C
        PUSHJ $2,Fmt:Parse
        LDOU  $4,$1,16
        PUSHJ $3,Fmt:Parse
        LDOU  $5,$1,24
        PUSHJ $4,Fmt:Parse
        SET   $8,0
        FCMP  $8,$2,$8          % A = 0?
        BNZ   $8,2F
        LDA   $12,NotQ
        PUSHJ $11,Std:Puts
        PUSHJ $11,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0
2H      FMUL  $5,$3,$3          % $5 = disc = B*B - 4*A*C
        FMUL  $6,$2,$4
        FMUL  $6,$6,Four
        FSUB  $5,$5,$6
        FMUL  $6,$2,Two         % $6 = 2A
        XOR   $7,$3,Sign        % $7 = -B
        SET   $8,0
        FCMP  $9,$5,$8
        BZ    $9,5F             % disc = 0
        BN    $9,6F             % disc < 0
        FSQRT $8,$5             % $8 = sqrt(disc)
        FADD  $9,$7,$8
        FDIV  $9,$9,$6          % $9 = (-B + sqrt(disc)) / 2A
        FSUB  $10,$7,$8
        FDIV  $10,$10,$6        % $10 = (-B - sqrt(disc)) / 2A
        LDA   $12,Real
        PUSHJ $11,Std:Puts
        SET   $12,$9
        PUSHJ $11,Pf6
        LDA   $12,Comma2
        PUSHJ $11,Std:Puts
        SET   $12,$10
        PUSHJ $11,Pf6
        JMP   9F
5H      FDIV  $9,$7,$6          % -B / 2A
        LDA   $12,Repeat
        PUSHJ $11,Std:Puts
        SET   $12,$9
        PUSHJ $11,Pf6
        JMP   9F
6H      XOR   $8,$5,Sign        % disc < 0: -disc
        FSQRT $8,$8
        FDIV  $8,$8,$6          % sqrt(-disc) / 2A
        SLU   $8,$8,1
        SRU   $8,$8,1           % the absolute value
        FDIV  $9,$7,$6          % the real part -B / 2A
        LDA   $12,Cplx
        PUSHJ $11,Std:Puts
        SET   $12,$9
        PUSHJ $11,Pf6
        LDA   $12,Plus
        PUSHJ $11,Std:Puts
        SET   $12,$8
        PUSHJ $11,Pf6
        LDA   $12,ICom
        PUSHJ $11,Std:Puts
        SET   $12,$9
        PUSHJ $11,Pf6
        LDA   $12,Minus
        PUSHJ $11,Std:Puts
        SET   $12,$8
        PUSHJ $11,Pf6
        LDA   $12,IEnd
        PUSHJ $11,Std:Puts
9H      PUSHJ $11,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0

% Pf6: $0 = a double: written with 6 digits after the point
Pf6     GET   $1,rJ
        SET   $3,$0
        SET   $4,6
        PUSHJ $2,Fmt:PutFixed
        PUT   rJ,$1
        POP   0,0
