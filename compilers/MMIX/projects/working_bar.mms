% working_bar.mms - a dot that swings back and forth in a bar.
%
% MMIX version of working_bar.asm.  Usage: working_bar [-n]
% Seven runs of 301 frames in other colors and with shorter pauses (60,
% 50, 40, 30, 20, 10, 5 ms; a busy loop, see lib/console.mms, and -n
% leaves it out).  In frame i the dot is in the cell
% round (39 (1 - (cos (2 pi i / 100) + 1) / 2)) + 1 of 40.  The cosine
% is Math:Cos, which agrees with the C library to the last bit; that is
% needed, because in some frames the dot is on the tie of two cells.
%
% Options: lib/std.mms lib/console.mms lib/math.mms
Pi      GREG  #400921FB54442D18         % pi
Two     GREG  #4000000000000000         % 2.0
One     GREG  #3FF0000000000000         % 1.0
Half    GREG  #3FE0000000000000         % 0.5
Hundred GREG  #4059000000000000         % 100.0
Sign    GREG  #8000000000000000
Dot     GREG  #25CF                     % BLACK CIRCLE
Frames  GREG  300                       % the last i
        LOC   Data_Segment
        GREG  @
Proc    BYTE  "Processing: ",0
Open    BYTE  "[",0
Done    BYTE  " Done",0

        LOC   #100
Main    SET   $3,$0             % $0 = argc, $1 = argv
        SET   $4,$1
        PUSHJ $2,Con:Init
        PUSHJ $2,Con:HideCursor
        SET   $3,60             % the seven runs: pause and color
        SET   $4,96             % bright cyan
        PUSHJ $2,Demo
        SET   $3,50
        SET   $4,95             % bright magenta
        PUSHJ $2,Demo
        SET   $3,40
        SET   $4,93             % bright yellow
        PUSHJ $2,Demo
        SET   $3,30
        SET   $4,91             % bright red
        PUSHJ $2,Demo
        SET   $3,20
        SET   $4,92             % bright green
        PUSHJ $2,Demo
        SET   $3,10
        SET   $4,94             % bright blue
        PUSHJ $2,Demo
        SET   $3,5
        SET   $4,97             % bright white
        PUSHJ $2,Demo
        PUSHJ $2,Con:ShowCursor
        SET   $255,0
        TRAP  0,Halt,0

% Demo: $0 = the pause in ms, $1 = the color
Demo    GET   $2,rJ
        SET   $3,0              % i
1H      CMP   $4,$3,Frames
        BP    $4,2F
        LDA   $6,Proc
        PUSHJ $5,Std:Puts
        FLOT  $7,$3
        FDIV  $7,$7,Hundred     % p = i / 100
        SET   $6,40
        SET   $8,$1
        PUSHJ $5,WorkingBar
        SET   $6,$0
        PUSHJ $5,Con:Sleep
        SET   $6,13             % a carriage return
        PUSHJ $5,Std:PutChar
        ADD   $3,$3,1
        JMP   1B
2H      LDA   $6,Proc           % the last frame: p = 3, no color
        PUSHJ $5,Std:Puts
        SET   $6,300
        FLOT  $7,$6
        FDIV  $7,$7,Hundred
        SET   $6,40
        SET   $8,0
        PUSHJ $5,WorkingBar
        LDA   $6,Done
        PUSHJ $5,Std:Puts
        PUSHJ $5,Std:PutNl
        PUT   rJ,$2
        POP   0,0

% WorkingBar: $0 = width, $1 = p (a double), $2 = color (0: none)
WorkingBar GET $3,rJ
        FMUL  $4,Pi,Two
        FMUL  $4,$4,$1          % 2 pi p
        SET   $11,$4
        PUSHJ $10,Math:Cos
        FADD  $4,$10,One
        FDIV  $4,$4,Two         % (cos + 1) / 2
        FSUB  $4,$4,One
        XOR   $4,$4,Sign        % 1 - (cos + 1) / 2
        SUB   $5,$0,1
        FLOT  $5,$5
        FMUL  $4,$4,$5          % times (width - 1)
        FADD  $4,$4,Half
        FIX   $4,ROUND_OFF,$4   % round
        ADD   $4,$4,1           % $4 = the cell of the dot, 1 ...
        SET   $11,0             % ESC [ 0 m, [
        PUSHJ $10,Con:Sgr
        SET   $11,'['
        PUSHJ $10,Std:PutChar
        SUB   $5,$4,1           % the spaces before the dot
1H      BZ    $5,2F
        SET   $11,' '
        PUSHJ $10,Std:PutChar
        SUB   $5,$5,1
        JMP   1B
2H      BZ    $2,3F
        SET   $11,$2            % the dot in its color
        PUSHJ $10,Con:Sgr
        SET   $11,Dot
        PUSHJ $10,Std:PutChar
        SET   $11,0
        PUSHJ $10,Con:Sgr
        JMP   4F
3H      SET   $11,' '           % no color: a space
        PUSHJ $10,Std:PutChar
4H      SUB   $5,$0,$4          % the spaces after the dot
5H      BZ    $5,6F
        SET   $11,' '
        PUSHJ $10,Std:PutChar
        SUB   $5,$5,1
        JMP   5B
6H      SET   $11,']'
        PUSHJ $10,Std:PutChar
        PUT   rJ,$3
        POP   0,0
