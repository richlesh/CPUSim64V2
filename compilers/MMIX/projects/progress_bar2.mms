% progress_bar2.mms - a bar with an eye that runs back and forth.
%
% MMIX version of progress_bar2.asm.  Usage: progress_bar2 [-n]
% Six runs of 151 frames, each in another color and with a shorter
% pause (100, 80, 60, 40, 20, 10 ms) between the frames.  The pause is
% a busy loop (see lib/console.mms) and -n leaves it out.
%
% Options: lib/std.mms lib/console.mms
        LOC   Data_Segment
        GREG  @
Ruler   BYTE  "Copying: 12345678901234567890123456789012345678901234"
        BYTE  "567890123456789012345678901234567890",#a,0
Copy    BYTE  "Copying: ",0
Done    BYTE  " Done",0

        LOC   #100
Main    SET   $3,$0             % $0 = argc, $1 = argv
        SET   $4,$1
        PUSHJ $2,Con:Init
        LDA   $3,Ruler
        PUSHJ $2,Std:Puts
        PUSHJ $2,Con:HideCursor
        SET   $3,100            % the six runs: pause and color
        SET   $4,96             % bright cyan
        PUSHJ $2,Demo
        SET   $3,80
        SET   $4,95             % bright magenta
        PUSHJ $2,Demo
        SET   $3,60
        SET   $4,93             % bright yellow
        PUSHJ $2,Demo
        SET   $3,40
        SET   $4,91             % bright red
        PUSHJ $2,Demo
        SET   $3,20
        SET   $4,92             % bright green
        PUSHJ $2,Demo
        SET   $3,10
        SET   $4,94             % bright blue
        PUSHJ $2,Demo
        PUSHJ $2,Con:ShowCursor
        SET   $255,0
        TRAP  0,Halt,0

% Demo: $0 = the pause in ms, $1 = the color
Demo    GET   $2,rJ
        SET   $3,0              % p
1H      CMP   $4,$3,150
        BP    $4,2F
        LDA   $6,Copy
        PUSHJ $5,Std:Puts
        SET   $6,40
        SET   $7,$3
        SET   $8,$1
        PUSHJ $5,Con:Bar
        SET   $6,$0
        PUSHJ $5,Con:Sleep
        SET   $6,13             % a carriage return
        PUSHJ $5,Std:PutChar
        ADD   $3,$3,1
        JMP   1B
2H      LDA   $6,Copy
        PUSHJ $5,Std:Puts
        SET   $6,40
        SET   $7,150
        SET   $8,0              % no color
        PUSHJ $5,Con:Bar
        LDA   $6,Done
        PUSHJ $5,Std:Puts
        PUSHJ $5,Std:PutNl
        PUT   rJ,$2
        POP   0,0
