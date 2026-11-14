% GREG allocation, base addresses, alignment, LDA/LDO/STO with symbolic addresses
        LOC   Data_Segment
base    GREG  @
buf     BYTE  1,2,3
w       WYDE  7
t       TETRA 8
o       OCTA  9
        GREG  @                 % same value as base: reused
str     BYTE  "hello",0
far     GREG  @+1000
zero    GREG
        LOC   #100
Main    LDA   $1,buf
        LDO   $2,o
        LDB   $3,buf+2
        STO   $2,t
        LDO   $4,base,8
        SET   $5,$4
        SET   $6,#1234
        GETA  $7,Main
        GET   $8,rJ
        PUT   rA,0
        TRAP  0,Fputs,StdOut
        TRAP  0,Halt,0
