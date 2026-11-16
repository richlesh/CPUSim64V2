% Hailstone.mms - the length of the Hailstone (Collatz) sequence.
%
% MMIX version of Hailstone.asm.  With an argument n, prints the length
% of the sequence that starts at n; without one, prints a table of the
% lengths for n = 1 ... 100, ending with the result 1 as the original
% does.  The length is computed recursively, without memoization.
%
%     f(n) = n / 2       if n is even
%     f(n) = 3 * n + 1   if n is odd
%
% Options: lib/std.mms lib/args.mms
        LOC   Data_Segment
        GREG  @
Colon   BYTE  ": ",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BN    $2,Demo
        LDOU  $4,$1,8           % argv[1]
        PUSHJ $3,Args:ParseInt
        SET   $5,$3
        PUSHJ $4,Hail
        SET   $5,$4
        PUSHJ $4,Std:PutDec
        PUSHJ $4,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0

% No argument: the table n: length
Demo    SET   $2,1              % n
1H      SET   $4,$2
        PUSHJ $3,Hail           % $3 = the length
        SET   $5,$2
        PUSHJ $4,Std:PutDec
        LDA   $5,Colon
        PUSHJ $4,Std:Puts
        SET   $5,$3
        PUSHJ $4,Std:PutDec
        PUSHJ $4,Std:PutNl
        ADD   $2,$2,1
        CMP   $3,$2,100
        BNP   $3,1B
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
