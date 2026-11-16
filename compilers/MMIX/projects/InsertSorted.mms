% InsertSorted.mms - inserts random integers into a sorted array.
%
% MMIX version of InsertSorted.asm.  Usage: InsertSorted [seed]
% Inserts 30 random integers in [1,999] one at a time, each into its
% place in the sorted array (found by binary search, the later of equal
% numbers goes last), and prints the array after each insertion.  The
% array (count in [0]) is at Data_Segment+#400.
%
% Options: lib/std.mms lib/args.mms lib/rand.mms lib/array.mms
Array   IS    Data_Segment+#400
        GREG  Array
        LOC   Data_Segment
        GREG  @
Head    BYTE  "Inserting 30 random integers [1,999] one at a time:",0
InsS    BYTE  "Insert ",0
Colon   BYTE  ": ",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BN    $2,1F
        LDOU  $4,$1,8           % an optional seed
        PUSHJ $3,Args:ParseInt
        SET   $5,$3
        PUSHJ $4,Rand:Seed
1H      LDA   $0,Array          % the array starts empty
        LDA   $5,Head
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $1,1              % i
2H      SET   $4,1
        SET   $5,999
        PUSHJ $3,Rand:Range     % $3 = the value
        SET   $5,$0
        SET   $6,$3
        PUSHJ $4,Insert
        LDA   $5,InsS
        PUSHJ $4,Std:Puts
        SET   $5,$3
        SET   $6,3
        PUSHJ $4,Std:PutDecW
        LDA   $5,Colon
        PUSHJ $4,Std:Puts
        SET   $5,$0
        PUSHJ $4,Arr:Print
        PUSHJ $4,Std:PutNl
        ADD   $1,$1,1
        CMP   $4,$1,30
        BNP   $4,2B
        SET   $255,0
        TRAP  0,Halt,0

% Insert: $0 = an array, $1 = a value: puts the value in its place
Insert  LDO   $2,$0,0           % n
        SET   $3,1              % lo
        SET   $4,$2             % hi
1H      CMP   $5,$3,$4
        BP    $5,3F             % lo > hi: found the place lo
        ADD   $5,$3,$4
        SR    $5,$5,1           % mid
        SL    $6,$5,3
        LDO   $6,$0,$6
        CMP   $7,$6,$1
        BP    $7,2F             % a[mid] > value
        ADD   $3,$5,1
        JMP   1B
2H      SUB   $4,$5,1
        JMP   1B
3H      SET   $5,$2             % move a[lo] ... a[n] up by one
4H      CMP   $6,$5,$3
        BN    $6,5F
        SL    $6,$5,3
        LDO   $7,$0,$6
        ADD   $6,$6,8
        STO   $7,$0,$6
        SUB   $5,$5,1
        JMP   4B
5H      SL    $6,$3,3
        STO   $1,$0,$6
        ADD   $2,$2,1
        STO   $2,$0,0
        POP   0,0
