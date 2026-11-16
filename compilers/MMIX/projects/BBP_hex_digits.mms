% BBP_hex_digits.mms - hexadecimal digits of pi at any position.
%
% MMIX version of BBP_hex_digits.asm.  Usage: BBP_hex_digits n
% Writes the 32 hexadecimal digits of pi that start at position n (n=1
% is the first digit after the point), computed with the
% Bailey-Borwein-Plouffe formula
%   pi = sum 16^-k (4/(8k+1) - 2/(8k+4) - 1/(8k+5) - 1/(8k+6))
% in 192-bit fixed point: three octabytes (high, middle, low).
% Each sum S_j = sum 16^(d-k) / (8k+j) is taken modulo 1: the first d+1
% terms with 16^(d-k) mod (8k+j), then 47 more terms of the tail.
%
% Options: lib/std.mms lib/args.mms
W       GREG  Data_Segment+#400         % S at W, T at W+32, A at W+64
        LOC   Data_Segment
        GREG  @
Usage   BYTE  "Usage: BBP_hex_digits <n>   "
        BYTE  "(n = 1 is the first hex digit after the point)",0
Small   BYTE  "n must be >= 1",0
Large   BYTE  "n must be <= 10000000",0
Pos     BYTE  "Hex digits of pi at position ",0
Colon   BYTE  ": ",0
        GREG  @
Warn0   BYTE  "Warning: guard bits are all 0s; "
        BYTE  "the last digit(s) may be 1 too high.",0
Warn1   BYTE  "Warning: guard bits are all 1s; "
        BYTE  "the last digit(s) may be 1 too low.",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BZ    $2,1F
        LDA   $5,Usage
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      LDOU  $3,$1,8
        PUSHJ $2,Args:ParseInt  % $2 = n
        BP    $2,2F
        LDA   $5,Small
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
2H      SETL  $3,#9680          % 10000000
        INCML $3,#98
        CMP   $3,$2,$3
        BNP   $3,3F
        LDA   $5,Large
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
3H      SUB   $3,$2,1           % $3 = d = n - 1
        SET   $11,1             % A = 4 S1 - 2 S4 - S5 - S6
        SET   $12,$3
        PUSHJ $10,Series
        ADDU  $11,W,64
        SET   $12,W
        PUSHJ $10,Copy192
        ADDU  $11,W,64
        SET   $12,2
        PUSHJ $10,Shl192
        SET   $11,4
        SET   $12,$3
        PUSHJ $10,Series
        SET   $11,W
        SET   $12,1
        PUSHJ $10,Shl192
        ADDU  $11,W,64
        SET   $12,W
        PUSHJ $10,Sub192
        SET   $11,5
        SET   $12,$3
        PUSHJ $10,Series
        ADDU  $11,W,64
        SET   $12,W
        PUSHJ $10,Sub192
        SET   $11,6
        SET   $12,$3
        PUSHJ $10,Series
        ADDU  $11,W,64
        SET   $12,W
        PUSHJ $10,Sub192
        LDA   $11,Pos           % Hex digits of pi at position n: ...
        PUSHJ $10,Std:Puts
        SET   $11,$2
        PUSHJ $10,Std:PutDec
        LDA   $11,Colon
        PUSHJ $10,Std:Puts
        LDO   $11,W,64          % the high and middle octabytes
        SET   $12,16
        PUSHJ $10,Std:PutHexU
        LDO   $11,W,72
        SET   $12,16
        PUSHJ $10,Std:PutHexU
        PUSHJ $10,Std:PutNl
        LDO   $4,W,80           % the guard bits
        SR    $4,$4,32
        BNZ   $4,4F
        LDA   $11,Warn0
        PUSHJ $10,Std:Puts
        PUSHJ $10,Std:PutNl
        JMP   9F
4H      ADD   $4,$4,1
        BNZ   $4,9F
        LDA   $11,Warn1
        PUSHJ $10,Std:Puts
        PUSHJ $10,Std:PutNl
9H      SET   $255,0
        TRAP  0,Halt,0

% Series: $0 = j, $1 = d: S = sum for k = 0 ... d of 16^(d-k) mod m / m
% m = 8k + j, plus the terms 1/m 16^-t for k = d+t, t = 1 ... 47
Series  GET   $6,rJ
        STCO  0,W,0
        STCO  0,W,8
        STCO  0,W,16
        SET   $2,0              % k
1H      CMP   $4,$2,$1
        BP    $4,2F
        SL    $3,$2,3
        ADD   $3,$3,$0          % m
        SUB   $11,$1,$2         % p = d - k
        SET   $12,$3
        PUSHJ $10,PowMod        % $10 = 16^p mod m
        ADDU  $11,W,32          % T = $10 / m
        SET   $12,$10
        SET   $13,$3
        PUSHJ $10,Frac192
        SET   $11,W             % S = S + T
        ADDU  $12,W,32
        PUSHJ $10,Add192
        ADD   $2,$2,1
        JMP   1B
2H      SET   $2,1              % t
3H      CMP   $4,$2,48
        BNN   $4,9F
        ADD   $5,$1,$2          % k = d + t
        SL    $3,$5,3
        ADD   $3,$3,$0          % m
        ADDU  $11,W,32          % T = 1 / m
        SET   $12,1
        SET   $13,$3
        PUSHJ $10,Frac192
        ADDU  $11,W,32          % shifted right by 4 t bits
        SL    $12,$2,2
        PUSHJ $10,Shr192
        SET   $11,W             % S = S + T
        ADDU  $12,W,32
        PUSHJ $10,Add192
        ADD   $2,$2,1
        JMP   3B
