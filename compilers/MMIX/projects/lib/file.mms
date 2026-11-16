% file.mms - byte files.  Listed after the program in Options.
%
% One input and one output, each a file or the console, handled a byte
% at a time.  The files are opened in binary mode, so every byte is
% passed unchanged.
%
%   File:OpenIn    $0 = a file name -> $0 = 0, or -1 if not opened
%   File:OpenOut   $0 = a file name -> $0 = 0, or -1 if not made
%   File:UseStdIn  the input is standard input
%   File:UseStdOut the output is standard output
%   File:Get       -> $0 = the next byte of the input, or -1 at its end
%   File:Put       $0 = a byte to write to the output
%   File:CloseIn, File:CloseOut   (do nothing for the console)
%
% The files use the handles 3 (input) and 4 (output).
% Data: Data_Segment+#10200 .. +#1021F.
        PREFIX File:
Home    IS    @
        LOC   :Data_Segment+#10200
Base    GREG  @
Arg     OCTA  0,0               % the arguments of the traps
Chr     OCTA  0                 % the byte that is read or written
        LOC   Home
InStd   GREG  1                 % 1 if the input is the console
OutStd  GREG  1                 % 1 if the output is the console

OpenIn  LDA   $2,Arg
        STOU  $0,$2,0
        SET   $3,:BinaryRead
        STOU  $3,$2,8
        SET   $255,$2
        TRAP  0,:Fopen,3
        SET   InStd,0
        SET   $0,$255
        POP   1,0

OpenOut LDA   $2,Arg
        STOU  $0,$2,0
        SET   $3,:BinaryWrite
        STOU  $3,$2,8
        SET   $255,$2
        TRAP  0,:Fopen,4
        SET   OutStd,0
        SET   $0,$255
        POP   1,0

UseStdIn SET  InStd,1
        POP   0,0

UseStdOut SET OutStd,1
        POP   0,0

Get     LDA   $1,Arg
        LDA   $2,Chr
        STOU  $2,$1,0
        SET   $3,1
        STOU  $3,$1,8
        SET   $255,$1
        BNZ   InStd,1F
        TRAP  0,:Fread,3
        JMP   2F
1H      TRAP  0,:Fread,:StdIn
2H      BN    $255,9F           % the byte was not read
        LDBU  $0,$2,0
        POP   1,0
9H      NEG   $0,0,1
        POP   1,0

Put     LDA   $2,Chr
        STBU  $0,$2,0
        LDA   $1,Arg
        STOU  $2,$1,0
        SET   $3,1
        STOU  $3,$1,8
        SET   $255,$1
        BNZ   OutStd,1F
        TRAP  0,:Fwrite,4
        POP   0,0
1H      TRAP  0,:Fwrite,:StdOut
        POP   0,0

CloseIn BNZ   InStd,1F
        TRAP  0,:Fclose,3
1H      POP   0,0

CloseOut BNZ  OutStd,1F
        TRAP  0,:Fclose,4
1H      POP   0,0
        PREFIX :
