% array.mms - integer arrays (with std.mms and rand.mms).
% Listed after the program in Options, after std.mms and rand.mms.
%
% An array is a block of octabytes: the count in [0], the numbers in
% [1] ... [count].
%
%   Arr:Fill    $0 = an array whose count is set: fills it with random
%               integers 1 ... 999
%   Arr:Print   $0 = an array: writes its numbers, separated by commas
        PREFIX Arr:
Fill    GET   $1,:rJ
        LDO   $2,$0,0
        SET   $3,1
1H      CMP   $4,$3,$2
        BP    $4,9F
        SET   $6,1
        SET   $7,999
        PUSHJ $5,:Rand:Range
        SL    $4,$3,3
        STO   $5,$0,$4
        ADD   $3,$3,1
        JMP   1B
9H      PUT   :rJ,$1
        POP   0,0

Print   GET   $1,:rJ
        LDO   $2,$0,0
        SET   $3,1
1H      CMP   $4,$3,$2
        BP    $4,9F
        CMP   $4,$3,1
        BNP   $4,2F             % no comma before the first number
        SET   $5,','
        PUSHJ $4,:Std:PutChar
2H      SL    $4,$3,3
        LDO   $5,$0,$4
        PUSHJ $4,:Std:PutDec
        ADD   $3,$3,1
        JMP   1B
9H      PUT   :rJ,$1
        POP   0,0
        PREFIX :
