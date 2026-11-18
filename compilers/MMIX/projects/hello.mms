* Print "Hello, World!" in seven languages (UTF-8 output)

        LOC   Data_Segment
        GREG  @
English BYTE  "English:  Hello, World!",10,0
Spanish BYTE  "Spanish:  ",#C2,#A1,"Hola, mundo!",10,0
French  BYTE  "French:   Bonjour, le monde !",10,0
* Greek: Γεια σου, κόσμε!
Greek   BYTE  "Greek:    ",#CE,#93,#CE,#B5,#CE,#B9,#CE,#B1," "
        BYTE  #CF,#83,#CE,#BF,#CF,#85,", "
        BYTE  #CE,#BA,#CF,#8C,#CF,#83,#CE,#BC,#CE,#B5,"!",10,0
* Russian: Привет, мир!
Russian BYTE  "Russian:  ",#D0,#9F,#D1,#80,#D0,#B8
        BYTE  #D0,#B2,#D0,#B5,#D1,#82,", "
        BYTE  #D0,#BC,#D0,#B8,#D1,#80,"!",10,0
* Japanese: こんにちは、世界！
Japanese BYTE "Japanese: ",#E3,#81,#93,#E3,#82,#93,#E3,#81,#AB
        BYTE  #E3,#81,#A1,#E3,#81,#AF,#E3,#80,#81
        BYTE  #E4,#B8,#96,#E7,#95,#8C,#EF,#BC,#81,10,0
* Chinese: 你好，世界！
Chinese BYTE  "Chinese:  ",#E4,#BD,#A0,#E5,#A5,#BD,#EF,#BC,#8C
        BYTE  #E4,#B8,#96,#E7,#95,#8C,#EF,#BC,#81,10,0

* Zero-terminated table of string addresses
Table   OCTA  English,Spanish,French,Greek,Russian,Japanese,Chinese,0

        LOC   #100
Main    LDA   $1,Table        $1 -> current table entry
1H      LDO   $255,$1,0       $255 <- address of next string
        BZ    $255,2F         0 marks the end of the table
        TRAP  0,Fputs,StdOut
        ADD   $1,$1,8         advance to next octabyte
        JMP   1B
2H      SETL  $255,0          exit code 0
        TRAP  0,Halt,0
