% library 2: the names of library 1 under another prefix, a local label
        PREFIX Other:
Text    BYTE  "other",#a,0
        LOC   (@+3)&-4
Say     SET   $1,2
1H      GETA  $255,Text
        TRAP  0,:Fputs,:StdOut
        SUB   $1,$1,1
        PBP   $1,1B
        POP   0,0
