% math.mms - functions of doubles.  Listed after the program.
% A double lives in an ordinary register (see FADD, FMUL, FCMP ...).
%
%   Math:PowI   $0 = x, $1 = n >= 0 (an integer) -> x^n
%   Math:Root   $0 = x >= 0, $1 = n >= 1 (an integer) -> x^(1/n),
%               by Newton's method
%   Math:Cbrt   $0 = x -> the cube root of x (negative for negative x)
%   Math:Cos    $0 = x -> cos x
%   Math:Acos   $0 = x, -1 <= x <= 1 -> acos x, 0 ... pi (by bisection)
%
% The square root is the instruction FSQRT.
        PREFIX Math:
Pi      GREG  #400921FB54442D18         % pi
TwoPi   GREG  #401921FB54442D18         % 2 pi
One     GREG  #3FF0000000000000         % 1.0
Half    GREG  #3FE0000000000000         % 0.5
Sign    GREG  #8000000000000000
InvPio2 GREG  #3FE45F306DC9C883         % 2/pi, and pi/2 in three parts
Pio2_1  GREG  #3FF921FB54400000
Pio2_1t GREG  #3DD0B4611A626331
Pio2_2  GREG  #3DD0B4611A600000
Pio2_2t GREG  #3BA3198A2E037073
Pio2_3  GREG  #3BA3198A2E000000
Pio2_3t GREG  #397B839A252049C1
C1      GREG  #3FA555555555554C         % kernel cos: 1/24 ...
C2      GREG  #BF56C16C16C15177
C3      GREG  #3EFA01A019CB1590
C4      GREG  #BE927E4F809C52AD
C5      GREG  #3E21EE9EBDB4B1C4
C6      GREG  #BDA8FAE9BE8838D4
S1      GREG  #BFC5555555555549         % kernel sin: -1/6 ...
S2      GREG  #3F8111111110F8A6
S3      GREG  #BF2A01A019C161D5
S4      GREG  #3EC71DE357B1FE7D
S5      GREG  #BE5AE5E68A2B9CEB
S6      GREG  #3DE5D93A5ACFD57C
Q28125  GREG  #3FD2000000000000         % 0.28125
K3E400000 GREG #3E400000                % high words of |x|: 2^-27
K3FD33333 GREG #3FD33333                % 0.3
K3FE90000 GREG #3FE90000                % 0.78125
K3FE921FB GREG #3FE921FB                % pi/4
K200000 GREG  #200000

PowI    SET   $2,One            % the result
1H      BZ    $1,9F
        AND   $3,$1,1
        BZ    $3,2F
        FMUL  $2,$2,$0
2H      FMUL  $0,$0,$0
        SRU   $1,$1,1
        JMP   1B
9H      SET   $0,$2
        POP   1,0

% y' = ((n - 1) y + x / y^(n-1)) / n from y = x + 1, until y is steady
Root    GET   $2,:rJ
        SET   $3,0
        FCMP  $3,$0,$3
        BP    $3,1F
        SET   $4,0              % x <= 0: the root is 0
        JMP   9F
1H      FADD  $4,$0,One         % y
        SUB   $5,$1,1           % n - 1
        FLOT  $6,$1             % n as a double
        SET   $7,500            % at most 500 steps
2H      SET   $10,$4
        SET   $11,$5
        PUSHJ $9,PowI           % $9 = y^(n-1)
        FDIV  $9,$0,$9
        FLOT  $8,$5
        FMUL  $8,$8,$4
        FADD  $8,$8,$9
        FDIV  $8,$8,$6          % the new y
        FCMP  $9,$8,$4
        SET   $4,$8
        BZ    $9,9F
        SUB   $7,$7,1
        BNZ   $7,2B
9H      PUT   :rJ,$2
        SET   $0,$4
        POP   1,0

Cbrt    GET   $1,:rJ
        SRU   $2,$0,63          % the sign
        SLU   $0,$0,1
        SRU   $0,$0,1
        SET   $5,$0
        SET   $6,3
        PUSHJ $4,Root
        PUT   :rJ,$1
        SET   $0,$4
        BZ    $2,9F
        XOR   $0,$0,Sign
9H      POP   1,0

% cos x as in fdlibm: |x| is reduced by multiples of pi/2 (with pi/2 in
% three parts, so that the result is exact near the zeros of cos), then
% a polynomial kernel gives cos or sin of the remainder.
Cos     GET   $1,:rJ
        SLU   $2,$0,1
        SRU   $2,$2,1           % |x|
        SRU   $3,$2,32          % the high word of |x|
        CMP   $4,$3,K3FE921FB
        BP    $4,1F             % |x| > pi/4
        SET   $6,$0
        SET   $7,0
        PUSHJ $5,KCos
        PUT   :rJ,$1
        SET   $0,$5
        POP   1,0
