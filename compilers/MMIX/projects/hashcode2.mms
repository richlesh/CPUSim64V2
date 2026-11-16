% hashcode2.mms - the same hash as hashcode, in groups of 8 bytes.
%
% MMIX version of hashcode2.asm.  Usage: hashcode2 [file]
% The hash is the one of hashcode.  The input is taken in groups of 8
% bytes; the bytes left over at the end (fewer than 8) are the tail,
% and each of them is written as "tail byte: xx" before the hash.
%
% Options: lib/std.mms lib/file.mms
Group   GREG  Data_Segment+#400        % the 8 bytes of a group
        LOC   Data_Segment
        GREG  @
Failed  BYTE  "Failed to open: ",0
Tail    BYTE  "tail byte: ",0

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
        SET   $1,0              % the hash
        SET   $2,0              % the number of bytes in the group
1H      PUSHJ $3,File:Get
        BN    $3,5F
        STBU  $3,Group,$2
        ADD   $2,$2,1
        CMP   $4,$2,8
        BN    $4,1B
        SET   $2,0              % a full group: 8 bytes into the hash
2H      LDBU  $3,Group,$2
        MULU  $1,$1,31
        ADDU  $1,$1,$3
        ADD   $2,$2,1
        CMP   $4,$2,8
        BN    $4,2B
        SET   $2,0
        JMP   1B
5H      SET   $4,0              % the end: $2 bytes in the tail
6H      CMP   $3,$4,$2
        BNN   $3,9F
        LDA   $6,Tail
        PUSHJ $5,Std:Puts
        LDBU  $6,Group,$4
        SET   $7,2
        PUSHJ $5,Std:PutHex
        PUSHJ $5,Std:PutNl
        LDBU  $3,Group,$4
        MULU  $1,$1,31
        ADDU  $1,$1,$3
        ADD   $4,$4,1
        JMP   6B
9H      PUT   rJ,$0
        SET   $0,$1
        POP   1,0
