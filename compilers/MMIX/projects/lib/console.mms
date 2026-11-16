% console.mms - ANSI terminal output and delays.  Listed after the
% program in Options, with std.mms.
%
%   Con:Init        $0 = argc, $1 = argv: the argument -n (no delays)
%                   turns the delays of Con:Sleep off
%   Con:Sleep       $0 = milliseconds: a busy loop (MMIX has no clock,
%                   so it is calibrated for the native simulator, about
%                   7000 passes a millisecond), or nothing after -n
%   Con:Sgr         $0 = n: writes the escape sequence ESC [ n m
%   Con:HideCursor, Con:ShowCursor
%   Con:BarPercent  $0 = width, $1 = a double 0 ... 1, $2 = color (an
%                   SGR number): a bar of full and light blocks, and
%                   the percentage when it is below 100
%   Con:Bar         $0 = width, $1 = a count, $2 = color or 0: a bar
%                   with an eye that runs back and forth
%
% Data: Data_Segment+#10400 .. +#1043F.
        PREFIX Con:
Home    IS    @
        LOC   :Data_Segment+#10400
Base    GREG  @
SBuf    OCTA  0,0               % Sgr: the escape sequence
Hide    BYTE  #1b,"[?25l",0
Show    BYTE  #1b,"[?25h",0
        LOC   Home
NoDelay GREG  0                 % 1 after -n
PerMs   GREG  7000              % passes of the loop for 1 millisecond
Half    GREG  #3FE0000000000000
Hundred GREG  #4059000000000000
Block   GREG  #2588             % FULL BLOCK
Shade1  GREG  #2591             % LIGHT SHADE
Shade2  GREG  #2592             % MEDIUM SHADE
Shade3  GREG  #2593             % DARK SHADE

Init    SET   $2,1
1H      CMP   $3,$2,$0
        BNN   $3,9F
        SL    $3,$2,3
        LDOU  $3,$1,$3          % the argument
        LDBU  $4,$3,0
        CMP   $4,$4,'-'
        BNZ   $4,2F
        LDBU  $4,$3,1
        CMP   $4,$4,'n'
        BNZ   $4,2F
        LDBU  $4,$3,2
        BNZ   $4,2F
        SET   NoDelay,1
2H      ADD   $2,$2,1
        JMP   1B
9H      POP   0,0

Sleep   BNZ   NoDelay,9F
        MUL   $0,$0,PerMs
1H      BZ    $0,9F
        SUB   $0,$0,1
        JMP   1B
9H      POP   0,0

Sgr     LDA   $1,SBuf
        SET   $2,27
        STBU  $2,$1,0
        SET   $2,'['
        STBU  $2,$1,1
        SET   $3,2              % the place of the next character
        CMP   $4,$0,100
        BN    $4,1F
        DIV   $4,$0,100         % three digits
        GET   $0,:rR
        ADD   $4,$4,'0'
        STBU  $4,$1,$3
        ADD   $3,$3,1
        JMP   2F
1H      CMP   $4,$0,10
        BN    $4,3F
2H      DIV   $4,$0,10          % two digits
        GET   $0,:rR
        ADD   $4,$4,'0'
        STBU  $4,$1,$3
        ADD   $3,$3,1
3H      ADD   $4,$0,'0'
        STBU  $4,$1,$3
        ADD   $3,$3,1
        SET   $4,'m'
        STBU  $4,$1,$3
        ADD   $3,$3,1
        SET   $4,0
        STBU  $4,$1,$3
        SET   $255,$1
        TRAP  0,:Fputs,:StdOut
        POP   0,0

HideCursor LDA $255,Hide
        TRAP  0,:Fputs,:StdOut
        POP   0,0

ShowCursor LDA $255,Show
        TRAP  0,:Fputs,:StdOut
        POP   0,0

% BarPercent: $0 = width, $1 = the fraction, $2 = the color
BarPercent GET $3,:rJ
        FLOT  $4,$0
        FMUL  $4,$4,$1          % fraction * width
        FADD  $4,$4,Half
        FIX   $4,:ROUND_OFF,$4  % filled = round (...)
        FMUL  $5,$1,Hundred
        FADD  $5,$5,Half
        FIX   $5,:ROUND_OFF,$5  % the percentage
        SET   $11,$2
        PUSHJ $10,Sgr
        SET   $6,0
1H      CMP   $7,$6,$4
        BNN   $7,2F
        SET   $11,Block
        PUSHJ $10,:Std:PutChar
        ADD   $6,$6,1
        JMP   1B
2H      SET   $11,90            % bright black
        PUSHJ $10,Sgr
        ADD   $6,$4,1
3H      CMP   $7,$6,$0
        BP    $7,4F
        SET   $11,Shade1
        PUSHJ $10,:Std:PutChar
        ADD   $6,$6,1
        JMP   3B
4H      SET   $11,0
        PUSHJ $10,Sgr
        CMP   $7,$5,100
        BNN   $7,9F
        SET   $11,' '
        PUSHJ $10,:Std:PutChar
        CMP   $7,$5,10
        BNN   $7,5F
        SET   $11,' '
        PUSHJ $10,:Std:PutChar
        SET   $11,' '
        PUSHJ $10,:Std:PutChar
        JMP   6F
5H      SET   $11,' '
        PUSHJ $10,:Std:PutChar
6H      SET   $11,$5
        PUSHJ $10,:Std:PutDec
        SET   $11,'%'
        PUSHJ $10,:Std:PutChar
9H      PUT   :rJ,$3
        POP   0,0

% Bar: $0 = width, $1 = count, $2 = color (0: no color)
Bar     GET   $3,:rJ
        SUB   $4,$0,2           % span = width - 3 + 1
        SUB   $5,$4,1
        MUL   $5,$5,2           % period = 2 (span - 1)
        DIV   $6,$1,$5
        GET   $6,:rR            % count mod period
        CMP   $7,$6,$4
        BN    $7,1F
        SUB   $7,$5,$6          % the left edge of the eye: a triangle
        JMP   2F
1H      SET   $7,$6
2H      SUB   $8,$4,1
        CMP   $8,$6,$8
        SET   $9,1              % the direction: 1 going right
        BN    $8,3F
        NEG   $9,0,1
3H      SET   $4,0              % the column
4H      CMP   $5,$4,$0
        BNN   $5,9F
        BNZ   $2,5F
        SET   $11,90            % no color: light shades
        PUSHJ $10,Sgr
        SET   $11,Shade1
        PUSHJ $10,:Std:PutChar
        JMP   8F
5H      ADD   $5,$7,2           % the eye: the columns pos ... pos+2
        CMP   $6,$4,$7
        BN    $6,6F
        CMP   $6,$4,$5
        BP    $6,6F
        SET   $11,$2
        PUSHJ $10,Sgr
        SET   $11,Block
        PUSHJ $10,:Std:PutChar
        JMP   8F
6H      BN    $9,7F
        SUB   $6,$7,$4          % going right: t = pos - 1 - column
        SUB   $6,$6,1
        JMP   0F
7H      SUB   $6,$4,$5          % going left: t = column - eyeEnd - 1
        SUB   $6,$6,1
0H      SET   $11,90            % the trail: dark, medium, light
        SET   $8,Shade1
        BN    $6,1F
        SET   $11,$2
        BZ    $6,2F
        CMP   $5,$6,1
        BZ    $5,3F
        CMP   $5,$6,2
        BZ    $5,1F
        SET   $11,90
        JMP   1F
2H      SET   $8,Shade3
        JMP   1F
3H      SET   $8,Shade2
1H      PUSHJ $10,Sgr
        SET   $11,$8
        PUSHJ $10,:Std:PutChar
8H      ADD   $4,$4,1
        JMP   4B
9H      SET   $11,0
        PUSHJ $10,Sgr
        PUT   :rJ,$3
        POP   0,0
        PREFIX :
