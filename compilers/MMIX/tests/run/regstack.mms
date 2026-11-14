% register stack tests
         LOC   Data_Segment
         GREG  @
HexDig   BYTE  "0123456789abcdef"
HexBuf   BYTE  "................",#a,0
g1       GREG  0
g2       GREG  #1234
v        IS    $60
         LOC   #100
% Pr prints $0 (pass v: SET $61,value; PUSHJ $60,Pr)
Main     SET   $5,77
         GET   $61,rL
         PUSHJ v,Pr            rL before = 6
         SET   $61,$30         marginal register: 0
         PUSHJ v,Pr
         GET   $61,rL
         PUSHJ v,Pr            62
         SET   $4,5
         PUSHJ $3,Ret2         $3,$4 = 2 results; rL = 5
         GET   $61,rL
         PUSHJ v,Pr
         SET   $61,$3
         PUSHJ v,Pr
         SET   $61,$4
         PUSHJ v,Pr
         PUT   rL,10
         PUSHJ $9,RetBig       POP 5 with callee rL=1
         GET   $61,rL
         PUSHJ v,Pr
         SET   $61,$9
         PUSHJ v,Pr
         SET   $61,$10
         PUSHJ v,Pr
         SET   $61,$13
         PUSHJ v,Pr
         PUSHJ g1,RetOne       global X: pushes all locals
         SET   $61,g1
         PUSHJ v,Pr
         GET   $61,rL
         PUSHJ v,Pr
         SET   $61,$3          locals kept
         PUSHJ v,Pr
         SET   $21,3000        recursion depth 3000
         PUSHJ $20,Sum
         SET   $61,$20
         PUSHJ v,Pr
         GETA  $2,RetOne       PUSHGO
         PUSHGO $1,$2,0
         SET   $61,$1
         PUSHJ v,Pr
         GET   $61,rO
         PUSHJ v,Pr
         PUSHJ $3,GetO
         SET   $61,$3
         PUSHJ v,Pr
         SET   $7,99           PUT rL shrinks and clears
         PUT   rL,3
         GET   $61,rL
         PUSHJ v,Pr
         SET   $61,$7
         PUSHJ v,Pr
         PUT   rL,100          PUT rL cannot grow
         GET   $61,rL
         PUSHJ v,Pr
         SET   $1,4
         PUSHJ $0,Deep
         GET   $61,rL
         PUSHJ v,Pr
         SET   $61,g2
         PUSHJ v,Pr
         SET   $61,$0
         PUSHJ v,Pr
         SET   $255,3
         TRAP  0,Halt,0

Ret2     ADD   $1,$0,10
         SET   $2,33
         POP   2,0
RetBig   SET   $0,1
         POP   5,0
RetOne   SET   $0,#abc
         POP   1,0
Sum      BZ    $0,1F
         GET   $1,rJ
         SUB   $3,$0,1
         PUSHJ $2,Sum
         ADD   $0,$0,$2
         PUT   rJ,$1
1H       POP   1,0
GetO     GET   $0,rO
         POP   1,0
Deep     BZ    $0,1F
         GET   $1,rJ
         SUB   $3,$0,1
         SET   $4,1
         SET   $5,2
         PUSHJ $2,Deep
         PUT   rJ,$1
1H       POP   0,0
Pr       GET   $1,rJ
         SET   $2,16
         LDA   $3,HexBuf+16
         LDA   $5,HexDig
1H       SUB   $3,$3,1
         AND   $4,$0,#f
         LDBU  $4,$5,$4
         STBU  $4,$3,0
         SRU   $0,$0,4
         SUB   $2,$2,1
         PBP   $2,1B
         LDA   $255,HexBuf
         TRAP  0,Fputs,StdOut
         PUT   rJ,$1
         POP   0,0
