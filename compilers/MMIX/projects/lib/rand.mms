% rand.mms - pseudo-random numbers.
% Listed after the program in Options.
%
%   Rand:Seed    $0 = the seed (the start value is a constant)
%   Rand:Range   $0 = lo, $1 = hi -> $0 = a random integer lo ... hi
%   Rand:Float   -> $0 = a random double, 0 <= x < 1
%
% A linear congruential generator with Knuth's MMIX constants; the
% number comes from the upper bits.  MMIX has no clock, so the numbers
% are the same in every run unless the program seeds the generator.
        PREFIX Rand:
State   GREG  88172645463325252
Mult    GREG  #5851F42D4C957F2D
Incr    GREG  #14057B7EF767814F
Scale   GREG  #3CA0000000000000         % 2^-53 as a double

Seed    SET   State,$0
        POP   0,0

Range   MULU  State,State,Mult
        ADDU  State,State,Incr
        SRU   $2,State,33       % 31 random bits
        SUB   $3,$1,$0
        ADD   $3,$3,1           % the number of values
        DIVU  $2,$2,$3
        GET   $2,:rR            % the remainder
        ADD   $0,$0,$2
        POP   1,0

Float   MULU  State,State,Mult
        ADDU  State,State,Incr
        SRU   $0,State,11       % 53 random bits
        FLOTU $0,$0
        FMUL  $0,$0,Scale       % * 2^-53
        POP   1,0
        PREFIX :
