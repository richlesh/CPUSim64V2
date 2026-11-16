% library 1: Hello:Say (the prefix is still set at the end of the file)
        PREFIX Hello:
Text    BYTE  "hello",#a,0
        LOC   (@+3)&-4
Say     GETA  $255,Text
        TRAP  0,:Fputs,:StdOut
        POP   0,0
