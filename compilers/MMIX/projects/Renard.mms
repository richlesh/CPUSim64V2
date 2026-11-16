% Renard.mms - the Renard series of preferred numbers.
%
% MMIX version of Renard.asm.  Usage: Renard series   (5, 10, 20, 40)
% Writes the numbers 10^(i/series) for i = 0 ... series-1, each with 2
% digits after the point and a space after it.  The root comes from
% Newton's method (Math:Root), the power by repeated multiplication.
%
% Options: lib/std.mms lib/args.mms lib/fmt.mms lib/math.mms
Ten     GREG  #4024000000000000         % 10.0
        LOC   Data_Segment
        GREG  @
Need    BYTE  "You must supply a series value 5, 10, 20 or 40",0
Small   BYTE  "You must supply a series value > 2",0
Space   BYTE  " ",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BZ    $2,1F
        LDA   $5,Need
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      LDOU  $5,$1,8
        PUSHJ $4,Args:ParseInt
        SET   $2,$4             % $2 = the series
        CMP   $3,$2,2
        BNN   $3,2F
        LDA   $5,Small
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,2
        TRAP  0,Halt,0
2H      SET   $4,Ten            % $3 = the factor: 10^(1/series)
        SET   $5,$2
        PUSHJ $3,Math:Root
        SET   $4,0              % i
3H      CMP   $5,$4,$2
        BNN   $5,4F
        SET   $8,$3             % factor^i
        SET   $9,$4
        PUSHJ $7,Math:PowI
        SET   $9,$7
        SET   $10,2
        PUSHJ $8,Fmt:PutFixed
        LDA   $9,Space
        PUSHJ $8,Std:Puts
        ADD   $4,$4,1
        JMP   3B
4H      PUSHJ $5,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0
