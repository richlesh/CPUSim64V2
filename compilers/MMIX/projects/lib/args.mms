% args.mms - the command line.  Listed after the program in Options.
%
% At the start $0 is argc (the program name counts) and $1 is argv, an
% array of octabytes that point to zero-terminated strings; argv[i] is
% LDOU $x,$1,8*i.
%
%   Args:ParseInt   $0 = a string -> $0 = its number: an optional '-'
%                   and digits, up to the first other character
        PREFIX Args:
ParseInt SET  $1,0              % the value
        SET   $2,0              % 1 if there was a '-'
        LDBU  $3,$0,0
        CMP   $4,$3,'-'
        BNZ   $4,1F
        SET   $2,1
        ADD   $0,$0,1
1H      LDBU  $3,$0,0
        SUB   $3,$3,'0'
        CMPU  $4,$3,10
        BNN   $4,2F             % not a digit
        MUL   $1,$1,10
        ADD   $1,$1,$3
        ADD   $0,$0,1
        JMP   1B
2H      BZ    $2,3F
        NEG   $1,$1
3H      SET   $0,$1
        POP   1,0
        PREFIX :
