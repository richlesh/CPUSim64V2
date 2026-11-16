% Hailstone2.mms - the starting number up to a limit with the longest
% Hailstone (Collatz) sequence.
%
% MMIX version of Hailstone2.asm.  Usage: Hailstone2 limit
% Tests every starting number from 2 to the limit, computing each
% sequence recursively without memoization, and prints
% "number: length" for the best one.  Without an argument it prints a
% message and ends with the result 1.
%
% Options: lib/std.mms lib/args.mms
        LOC   Data_Segment
        GREG  @
Colon   BYTE  ": ",0
Msg     BYTE  "You must supply a positive integer argument.",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BN    $2,NoArg
        LDOU  $4,$1,8           % argv[1]
        PUSHJ $3,Args:ParseInt  % $3 = the limit
        SET   $4,1              % imax: the best starting number
        SET   $5,1              % max: its length
        SET   $2,2              % n
1H      CMP   $6,$2,$3
        BP    $6,9F             % n > limit
        SET   $8,$2
        PUSHJ $7,Hail           % $7 = the length of n
        CMP   $6,$7,$5
        BNP   $6,2F
        SET   $4,$2
        SET   $5,$7
2H      ADD   $2,$2,1
        JMP   1B
9H      SET   $7,$4
        PUSHJ $6,Std:PutDec
        LDA   $7,Colon
        PUSHJ $6,Std:Puts
        SET   $7,$5
        PUSHJ $6,Std:PutDec
        PUSHJ $6,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0

NoArg   LDA   $4,Msg
        PUSHJ $3,Std:Puts
        PUSHJ $3,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0

% Hail: $0 = n -> $0 = the number of terms of the sequence from n
Hail    CMP   $1,$0,1
        BP    $1,1F
        SET   $0,1              % n <= 1: the length is 1
        POP   1,0
1H      GET   $1,rJ
        AND   $2,$0,1
        BNZ   $2,2F
        SRU   $4,$0,1           % even: n / 2
        JMP   3F
2H      MUL   $4,$0,3           % odd: 3 * n + 1
        ADD   $4,$4,1
3H      PUSHJ $3,Hail
        ADD   $0,$3,1
        PUT   rJ,$1
        POP   1,0
