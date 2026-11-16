% a program of five files: main + libraries, each with its own PREFIX
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
