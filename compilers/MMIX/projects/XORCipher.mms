% XORCipher.mms - XORs a file with a repeating keyword.
%
% MMIX version of XORCipher.asm.
% Usage: XORCipher keyword input_file output_file
% Each byte of the file is XORed with the next character of the keyword
% (repeated as needed), so running the program again decrypts.  As in
% the original, the result is always 0, also after an error.
%
% Options: lib/std.mms lib/string.mms lib/file.mms
        LOC   Data_Segment
        GREG  @
Use     BYTE  "Syntax: XORCipher keyword input_file output_file",#a,0
InFail  BYTE  "Input file open failed!",0
OutFail BYTE  "Output file creation failed!",0

        LOC   #100
Main    CMP   $2,$0,4           % $0 = argc, $1 = argv
        BNN   $2,1F
        LDA   $5,Use
        PUSHJ $4,Std:Puts
        JMP   9F
1H      LDOU  $5,$1,16
        PUSHJ $4,File:OpenIn
        BNN   $4,2F
        LDA   $5,InFail
        PUSHJ $4,Std:Puts
        JMP   9F
2H      LDOU  $5,$1,24
        PUSHJ $4,File:OpenOut
        BNN   $4,3F
        PUSHJ $4,File:CloseIn
        LDA   $5,OutFail
        PUSHJ $4,Std:Puts
        JMP   9F
3H      LDOU  $2,$1,8           % $2 = the keyword
        SET   $4,$2
        PUSHJ $3,Str:Len        % $3 = the length of the key
        SET   $4,0              % $4 = the index in the key
4H      PUSHJ $5,File:Get       % $5 = the next byte
        BN    $5,8F
        BZ    $3,6F             % an empty key: no change
        LDBU  $6,$2,$4
        XOR   $5,$5,$6
        ADD   $4,$4,1
        CMP   $6,$4,$3
        BN    $6,6F
        SET   $4,0
6H      SET   $6,$5
        PUSHJ $5,File:Put
        JMP   4B
8H      PUSHJ $5,File:CloseOut
        PUSHJ $5,File:CloseIn
9H      SET   $255,0
        TRAP  0,Halt,0
