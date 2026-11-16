% library 3: Util:Twice, with a local label 1H (the prefix is kept)
        PREFIX Util:
Twice   ADD   $0,$0,$0
        BNZ   $0,1F
        SET   $0,0
1H      POP   1,0
