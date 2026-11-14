% expressions: precedence, operators, registers, characters
a       IS    #10
b       IS    3
r       IS    $5
        LOC   #200
Main    SET   $1,a+b*2          % strong before weak: 16
        SET   $2,(a+b)*2
        SET   $3,a-b-1          % left to right
        SET   $4,a<<b|1
        SET   $6,#ff&#0f^#3
        SET   $7,-1&#ffff
        SET   $8,~0&#7f
        SET   $9,100/7
        SET   $10,100%7
        OCTA  1//3,7//8,#ffffffffffffffff
        OCTA  #8000000000000000>>63,1<<63
        ADD   r,r,1
        ADD   $(b+1),$(b),r
        BYTE  'A','b'+1,"xyz",0
        WYDE  1,"AB"
        TETRA "Q",#12345678
        BYTE  "a","b"
        OCTA  @
        TRAP  0,Halt,0
