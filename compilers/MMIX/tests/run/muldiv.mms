% muldiv.mms - MUL MULU DIV DIVU on boundary values, with rH, rR, rA
% and rD.  For every pair (y, z) of the table the program writes 12
% octabytes (binary): MUL, rA, MULU, rH, DIV, rR, rA, DIVU, rR, rA,
% and DIVU with rD = 5 and its rR.
        LOC   Data_Segment
        GREG  @
Vals    OCTA  0,1,2,3,10,255,256,#7FFFFFFF,#80000000,#80000001
        OCTA  #FFFFFFFF,#100000000,#100000001,#7FFFFFFFFFFFFFFF
        OCTA  #8000000000000000,#8000000000000001,#FFFFFFFFFFFFFFFF
        OCTA  #FFFFFFFFFFFFFFFE,#FFFFFFFF80000000,#FFFFFFFF7FFFFFFF
        OCTA  #123456789ABCDEF0,#FEDCBA9876543210
NVals   IS    22
Arg     OCTA  Out,12*8
Out     OCTA  0,0,0,0,0,0,0,0,0,0,0,0
Tab     GREG  Vals
ArgR    GREG  Arg
OutR    GREG  Out

        LOC   #100
Main    SET   $0,0                      % index of y
1H      SET   $1,0                      % index of z
2H      SL    $2,$0,3
        LDO   $2,Tab,$2                 % y
        SL    $3,$1,3
        LDO   $3,Tab,$3                 % z
        PUT   rA,0
        PUT   rD,0
        MUL   $4,$2,$3
        GET   $5,rA
        STO   $4,OutR,0
        STO   $5,OutR,8
        PUT   rA,0
        MULU  $4,$2,$3
        GET   $5,rH
        STO   $4,OutR,16
        STO   $5,OutR,24
        PUT   rA,0
        DIV   $4,$2,$3
        GET   $5,rR
        GET   $6,rA
        STO   $4,OutR,32
        STO   $5,OutR,40
        STO   $6,OutR,48
        PUT   rA,0
        DIVU  $4,$2,$3
        GET   $5,rR
        GET   $6,rA
        STO   $4,OutR,56
        STO   $5,OutR,64
        STO   $6,OutR,72
        PUT   rD,5
        DIVU  $4,$2,$3
        GET   $5,rR
        PUT   rD,0
        STO   $4,OutR,80
        STO   $5,OutR,88
        LDA   $255,Arg
        TRAP  0,Fwrite,StdOut
        ADD   $1,$1,1
        CMP   $4,$1,NVals
        BN    $4,2B
        ADD   $0,$0,1
        CMP   $4,$0,NVals
        BN    $4,1B
        SET   $255,0
        TRAP  0,Halt,0
