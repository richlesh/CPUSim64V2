% trips, TRIP, RESUME, rW rX rY rZ
         LOC   Data_Segment
         GREG  @
HexDig   BYTE  "0123456789abcdef"
HexBuf   BYTE  "................",#a,0
Msg      BYTE  "handler ",0
cnt      GREG  0
v        IS    $60
% TRIP goes to location 0
         LOC   0
         PUSHJ $255,Handler
         PUT   rJ,$255
         GET   $255,rB
         RESUME
% integer divide check
         LOC   #10
         PUSHJ $255,Handler
         PUT   rJ,$255
         GET   $255,rB
         RESUME
% integer overflow
         LOC   #20
         PUSHJ $255,Handler
         PUT   rJ,$255
         GET   $255,rB
         RESUME
% float-to-fix overflow
         LOC   #30
         PUSHJ $255,Handler
         PUT   rJ,$255
         GET   $255,rB
         RESUME
% invalid
         LOC   #40
         PUSHJ $255,Handler
         PUT   rJ,$255
         GET   $255,rB
         RESUME
% division by zero (float)
         LOC   #70
         PUSHJ $255,Handler
         PUT   rJ,$255
         GET   $255,rB
         RESUME
         LOC   #100
Main     SETL  $0,#ff00         enable all trips
         PUT   rA,$0
         SETH  $1,#7fff
         ADD   $2,$1,$1         V trip
         SET   $61,$2
         PUSHJ v,Pr
         SET   $3,0
         DIV   $4,$1,$3         D trip
         SET   $61,$4
         PUSHJ v,Pr
         SETH  $5,#7ff0          +inf
         FIX   $6,0,$5           I trip
         SET   $61,$6
         PUSHJ v,Pr
         SETH  $7,#3ff0          1.0
         FDIV  $8,$7,$3          Z trip
         SET   $61,$8
         PUSHJ v,Pr
         SETH  $9,#43f0
         FIX   $10,0,$9          W trip
         SET   $61,$10
         PUSHJ v,Pr
         TRIP  1,2,3
         SET   $61,cnt
         PUSHJ v,Pr
         SET   $255,#3f
         SL    $11,$1,1          V trip from a shift
         SET   $61,$255           $255 restored from rB
         PUSHJ v,Pr
         STB   $1,HexBuf          V trip from a store
         SET   $61,cnt
         PUSHJ v,Pr
         SETL  $0,#0000          no trips: events only
         PUT   rA,$0
         ADD   $2,$1,$1
         FDIV  $8,$7,$3
         GET   $61,rA
         PUSHJ v,Pr
         TRAP  0,Halt,0

% the handler prints rW, rX, rY, rZ and counts the trips
Handler  GET   $0,rJ
         INCL  cnt,1
         LDA   $255,Msg
         TRAP  0,Fputs,StdOut
         GET   $2,rW
         PUSHJ $1,Pr
         GET   $2,rX
         PUSHJ $1,Pr
         GET   $2,rY
         PUSHJ $1,Pr
         GET   $2,rZ
         PUSHJ $1,Pr
         GET   $2,rA
         PUSHJ $1,Pr
         PUT   rJ,$0
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
