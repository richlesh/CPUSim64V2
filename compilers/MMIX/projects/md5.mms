% md5.mms - the MD5 hash of a file.
%
% MMIX version of md5.asm.  Usage: md5 [file]
% Writes an empty line and then the 128-bit MD5 hash as 32 hexadecimal
% digits (capitals), and the file name.  Without a file name it reads
% standard input and writes no name.
%
% Options: lib/std.mms lib/file.mms
        LOC   Data_Segment
K       OCTA  #D76AA478,#E8C7B756,#242070DB,#C1BDCEEE
        OCTA  #F57C0FAF,#4787C62A,#A8304613,#FD469501
        OCTA  #698098D8,#8B44F7AF,#FFFF5BB1,#895CD7BE
        OCTA  #6B901122,#FD987193,#A679438E,#49B40821
        OCTA  #F61E2562,#C040B340,#265E5A51,#E9B6C7AA
        OCTA  #D62F105D,#02441453,#D8A1E681,#E7D3FBC8
        OCTA  #21E1CDE6,#C33707D6,#F4D50D87,#455A14ED
        OCTA  #A9E3E905,#FCEFA3F8,#676F02D9,#8D2A4C8A
        OCTA  #FFFA3942,#8771F681,#6D9D6122,#FDE5380C
        OCTA  #A4BEEA44,#4BDECFA9,#F6BB4B60,#BEBFBC70
        OCTA  #289B7EC6,#EAA127FA,#D4EF3085,#04881D05
        OCTA  #D9D4D039,#E6DB99E5,#1FA27CF8,#C4AC5665
        OCTA  #F4292244,#432AFF97,#AB9423A7,#FC93A039
        OCTA  #655B59C3,#8F0CCC92,#FFEFF47D,#85845DD1
        OCTA  #6FA87E4F,#FE2CE6E0,#A3014314,#4E0811A1
        OCTA  #F7537E82,#BD3AF235,#2AD7D2BB,#EB86D391
S       BYTE  7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22
        BYTE  5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20
        BYTE  4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23
        BYTE  6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21
Blk     OCTA  0,0,0,0,0,0,0,0           % the block of 64 bytes
Mt      OCTA  0,0,0,0,0,0,0,0           % its 16 words (little-endian)
        OCTA  0,0,0,0,0,0,0,0
Sum     OCTA  0,0,0,0                   % the state A, B, C, D
Kt      GREG  K
St      GREG  S
BlkR    GREG  Blk
MtR     GREG  Mt
SumR    GREG  Sum
Mask    GREG  #FFFFFFFF
        GREG  @
Failed  BYTE  "File open failed!",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BN    $2,1F
        LDOU  $5,$1,8
        PUSHJ $4,File:OpenIn
        BNN   $4,2F
        LDA   $5,Failed
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      PUSHJ $4,File:UseStdIn  % no file: standard input
        PUSHJ $4,Md5
        PUSHJ $4,Std:PutNl
        PUSHJ $4,PrintSum
        PUSHJ $4,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0
2H      PUSHJ $4,Md5
        PUSHJ $4,Std:PutNl
        PUSHJ $4,PrintSum
        SET   $5,' '
        PUSHJ $4,Std:PutChar
        LDOU  $5,$1,8
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        PUSHJ $4,File:CloseIn
        SET   $255,0
        TRAP  0,Halt,0

% Md5: reads the input and leaves the hash in Sum
Md5     GET   $0,rJ
        SETL  $4,#2301          % the start values
        INCML $4,#6745
        STO   $4,SumR,0
        SETL  $4,#AB89
        INCML $4,#EFCD
        STO   $4,SumR,8
        SETL  $4,#DCFE
        INCML $4,#98BA
        STO   $4,SumR,16
        SETL  $4,#5476
        INCML $4,#1032
        STO   $4,SumR,24
        SET   $1,0              % the number of bytes read
        SET   $2,0              % the number of bytes in the block
1H      PUSHJ $3,File:Get
        BN    $3,5F
        STBU  $3,BlkR,$2
        ADD   $1,$1,1
        ADD   $2,$2,1
        CMP   $4,$2,64
        BN    $4,1B
        PUSHJ $3,Proc           % a full block
        SET   $2,0
        JMP   1B
5H      SET   $3,#80            % the end: a 1 bit, zeros, the length
        STBU  $3,BlkR,$2
        ADD   $2,$2,1
        SET   $5,0
        CMP   $4,$2,56
        BNP   $4,7F             % the length fits into this block
6H      CMP   $4,$2,64          % no: fill this block with zeros
        BNN   $4,8F
        STBU  $5,BlkR,$2
        ADD   $2,$2,1
        JMP   6B
8H      PUSHJ $3,Proc
        SET   $5,0
        SET   $2,0
