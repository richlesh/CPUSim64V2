#!/usr/bin/env bash
###############################################################################
# run_projects.sh - translates the MMIX versions of the documentation projects
# and compares their output with the output of the assembly versions
# (reference/, made by documentation/projects/gen_output.sh).
#
#   ./run_projects.sh [name ...]      all projects, or the named ones
#
#   SIM=java ./run_projects.sh        run on the Java simulator (default: native)
#   MMIXC=native ./run_projects.sh    use the native development build of the
#                                     translator (../build.sh --native)
#   REAL=1 ./run_projects.sh          run on Knuth's mmixal and mmix instead
#                                     (needs both, e.g. brew install mmix)
#
# Each NAME.mms names its library files in a line "% Options: lib/std.mms ..."
# (the text of the IDE's Options field); they are given to the translator
# after the program.  Each run's program output and result ("Result: N") is
# compared with the same part of the reference file (or with expected/NAME.txt,
# where the MMIX version prints differently); the files that a program writes
# (the ciphers and decoded texts) are compared with the reference files.  The
# programs that use random numbers are checked instead (check_sort,
# check_insert).
###############################################################################
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
MMIX="$ROOT/compilers/MMIX"
NATIVE="$ROOT/bin/cpusim64_native"
JAR="$(ls "$ROOT"/lib/cpusim64-*.jar 2>/dev/null | head -1)"
OUT="$HERE/output"
MEM=16M
STACK=1M
TMP="$(mktemp -d)"
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

# the program output and result of every run in a run.sh log
extract() {
    perl -e 'local $/; my $s = <STDIN>;
             while ($s =~ /^Read \d+ words[^\n]*\n(.*?)Result: (-?\d+)\n/msg) { print $1, "[Result $2]\n"; }'
}

# the translator: the CPUSim64 build (obj/mmixc.o64, assembled from mmixc.asm when needed) or the native one
if [[ -n "$REAL" ]]; then
    :                                   # Knuth's mmixal and mmix: no translator
elif [[ "$MMIXC" == "native" ]]; then
    "$MMIX/build.sh" --native > /dev/null || { echo "FATAL: native build failed"; exit 1; }
    translate() { "$MMIX/obj/mmixc-native" "$@"; }
else
    if [[ ! -f "$MMIX/obj/mmixc.o64" || "$MMIX/mmixc.asm" -nt "$MMIX/obj/mmixc.o64" ]]; then
        "$ROOT/assemble.sh" "$MMIX/mmixc" > "$TMP/asmlog" 2>&1
        [[ -f "$MMIX/obj/mmixc.o64" ]] || { cat "$TMP/asmlog"; echo "FATAL: mmixc.asm does not assemble"; exit 1; }
    fi
    translate() { ( cd "$MMIX" && "$NATIVE" obj/mmixc.o64 --verbose --mem=64M --stack=1M "$@" ); }
fi

