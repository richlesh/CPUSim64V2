% Spinner.mms - a spinning bar in six colors.
%
% MMIX version of Spinner.asm.  Usage: Spinner [-n]
% Writes "Working: " and one of | / - \ for 48 frames, 8 frames in each
% color, with a pause of 300 ms between the frames (a busy loop, see
% lib/console.mms; -n leaves it out), then "Working: Done!".
%
% Options: lib/std.mms lib/console.mms
        LOC   Data_Segment
        GREG  @
Spin    BYTE  "|/-\"
Colors  BYTE  96,95,93,91,92,94         % the six colors
Work    BYTE  "Working: ",0
DoneS   BYTE  "Working: Done!        ",0

        LOC   #100
Main    SET   $3,$0             % $0 = argc, $1 = argv
        SET   $4,$1
        PUSHJ $2,Con:Init
        PUSHJ $2,Con:HideCursor
        SET   $2,0              % the frame i
1H      CMP   $3,$2,48
        BNN   $3,9F
        AND   $3,$2,3
        LDA   $4,Spin
        LDBU  $4,$4,$3          % the character
        SRU   $5,$2,3
        DIV   $5,$5,6
        GET   $5,rR             % the color: i / 8 mod 6
        LDA   $6,Colors
        LDBU  $7,$6,$5
        SET   $9,$7
        PUSHJ $8,Con:Sgr
        LDA   $9,Work
        PUSHJ $8,Std:Puts
        SET   $9,$4
        PUSHJ $8,Std:PutChar
        SET   $9,300
        PUSHJ $8,Con:Sleep
        SET   $9,13             % a carriage return
        PUSHJ $8,Std:PutChar
        ADD   $2,$2,1
        JMP   1B
9H      SET   $9,0              % reset the color, show the cursor
        PUSHJ $8,Con:Sgr
        PUSHJ $8,Con:ShowCursor
        LDA   $9,DoneS
        PUSHJ $8,Std:Puts
        PUSHJ $8,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0
