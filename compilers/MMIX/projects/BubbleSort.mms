% BubbleSort.mms - a stable bubble sort of random integers.
%
% MMIX version of BubbleSort.asm.  Usage: BubbleSort [seed]
% Ten times: an array of 15 to 30 random integers in [1,999] is
% printed, sorted in place with a stable bubble sort (only strictly
% greater neighbours are swapped), and printed again.  The array
% (count in [0]) is at Data_Segment+#400.
%
% Options: lib/std.mms lib/args.mms lib/rand.mms lib/array.mms
Array   IS    Data_Segment+#400
        GREG  Array
        LOC   Data_Segment
        GREG  @
TestS   BYTE  "Test ",0
RandS   BYTE  "random: ",0
SortS   BYTE  "sorted: ",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BN    $2,1F
        LDOU  $4,$1,8           % an optional seed
        PUSHJ $3,Args:ParseInt
        SET   $5,$3
        PUSHJ $4,Rand:Seed
1H      SET   $0,1              % the test number
        LDA   $2,Array
2H      SET   $4,15
        SET   $5,30
        PUSHJ $3,Rand:Range     % the number of elements
        STO   $3,$2,0
        SET   $5,$2
        PUSHJ $4,Arr:Fill
        LDA   $5,TestS
        PUSHJ $4,Std:Puts
        SET   $5,$0
        PUSHJ $4,Std:PutDec
        PUSHJ $4,Std:PutNl
        LDA   $5,RandS
        PUSHJ $4,Std:Puts
        SET   $5,$2
        PUSHJ $4,Arr:Print
        PUSHJ $4,Std:PutNl
        SET   $5,$2
        PUSHJ $4,Sort
        LDA   $5,SortS
        PUSHJ $4,Std:Puts
        SET   $5,$2
        PUSHJ $4,Arr:Print
        PUSHJ $4,Std:PutNl
        ADD   $0,$0,1
        CMP   $4,$0,10
        BNP   $4,2B
        SET   $255,0
        TRAP  0,Halt,0

% Sort: $0 = an array
Sort    LDO   $1,$0,0           % n
        SET   $2,1              % i
1H      CMP   $3,$2,$1
        BNN   $3,9F             % i >= n
        SUB   $4,$1,$2          % limit
        SET   $5,1              % j
2H      CMP   $3,$5,$4
        BP    $3,3F             % j > limit
        SL    $6,$5,3
        ADD   $7,$6,8
        LDO   $8,$0,$6          % a[j]
        LDO   $9,$0,$7          % a[j+1]
        CMP   $3,$8,$9
        BNP   $3,4F
        STO   $9,$0,$6          % swap
        STO   $8,$0,$7
4H      ADD   $5,$5,1
        JMP   2B
3H      ADD   $2,$2,1
        JMP   1B
9H      POP   0,0
