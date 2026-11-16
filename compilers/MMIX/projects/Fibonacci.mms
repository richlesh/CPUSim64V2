% Fibonacci.mms - prints the first 45 Fibonacci numbers.
%
% MMIX version of Fibonacci.asm (see README.md).
% Options: lib/std.mms
        LOC   #100
Main    SET   $0,1              % a
        SET   $1,1              % b
        SET   $2,45             % how many
1H      SET   $4,$0
        PUSHJ $3,Std:PutDec
        PUSHJ $3,Std:PutNl
        ADD   $5,$0,$1
        SET   $0,$1
        SET   $1,$5
        SUB   $2,$2,1
        BNZ   $2,1B
        SET   $255,0
        TRAP  0,Halt,0
