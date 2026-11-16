% PI_spigot.mms - digits of pi with the Rabinowitz-Wagon spigot.
%
% MMIX version of PI_spigot.asm.  Usage: PI_spigot n
% Writes pi with n digits after the point (n <= 20000, so that the
% array of about 3.4 n octabytes fits in the data segment).  Ten guard
% digits are computed so that a held run of nines can be settled.
%
% Options: lib/std.mms lib/args.mms
A       GREG  Data_Segment+#20000       % the spigot array a[1] ...
EIdx    GREG  #FFFFFFFFFFFFFFFE         % place of the last digit: -2
EEnd    GREG  0                         % one past the last place
        LOC   Data_Segment
        GREG  @
Usage   BYTE  "Usage: PI_spigot <n>   "
        BYTE  "(n = number of digits after the decimal point)",0
Small   BYTE  "n must be >= 1",0
Large   BYTE  "n must be <= 20000",0

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
2H      SET   $3,20000
        CMP   $3,$2,$3
        BNP   $3,3F
        LDA   $5,Large
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
3H      ADD   EEnd,$2,1         % the digits 0 ... n are written
        ADD   $4,$2,11          % total = n + 1 + 10 digits to compute
        MUL   $3,$4,10
        DIV   $3,$3,3
        ADD   $3,$3,1           % $3 = len = total * 10 / 3 + 1
        SET   $5,2              % a[i] = 2 for i = 1 ... len
        SET   $6,1
4H      CMP   $7,$6,$3
        BP    $7,5F
        SL    $7,$6,3
        STO   $5,A,$7
        ADD   $6,$6,1
        JMP   4B
5H      SET   $11,0             % $11 = predigit, $12 = nines
        SET   $12,0
        SET   $5,0              % $5 = j
6H      CMP   $13,$5,$4         % for j < total
        BNN   $13,9F
        SET   $6,0              % $6 = q
        SET   $7,$3             % $7 = i = len
7H      BZ    $7,8F
        ADD   $8,$7,$7
        SUB   $8,$8,1           % den = 2 i - 1
        SL    $9,$7,3
        LDO   $10,A,$9
        MUL   $10,$10,10
        MUL   $13,$6,$7
        ADD   $10,$10,$13       % x = 10 a[i] + q i
        DIV   $6,$10,$8         % q = x / den
        GET   $13,rR
        STO   $13,A,$9          % a[i] = x mod den
        SUB   $7,$7,1
        JMP   7B
8H      DIV   $6,$6,10          % the digit is q mod 10, q / 10 carries
        GET   $13,rR
        STO   $13,A,8           % a[1]
        CMP   $13,$6,9
        BNZ   $13,4F
        ADD   $12,$12,1         % a 9: held back
        JMP   0F
4H      CMP   $13,$6,10
        BNZ   $13,5F
        ADD   $21,$11,1         % a 10: carry into the held digits
        PUSHJ $20,Emit
        SET   $21,0
        SET   $22,$12
        PUSHJ $20,EmitRun
        SET   $11,0
        SET   $12,0
        JMP   0F
5H      SET   $21,$11
        PUSHJ $20,Emit
        SET   $11,$6
        SET   $21,9
        SET   $22,$12
        PUSHJ $20,EmitRun
        SET   $12,0
0H      ADD   $13,EIdx,1
        CMP   $13,$13,EEnd
        BNN   $13,9F
        ADD   $5,$5,1
        JMP   6B
9H      PUSHJ $20,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0

% Emit: $0 = a digit: written if its place is 0 ... n (a '.' after 3)
Emit    ADD   EIdx,EIdx,1
        BN    EIdx,9F
        CMP   $1,EIdx,EEnd
        BNN   $1,9F
        GET   $2,rJ
        ADD   $4,$0,'0'
        PUSHJ $3,Std:PutChar
        BNZ   EIdx,8F
        SET   $4,'.'
        PUSHJ $3,Std:PutChar
8H      PUT   rJ,$2
9H      POP   0,0

% EmitRun: $0 = a digit, $1 = how many times
EmitRun GET   $2,rJ
1H      BZ    $1,9F
        SET   $4,$0
        PUSHJ $3,Emit
        SUB   $1,$1,1
        JMP   1B
9H      PUT   rJ,$2
        POP   0,0
