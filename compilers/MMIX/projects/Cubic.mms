% Cubic.mms - the roots of a cubic equation.
%
% MMIX version of Cubic.asm.  Usage: Cubic A B C D
% Solves A*x^3 + B*x^2 + C*x + D = 0 (Cardano's formulas, or the
% trigonometric form for three real roots) and writes the roots with 6
% digits after the point.  The cube root is Math:Cbrt, cos and acos are
% in lib/math.mms.
%
% Options: lib/std.mms lib/fmt.mms lib/math.mms
Two     GREG  #4000000000000000         % 2.0
Three   GREG  #4008000000000000         % 3.0
TwoSev  GREG  #403B000000000000         % 27.0
Eps     GREG  #3E112E0BE826D695         % 1e-9
Pi      GREG  #400921FB54442D18         % pi
Sign    GREG  #8000000000000000
        LOC   Data_Segment
        GREG  @
Syn     BYTE  "Syntax: Cubic A B C D   "
        BYTE  "(solves A*x^3 + B*x^2 + C*x + D = 0)",0
NotC    BYTE  "Error: coefficient A must be non-zero for a cubic "
        BYTE  "equation.",0
        GREG  @
Solv    BYTE  "Solving ",0
XT3     BYTE  "*x^3 + ",0
XT2     BYTE  "*x^2 + ",0
XT1     BYTE  "*x + ",0
Zero    BYTE  " = 0",0
        GREG  @
XS      BYTE  "x",0
Eq      BYTE  " = ",0
X1S     BYTE  "x1 = ",0
X2S     BYTE  "x2 = ",0
X3S     BYTE  "x3 = ",0
Plus    BYTE  " + ",0
Minus   BYTE  " - ",0
Im      BYTE  "i",0

        LOC   #100
Main    CMP   $2,$0,5           % $0 = argc, $1 = argv
        BZ    $2,1F
        LDA   $5,Syn
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      LDOU  $3,$1,8           % $2 = A, $3 = B, $4 = C, $5 = D
        PUSHJ $2,Fmt:Parse
        LDOU  $4,$1,16
        PUSHJ $3,Fmt:Parse
        LDOU  $5,$1,24
        PUSHJ $4,Fmt:Parse
        LDOU  $6,$1,32
        PUSHJ $5,Fmt:Parse
        SET   $8,0
        FCMP  $8,$2,$8          % A = 0?
        BNZ   $8,2F
        LDA   $21,NotC
        PUSHJ $20,Std:Puts
        PUSHJ $20,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
2H      LDA   $21,Solv          % Solving A*x^3 + B*x^2 + C*x + D = 0
        PUSHJ $20,Std:Puts
        SET   $21,$2
        PUSHJ $20,Pf6
        LDA   $21,XT3
        PUSHJ $20,Std:Puts
        SET   $21,$3
        PUSHJ $20,Pf6
        LDA   $21,XT2
        PUSHJ $20,Std:Puts
        SET   $21,$4
        PUSHJ $20,Pf6
        LDA   $21,XT1
        PUSHJ $20,Std:Puts
        SET   $21,$5
        PUSHJ $20,Pf6
        LDA   $21,Zero
        PUSHJ $20,Std:Puts
        PUSHJ $20,Std:PutNl
        FDIV  $3,$3,$2          % b, c, d divided by a
        FDIV  $4,$4,$2
        FDIV  $5,$5,$2
        FDIV  $6,$3,Three       % $6 = shift = b / 3
        FMUL  $8,$3,$3
        FDIV  $8,$8,Three
        FSUB  $7,$4,$8          % $7 = p = c - b^2 / 3
        FMUL  $8,$3,$3
        FMUL  $8,$8,$3
        FMUL  $8,$8,Two
        FDIV  $8,$8,TwoSev
        FMUL  $9,$3,$4
        FDIV  $9,$9,Three
        FSUB  $8,$8,$9
        FADD  $9,$8,$5          % $9 = q = 2 b^3 / 27 - b c / 3 + d
        FDIV  $8,$9,Two
        FMUL  $8,$8,$8          % (q / 2)^2
        FDIV  $10,$7,Three
        FMUL  $11,$10,$10
        FMUL  $11,$11,$10       % (p / 3)^3
        FADD  $10,$8,$11        % $10 = disc
        FSQRT $11,Three         % $11 = sqrt(3)
        FCMP  $8,$10,Eps
        BP    $8,3F             % disc > eps: one real root
        XOR   $12,$10,Sign
        XOR   $13,Eps,Sign
        FCMP  $8,$10,$13        % disc < -eps?
        BN    $8,5F             % three real roots
        FDIV  $12,$9,Two        % disc = 0: u = cbrt(-q / 2)
        XOR   $12,$12,Sign
        SET   $21,$12
        PUSHJ $20,Math:Cbrt
        FMUL  $12,$20,Two       % x1 = 2 u - shift
        FSUB  $12,$12,$6
        XOR   $13,$20,Sign      % x2 = -u - shift
        FSUB  $13,$13,$6
        LDA   $21,X1S
        PUSHJ $20,Std:Puts
        SET   $21,$12
        PUSHJ $20,Pf6
        PUSHJ $20,Std:PutNl
        LDA   $21,X2S
        PUSHJ $20,Std:Puts
        SET   $21,$13
        PUSHJ $20,Pf6
        PUSHJ $20,Std:PutNl
        LDA   $21,X3S
        PUSHJ $20,Std:Puts
        SET   $21,$13
        PUSHJ $20,Pf6
        PUSHJ $20,Std:PutNl
        JMP   9F
