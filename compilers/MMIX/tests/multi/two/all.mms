% the five files of this directory as one file, for the oracle (mmixal has
% no multi-file input: the prefix is reset by hand after each library)
        LOC   #100
Main    GETA  $255,Msg
        TRAP  0,Fputs,StdOut
        PUSHJ $0,Hello:Say
        PUSHJ $0,Other:Say
        SET   $2,5
        PUSHJ $1,Util:Twice
        JMP   Zed
Msg     BYTE  "main",#a,0
        LOC   (@+3)&-4
        PREFIX Hello:
Text    BYTE  "hello",#a,0
        LOC   (@+3)&-4
Say     GETA  $255,Text
        TRAP  0,:Fputs,:StdOut
        POP   0,0
        PREFIX :
        PREFIX Other:
Text    BYTE  "other",#a,0
        LOC   (@+3)&-4
Say     SET   $1,2
1H      GETA  $255,Text
        TRAP  0,:Fputs,:StdOut
        SUB   $1,$1,1
        PBP   $1,1B
        POP   0,0
        PREFIX :
        PREFIX Util:
Twice   ADD   $0,$0,$0
        BNZ   $0,1F
        SET   $0,0
1H      POP   1,0
        PREFIX :
Zed     ADD   $255,$1,7
        TRAP  0,Halt,0