# build NAME: translates NAME.mms with its libraries and assembles it; 0 on success
build() {
    local name="$1" opts=() o
    rm -f "$HERE/$name.asm" "$HERE/obj/$name.o64"
    # mmixal drops the end of a line of 72 or more characters (and warns)
    if awk 'length($0) > 71 { print FILENAME ": line " FNR " is longer than 71 characters"; found = 1 } END { exit !found }' \
            "$HERE/$name.mms" "$HERE"/lib/*.mms; then
        return 1
    fi
    for o in $(grep '^% Options:' "$HERE/$name.mms" | sed 's/^% Options://'); do opts+=("$HERE/$o"); done
    if [[ -n "$REAL" ]]; then
        # Knuth's mmixal: the files joined into one (the libraries end with PREFIX ":")
        mkdir -p "$TMP/real"
        cat "$HERE/$name.mms" "${opts[@]}" > "$TMP/real/$name.mms"
        if ! mmixal -o "$TMP/real/$name.mmo" "$TMP/real/$name.mms" > "$TMP/cc.log" 2>&1 || grep -q "warning" "$TMP/cc.log"; then
            head -5 "$TMP/cc.log"
            return 1
        fi
        return 0
    fi
    translate "$HERE/$name.mms" "${opts[@]}" > "$TMP/cc.log" 2>&1
    # an error or a warning (e.g. "Z field doesn't fit in one byte") fails the build
    if [[ ! -f "$HERE/$name.asm" ]] || grep -q "^Result: [1-9]" "$TMP/cc.log" || grep -q "error\|!$\|warning" "$TMP/cc.log"; then
        grep -i "error\|!$\|warning" "$TMP/cc.log" | head -5
        return 1
    fi
    ( cd "$HERE" && "$ROOT/assemble.sh" "$name" ) > "$TMP/asm.log" 2>&1
    if [[ ! -f "$HERE/obj/$name.o64" ]]; then
        grep -i "error" "$TMP/asm.log" | head -3
        return 1
    fi
}

# run NAME [ARGS...] : runs the program (stdin from $STDIN or /dev/null) and appends to $OUT/NAME.log
run() {
    local name="$1"; shift
    local input="${STDIN:-/dev/null}"
    if [[ -n "$REAL" ]]; then
        # the real mmix, with its output and exit code made to look like a simulator log
        local out status f=()
        [[ -n "$STDIN" ]] && f=("-f$STDIN")
        out="$( cd "$HERE" && mmix "${f[@]}" "$TMP/real/$name" "$@" < /dev/null 2> /dev/null; echo "[$?]" )"
        status="${out##*[}"
        out="${out%\[*}"
        { echo "Read 0 words from $name.o64"; printf '%s' "$out"; echo "Result: ${status%]}"; } >> "$OUT/$name.log"
    elif [[ "$SIM" == "java" ]]; then
        ( cd "$HERE" && java -Dfile.encoding=UTF8 -cp "$JAR" cloud.lesh.CPUSim64.Simulation \
            "obj/$name.o64" --verbose --mem=$MEM --stack=$STACK "$@" < "$input" ) >> "$OUT/$name.log" 2>&1
    else
        ( cd "$HERE" && "$NATIVE" "obj/$name.o64" --verbose --mem=$MEM --stack=$STACK "$@" < "$input" ) >> "$OUT/$name.log" 2>&1
    fi
}

ok() { echo "PASS  $1"; pass=$((pass + 1)); }
bad() { echo "FAIL  $1: $2"; fail=$((fail + 1)); failed+=("$1"); }

# project NAME RUNS [OURS:REF ...] : RUNS is a function that runs the program; each
# OURS:REF pair is a file the program wrote to output/ and the reference file it must equal.
# The program output is compared when reference/NAME.out is a run log.
project() {
    local name="$1" runs="$2"; shift 2
    want "$name" || return 0
    if [[ ! -f "$HERE/$name.mms" ]]; then return 0; fi
    if ! build "$name" > "$TMP/build.err"; then
        bad "$name" "build failed: $(cat "$TMP/build.err")"
        return 0
    fi
    rm -f "$OUT/$name.log"
    local f
    for f in "$@"; do rm -f "$OUT/${f%%:*}"; done
    $runs
    if [[ -f "$HERE/expected/$name.txt" ]]; then
        # where the MMIX version prints differently from the assembly version (see README.md)
        if ! diff "$HERE/expected/$name.txt" <(extract < "$OUT/$name.log") > "$TMP/diff"; then
            bad "$name" "output differs from expected/$name.txt (< expected, > MMIX)"
            head -10 "$TMP/diff"
            return 0
        fi
    elif grep -q "^Read [0-9]* words" "$HERE/reference/$name.out" 2>/dev/null; then
        if ! diff <(extract < "$HERE/reference/$name.out") <(extract < "$OUT/$name.log") > "$TMP/diff"; then
            bad "$name" "output differs (< reference, > MMIX)"
            head -10 "$TMP/diff"
            return 0
        fi
    elif grep -q "Result: [1-9]" "$OUT/$name.log"; then
        bad "$name" "a run failed: $(grep -m1 "Result:" "$OUT/$name.log")"
        return 0
    fi
    for f in "$@"; do
        if ! cmp -s "$HERE/reference/${f#*:}" "$OUT/${f%%:*}"; then
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
    if [[ ! -f "$HERE/$name.mms" ]]; then return 0; fi
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
L=data/Lincoln.txt
ND=-n                     # the console animations: no delays (see README.md)

r_Base64() { run Base64 e $L output/Base64.cipher; run Base64 d output/Base64.cipher output/Base64.out2; }
r_BBP_hex_digits() { run BBP_hex_digits 100000; }
r_BubbleSort() { run BubbleSort; }
r_BubbleSortFloat() { run BubbleSortFloat; }
r_Caesar() { run Caesar 13 $L output/Caesar.cipher; run Caesar -13 output/Caesar.cipher output/Caesar.out2; }
r_Congruent() { run Congruent 180; }
r_Cubic() { run Cubic 1 -1 -2 2; run Cubic 1 -2 2 -1; }
r_Fibonacci() { run Fibonacci; }
r_Hailstone() { run Hailstone; }
r_Hailstone2() { run Hailstone2 10000; }
r_Hailstone3() { run Hailstone3 10000; }
r_hashcode() { run hashcode $L; }
r_hashcode2() { run hashcode2 $L; }
r_holy_grail() {
    printf 'Rich\ngrail\n800\n' > "$TMP/hg1"
    printf 'Rich\ngrail\nEuropean or African\n' > "$TMP/hg2"
    STDIN="$TMP/hg1" run holy_grail
    STDIN="$TMP/hg2" run holy_grail
}
r_InsertSorted() { run InsertSorted; }
r_md5() { run md5 $L; }
r_Pi() { run Pi 11; }
r_PI_spigot() { run PI_spigot 1000; }
r_progress_bar() { run progress_bar $ND; }
r_progress_bar2() { run progress_bar2 $ND; }
r_Quadratic() { run Quadratic 1 -1 -2; run Quadratic 1 -2 2; }
r_QuickSort() { run QuickSort; }
r_QuickSortFloat() { run QuickSortFloat; }
r_rainbow() { run rainbow; }
r_Renard() { run Renard 20; }
r_Renard2() { run Renard2 20; }
r_rot13() { run rot13 $L output/rot13.cipher; run rot13 output/rot13.cipher output/rot13.out2; }
r_Spinner() { run Spinner $ND; }
r_Tower_of_Hanoi() { run Tower_of_Hanoi 10; }
r_Unicode() { run Unicode; }
r_Vigenere() { run Vigenere password $L output/Vigenere.cipher; run Vigenere -password output/Vigenere.cipher output/Vigenere.out2; }
r_working_bar() { run working_bar $ND; }
r_XORCipher() { run XORCipher password $L output/XORCipher.cipher; run XORCipher password output/XORCipher.cipher output/XORCipher.out2; }

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
project progress_bar r_progress_bar
project progress_bar2 r_progress_bar2
project Quadratic r_Quadratic
project_checked QuickSort r_QuickSort check_sort 15 30 INT
project_checked QuickSortFloat r_QuickSortFloat check_sort 9 14 FLOAT
project rainbow r_rainbow
project Renard r_Renard
project Renard2 r_Renard2
project rot13 r_rot13 rot13.cipher:rot13.cipher rot13.out2:rot13.out
project Spinner r_Spinner
project Tower_of_Hanoi r_Tower_of_Hanoi
project Unicode r_Unicode
project Vigenere r_Vigenere Vigenere.cipher:Vigenere.cipher Vigenere.out2:Vigenere.out
project working_bar r_working_bar
project XORCipher r_XORCipher XORCipher.cipher:XORCipher.cipher XORCipher.out2:XORCipher.out

echo "----------------------------------------"
echo "passed: $pass  failed: $fail"
for f in "${failed[@]}"; do echo "  failed: $f"; done
[[ $fail -eq 0 ]]
