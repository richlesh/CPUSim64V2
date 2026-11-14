% SAVE and UNSAVE
         LOC   Data_Segment
         GREG  @
HexDig   BYTE  "0123456789abcdef"
HexBuf   BYTE  "................",#a,0
ctx      GREG  0
ga       GREG  #aaaa
v        IS    $60
         LOC   #100
Main     SET   $0,11
         SET   $1,22
         SET   $2,33
         SETML $4,3
         INCL  $4,#ff
         PUT   rA,$4
         SETL  $4,#5
         PUT   rD,$4
         PUSHJ $5,Sub
         GET   $61,rL
         PUSHJ v,Pr
         SET   $61,$0
         PUSHJ v,Pr
         SET   $61,$2
         PUSHJ v,Pr
         SET   $61,ga
         PUSHJ v,Pr
         GET   $61,rD
         PUSHJ v,Pr
         GET   $61,rA
         PUSHJ v,Pr
         TRAP  0,Halt,0
Sub      SET   $0,44
         SET   $1,55
         GET   $9,rJ
         SAVE  ctx,0
         SET   $61,ctx
         PUSHJ v,Pr
         GET   $61,rL
         PUSHJ v,Pr
         SET   $0,0               change everything
         SET   $1,0
         SET   ga,0
         PUT   rD,$1
         PUT   rA,$1
         SET   $10,123
         UNSAVE 0,ctx
         GET   $61,rL
         PUSHJ v,Pr
         SET   $61,$0
         PUSHJ v,Pr
         SET   $61,$1
         PUSHJ v,Pr
         SET   $61,$10
         PUSHJ v,Pr
         PUT   rJ,$9
         POP   0,0

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