7H      CMP   $4,$2,56          % zeros up to the length
        BNN   $4,9F
        STBU  $5,BlkR,$2
        ADD   $2,$2,1
        JMP   7B
9H      SL    $3,$1,3           % the length in bits, little-endian
        SET   $2,56
4H      STBU  $3,BlkR,$2
        SRU   $3,$3,8
        ADD   $2,$2,1
        CMP   $4,$2,64
        BN    $4,4B
        PUSHJ $3,Proc
        PUT   rJ,$0
        POP   0,0

% Proc: makes the 16 words of the block and runs Block on them
Proc    GET   $0,rJ
        SET   $1,0
1H      SL    $2,$1,2           % the words are little-endian
        LDBU  $3,BlkR,$2
        ADD   $2,$2,1
        LDBU  $4,BlkR,$2
        SL    $4,$4,8
        OR    $3,$3,$4
        ADD   $2,$2,1
        LDBU  $4,BlkR,$2
        SL    $4,$4,16
        OR    $3,$3,$4
        ADD   $2,$2,1
        LDBU  $4,BlkR,$2
        SL    $4,$4,24
        OR    $3,$3,$4
        SL    $2,$1,3
        STO   $3,MtR,$2
        ADD   $1,$1,1
        CMP   $4,$1,16
        BN    $4,1B
        PUSHJ $2,Block
        PUT   rJ,$0
        POP   0,0

% Block: the 64 steps on the words in Mt
Block   LDO   $0,SumR,0         % a
        LDO   $1,SumR,8         % b
        LDO   $2,SumR,16        % c
        LDO   $3,SumR,24        % d
        SET   $4,0              % i
1H      CMP   $5,$4,16
        BNN   $5,2F
        XOR   $5,$2,$3          % F = (b and c) or (not b and d)
        AND   $5,$5,$1
        XOR   $5,$5,$3
        SET   $6,$4             % g = i
        JMP   6F
2H      CMP   $5,$4,32
        BNN   $5,3F
        XOR   $5,$1,$2          % F = (d and b) or (not d and c)
        AND   $5,$5,$3
        XOR   $5,$5,$2
        MUL   $6,$4,5           % g = (5 i + 1) mod 16
        ADD   $6,$6,1
        AND   $6,$6,15
        JMP   6F
3H      CMP   $5,$4,48
        BNN   $5,4F
        XOR   $5,$1,$2          % F = b xor c xor d
        XOR   $5,$5,$3
        MUL   $6,$4,3           % g = (3 i + 5) mod 16
        ADD   $6,$6,5
        AND   $6,$6,15
        JMP   6F
4H      ORN   $5,$1,$3          % F = c xor (b or not d)
        XOR   $5,$5,$2
        MUL   $6,$4,7           % g = 7 i mod 16
        AND   $6,$6,15
6H      ADDU  $5,$5,$0          % F = F + a + K[i] + M[g]
        SL    $7,$4,3
        LDO   $8,Kt,$7
        ADDU  $5,$5,$8
        SL    $7,$6,3
        LDO   $8,MtR,$7
        ADDU  $5,$5,$8
        AND   $5,$5,Mask
        SET   $0,$3             % a = d, d = c, c = b
        SET   $3,$2
        SET   $2,$1
        LDBU  $7,St,$4          % b = b + (F rotated left by s[i])
        SL    $8,$5,$7
        SET   $9,32
        SUB   $9,$9,$7
        SRU   $5,$5,$9
        OR    $5,$5,$8
        AND   $5,$5,Mask
        ADDU  $1,$1,$5
        AND   $1,$1,Mask
        ADD   $4,$4,1
        CMP   $5,$4,64
        BN    $5,1B
        LDO   $5,SumR,0         % add the result to the state
        ADDU  $0,$0,$5
        AND   $0,$0,Mask
        STO   $0,SumR,0
        LDO   $5,SumR,8
        ADDU  $1,$1,$5
        AND   $1,$1,Mask
        STO   $1,SumR,8
        LDO   $5,SumR,16
        ADDU  $2,$2,$5
        AND   $2,$2,Mask
        STO   $2,SumR,16
        LDO   $5,SumR,24
        ADDU  $3,$3,$5
        AND   $3,$3,Mask
        STO   $3,SumR,24
        POP   0,0

% PrintSum: the four words of Sum, each as 4 bytes, low byte first
PrintSum GET  $0,rJ
        SET   $1,0              % the word
1H      SL    $2,$1,3
        LDO   $2,SumR,$2
        SET   $3,0              % the byte
2H      SL    $4,$3,3
        SRU   $6,$2,$4
        AND   $6,$6,#FF
        SET   $7,2
        PUSHJ $5,Std:PutHexU
        ADD   $3,$3,1
        CMP   $4,$3,4
        BN    $4,2B
        ADD   $1,$1,1
        CMP   $4,$1,4
        BN    $4,1B
        PUT   rJ,$0
        POP   0,0
