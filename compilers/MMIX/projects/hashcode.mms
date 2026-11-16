% hashcode.mms - a simple hash of a file.
%
% MMIX version of hashcode.asm.  Usage: hashcode [file]
% hash = hash * 31 + byte for every byte (modulo 2^64), written as 16
% hexadecimal digits and the file name.  Without a file name the
% program reads standard input and writes only the hash.
%
% Options: lib/std.mms lib/file.mms
        LOC   Data_Segment
        GREG  @
Failed  BYTE  "Failed to open: ",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BN    $2,1F
        LDOU  $5,$1,8
        PUSHJ $4,File:OpenIn
        BNN   $4,2F
        LDA   $5,Failed
        PUSHJ $4,Std:Puts
        LDOU  $5,$1,8
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,2
        TRAP  0,Halt,0
1H      PUSHJ $4,File:UseStdIn  % no file: standard input
        PUSHJ $3,Hash
        SET   $5,$3
        SET   $6,16
        PUSHJ $4,Std:PutHex
        PUSHJ $4,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0
2H      PUSHJ $3,Hash           % $3 = the hash
        SET   $5,$3
        SET   $6,16
        PUSHJ $4,Std:PutHex
        SET   $5,' '
        PUSHJ $4,Std:PutChar
        LDOU  $5,$1,8
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        PUSHJ $4,File:CloseIn
        SET   $255,0
        TRAP  0,Halt,0

% Hash: the hash of the input -> $0
Hash    GET   $0,rJ
        SET   $1,0
1H      PUSHJ $2,File:Get
        BN    $2,9F
        MULU  $1,$1,31
        ADDU  $1,$1,$2
        JMP   1B
9H      PUT   rJ,$0
        SET   $0,$1
        POP   1,0