3H      FSQRT $12,$10           % one real root and two complex
        FDIV  $13,$9,Two
        XOR   $13,$13,Sign      % $13 = -q / 2
        FADD  $21,$13,$12
        PUSHJ $20,Math:Cbrt
        SET   $14,$20           % $14 = u
        FSUB  $21,$13,$12
        PUSHJ $20,Math:Cbrt
        SET   $15,$20           % $15 = v
        FADD  $16,$14,$15       % $16 = u + v
        FSUB  $17,$16,$6        % $17 = x1
        FDIV  $18,$16,Two
        XOR   $18,$18,Sign
        FSUB  $19,$14,$15
        FMUL  $19,$19,$11
        FDIV  $19,$19,Two       % $19 = the imaginary part
        FSUB  $18,$18,$6        % $18 = the real part
        LDA   $21,X1S
        PUSHJ $20,Std:Puts
        SET   $21,$17
        PUSHJ $20,Pf6
        PUSHJ $20,Std:PutNl
        LDA   $21,X2S
        PUSHJ $20,Std:Puts
        SET   $21,$18
        PUSHJ $20,Pf6
        LDA   $21,Plus
        PUSHJ $20,Std:Puts
        SET   $21,$19
        PUSHJ $20,Pf6
        LDA   $21,Im
        PUSHJ $20,Std:Puts
        PUSHJ $20,Std:PutNl
        LDA   $21,X3S
        PUSHJ $20,Std:Puts
        SET   $21,$18
        PUSHJ $20,Pf6
        LDA   $21,Minus
        PUSHJ $20,Std:Puts
        SET   $21,$19
        PUSHJ $20,Pf6
        LDA   $21,Im
        PUSHJ $20,Std:Puts
        PUSHJ $20,Std:PutNl
        JMP   9F
5H      FDIV  $12,$7,Three      % three real roots: -p / 3
        XOR   $12,$12,Sign
        FSQRT $12,$12
        FMUL  $12,$12,Two       % $12 = m = 2 sqrt(-p / 3)
        FMUL  $13,$9,Three
        FMUL  $14,$7,Two
        FDIV  $13,$13,$14       % 3 q / (2 p)
        FDIV  $14,Three,$7
        XOR   $14,$14,Sign
        FSQRT $14,$14           % sqrt(-3 / p)
        FMUL  $13,$13,$14
        SET   $21,$13
        PUSHJ $20,Math:Acos
        FDIV  $13,$20,Three     % $13 = theta
        SET   $14,0             % k
6H      CMP   $8,$14,3
        BNN   $8,9F
        SET   $15,Pi            % 2 pi k / 3
        FMUL  $15,$15,Two
        FLOT  $16,$14
        FMUL  $15,$15,$16
        FDIV  $15,$15,Three
        FSUB  $21,$13,$15
        PUSHJ $20,Math:Cos
        FMUL  $20,$20,$12       % x = m cos(theta - 2 pi k / 3) - shift
        FSUB  $17,$20,$6
        LDA   $21,XS
        PUSHJ $20,Std:Puts
        ADD   $21,$14,1
        PUSHJ $20,Std:PutDec
        LDA   $21,Eq
        PUSHJ $20,Std:Puts
        SET   $21,$17
        PUSHJ $20,Pf6
        PUSHJ $20,Std:PutNl
        ADD   $14,$14,1
        JMP   6B
9H      SET   $255,0
        TRAP  0,Halt,0

% Pf6: $0 = a double: written with 6 digits after the point
Pf6     GET   $1,rJ
        SET   $3,$0
        SET   $4,6
        PUSHJ $2,Fmt:PutFixed
        PUT   rJ,$1
        POP   0,0
