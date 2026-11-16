% holy_grail.mms - the Bridge of Death.
%
% MMIX version of holy_grail.asm: the Bridgekeeper asks three questions
% (name, quest, and the air-speed velocity of an unladen swallow) and
% reads the answers from standard input.  An answer to the third
% question that mentions both "african" and "european" (in any case)
% outwits him.
%
% Options: lib/std.mms lib/string.mms
A1      IS    Data_Segment+#400         % the answers, 256 bytes each
A2      IS    Data_Segment+#500
A3      IS    Data_Segment+#600
        GREG  A1
        GREG  A2
        GREG  A3
        LOC   Data_Segment
        GREG  @
Intro   BYTE  "STOP! Who would cross the Bridge of Death must answer "
        BYTE  "me"
        BYTE  #a,"these questions three, ere the other side he see.",0
Q1      BYTE  "BRIDGEKEEPER: What... is your name? ",0
Q2      BYTE  "BRIDGEKEEPER: What... is your quest? ",0
        GREG  @
Q3      BYTE  "BRIDGEKEEPER: What... is the air-speed velocity of an "
        BYTE  "unladen swallow? ",0
        GREG  @
Win     BYTE  "BRIDGEKEEPER: Huh? I-- I don't know that... AAAAARGH!"
        BYTE  #a,#a,0
Lose    BYTE  "BRIDGEKEEPER: You are cast into the Gorge of Eternal "
        BYTE  "Peril!",#a,#a,0
        GREG  @
Red     BYTE  #1b,"[31m",0
Blink   BYTE  #1b,"[5m",0
NoBlink BYTE  #1b,"[25m",0
NoColor BYTE  #1b,"[39m",0
African BYTE  "african",0
Europe  BYTE  "european",0

        LOC   #100
Main    LDA   $4,Intro
        PUSHJ $3,Std:Puts
        PUSHJ $3,Std:PutNl
        PUSHJ $3,Std:PutNl
        LDA   $4,Q1
        PUSHJ $3,Std:Puts
        LDA   $4,A1
        SET   $5,256
        PUSHJ $3,Str:GetLine
        LDA   $4,Q2
        PUSHJ $3,Std:Puts
        LDA   $4,A2
        SET   $5,256
        PUSHJ $3,Str:GetLine
        LDA   $4,Q3
        PUSHJ $3,Std:Puts
        LDA   $4,A3
        SET   $5,256
        PUSHJ $3,Str:GetLine
        PUSHJ $3,Std:PutNl
        LDA   $4,A3             % judge the third answer in lower case
        PUSHJ $3,Str:Lower
        LDA   $4,A3
        LDA   $5,African
        PUSHJ $3,Str:Find       % $3 = its index, or -1
        ADD   $2,$3,1           % 0 if not found
        LDA   $4,A3
        LDA   $5,Europe
        PUSHJ $3,Str:Find
        ADD   $3,$3,1
        LDA   $6,Red
        PUSHJ $5,Std:Puts
        LDA   $6,Blink
        PUSHJ $5,Std:Puts
        LDA   $6,Lose
        BZ    $2,1F
        BZ    $3,1F
        LDA   $6,Win
1H      PUSHJ $5,Std:Puts
        LDA   $6,NoBlink
        PUSHJ $5,Std:Puts
        LDA   $6,NoColor
        PUSHJ $5,Std:Puts
        SET   $255,0
        TRAP  0,Halt,0
