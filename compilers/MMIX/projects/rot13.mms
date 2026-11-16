% rot13.mms - the ROT13 cipher.
%
% MMIX version of rot13.asm.  Usage: rot13 [input_file [output_file]]
% Rotates every letter 13 places in the alphabet; other characters are
% copied.  Without file names it reads standard input and writes
% standard output.  Applied twice, it gives back the text.
%
% Options: lib/std.mms lib/file.mms
        LOC   Data_Segment
        GREG  @
Cant    BYTE  "Can't open ",0
ForRd   BYTE  " for reading.",0
ForWr   BYTE  " for writing.",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BN    $2,1F
        LDOU  $5,$1,8
        PUSHJ $4,File:OpenIn
        BNN   $4,2F
        LDA   $5,Cant           % Can't open <name> for reading.
        PUSHJ $4,Std:Puts
        LDOU  $5,$1,8
        PUSHJ $4,Std:Puts
        LDA   $5,ForRd
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      PUSHJ $4,File:UseStdIn
2H      CMP   $2,$0,3
        BN    $2,3F
        LDOU  $5,$1,16
        PUSHJ $4,File:OpenOut
        BNN   $4,4F
        LDA   $5,Cant           % Can't open <name> for writing.
        PUSHJ $4,Std:Puts
        LDOU  $5,$1,16
        PUSHJ $4,Std:Puts
        LDA   $5,ForWr
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
3H      PUSHJ $4,File:UseStdOut
4H      PUSHJ $4,File:Get       % the cipher
        BN    $4,5F
        SET   $6,$4
        PUSHJ $5,Rot
        PUSHJ $4,File:Put
        JMP   4B
5H      PUSHJ $4,File:CloseOut
        PUSHJ $4,File:CloseIn
        SET   $255,0
        TRAP  0,Halt,0

% Rot: $0 = a character -> $0 = it, rotated if it is a letter
Rot     SET   $3,'A'
        SUB   $1,$0,$3
        CMPU  $2,$1,26
        BN    $2,1F             % an upper case letter
        SET   $3,'a'
        SUB   $1,$0,$3
        CMPU  $2,$1,26
        BN    $2,1F             % a lower case letter
        POP   1,0
1H      ADD   $1,$1,13
        CMP   $2,$1,26
        BN    $2,2F
        SUB   $1,$1,26
2H      ADD   $0,$1,$3
        POP   1,0
