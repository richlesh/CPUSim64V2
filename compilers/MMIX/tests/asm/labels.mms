% local labels, forward references, backward fixups, PREFIX, GREG reuse
        LOC   #100
Main    JMP   1F
1H      BZ    $0,2F
        PBNZ  $1,1B
2H      GETA  $255,1B
        GETA  $255,3F
3H      PUSHJ $3,Foo:Sub
        JMP   1B
        PREFIX Foo:
x       IS    5
Sub     SET   $0,x
        POP   1,0
        PREFIX :
        SET   $2,Foo:x
        JMP   Foo:Sub
        BN    $0,Far
        OCTA  Later,Main
Later   SWYM
        LOC   #2000
Far     TRAP  0,Halt,0
