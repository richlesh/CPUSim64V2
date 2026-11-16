% QuickSortFloat.mms - a quicksort of random doubles.
%
% MMIX version of QuickSortFloat.asm.  Usage: QuickSortFloat [seed]
% Ten times: an array of 9 to 14 random numbers in [0,1) is printed
% with 6 digits, sorted in place with a recursive quicksort (the last
% element is the pivot), and printed again.  The array (count in [0])
% is at Data_Segment+#400.
%
% Options: lib/std.mms lib/args.mms lib/rand.mms
% Options: lib/fmt.mms lib/farray.mms
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
2H      SET   $4,9
        SET   $5,14
        PUSHJ $3,Rand:Range     % the number of elements
        STO   $3,$2,0
        SET   $5,$2
        PUSHJ $4,FArr:Fill
        LDA   $5,TestS
        PUSHJ $4,Std:Puts
        SET   $5,$0
        PUSHJ $4,Std:PutDec
        PUSHJ $4,Std:PutNl
        LDA   $5,RandS
        PUSHJ $4,Std:Puts
        SET   $5,$2
        PUSHJ $4,FArr:Print
        PUSHJ $4,Std:PutNl
        SET   $5,$2
        PUSHJ $4,Sort
        LDA   $5,SortS
        PUSHJ $4,Std:Puts
        SET   $5,$2
        PUSHJ $4,FArr:Print
        PUSHJ $4,Std:PutNl
        ADD   $0,$0,1
        CMP   $4,$0,10
        BNP   $4,2B
        SET   $255,0
        TRAP  0,Halt,0

% Sort: $0 = an array of doubles
Sort    GET   $1,rJ
        SET   $3,$0
        SET   $4,1
        LDO   $5,$0,0
        PUSHJ $2,Range          % Range(array, 1, n)
        PUT   rJ,$1
        POP   0,0

% Range: $0 = an array, $1 = lo, $2 = hi: sorts a[lo] ... a[hi]
Range   CMP   $3,$1,$2
        BN    $3,1F
        POP   0,0
1H      GET   $3,rJ
        SET   $5,$0
        SET   $6,$1
        SET   $7,$2
        PUSHJ $4,Part           % $4 = the place of the pivot
        SET   $6,$0
        SET   $7,$1
        SUB   $8,$4,1
        PUSHJ $5,Range          % Range(array, lo, p - 1)
        SET   $6,$0
        ADD   $7,$4,1
        SET   $8,$2
        PUSHJ $5,Range          % Range(array, p + 1, hi)
        PUT   rJ,$3
        POP   0,0

% Part: $0 = an array, $1 = lo, $2 = hi: moves the elements <= a[hi] in
% front of it and returns the place of a[hi]
Part    SL    $3,$2,3
        LDO   $3,$0,$3          % the pivot
        SUB   $4,$1,1           % i
        SET   $5,$1             % j
1H      CMP   $6,$5,$2
        BNN   $6,2F             % j >= hi
        SL    $6,$5,3
        LDO   $7,$0,$6          % a[j]
        FCMP  $8,$7,$3
        BP    $8,3F             % a[j] > pivot
        ADD   $4,$4,1
        SL    $8,$4,3
        LDO   $9,$0,$8          % a[i]
        STO   $7,$0,$8          % swap a[i] and a[j]
        STO   $9,$0,$6
3H      ADD   $5,$5,1
        JMP   1B
2H      ADD   $4,$4,1
        SL    $8,$4,3
        LDO   $9,$0,$8
        SL    $6,$2,3
        LDO   $7,$0,$6
        STO   $7,$0,$8          % swap a[i] and a[hi]
        STO   $9,$0,$6
        SET   $0,$4
        POP   1,0
