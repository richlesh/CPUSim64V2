% Pi.mms - pi from the Bailey-Borwein-Plouffe series.
%
% MMIX version of Pi.asm.  Usage: Pi terms      (1 ... 11)
% Sums the first terms of
%   pi = sum for k >= 0 of 1/16^k * (4/(8k+1) - 2/(8k+4)
%                                    - 1/(8k+5) - 1/(8k+6))
% in doubles and writes the sum with 16 digits.  The digits are those
% of the exact value of the double (the assembly version writes the
% shortest digits and fills in zeros), see expected/Pi.txt.
%
% Options: lib/std.mms lib/args.mms lib/fmt.mms lib/math.mms
Four    GREG  #4010000000000000         % 4.0
Two     GREG  #4000000000000000         % 2.0
One     GREG  #3FF0000000000000         % 1.0
Sixteen GREG  #4030000000000000         % 16.0
        LOC   Data_Segment
        GREG  @
Need    BYTE  "You must supply the number of terms 1-11",0
Range   BYTE  "You must supply the number of terms in the range [1-11]"
        BYTE  0
PiS     BYTE  "PI(",0
Colon   BYTE  "): ",0

        LOC   #100
Main    CMP   $2,$0,2           % $0 = argc, $1 = argv
        BZ    $2,1F
        LDA   $5,Need
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
1H      LDOU  $5,$1,8
        PUSHJ $4,Args:ParseInt
        SET   $2,$4             % $2 = terms
        BNP   $2,2F
        CMP   $3,$2,11
        BNP   $3,3F
2H      LDA   $5,Range
        PUSHJ $4,Std:Puts
        PUSHJ $4,Std:PutNl
        SET   $255,1
        TRAP  0,Halt,0
3H      SET   $4,$2
        PUSHJ $3,Compute        % $3 = pi
        LDA   $6,PiS
        PUSHJ $5,Std:Puts
        SET   $6,$2
        PUSHJ $5,Std:PutDec
        LDA   $6,Colon
        PUSHJ $5,Std:Puts
        SET   $6,$3
        SET   $7,16
        PUSHJ $5,Fmt:PutFixed
        PUSHJ $5,Std:PutNl
        SET   $255,0
        TRAP  0,Halt,0

% Compute: $0 = the number of terms -> $0 = the sum, a double
Compute GET   $1,rJ
        SET   $2,0              % the sum
        SET   $3,0              % k
1H      CMP   $4,$3,$0
        BNN   $4,9F
        SL    $4,$3,3           % 8k
        ADD   $5,$4,1           % 4/(8k+1)
        FLOT  $5,$5
        FDIV  $6,Four,$5
        ADD   $5,$4,4           % - 2/(8k+4)
        FLOT  $5,$5
        FDIV  $7,Two,$5
        FSUB  $6,$6,$7
        ADD   $5,$4,5           % - 1/(8k+5)
        FLOT  $5,$5
        FDIV  $7,One,$5
        FSUB  $6,$6,$7
        ADD   $5,$4,6           % - 1/(8k+6)
        FLOT  $5,$5
        FDIV  $7,One,$5
        FSUB  $6,$6,$7
        SET   $9,Sixteen        % divided by 16^k
        SET   $10,$3
        PUSHJ $8,Math:PowI
        FDIV  $7,$6,$8
        FADD  $2,$2,$7
        ADD   $3,$3,1
        JMP   1B
9H      PUT   rJ,$1
        SET   $0,$2
        POP   1,0
