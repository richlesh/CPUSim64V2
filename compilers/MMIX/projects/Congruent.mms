% Congruent.mms - is n a congruent number?  (Tunnell's theorem)
%
% MMIX version of Congruent.asm.  Usage: Congruent n
% Counts the integer solutions of the two ternary quadratic forms of
% Tunnell's theorem for the squarefree kernel m of n and compares the
% counts (t1 = 2 t2 means congruent, assuming the BSD conjecture).  The
% limits of the loops come from FSQRT, as in the original.
%
% Options: lib/std.mms lib/args.mms
        LOC   Data_Segment
        GREG  @
Usage   BYTE  "Usage: Congruent <n>",0
NotPos  BYTE  "n must be a positive integer",0
NS      BYTE  "n = ",0
Kern    BYTE  " (squarefree kernel = ",0
Close   BYTE  ")",0
        GREG  @
T1S     BYTE  "t1 = ",0
T2S     BYTE  ", 2*t2 = ",0
IsS     BYTE  " IS a congruent number (assuming BSD)",0
NotS    BYTE  " is NOT a congruent number",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BNN   $2,1F
        LDA   $5,Usage
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      LDOU  $3,$1,8
        PUSHJ $2,Args:ParseInt  % $2 = n
        BP    $2,2F
        LDA   $5,NotPos
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
2H      SET   $4,$2
        PUSHJ $3,Kernel         % $3 = m
        AND   $4,$3,1
        BZ    $4,3F
        SET   $9,$3             % m odd: counts for 2, 1, 8 and 32
        SET   $10,2
        SET   $11,1
        SET   $12,8
        PUSHJ $8,Count
        SET   $4,$8             % $4 = t1
        SET   $9,$3
        SET   $10,2
        SET   $11,1
        SET   $12,32
        PUSHJ $8,Count
        JMP   4F
3H      SET   $9,$3             % m even: counts for 8, 2, 16 and 64
        SET   $10,8
        SET   $11,2
        SET   $12,16
        PUSHJ $8,Count
        SET   $4,$8
        SET   $9,$3
        SET   $10,8
        SET   $11,2
        SET   $12,64
        PUSHJ $8,Count
4H      MUL   $5,$8,2           % $5 = 2 t2
        LDA   $10,NS            % n = N (squarefree kernel = M)
        PUSHJ $9,Std:Puts
        SET   $10,$2
        PUSHJ $9,Std:PutDec
        LDA   $10,Kern
        PUSHJ $9,Std:Puts
        SET   $10,$3
        PUSHJ $9,Std:PutDec
        LDA   $10,Close
        PUSHJ $9,Std:Puts
        PUSHJ $9,Std:PutNl
        LDA   $10,T1S           % t1 = T, 2*t2 = U
        PUSHJ $9,Std:Puts
        SET   $10,$4
        PUSHJ $9,Std:PutDec
        LDA   $10,T2S
        PUSHJ $9,Std:Puts
        SET   $10,$5
        PUSHJ $9,Std:PutDec
        PUSHJ $9,Std:PutNl
        SET   $10,$2
        PUSHJ $9,Std:PutDec
        CMP   $6,$4,$5
        LDA   $10,IsS
        BZ    $6,5F
        LDA   $10,NotS
5H      PUSHJ $9,Std:Puts
        PUSHJ $9,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0

% Kernel: $0 = n -> the squarefree kernel: the squares are divided out
Kernel  SET   $1,$0             % m
        SET   $2,2              % i
        MUL   $3,$2,$2          % i^2
1H      CMP   $4,$3,$1
        BP    $4,9F
        DIV   $5,$1,$3
        GET   $6,rR
        BNZ   $6,2F
        SET   $1,$5             % i^2 divides m
        JMP   1B
2H      ADD   $2,$2,1
        MUL   $3,$2,$2
        JMP   1B
9H      SET   $0,$1
        POP   1,0

% Limit: $0 = t, $1 = d -> floor(sqrt(t / d)) with doubles
Limit   FLOT  $2,$0
        FLOT  $3,$1
        FDIV  $2,$2,$3
        FSQRT $2,$2
        FIX   $0,ROUND_OFF,$2
        POP   1,0

% Isqrt: $0 = n -> floor(sqrt(n)) with doubles
Isqrt   FLOT  $1,$0
        FSQRT $1,$1
        FIX   $0,ROUND_OFF,$1
        POP   1,0

% Count: $0 = target, $1 = a, $2 = b, $3 = c -> the number of integer
% solutions of a x^2 + b y^2 + c z^2 = target
Count   GET   $14,rJ
        SET   $4,0              % the count
        SET   $16,$0
        SET   $17,$1
        PUSHJ $15,Limit
        SET   $5,$15            % the limit for x
        NEG   $6,$5             % x
1H      CMP   $15,$6,$5
        BP    $15,9F
        MUL   $7,$6,$6
        MUL   $7,$7,$1          % a x^2
        CMP   $15,$7,$0
        BP    $15,8F            % more than the target
        SUB   $7,$0,$7          % what is left for b y^2 + c z^2
        SET   $16,$7
        SET   $17,$2
        PUSHJ $15,Limit
        SET   $8,$15            % the limit for y
        NEG   $9,$8             % y
2H      CMP   $15,$9,$8
        BP    $15,8F
        MUL   $10,$9,$9
        MUL   $10,$10,$2        % b y^2
        CMP   $15,$10,$7
        BP    $15,7F
        SUB   $10,$7,$10        % what is left for c z^2
        DIV   $11,$10,$3
        GET   $12,rR
        BNZ   $12,7F            % not a multiple of c
        SET   $16,$11
        PUSHJ $15,Isqrt
        SET   $13,$15           % z
        MUL   $15,$13,$13
        CMP   $15,$15,$11
        BNZ   $15,7F            % z^2 is not the number
        BZ    $13,6F
        ADD   $4,$4,2           % z and -z
        JMP   7F
6H      ADD   $4,$4,1           % z = 0
7H      ADD   $9,$9,1
        JMP   2B
8H      ADD   $6,$6,1
        JMP   1B
9H      PUT   rJ,$14
        SET   $0,$4
        POP   1,0
