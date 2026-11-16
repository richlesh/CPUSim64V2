% Renard2.mms - the Renard series of preferred numbers, several sizes.
%
% MMIX version of Renard2.asm.  Usage: Renard2 series ...
% For each argument writes "series:" and then the numbers
% 10^(i/series) for i = 0 ... series-1, each with a space and 2 digits
% after the point.
%
% Options: lib/std.mms lib/args.mms lib/fmt.mms lib/math.mms
Ten     GREG  #4024000000000000         % 10.0
        LOC   Data_Segment
        GREG  @
Need    BYTE  "You must supply a series value 5, 10, 20 or 40",0
Space   BYTE  " ",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BNN   $2,1F
        LDA   $5,Need
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      SET   $2,1              % the argument
2H      CMP   $3,$2,$0
        BNN   $3,3F
        SL    $3,$2,3
        LDOU  $5,$1,$3
        PUSHJ $4,Args:ParseInt
        SET   $5,$4
        PUSHJ $4,Print
        ADD   $2,$2,1
        JMP   2B
3H      SET   $255,0
        TRAP  0,Halt,0

% Print: $0 = the series: one line
Print   GET   $1,rJ
        SET   $5,$0
        PUSHJ $4,Std:PutDec
        SET   $5,':'
        PUSHJ $4,Std:PutChar
        SET   $3,Ten            % $2 = the factor: 10^(1/series)
        SET   $4,$0
        PUSHJ $2,Math:Root
        SET   $3,0              % i
1H      CMP   $4,$3,$0
        BNN   $4,2F
        LDA   $5,Space
        PUSHJ $4,Std:Puts
        SET   $8,$2             % factor^i
        SET   $9,$3
        PUSHJ $7,Math:PowI
        SET   $9,$7
        SET   $10,2
        PUSHJ $8,Fmt:PutFixed
        ADD   $3,$3,1
        JMP   1B
2H      PUSHJ $4,Std:PutNl
        PUT   rJ,$1
        POP   0,0
