% Caesar.mms - the Caesar cipher on the printable ASCII characters.
%
% MMIX version of Caesar.asm.  Usage: Caesar key input_file output_file
% Every character from ' ' to '~' (95 of them) moves key places
% forward in that range, wrapping around; other characters are copied.
% key in 1 ... 94 encrypts, -94 ... -1 decrypts, 0 copies.
%
% Options: lib/std.mms lib/args.mms lib/file.mms
        LOC   Data_Segment
        GREG  @
Use1    BYTE  "Syntax: Caesar <key> <input_file> <output_file>",0
Use2    BYTE  "    key can be in the range [1,94] to encrypt.",0
Use3    BYTE  "    key can be in the range [-94,-1] to decrypt.",0
        GREG  @
Use4    BYTE  "    key == 0 is a no-op",0
InFail  BYTE  "Input file open failed!",0
OutFail BYTE  "Output file creation failed!",0

        LOC   #100
Main    CMP   $2,$0,4           % $0 = argc, $1 = argv
        BNN   $2,1F
        LDA   $5,Use1
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        LDA   $5,Use2
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        LDA   $5,Use3
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        LDA   $5,Use4
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      LDOU  $5,$1,8           % the key, modulo 95
        PUSHJ $4,Args:ParseInt
        DIV   $4,$4,95
        GET   $3,rR             % $3 = key mod 95, 0 ... 94
        LDOU  $5,$1,16
        PUSHJ $4,File:OpenIn
        BNN   $4,2F
        LDA   $5,InFail
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,3
        TRAP  0,Halt,0
2H      LDOU  $5,$1,24
        PUSHJ $4,File:OpenOut
        BNN   $4,3F
        PUSHJ $4,File:CloseIn
        LDA   $5,OutFail
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,2
        TRAP  0,Halt,0
3H      PUSHJ $4,File:Get       % the cipher
        BN    $4,5F
        SUB   $5,$4,' '         % ' ' ... '~' becomes 0 ... 94
        CMPU  $6,$5,95
        BNN   $6,4F             % any other character is copied
        ADD   $5,$5,$3
        CMP   $6,$5,95
        BN    $6,6F
        SUB   $5,$5,95
6H      ADD   $4,$5,' '
4H      SET   $5,$4
        PUSHJ $4,File:Put
        JMP   3B
5H      PUSHJ $4,File:CloseOut
        PUSHJ $4,File:CloseIn
        SET   $255,0
        TRAP  0,Halt,0