1H      FMUL  $4,$2,InvPio2     % n = (int) (|x| 2/pi + 1/2)
        FADD  $4,$4,Half
        FIX   $4,:ROUND_OFF,$4
        FLOT  $5,$4             % n as a double
        FMUL  $6,$5,Pio2_1
        FSUB  $6,$2,$6          % r = |x| - n pio2_1: the first 33 bits
        FMUL  $7,$5,Pio2_1t
        SET   $9,$6             % the next 33 bits
        FMUL  $7,$5,Pio2_2
        FSUB  $6,$9,$7
        FSUB  $10,$9,$6
        FSUB  $10,$10,$7
        FMUL  $7,$5,Pio2_2t
        FSUB  $7,$7,$10
        SET   $9,$6             % and the rest
        FMUL  $7,$5,Pio2_3
        FSUB  $6,$9,$7
        FSUB  $10,$9,$6
        FSUB  $10,$10,$7
        FMUL  $7,$5,Pio2_3t
        FSUB  $7,$7,$10
        FSUB  $8,$6,$7          % y0 = r - w
        FSUB  $9,$6,$8
        FSUB  $9,$9,$7          % y1 = (r - y0) - w
        AND   $10,$4,3
        SET   $13,$8
        SET   $14,$9
        BZ    $10,3F            % n mod 4 = 0: cos y
        CMP   $11,$10,2
        BZ    $11,4F            % 2: -cos y
        PUSHJ $12,KSin          % 1: -sin y, 3: sin y
        CMP   $11,$10,3
        BZ    $11,5F
        XOR   $12,$12,Sign
        JMP   5F
3H      PUSHJ $12,KCos
        JMP   5F
4H      PUSHJ $12,KCos
        XOR   $12,$12,Sign
5H      PUT   :rJ,$1
        SET   $0,$12
        POP   1,0

% KCos: $0 = x, $1 = y, |x| < pi/4 -> cos (x + y)
KCos    SLU   $2,$0,1
        SRU   $2,$2,1
        SRU   $2,$2,32          % ix: the high word of |x|
        CMP   $3,$2,K3E400000
        BNN   $3,1F
        SET   $0,One            % |x| < 2^-27
        POP   1,0
1H      FMUL  $3,$0,$0          % z
        FMUL  $4,$3,C6
        FADD  $4,$4,C5
        FMUL  $4,$4,$3
        FADD  $4,$4,C4
        FMUL  $4,$4,$3
        FADD  $4,$4,C3
        FMUL  $4,$4,$3
        FADD  $4,$4,C2
        FMUL  $4,$4,$3
        FADD  $4,$4,C1
        FMUL  $4,$4,$3          % r
        FMUL  $5,$3,$4
        FMUL  $6,$0,$1
        FSUB  $5,$5,$6          % z r - x y
        CMP   $7,$2,K3FD33333
        BNN   $7,2F
        FMUL  $6,$3,Half        % |x| < 0.3: 1 - (z/2 - (z r - x y))
        FSUB  $6,$6,$5
        FSUB  $0,One,$6
        POP   1,0
2H      CMP   $7,$2,K3FE90000
        BNP   $7,3F
        SET   $6,Q28125         % qx = 0.28125
        JMP   4F
3H      SUB   $6,$2,K200000     % qx = x / 4, with the low word 0
        SLU   $6,$6,32
4H      FMUL  $7,$3,Half
        FSUB  $7,$7,$6          % hz = z/2 - qx
        FSUB  $6,One,$6         % a = 1 - qx
        FSUB  $7,$7,$5
        FSUB  $0,$6,$7          % a - (hz - (z r - x y))
        POP   1,0

% KSin: $0 = x, $1 = y, |x| < pi/4 -> sin (x + y)
KSin    SLU   $2,$0,1
        SRU   $2,$2,1
        SRU   $2,$2,32
        CMP   $3,$2,K3E400000
        BNN   $3,1F
        POP   1,0               % |x| < 2^-27: x
1H      FMUL  $3,$0,$0          % z
        FMUL  $4,$3,$0          % v = z x
        FMUL  $5,$3,S6
        FADD  $5,$5,S5
        FMUL  $5,$5,$3
        FADD  $5,$5,S4
        FMUL  $5,$5,$3
        FADD  $5,$5,S3
        FMUL  $5,$5,$3
        FADD  $5,$5,S2          % r
        FMUL  $6,$4,$5          % v r
        FMUL  $7,$1,Half
        FSUB  $7,$7,$6          % y/2 - v r
        FMUL  $7,$3,$7
        FSUB  $7,$7,$1
        FMUL  $6,$4,S1
        FSUB  $7,$7,$6
        FSUB  $0,$0,$7          % x - ((z (y/2 - v r) - y) - v S1)
        POP   1,0

% acos x: the y in 0 ... pi with cos y = x; cos falls from 1 to -1
Acos    GET   $1,:rJ
        SET   $2,0              % low
        SET   $3,Pi             % high
        SET   $4,70             % steps
1H      FADD  $5,$2,$3
        FMUL  $5,$5,Half        % the middle
        SET   $8,$5
        PUSHJ $7,Cos            % $7 = cos of the middle
        FCMP  $6,$7,$0
        BNP   $6,2F
        SET   $2,$5             % cos(mid) > x: y is above
        JMP   3F
2H      SET   $3,$5
3H      SUB   $4,$4,1
        BNZ   $4,1B
        PUT   :rJ,$1
        FADD  $0,$2,$3
        FMUL  $0,$0,Half
        POP   1,0
        PREFIX :
