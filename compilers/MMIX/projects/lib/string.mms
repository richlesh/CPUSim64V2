% string.mms - strings and lines.  Listed after the program in Options.
%
%   Str:Len       $0 = a string -> $0 = its length
%   Str:Lower     $0 = a string: its letters become lower case
%   Str:Find      $0 = a string, $1 = another -> $0 = the index of the
%                 first occurrence of the second in the first, or -1
%   Str:GetLine   $0 = a buffer, $1 = its size: reads a line from
%                 standard input, without the newline: $0 = its length,
%                 or -1 at the end of the input
%
% Data: Data_Segment+#10100 .. +#1010F.
        PREFIX Str:
Home    IS    @
        LOC   :Data_Segment+#10100
Base    GREG  @
Arg     OCTA  0,0               % the arguments of the Fgets trap
        LOC   Home

Len     SET   $1,$0
1H      LDBU  $2,$1,0
        BZ    $2,2F
        ADD   $1,$1,1
        JMP   1B
2H      SUB   $0,$1,$0
        POP   1,0

Lower   LDBU  $1,$0,0
        BZ    $1,9F
        SUB   $2,$1,'A'
        CMPU  $3,$2,26
        BNN   $3,1F
        ADD   $1,$1,32
        STBU  $1,$0,0
1H      ADD   $0,$0,1
        JMP   Lower
9H      POP   0,0

Find    SET   $2,0              % the index in the first string
1H      SET   $3,0              % the index in the second
2H      LDBU  $4,$1,$3
        BZ    $4,8F             % all of the second string matched
        ADD   $5,$2,$3
        LDBU  $6,$0,$5
        BZ    $6,9F             % the first string ended
        CMP   $7,$4,$6
        BNZ   $7,3F
        ADD   $3,$3,1
        JMP   2B
3H      ADD   $2,$2,1
        JMP   1B
8H      SET   $0,$2
        POP   1,0
9H      NEG   $0,0,1
        POP   1,0

GetLine LDA   $2,Arg
        STOU  $0,$2,0
        STOU  $1,$2,8
        SET   $255,$2
        TRAP  0,:Fgets,:StdIn
        SET   $3,$255           % the number of characters, or -1
        BNP   $3,9F
        ADDU  $4,$0,$3
        SUB   $4,$4,1
        LDBU  $5,$4,0
        CMP   $5,$5,#a
        BNZ   $5,9F
        SET   $5,0
        STBU  $5,$4,0           % the newline becomes the 0
        SUB   $3,$3,1
9H      SET   $0,$3
        POP   1,0
        PREFIX :