9H      PUT   rJ,$6
        POP   0,0

% PowMod: $0 = e, $1 = m -> 16^e mod m
PowMod  CMP   $2,$1,1
        BNZ   $2,1F
        SET   $0,0              % anything mod 1 is 0
        POP   1,0
1H      SET   $2,1              % the result
        SET   $3,16
        DIVU  $4,$3,$1
        GET   $3,rR             % b = 16 mod m
2H      BZ    $0,9F
        AND   $4,$0,1
        BZ    $4,3F
        MULU  $5,$2,$3
        DIVU  $6,$5,$1
        GET   $2,rR             % result = result * b mod m
3H      MULU  $5,$3,$3
        DIVU  $6,$5,$1
        GET   $3,rR             % b = b * b mod m
        SRU   $0,$0,1
        JMP   2B
9H      SET   $0,$2
        POP   1,0

% Frac192: $0 = the address of a 192-bit number, $1 = num, $2 = den
% (num < den): stores the fraction num / den, 32 bits at a time
Frac192 SET   $3,$1             % the remainder
        SET   $4,0              % the octabyte being made
        SET   $5,0              % the number of 32-bit pieces
1H      SLU   $6,$3,32
        DIVU  $7,$6,$2
        GET   $3,rR
        SLU   $4,$4,32
        OR    $4,$4,$7
        ADD   $5,$5,1
        AND   $8,$5,1
        BNZ   $8,1B             % half an octabyte: go on
        SUB   $8,$5,2           % 0, 2 or 4: the place is 4 * that
        SL    $8,$8,2
        STO   $4,$0,$8
        SET   $4,0
        CMP   $8,$5,6
        BN    $8,1B
        POP   0,0

% Copy192: $0 = to, $1 = from
Copy192 LDO   $2,$1,0
        STO   $2,$0,0
        LDO   $2,$1,8
        STO   $2,$0,8
        LDO   $2,$1,16
        STO   $2,$0,16
        POP   0,0

% Add192: $0 += $1 (addresses of 192-bit numbers)
Add192  LDO   $2,$0,16          % the low octabytes
        LDO   $3,$1,16
        ADDU  $4,$2,$3
        CMPU  $5,$4,$2
        ZSN   $5,$5,1           % $5 = the carry
        STO   $4,$0,16
        LDO   $2,$0,8           % the middle octabytes
        LDO   $3,$1,8
        ADDU  $4,$2,$3
        CMPU  $6,$4,$2
        ZSN   $6,$6,1           % the carry of the sum
        ADDU  $7,$4,$5
        CMPU  $8,$7,$4
        ZSN   $8,$8,1           % the carry of adding the low carry
        OR    $6,$6,$8
        STO   $7,$0,8
        LDO   $2,$0,0           % the high octabytes
        LDO   $3,$1,0
        ADDU  $2,$2,$3
        ADDU  $2,$2,$6
        STO   $2,$0,0
        POP   0,0

% Sub192: $0 -= $1
Sub192  LDO   $2,$0,16
        LDO   $3,$1,16
        SUBU  $4,$2,$3
        CMPU  $5,$2,$3
        ZSN   $5,$5,1           % $5 = the borrow
        STO   $4,$0,16
        LDO   $2,$0,8
        LDO   $3,$1,8
        SUBU  $4,$2,$3
        CMPU  $6,$2,$3
        ZSN   $6,$6,1           % the borrow of the difference
        SUBU  $7,$4,$5
        CMPU  $8,$4,$5
        ZSN   $8,$8,1           % the borrow of the low borrow
        OR    $6,$6,$8
        STO   $7,$0,8
        LDO   $2,$0,0
        LDO   $3,$1,0
        SUBU  $2,$2,$3
        SUBU  $2,$2,$6
        STO   $2,$0,0
        POP   0,0

% Shl192: $0 = an address, $1 = n (1 ... 63): the number times 2^n
Shl192  LDO   $2,$0,0
        LDO   $3,$0,8
        LDO   $4,$0,16
        SET   $5,64
        SUB   $5,$5,$1
        SLU   $2,$2,$1
        SRU   $6,$3,$5
        OR    $2,$2,$6
        STO   $2,$0,0
        SLU   $3,$3,$1
        SRU   $6,$4,$5
        OR    $3,$3,$6
        STO   $3,$0,8
        SLU   $4,$4,$1
        STO   $4,$0,16
        POP   0,0

% Shr192: $0 = an address, $1 = s (1 ... 191): the number / 2^s
Shr192  LDO   $2,$0,0
        LDO   $3,$0,8
        LDO   $4,$0,16
1H      CMP   $5,$1,64
        BN    $5,2F
        SET   $4,$3             % 64 bits: move the octabytes down
        SET   $3,$2
        SET   $2,0
        SUB   $1,$1,64
        JMP   1B
2H      BZ    $1,3F
        SET   $5,64
        SUB   $5,$5,$1
        SRU   $4,$4,$1
        SLU   $6,$3,$5
        OR    $4,$4,$6
        SRU   $3,$3,$1
        SLU   $6,$2,$5
        OR    $3,$3,$6
        SRU   $2,$2,$1
3H      STO   $2,$0,0
        STO   $3,$0,8
        STO   $4,$0,16
        POP   0,0
