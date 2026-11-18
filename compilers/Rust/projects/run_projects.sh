#!/usr/bin/env bash
###############################################################################
# run_projects.sh - translates the Rust versions of the documentation projects
# (rustc, then the C11 compiler) and compares their output with the output of
# the assembly versions (compilers/C11/projects/reference/, made by
# documentation/projects/gen_output.sh).
#
#   ./run_projects.sh [name ...]      all projects, or the named ones
#
#   SIM=java ./run_projects.sh        run on the Java simulator (default: native)
#   RUSTC=asm ./run_projects.sh       run rustc.o64 on the simulator (default: the
#                                     native build, obj/rustc-native)
#   CPUSIM64_HOME=/path ./run_projects.sh
#                                     use that CPUSim64 install (assemble.sh,
#                                     bin/cpusim64_native) instead of the repository
#
# A Rust program on CPUSim64 has no command line and no files (std::env::args()
# is only the program name, there is no std::fs).  So a program that takes
# arguments reads them from the FIRST LINE OF STANDARD INPUT, and a program that
# reads a file reads the REST OF STANDARD INPUT; the file it would write goes to
# standard output.  This script feeds "ARGS\n" followed by the input file and
# compares the program's standard output with the reference file.
#
# Each run's program output and result ("Result: N") is compared with the same
# part of the reference file (or with expected/NAME.txt, where Rust prints
# differently).  The programs that use random numbers are checked instead
# (check_sort, check_insert); the Mandelbrot programs are compared by the image
# they print (PGM on standard output).
###############################################################################
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
HOME_DIR="${CPUSIM64_HOME:-$ROOT}"
C11="$ROOT/compilers/C11"
RUST="$ROOT/compilers/Rust"
REF="$C11/projects/reference"          # shared with the C11 and C++14 versions
NATIVE="$HOME_DIR/bin/cpusim64_native"
JAR="$(ls "$HOME_DIR"/lib/cpusim64-*.jar 2>/dev/null | grep -v ' ' | head -1)"
OUT="$HERE/output"
MEM=128M
STACK=1M
TMP="$(mktemp -d)"
export NO_DELAY=1
export CPUSIM64_CACHE="$TMP/cache"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$OUT"

pass=0
fail=0
failed=()
only=("$@")

