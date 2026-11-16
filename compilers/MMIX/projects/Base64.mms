% Base64.mms - Base64 encoding and decoding of a file.
%
% MMIX version of Base64.asm.  Usage: Base64 e|d input output
% e encodes the bytes of the input as the characters A-Z a-z 0-9 + /,
% four for every three bytes, with = as padding and a new line after
% every 57 bytes (76 characters).  d decodes them: other characters
% (such as the new lines) are ignored, and a = ends the data.
%
% Options: lib/std.mms lib/file.mms
        LOC   Data_Segment
Alpha   BYTE  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
        BYTE  "0123456789+/",0
AlphaR  GREG  Alpha             % the address of the alphabet
        GREG  @
Use     BYTE  "Syntax: Base64 <e|d> <input_file> <output_file>",0
BadMode BYTE  "Mode can only be E or D!",0
        GREG  @
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
1H      LDOU  $2,$1,8           % the mode: the first character,
        LDBU  $3,$2,0           % in lower case
        BNZ   $3,2F
        SET   $3,' '
2H      SUB   $4,$3,'A'
        CMPU  $5,$4,26
        BNN   $5,3F
        ADD   $3,$3,32
3H      CMP   $4,$3,'e'
        BZ    $4,4F
        CMP   $4,$3,'d'
        BZ    $4,4F
        LDA   $5,BadMode
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
4H      LDOU  $5,$1,16
        PUSHJ $4,File:OpenIn
        BNN   $4,5F
        LDA   $5,InFail
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,2
        TRAP  0,Halt,0
5H      LDOU  $5,$1,24
        PUSHJ $4,File:OpenOut
        BNN   $4,6F
        PUSHJ $4,File:CloseIn
        LDA   $5,OutFail
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,3
        TRAP  0,Halt,0
6H      CMP   $4,$3,'e'
        BNZ   $4,7F
        PUSHJ $4,Encode
        JMP   8F
7H      PUSHJ $4,Decode
8H      PUSHJ $4,File:CloseOut
        PUSHJ $4,File:CloseIn
        SET   $255,0
        TRAP  0,Halt,0

% PutA: $0 = a value 0 ... 63: writes its character of the alphabet
PutA    GET   $1,rJ
        LDBU  $5,AlphaR,$0
        PUSHJ $4,File:Put
        PUT   rJ,$1
        POP   0,0

% Encode: the input to the output
Encode  GET   $0,rJ
        SET   $1,0              % the number of bytes read
        SET   $2,0              % the bits left over from the last byte
1H      PUSHJ $3,File:Get       % $3 = the next byte
        BN    $3,8F
        DIV   $4,$1,3
        GET   $4,rR             % 0, 1 or 2: the place in a group of 3
        BNZ   $4,2F
        SRU   $10,$3,2
        PUSHJ $9,PutA
        AND   $2,$3,3
        JMP   5F
2H      CMP   $5,$4,1
        BNZ   $5,3F
        SL    $10,$2,4
        SRU   $5,$3,4
        OR    $10,$10,$5
        PUSHJ $9,PutA
        AND   $2,$3,15
        JMP   5F
3H      SL    $10,$2,2
        SRU   $5,$3,6
        OR    $10,$10,$5
        PUSHJ $9,PutA
        AND   $10,$3,63
        PUSHJ $9,PutA
5H      ADD   $1,$1,1
        DIV   $4,$1,57
        GET   $4,rR
        BNZ   $4,1B
        SET   $10,#a            % a new line after 57 bytes
        PUSHJ $9,File:Put
        JMP   1B
8H      DIV   $4,$1,3           % the end: padding
        GET   $4,rR
        CMP   $5,$4,1
        BNZ   $5,6F
        SL    $10,$2,4
        PUSHJ $9,PutA
        SET   $10,'='
        PUSHJ $9,File:Put
        SET   $10,'='
        PUSHJ $9,File:Put
        JMP   7F
6H      CMP   $5,$4,2
        BNZ   $5,7F
        SL    $10,$2,2
        PUSHJ $9,PutA
        SET   $10,'='
        PUSHJ $9,File:Put
7H      DIV   $4,$1,57
        GET   $4,rR
        BZ    $4,9F
        SET   $10,#a            % a new line ends a short last line
        PUSHJ $9,File:Put
9H      PUT   rJ,$0
        POP   0,0

% Val: $0 = a character -> its value 0 ... 63; 64 for =; -1 else
Val     SUB   $1,$0,'A'
        CMPU  $2,$1,26
        BNN   $2,1F
        SET   $0,$1
        POP   1,0
1H      SUB   $1,$0,'a'
        CMPU  $2,$1,26
        BNN   $2,2F
        ADD   $0,$1,26
        POP   1,0
2H      SUB   $1,$0,'0'
        CMPU  $2,$1,10
        BNN   $2,3F
        ADD   $0,$1,52
        POP   1,0
3H      CMP   $2,$0,'+'
        BNZ   $2,4F
        SET   $0,62
        POP   1,0
4H      CMP   $2,$0,'/'
        BNZ   $2,5F
        SET   $0,63
        POP   1,0
5H      CMP   $2,$0,'='
        BNZ   $2,6F
        SET   $0,64
        POP   1,0
6H      NEG   $0,0,1
        POP   1,0

% Decode: the input to the output
Decode  GET   $0,rJ
        SET   $1,0              % the number of characters used
        SET   $2,0              % the bits left over
1H      PUSHJ $3,File:Get       % $3 = the next character
        BN    $3,9F
        SET   $5,$3
        PUSHJ $4,Val            % $4 = its value
        BN    $4,1B             % not a Base64 character: ignored
        CMP   $5,$4,64
        BZ    $5,9F             % = ends the data
        DIV   $5,$1,4
        GET   $5,rR             % 0 ... 3: the place in a group of 4
        BNZ   $5,2F
        SET   $2,$4
        JMP   7F
2H      SL    $2,$2,6
        OR    $2,$2,$4
        CMP   $6,$5,1
        BNZ   $6,3F
        SRU   $9,$2,4           % 8 bits are complete
        AND   $2,$2,15
        JMP   6F
3H      CMP   $6,$5,2
        BNZ   $6,4F
        SRU   $9,$2,2
        AND   $2,$2,3
        JMP   6F
4H      SET   $9,$2
6H      PUSHJ $8,File:Put
7H      ADD   $1,$1,1
        JMP   1B
9H      PUT   rJ,$0
        POP   0,0
