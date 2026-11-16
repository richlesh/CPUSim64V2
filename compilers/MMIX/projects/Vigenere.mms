% Vigenere.mms - the Vigenere cipher on the printable ASCII characters.
%
% MMIX version of Vigenere.asm.
% Usage: Vigenere [-]keyword input_file output_file
% Every character from '!' to '~' (94 of them) moves forward by the
% position of the next key character in that range (back, with a '-'
% before the keyword); the space and other characters are copied and
% do not use up a key character.
%
% Options: lib/std.mms lib/string.mms lib/file.mms
        LOC   Data_Segment
        GREG  @
Use     BYTE  "Syntax: Vigenere [-]keyword input_file output_file",0
InFail  BYTE  "Input file open failed!",0
OutFail BYTE  "Output file creation failed!",0

        LOC   #100
Main    CMP   $2,$0,4           % $0 = argc, $1 = argv
        BNN   $2,1F
        LDA   $5,Use
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      LDOU  $5,$1,16
        PUSHJ $4,File:OpenIn
        BNN   $4,2F
        LDA   $5,InFail
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,2
        TRAP  0,Halt,0
2H      LDOU  $5,$1,24
        PUSHJ $4,File:OpenOut
        BNN   $4,3F
        PUSHJ $4,File:CloseIn
        LDA   $5,OutFail
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,3
        TRAP  0,Halt,0
3H      LDOU  $2,$1,8           % $2 = the keyword
        SET   $3,0              % $3 = 1 to decrypt
        LDBU  $4,$2,0
        CMP   $4,$4,'-'
        BNZ   $4,4F
        SET   $3,1
        ADD   $2,$2,1           % the key starts after the '-'
4H      SET   $5,$2
        PUSHJ $4,Str:Len        % $4 = the length of the key
        BZ    $4,8F             % no key: nothing is done
        SET   $5,0              % $5 = the index in the key
5H      PUSHJ $6,File:Get       % $6 = the next character
        BN    $6,8F
        SUB   $7,$6,'!'
        CMPU  $8,$7,94
        BNN   $8,7F             % any other character is copied
        LDBU  $8,$2,$5          % the offset: the key character
        SUB   $8,$8,'!'
        BZ    $3,6F
        NEG   $8,$8
6H      ADD   $7,$7,$8
        DIV   $9,$7,94
        GET   $7,rR             % modulo 94, 0 ... 93
        ADD   $6,$7,'!'
        ADD   $5,$5,1           % the next key character
        CMP   $8,$5,$4
        BN    $8,7F
        SET   $5,0
7H      SET   $8,$6
        PUSHJ $7,File:Put
        JMP   5B
8H      PUSHJ $6,File:CloseOut
        PUSHJ $6,File:CloseIn
        SET   $255,0
        TRAP  0,Halt,0