want() {
    [[ ${#only[@]} -eq 0 ]] && return 0
    local n
    for n in "${only[@]}"; do [[ "$n" == "$1" ]] && return 0; done
    return 1
}

# the program output and result of every run in a log
extract() {
    perl -e 'local $/; my $s = <STDIN>;
             while ($s =~ /^Read \d+ words[^\n]*\n(.*?)Result: (-?\d+)\n/msg) { print $1, "[Result $2]\n"; }'
}

# the program output of the (single) run in a log, byte for byte
body() {
    perl -e 'local $/; my $s = <STDIN>; binmode STDOUT;
             if ($s =~ /^Read \d+ words[^\n]*\n(.*?)Result: (-?\d+)\n/ms) { print $1; }'
}

if [[ ! -f "$C11/obj/cc.o64" || -n "$(find "$C11" -maxdepth 1 \( -name '*.asm' -o -name '*.def' \) -newer "$C11/obj/cc.o64")" ]]; then
    "$HOME_DIR/assemble.sh" "$C11/cc" > "$TMP/asmlog" 2>&1 || { cat "$TMP/asmlog"; exit 1; }
fi

if [[ "$RUSTC" == "asm" ]]; then
    if [[ ! -f "$RUST/obj/rustc.o64" || "$RUST/rustc.asm" -nt "$RUST/obj/rustc.o64" ]]; then
        "$HOME_DIR/assemble.sh" "$RUST/rustc" > "$TMP/asmlog" 2>&1 || { cat "$TMP/asmlog"; exit 1; }
    fi
elif [[ ! -x "$RUST/obj/rustc-native" || -n "$(find "$RUST/src" "$RUST/lib" -newer "$RUST/obj/rustc-native" 2>/dev/null | head -1)" ]]; then
    "$RUST/build.sh" --native > /dev/null || { echo "FATAL: native build of rustc failed"; exit 1; }
fi

# build NAME: translates NAME.rs to NAME.gen.c, compiles that and assembles it
# (NAME.gen.asm becomes NAME.asm, as in the IDE); 0 on success
build() {
    local name="$1"
    rm -f "$HERE/$name.gen.c" "$HERE/$name.gen.asm" "$HERE/$name.asm" "$HERE/obj/$name.o64"
    if [[ "$RUSTC" == "asm" ]]; then
        ( cd "$HERE" && "$NATIVE" "$RUST/obj/rustc.o64" --verbose --mem=64M --stack=1M "$name.rs" ) > "$TMP/rustc.log" 2>&1
        grep -q "^Result: 0" "$TMP/rustc.log" || { grep -i "error" "$TMP/rustc.log" | head -3; return 1; }
    else
        ( cd "$HERE" && "$RUST/obj/rustc-native" "$name.rs" ) > "$TMP/rustc.log" 2>&1 || { grep -i "error" "$TMP/rustc.log" | head -3; return 1; }
    fi
    [[ -f "$HERE/$name.gen.c" ]] || { echo "no $name.gen.c"; return 1; }
    ( cd "$C11" && "$NATIVE" obj/cc.o64 --verbose --mem=64M --stack=1M "$HERE/$name.gen.c" ) > "$TMP/cc.log" 2>&1
    if ! grep -q "^Result: 0" "$TMP/cc.log" || [[ ! -f "$HERE/$name.gen.asm" ]]; then
        grep -i "error" "$TMP/cc.log" | head -3
        return 1
    fi
    mv "$HERE/$name.gen.asm" "$HERE/$name.asm"
    ( cd "$HERE" && "$HOME_DIR/assemble.sh" "$name" ) > "$TMP/asm.log" 2>&1
    if [[ ! -f "$HERE/obj/$name.o64" ]]; then
        grep -i "error" "$TMP/asm.log" | head -3
        return 1
    fi
}

# sim NAME INPUT : runs the program with INPUT as standard input; the raw output is $TMP/raw
sim() {
    local name="$1" input="$2"
    if [[ "$SIM" == "java" ]]; then
        ( cd "$HERE" && java -Dfile.encoding=UTF8 -cp "$JAR" cloud.lesh.CPUSim64.Simulation \
            "obj/$name.o64" --verbose --mem=$MEM --stack=$STACK < "$input" ) > "$TMP/raw" 2>&1
    else
        ( cd "$HERE" && "$NATIVE" "obj/$name.o64" --verbose --mem=$MEM --stack=$STACK < "$input" ) > "$TMP/raw" 2>&1
    fi
}

# run NAME [ARGS [FILE]] : runs the program; its standard input is the line ARGS, then FILE
# (no ARGS and no FILE: /dev/null, or $STDIN when that is set); appends to $OUT/NAME.log
run() {
    local name="$1" args="$2" file="$3" input="${STDIN:-/dev/null}"
    if [[ -n "$args" || -n "$file" ]]; then
        input="$TMP/stdin"
        { [[ -n "$args" ]] && printf '%s\n' "$args"; [[ -n "$file" ]] && cat "$file"; } > "$input"
    fi
    sim "$name" "$input"
    cat "$TMP/raw" >> "$OUT/$name.log"
}

# runf NAME OUTFILE ARGS FILE : like run, and the program's standard output (the file the C
# program writes) is saved as $OUT/OUTFILE
runf() {
    local name="$1" outfile="$2"
    run "$name" "$3" "$4"
    body < "$TMP/raw" > "$OUT/$outfile"
}

ok() { echo "PASS  $1"; pass=$((pass + 1)); }
bad() { echo "FAIL  $1: $2"; fail=$((fail + 1)); failed+=("$1"); }

# project NAME RUNS [OURS:REF ...] : RUNS is a function that runs the program; each
# OURS:REF pair is a file the program printed (saved in output/ by runf) and the reference
# file it must equal.  The program output is compared when reference/NAME.out is a run log.
project() {
    local name="$1" runs="$2"; shift 2
    want "$name" || return 0
    if [[ ! -f "$HERE/$name.rs" ]]; then return 0; fi
    if ! build "$name" > "$TMP/build.err"; then
        bad "$name" "build failed: $(cat "$TMP/build.err")"
        return 0
    fi
    rm -f "$OUT/$name.log"
    local f
    for f in "$@"; do rm -f "$OUT/${f%%:*}"; done
    $runs
    if [[ -f "$HERE/expected/$name.txt" ]]; then
        # where Rust prints differently from the assembly version (see README.md)
        if ! diff "$HERE/expected/$name.txt" <(extract < "$OUT/$name.log") > "$TMP/diff"; then
            bad "$name" "output differs from expected/$name.txt (< expected, > Rust)"
            head -10 "$TMP/diff"
            return 0
        fi
    elif grep -q "^Read [0-9]* words" "$REF/$name.out" 2>/dev/null; then
        if ! diff <(extract < "$REF/$name.out") <(extract < "$OUT/$name.log") > "$TMP/diff"; then
            bad "$name" "output differs (< reference, > Rust)"
            head -10 "$TMP/diff"
            return 0
        fi
    elif grep -q "Result: [1-9]" "$OUT/$name.log"; then
        bad "$name" "a run failed: $(grep -m1 "Result:" "$OUT/$name.log")"
        return 0
    fi
    for f in "$@"; do
        if ! cmp -s "$REF/${f#*:}" "$OUT/${f%%:*}"; then
            bad "$name" "output/${f%%:*} differs from reference/${f#*:}"
            return 0
        fi
    done
    ok "$name"
}

# The sorting programs use random numbers, so their output is checked instead of compared:
# check_sort LOG MIN MAX INT|FLOAT : 10 tests, each "random:" line has MIN..MAX numbers of the
# kind and the "sorted:" line is the same numbers in order
check_sort() {
    extract < "$1" | perl -e '
        my ($min, $max, $kind) = @ARGV;
        my $num = $kind eq "INT" ? qr/^[1-9]\d{0,2}$/ : qr/^0\.\d{6}$/;
        my @l = <STDIN>; chomp @l;
        my $t = 0;
        for (my $i = 0; $i < @l; ) {
            last if $l[$i] =~ /^\[Result/;
            $t++;
            $l[$i++] eq "Test $t" or die "line $i: expected Test $t\n";
            my ($r) = $l[$i++] =~ /^random: (.*)$/ or die "line $i: no random line\n";
            my ($s) = $l[$i++] =~ /^sorted: (.*)$/ or die "line $i: no sorted line\n";
            my @r = split /,/, $r; my @s = split /,/, $s;
            (@r >= $min && @r <= $max) or die "test $t: " . scalar(@r) . " numbers\n";
            for (@r) { /$num/ or die "test $t: bad number $_\n"; }
            my @want = sort { $a <=> $b } @r;
            "@want" eq "@s" or die "test $t: the sorted line is not the sorted random line\n";
        }
        $t == 10 or die "$t tests, expected 10\n";
        $l[-1] eq "[Result 0]" or die "result: $l[-1]\n";
    ' "$2" "$3" "$4"
}

# check_insert LOG : 30 insertions; each line is the previous list with the value inserted in order
check_insert() {
    extract < "$1" | perl -e '
        my @l = <STDIN>; chomp @l;
        shift(@l) eq "Inserting 30 random integers [1,999] one at a time:" or die "no header\n";
        my @prev;
        for my $i (1 .. 30) {
            my ($v, $list) = $l[$i - 1] =~ /^Insert ([ \d]{3}): (.*)$/ or die "line $i: $l[$i-1]\n";
            $v =~ s/ //g;
            ($v >= 1 && $v <= 999) or die "line $i: value $v\n";
            my @want = sort { $a <=> $b } (@prev, $v);
            "@want" eq join(" ", split /,/, $list) or die "line $i: the list is not sorted with $v\n";
            @prev = @want;
        }
        $l[30] eq "[Result 0]" or die "result: $l[30]\n";
    '
}

# project_checked NAME RUNS CHECK... : runs the program and checks its log with CHECK LOG ARGS
project_checked() {
    local name="$1" runs="$2" check="$3"; shift 3
    want "$name" || return 0
    if [[ ! -f "$HERE/$name.rs" ]]; then return 0; fi
    if ! build "$name" > "$TMP/build.err"; then
        bad "$name" "build failed: $(cat "$TMP/build.err")"
        return 0
    fi
    rm -f "$OUT/$name.log"
    $runs
    if ! $check "$OUT/$name.log" "$@" > "$TMP/check" 2>&1; then
        bad "$name" "$(head -3 "$TMP/check")"
        return 0
    fi
    ok "$name"
}

# ---------------------------------------------------------------- the runs (as in gen_output.sh)
# the arguments of the C runs become the first line of standard input
L="$C11/projects/data/Lincoln.txt"
LN=data/Lincoln.txt                    # the name that hashcode and md5 print

MB="-0.748127681290498 0.09408613781852182 1e-6 10000 128"
r_mandelbrot() { runf mandelbrot mandelbrot.pgm "$MB"; }
r_mandelbrot_binary() { runf mandelbrot_binary mandelbrot_binary.pgm "$MB"; }
r_mandelbrot_histogram() { runf mandelbrot_histogram mandelbrot_histogram.pgm "$MB"; }
r_mandelbrot_histogram_balanced() { runf mandelbrot_histogram_balanced mandelbrot_histogram_balanced.pgm "$MB"; }
r_Base64() { runf Base64 Base64.cipher "e" "$L"; runf Base64 Base64.out2 "d" "$OUT/Base64.cipher"; }
r_BBP_hex_digits() { run BBP_hex_digits 100000; }
r_BubbleSort() { run BubbleSort; }
r_BubbleSortFloat() { run BubbleSortFloat; }
r_Caesar() { runf Caesar Caesar.cipher "13" "$L"; runf Caesar Caesar.out2 "-13" "$OUT/Caesar.cipher"; }
r_Congruent() { run Congruent 180; }
r_Cubic() { run Cubic "1 -1 -2 2"; run Cubic "1 -2 2 -1"; }
r_Fibonacci() { run Fibonacci; }
r_Hailstone() { run Hailstone; }
r_Hailstone2() { run Hailstone2 10000; }
r_Hailstone3() { run Hailstone3 10000; }
r_hashcode() { run hashcode "$LN" "$L"; }
r_hashcode2() { run hashcode2 "$LN" "$L"; }
r_holy_grail() {
    printf 'Rich\ngrail\n800\n' > "$TMP/hg1"
    printf 'Rich\ngrail\nEuropean or African\n' > "$TMP/hg2"
    STDIN="$TMP/hg1" run holy_grail
    STDIN="$TMP/hg2" run holy_grail
}
r_InsertSorted() { run InsertSorted; }
r_md5() { run md5 "$LN" "$L"; }
r_Pi() { run Pi 11; }
r_PI_spigot() { run PI_spigot 1000; }
r_Quadratic() { run Quadratic "1 -1 -2"; run Quadratic "1 -2 2"; }
r_QuickSort() { run QuickSort; }
r_QuickSortFloat() { run QuickSortFloat; }
r_Renard() { run Renard 20; }
r_Renard2() { run Renard2 20; }
r_rot13() { runf rot13 rot13.cipher "" "$L"; runf rot13 rot13.out2 "" "$OUT/rot13.cipher"; }
r_Tower_of_Hanoi() { run Tower_of_Hanoi 10; }
r_Unicode() { run Unicode; }
r_Vigenere() { runf Vigenere Vigenere.cipher "password" "$L"; runf Vigenere Vigenere.out2 "-password" "$OUT/Vigenere.cipher"; }
r_XORCipher() { runf XORCipher XORCipher.cipher "password" "$L"; runf XORCipher XORCipher.out2 "password" "$OUT/XORCipher.cipher"; }

project mandelbrot r_mandelbrot mandelbrot.pgm:mandelbrot.pgm
project mandelbrot_binary r_mandelbrot_binary mandelbrot_binary.pgm:mandelbrot_binary.pgm
project mandelbrot_histogram r_mandelbrot_histogram mandelbrot_histogram.pgm:mandelbrot_histogram.pgm
project mandelbrot_histogram_balanced r_mandelbrot_histogram_balanced mandelbrot_histogram_balanced.pgm:mandelbrot_histogram_balanced.pgm
project Base64 r_Base64 Base64.cipher:Base64.cipher Base64.out2:Base64.out
project BBP_hex_digits r_BBP_hex_digits
project_checked BubbleSort r_BubbleSort check_sort 15 30 INT
project_checked BubbleSortFloat r_BubbleSortFloat check_sort 9 14 FLOAT
project Caesar r_Caesar Caesar.cipher:Caesar.cipher Caesar.out2:Caesar.out
project Congruent r_Congruent
project Cubic r_Cubic
project Fibonacci r_Fibonacci
project Hailstone r_Hailstone
project Hailstone2 r_Hailstone2
project Hailstone3 r_Hailstone3
project hashcode r_hashcode
project hashcode2 r_hashcode2
project holy_grail r_holy_grail
project_checked InsertSorted r_InsertSorted check_insert
project md5 r_md5
project Pi r_Pi
project PI_spigot r_PI_spigot
project Quadratic r_Quadratic
project_checked QuickSort r_QuickSort check_sort 15 30 INT
project_checked QuickSortFloat r_QuickSortFloat check_sort 9 14 FLOAT
project Renard r_Renard
project Renard2 r_Renard2
project rot13 r_rot13 rot13.cipher:rot13.cipher rot13.out2:rot13.out
project Tower_of_Hanoi r_Tower_of_Hanoi
project Unicode r_Unicode
project Vigenere r_Vigenere Vigenere.cipher:Vigenere.cipher Vigenere.out2:Vigenere.out
project XORCipher r_XORCipher XORCipher.cipher:XORCipher.cipher XORCipher.out2:XORCipher.out

echo "----------------------------------------"
echo "passed: $pass  failed: $fail"
for f in "${failed[@]}"; do echo "  failed: $f"; done
[[ $fail -eq 0 ]]
