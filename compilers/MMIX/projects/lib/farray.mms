% farray.mms - arrays of doubles (with std.mms, rand.mms and fmt.mms).
% Listed after the program in Options.
%
% An array is a block of octabytes: the count in [0], the doubles in
% [1] ... [count].
%
%   FArr:Fill    $0 = an array whose count is set: fills it with random
%                doubles, 0 <= x < 1
%   FArr:Print   $0 = an array: writes its numbers with 6 digits after
%                the point, separated by commas
        PREFIX FArr:
Fill    GET   $1,:rJ
        LDO   $2,$0,0
        SET   $3,1
1H      CMP   $4,$3,$2
        BP    $4,9F
        PUSHJ $5,:Rand:Float
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
        SET   $6,6
        PUSHJ $4,:Fmt:PutFixed
        ADD   $3,$3,1
        JMP   1B
9H      PUT   :rJ,$1
        POP   0,0
        PREFIX :
