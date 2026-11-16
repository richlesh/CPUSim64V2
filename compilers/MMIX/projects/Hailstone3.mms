% Hailstone3.mms - the starting number up to a limit with the longest
% Hailstone (Collatz) sequence, with memoization.
%
% MMIX version of Hailstone3.asm.  Usage: Hailstone3 limit
% As Hailstone2, but the lengths of the sequences that start below
% CacheN are kept in a table, and a line "n...imax:max" is printed for
% every 1000th n.  The table has 100000 entries (the original: 3000000)
% so that it fits in the default data segment.
%
% Options: lib/std.mms lib/args.mms
Cache   GREG  Data_Segment+#20000       % the table, 8 bytes per entry
CacheN  GREG  100000
Thou    GREG  1000
        LOC   Data_Segment
        GREG  @
Dots    BYTE  "...",0
Colon   BYTE  ":",0
Colons  BYTE  ": ",0
Msg     BYTE  "You must supply a positive integer argument.",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BN    $2,NoArg
        SET   $4,1
        STO   $4,Cache,8        % the length of 1 is 1
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
2H      DIV   $6,$2,Thou
        GET   $6,rR
        BNZ   $6,3F
        SET   $7,$2             % every 1000th: n...imax:max
        PUSHJ $6,Std:PutDec
        LDA   $7,Dots
        PUSHJ $6,Std:Puts
        SET   $7,$4
        PUSHJ $6,Std:PutDec
        LDA   $7,Colon
        PUSHJ $6,Std:Puts
        SET   $7,$5
        PUSHJ $6,Std:PutDec
        PUSHJ $6,Std:PutNl
3H      ADD   $2,$2,1
        JMP   1B
9H      SET   $7,$4             % imax: max
        PUSHJ $6,Std:PutDec
        LDA   $7,Colons
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
Hail    CMPU  $1,$0,CacheN
        BNN   $1,2F             % n >= CacheN: not in the table
        SL    $2,$0,3
        LDO   $3,Cache,$2
        BZ    $3,2F             % not computed yet
        SET   $0,$3
        POP   1,0
2H      GET   $1,rJ
        AND   $2,$0,1
        BNZ   $2,3F
        SRU   $4,$0,1           % even: n / 2
        JMP   4F
3H      MUL   $4,$0,3           % odd: 3 * n + 1
        ADD   $4,$4,1
4H      PUSHJ $3,Hail
        ADD   $3,$3,1
        CMPU  $2,$0,CacheN
        BNN   $2,5F
        SL    $2,$0,3
        STO   $3,Cache,$2       % keep the result
5H      PUT   rJ,$1
        SET   $0,$3
        POP   1,0
