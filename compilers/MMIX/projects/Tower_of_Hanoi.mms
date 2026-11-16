% Tower_of_Hanoi.mms - solves the Tower of Hanoi puzzle.
%
% MMIX version of Tower_of_Hanoi.asm.  Usage: Tower_of_Hanoi [disks]
% Prints the three pegs now and then (every PrintMod moves) and after
% the last move.  The pegs are a table of 3 * disks octabytes at
% Data_Segment+#1000 (peg * disks + slot, slot 0 is the top), so disks
% must be less than 2500.
%
% Options: lib/std.mms lib/args.mms
Pegs    GREG  Data_Segment+#1000
Nd      GREG  0                 % the number of disks
Mn      GREG  0                 % the number of the move
Pm      GREG  1                 % print every Pm-th move
        LOC   Data_Segment
        GREG  @
Syn     BYTE  "Syntax: Hanoi [disks]",0
MoveS   BYTE  "Move: ",0
Bar     BYTE  "|",0
Sp3     BYTE  "   ",0
Eq      BYTE  "=========",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BZ    $2,1F
        LDA   $4,Syn
        PUSHJ $3,Std:Puts
        PUSHJ $3,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      LDOU  $4,$1,8
        PUSHJ $3,Args:ParseInt
        BP    $3,2F
        SET   $3,3              % the default
2H      SET   Nd,$3
        SET   $2,0              % peg 0 holds the disks 1 ... n
3H      CMP   $4,$2,Nd
        BNN   $4,4F
        ADD   $5,$2,1
        SL    $6,$2,3
        STO   $5,Pegs,$6
        ADD   $2,$2,1
        JMP   3B
4H      SET   $4,1
        SL    $4,$4,Nd
        SUB   $4,$4,1           % steps = 2^n - 1
        DIV   $5,$4,10
        ADD   $5,$5,2           % PrintMod = steps / 10 + 2
        SET   $7,5
        CMP   $6,Nd,10
        BNP   $6,5F
        SET   $7,100
5H      DIV   $8,$5,$7
        GET   $8,rR
        SUB   $5,$5,$8          % less PrintMod mod 5 (100 if n > 10)
        CMP   $6,$5,1
        BNN   $6,6F
        SET   $5,1
6H      SET   Pm,$5
        PUSHJ $2,PrintPegs
        SET   $4,Nd             % Solve(n, 0, 2, 1)
        SET   $5,0
        SET   $6,2
        SET   $7,1
        PUSHJ $3,Solve
        DIV   $4,Mn,Pm
        GET   $4,rR
        BZ    $4,7F
        PUSHJ $3,PrintPegs      % the last state, if not just printed
7H      SET   $255,0
        TRAP  0,Halt,0

% PrintPegs: the move number and the pegs
PrintPegs GET $0,rJ
        LDA   $5,MoveS
        PUSHJ $4,Std:Puts
        SET   $5,Mn
        PUSHJ $4,Std:PutDec
        PUSHJ $4,Std:PutNl
        SET   $1,0              % slot
1H      CMP   $3,$1,Nd
        BNN   $3,4F
        SET   $2,0              % peg
2H      MUL   $3,$2,Nd
        ADD   $3,$3,$1
        SL    $3,$3,3
        LDO   $3,Pegs,$3
        BNZ   $3,3F
        LDA   $5,Bar            % no disk: |
        PUSHJ $4,Std:Puts
        JMP   5F
3H      SET   $5,$3             % the number of the disk
        PUSHJ $4,Std:PutDec
5H      LDA   $5,Sp3
        PUSHJ $4,Std:Puts
        ADD   $2,$2,1
        CMP   $3,$2,3
        BN    $3,2B
        PUSHJ $4,Std:PutNl
        ADD   $1,$1,1
        JMP   1B
4H      LDA   $5,Eq
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        PUSHJ $4,Std:PutNl
        PUT   rJ,$0
        POP   0,0

% MakeMove: $0 = source peg, $1 = destination peg
MakeMove ADD  Mn,Mn,1
        GET   $2,rJ
        SET   $3,0              % the disk that moves
        SET   $4,0              % slot
        MUL   $5,$0,Nd
1H      CMP   $6,$4,Nd          % the top disk of the source peg
        BNN   $6,2F
        ADD   $7,$5,$4
        SL    $7,$7,3
        LDO   $8,Pegs,$7
        BNZ   $8,3F
        ADD   $4,$4,1
        JMP   1B
3H      SET   $3,$8
        STCO  0,Pegs,$7
2H      SUB   $9,Nd,1           % where it lands on the destination peg
        MUL   $5,$1,Nd
        SET   $4,0
4H      CMP   $6,$4,Nd
        BNN   $6,5F
        ADD   $7,$5,$4
        SL    $7,$7,3
        LDO   $8,Pegs,$7
        BNZ   $8,6F
        ADD   $4,$4,1
        JMP   4B
6H      SUB   $9,$4,1
5H      ADD   $7,$5,$9
        SL    $7,$7,3
        STO   $3,Pegs,$7
        DIV   $6,Mn,Pm
        GET   $6,rR
        BNZ   $6,7F
        PUSHJ $4,PrintPegs
7H      PUT   rJ,$2
        POP   0,0

% Solve: $0 = disks, $1 = source, $2 = destination, $3 = the other peg
Solve   GET   $4,rJ
        CMP   $5,$0,1
        BNZ   $5,1F
        SET   $7,$1
        SET   $8,$2
        PUSHJ $6,MakeMove
        PUT   rJ,$4
        POP   0,0
1H      SUB   $5,$0,1
        SET   $7,$5             % Solve(n - 1, source, other, dest)
        SET   $8,$1
        SET   $9,$3
        SET   $10,$2
        PUSHJ $6,Solve
        SET   $7,$1
        SET   $8,$2
        PUSHJ $6,MakeMove
        SET   $7,$5             % Solve(n - 1, other, dest, source)
        SET   $8,$3
        SET   $9,$2
        SET   $10,$1
        PUSHJ $6,Solve
        PUT   rJ,$4
        POP   0,0
