% rainbow.mms - the ANSI colors of the terminal.
%
% MMIX version of rainbow.asm.  Usage: rainbow
% Sixteen bars in the background colors (each line is filled by "clear
% to the end of the line"), and the text in the foreground colors:
% regular, bright, bold and dim.  The escapes come from Con:Sgr.
%
% Options: lib/std.mms lib/console.mms
        LOC   Data_Segment
        GREG  @
Clear   BYTE  #1b,"[0K",0
Lorem   BYTE  "Lorem ipsum dolor sit amet, consectetur adipiscing elit"
        BYTE  0
        GREG  @
Reg     BYTE  #a,"Regular",0
Bri     BYTE  #a,"Bright",0
RegB    BYTE  #a,"Regular Bold",0
BriB    BYTE  #a,"Bright Bold",0
RegD    BYTE  #a,"Regular Dim",0
BriD    BYTE  #a,"Bright Dim",0

        LOC   #100
Main    SET   $3,40             % the bars: backgrounds 40 ... 47
        SET   $4,8
        PUSHJ $2,Bars
        SET   $3,100            % and the bright ones, 100 ... 107
        SET   $4,8
        PUSHJ $2,Bars
        LDA   $3,Reg            % the texts: a title, 8 colors
        SET   $4,30
        SET   $5,0
        PUSHJ $2,Texts
        LDA   $3,Bri
        SET   $4,90
        SET   $5,0
        PUSHJ $2,Texts
        LDA   $3,RegB
        SET   $4,30
        SET   $5,1              % bold
        PUSHJ $2,Texts
        LDA   $3,BriB
        SET   $4,90
        SET   $5,1
        PUSHJ $2,Texts
        LDA   $3,RegD
        SET   $4,30
        SET   $5,2              % dim
        PUSHJ $2,Texts
        LDA   $3,BriD
        SET   $4,90
        SET   $5,2
        PUSHJ $2,Texts
        SET   $255,0
        TRAP  0,Halt,0

% Bars: $0 = the first color, $1 = how many: a line in each
Bars    GET   $2,rJ
1H      BZ    $1,9F
        SET   $4,$0
        PUSHJ $3,Con:Sgr        % the background color
        LDA   $4,Clear          % fills the line with it
        PUSHJ $3,Std:Puts
        SET   $4,0              % back to normal
        PUSHJ $3,Con:Sgr
        PUSHJ $3,Std:PutNl
        ADD   $0,$0,1
        SUB   $1,$1,1
        JMP   1B
9H      PUT   rJ,$2
        POP   0,0

% Texts: $0 = a title, $1 = the first color, $2 = a style (0: none):
% the title and then 8 lines of text in the 8 colors
Texts   GET   $3,rJ
        SET   $8,$0
        PUSHJ $7,Std:Puts
        PUSHJ $7,Std:PutNl
        SET   $4,0              % the color
1H      CMP   $5,$4,8
        BNN   $5,9F
        ADD   $8,$1,$4
        PUSHJ $7,Con:Sgr
        BZ    $2,2F
        SET   $8,$2
        PUSHJ $7,Con:Sgr
2H      LDA   $8,Lorem
        PUSHJ $7,Std:Puts
        SET   $8,0              % back to normal
        PUSHJ $7,Con:Sgr
        PUSHJ $7,Std:PutNl
        ADD   $4,$4,1
        JMP   1B
9H      PUT   rJ,$3
        POP   0,0
