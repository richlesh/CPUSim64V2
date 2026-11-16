% Unicode.mms - prints ranges of Unicode code points.
%
% MMIX version of Unicode.asm: 32 characters per line, each range after
% its title.  Std:PutChar writes a code point as UTF-8.
%
% Options: lib/std.mms
        LOC   Data_Segment
        GREG  @
T1      BYTE  "ASCII",0
T2      BYTE  "Basic Multilingual",0
T3      BYTE  "Math Symbols",0
T4      BYTE  "Drawing",0
T5      BYTE  "Symbols",0
T6      BYTE  "Icons",0

        LOC   #100
Main    LDA   $4,T1
        PUSHJ $3,Std:Puts
        PUSHJ $3,Std:PutNl
        SET   $4,#20
        SET   $5,#7f
        PUSHJ $3,Range
        LDA   $4,T2
        PUSHJ $3,Std:Puts
        PUSHJ $3,Std:PutNl
        SET   $4,#a0
        SET   $5,#1ff
        PUSHJ $3,Range
        LDA   $4,T3
        PUSHJ $3,Std:Puts
        PUSHJ $3,Std:PutNl
        SET   $4,#2200
        SET   $5,#22ff
        PUSHJ $3,Range
        LDA   $4,T4
        PUSHJ $3,Std:Puts
        PUSHJ $3,Std:PutNl
        SET   $4,#2500
        SET   $5,#25ff
        PUSHJ $3,Range
        LDA   $4,T5
        PUSHJ $3,Std:Puts
        PUSHJ $3,Std:PutNl
        SET   $4,#2600
        SET   $5,#27ff
        PUSHJ $3,Range
        LDA   $4,T6
        PUSHJ $3,Std:Puts
        PUSHJ $3,Std:PutNl
        SETL  $4,#f300          % #1f300
        INCML $4,#1
        SETL  $5,#f4ff          % #1f4ff
        INCML $5,#1
        PUSHJ $3,Range
        SET   $255,0
        TRAP  0,Halt,0

% Range: $0 = the first code point, $1 = the last
Range   GET   $2,rJ
1H      CMP   $3,$0,$1
        BP    $3,9F
        SET   $5,$0
        PUSHJ $4,Std:PutChar
        AND   $3,$0,31          % a newline after every 32nd
        CMP   $3,$3,31
        BNZ   $3,2F
        PUSHJ $4,Std:PutNl
2H      ADD   $0,$0,1
        JMP   1B
9H      PUT   rJ,$2
        POP   0,0
