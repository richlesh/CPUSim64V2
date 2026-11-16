% progress_bar.mms - a progress bar with a percentage, in six colors.
%
% MMIX version of progress_bar.asm.  Usage: progress_bar [-n]
% Runs the bar six times from 0 to 100 percent with a step of 1, 2, ...
% 6 and a different color each time, drawing every frame over the last
% with a carriage return.  MMIX has no clock: the pause of 100 ms
% between frames is a busy loop (see lib/console.mms), and -n leaves
% it out.
%
% Options: lib/std.mms lib/console.mms
Hundred GREG  #4059000000000000         % 100.0
One     GREG  #3FF0000000000000         % 1.0
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
        SET   $3,1              % the six runs: step and color
        SET   $4,96             % bright cyan
        PUSHJ $2,Demo
        SET   $3,2
        SET   $4,95             % bright magenta
        PUSHJ $2,Demo
        SET   $3,3
        SET   $4,93             % bright yellow
        PUSHJ $2,Demo
        SET   $3,4
        SET   $4,91             % bright red
        PUSHJ $2,Demo
        SET   $3,5
        SET   $4,92             % bright green
        PUSHJ $2,Demo
        SET   $3,6
        SET   $4,94             % bright blue
        PUSHJ $2,Demo
        PUSHJ $2,Con:ShowCursor
        SET   $255,0
        TRAP  0,Halt,0

% Demo: $0 = the step, $1 = the color
Demo    GET   $2,rJ
        SET   $3,0              % p
1H      CMP   $4,$3,100
        BP    $4,2F
        LDA   $6,Copy
        PUSHJ $5,Std:Puts
        FLOT  $7,$3
        FDIV  $7,$7,Hundred     % p / 100
        SET   $6,74
        SET   $8,$1
        PUSHJ $5,Con:BarPercent
        SET   $6,100
        PUSHJ $5,Con:Sleep
        SET   $6,13             % a carriage return
        PUSHJ $5,Std:PutChar
        ADD   $3,$3,$0
        JMP   1B
2H      LDA   $6,Copy
        PUSHJ $5,Std:Puts
        SET   $6,74
        SET   $7,One
        SET   $8,0              % reset
        PUSHJ $5,Con:BarPercent
        LDA   $6,Done
        PUSHJ $5,Std:Puts
        PUSHJ $5,Std:PutNl
        PUT   rJ,$2
        POP   0,0
