#!/usr/bin/env bash
###############################################################################
# run_tests.sh - regression tests for the MMIX translator
#
# Usage: tests/run_tests.sh [filter]
#
# Test layout (relative to this directory):
#   asm/NAME.mms + NAME.dump   the assembled image (mmixc --dump) must equal
#                              NAME.dump, which oracle.sh makes from mmixal's
#                              .mmo file
#   errors/NAME.mms + NAME.err translation must fail and stderr must contain
#                              the line in NAME.err
#   run/NAME.mms               translated, assembled and run on CPUSim64;
#              + NAME.expected program stdout must equal NAME.expected
#              [+ NAME.exit]   expected exit code (low 8 bits of $255 at Halt,
#                              as the real mmix returns; default 0)
#              [+ NAME.argv]   command line arguments for the program
#              [+ NAME.stdin]  standard input for the program
#   multi/NAME/files           a program of several source files (see the multi-file
#                              tests below): image, errors and run, as above
#
# Environment:
#   MMIXC=native   use the native development build of the translator
#                  (build.sh --native) instead of the CPUSim64 build
#   SIM=java       run the translated programs on the Java simulator
#                  (default: the native cpusim64_native)
###############################################################################

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MMIX_DIR="$(dirname "$HERE")"
ROOT="$(cd "$MMIX_DIR/../.." && pwd)"
JAR="$(ls "$ROOT"/lib/cpusim64-*.jar | head -1)"
NATIVE="$ROOT/bin/cpusim64_native"
FILTER="${1:-}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
export CPUSIM64_CACHE="$TMP/cache"

pass=0
fail=0
failed=()
ok()  { pass=$((pass + 1)); echo "PASS  $1"; }
bad() { fail=$((fail + 1)); failed+=("$1"); echo "FAIL  $1: $2"; }
want() { [[ -z "$FILTER" || "$1" == *"$FILTER"* ]]; }

# sim OBJ ARGS... : runs an object file; $TMP/out = program stdout,
# $TMP/err = stderr, $TMP/result = program result
sim() {
    local obj="$1"; shift
    local input="/dev/null"
    if [[ -n "$SIM_STDIN" ]]; then input="$SIM_STDIN"; fi
    if [[ "$SIM" == "java" ]]; then
        java -Dfile.encoding=UTF8 -cp "$JAR" cloud.lesh.CPUSim64.Simulation \
            "$obj" --verbose --mem=16M --stack=256K "$@" < "$input" > "$TMP/raw" 2> "$TMP/err"
    else
        "$NATIVE" "$obj" --verbose --mem=16M --stack=256K "$@" < "$input" > "$TMP/raw" 2> "$TMP/err"
    fi
    perl -e '
        local $/; my $s = <STDIN>;
        $s =~ s/\A.*?^Read \d+ words from [^\n]*\n//ms;
        my $res = "none";
        if ($s =~ /\A(.*)Result: (-?\d+)\n.*\z/s) { $s = $1; $res = $2; }
        open(my $o, ">", $ARGV[0]); print $o $s; close $o;
        open(my $r, ">", $ARGV[1]); print $r "$res\n"; close $r;
    ' "$TMP/out" "$TMP/result" < "$TMP/raw"
}

# srcmap_ok ASM SRCMAP MMS : the lines named by the .source directives of ASM
# are the lines that SRCMAP gives for the absolute path of MMS
srcmap_ok() {
    perl -e '
        use Cwd qw(abs_path);
        my ($asm, $map, $mms) = @ARGV;
        my $want = abs_path($mms);
        my (%a, %m);
        open(my $f, "<", $asm) or die; while (<$f>) { $a{$1} = 1 if /^\.source\s+"[^"]*",\s*(\d+)/; } close $f;
        open($f, "<", $map) or do { print "no $map\n"; exit 1 };
        while (<$f>) { if (/\xc2\xab(.*)\xc2\xbb:(\d+)/) { my ($p, $l) = ($1, $2); $m{$l} = 1 if (abs_path($p) // "") eq $want; } }
        my @a = sort { $a <=> $b } keys %a; my @m = sort { $a <=> $b } keys %m;
        if (!@a || "@a" ne "@m") { print "asm lines @a[0..($#a<9?$#a:9)], srcmap lines @m[0..($#m<9?$#m:9)]\n"; exit 1 }
    ' "$1" "$2" "$3" > "$TMP/srcmap_err" 2>&1
}

# ---------------------------------------------------------------- translator
if [[ "$MMIXC" == "native" ]]; then
    "$MMIX_DIR/build.sh" --native > /dev/null || { echo "FATAL: native build failed"; exit 1; }
    mmixc() { "$MMIX_DIR/obj/mmixc-native" "$@" > "$TMP/cout" 2> "$TMP/cerr"; }
else
    if [[ ! -f "$MMIX_DIR/obj/mmixc.o64" || "$MMIX_DIR/mmixc.asm" -nt "$MMIX_DIR/obj/mmixc.o64" ]]; then
        "$ROOT/assemble.sh" "$MMIX_DIR/mmixc" > "$TMP/asmlog" 2>&1
        [[ -f "$MMIX_DIR/obj/mmixc.o64" ]] || { cat "$TMP/asmlog"; echo "FATAL: mmixc.asm does not assemble"; exit 1; }
    fi
    mmixc() {
        "$NATIVE" "$MMIX_DIR/obj/mmixc.o64" --verbose --mem=16M --stack=256K "$@" > "$TMP/craw" 2> "$TMP/cerr"
        perl -e '
            local $/; my $s = <STDIN>;
            $s =~ s/\A.*?^Read \d+ words from [^\n]*\n//ms;
            my $res = 1;
            if ($s =~ /\A(.*)Result: (-?\d+)\n.*\z/s) { $s = $1; $res = $2; }
            print $s; exit($res ? 1 : 0);
        ' < "$TMP/craw" > "$TMP/cout"
    }
fi

# ---------------------------------------------------------------- asm tests
for t in "$HERE"/asm/*.mms; do
    [[ -e "$t" ]] || continue
    name="asm/$(basename "$t" .mms)"
    want "$name" || continue
    base="${t%.mms}"
    if ! mmixc --dump "$t"; then
        bad "$name" "translator failed: $(head -2 "$TMP/cerr")"
    elif ! diff -q "$TMP/cout" "$base.dump" > /dev/null; then
        bad "$name" "image differs from mmixal"
        diff "$base.dump" "$TMP/cout" | head -10
    else
        ok "$name"
    fi
done

# ---------------------------------------------------------------- error tests
for t in "$HERE"/errors/*.mms; do
    [[ -e "$t" ]] || continue
    name="errors/$(basename "$t" .mms)"
    want "$name" || continue
    base="${t%.mms}"
    expect="$(cat "$base.err")"
    if mmixc "$t"; then
        bad "$name" "translator succeeded but should fail"
    elif ! grep -qF -- "$expect" "$TMP/cerr" "$TMP/cout"; then
        bad "$name" "missing '$expect', got: $(head -2 "$TMP/cerr")"
    else
        ok "$name"
    fi
done

# ---------------------------------------------------------------- run tests
for t in "$HERE"/run/*.mms; do
    [[ -e "$t" ]] || continue
    name="run/$(basename "$t" .mms)"
    want "$name" || continue
    base="${t%.mms}"
    rm -f "$base.asm" "$HERE/run/obj/$(basename "$base").o64"
    if ! mmixc "$t" || [[ ! -f "$base.asm" ]]; then
        bad "$name" "translation failed: $(head -3 "$TMP/cerr")"
        continue
    fi
    "$ROOT/assemble.sh" "$base" > "$TMP/asmlog" 2>&1
    if [[ ! -f "$HERE/run/obj/$(basename "$base").o64" ]]; then
        bad "$name" "generated assembly failed: $(grep -i error "$TMP/asmlog" | head -3)"
        continue
    fi
    # the source map gives the .mms line of the instructions (.source directives)
    if ! srcmap_ok "$base.asm" "$HERE/run/obj/$(basename "$base").srcmap" "$t"; then
        bad "$name" "source map: $(cat "$TMP/srcmap_err")"
        continue
    fi
    argv=""
    [[ -e "$base.argv" ]] && read -r argv < "$base.argv"
    stdin=""
    [[ -e "$base.stdin" ]] && stdin="$base.stdin"
    ( cd "$HERE/run" && SIM_STDIN="$stdin" sim "$HERE/run/obj/$(basename "$base").o64" $argv )
    expexit=0
    [[ -e "$base.exit" ]] && read -r expexit < "$base.exit"
    res="$(cat "$TMP/result")"
    [[ "$res" =~ ^-?[0-9]+$ ]] && res=$(( res & 255 ))
    if ! diff -q "$TMP/out" "$base.expected" > /dev/null; then
        bad "$name" "output differs"
        diff "$base.expected" "$TMP/out" | head -20
        head -5 "$TMP/err"
    elif [[ "$res" != "$expexit" ]]; then
        bad "$name" "exit code $res, expected $expexit; $(head -3 "$TMP/err")"
    else
        ok "$name"
    fi
done

# ---------------------------------------------------------------- multi-file tests
# multi/NAME/files   the source files (one per line, in order; the first names the .asm)
#           all.mms + all.dump   [optional] the files as one file (with PREFIX ":" after each
#                                library) and mmixal's image of it: mmixc --dump of the files must equal it
#           expected [+ exit]    [optional] the program is translated, assembled and run
#           err                  [optional] translation must fail and stderr must contain this line
# The .source directives of the generated assembly must name every file, with line numbers that exist.
srcfiles_ok() {
    perl -e '
        use File::Basename;
        my ($asm, @files) = @ARGV;
        my $dir = dirname($asm);
        my %seen;
        open(my $f, "<", $asm) or die "no $asm\n";
        while (<$f>) {
            next unless /^\.source\s+"([^"]*)",\s*(\d+)/;
            my ($p, $l) = ($1, $2);
            my $path = "$dir/$p";
            -f $path or die "missing source file $p\n";
            open(my $s, "<", $path) or die; my $n = 0; $n++ while <$s>; close $s;
            $l <= $n or die "$p has $n lines, not $l\n";
            $seen{basename($p)} = 1;
        }
        for (@files) { $seen{basename($_)} or die "no .source for $_\n"; }
    ' "$@" > "$TMP/srcmap_err" 2>&1
}

for d in "$HERE"/multi/*/; do
    [[ -d "$d" ]] || continue
    d="${d%/}"
    name="multi/$(basename "$d")"
    want "$name" || continue
    files=()
    while read -r f; do [[ -n "$f" ]] && files+=("$d/$f"); done < "$d/files"
    first="${files[0]}"
    base="${first%.mms}"
    if [[ -e "$d/err" ]]; then
        expect="$(cat "$d/err")"
        if mmixc "${files[@]}"; then
            bad "$name" "translator succeeded but should fail"
        elif ! grep -qF -- "$expect" "$TMP/cerr" "$TMP/cout"; then
            bad "$name" "missing '$expect', got: $(head -2 "$TMP/cerr")"
        else
            ok "$name"
        fi
        continue
    fi
    if [[ -e "$d/all.dump" ]]; then
        if ! mmixc --dump "${files[@]}"; then
            bad "$name" "translator failed: $(head -2 "$TMP/cerr")"
            continue
        elif ! diff -q "$TMP/cout" "$d/all.dump" > /dev/null; then
            bad "$name" "image differs from mmixal's image of all.mms"
            diff "$d/all.dump" "$TMP/cout" | head -10
            continue
        fi
    fi
    if [[ -e "$d/expected" ]]; then
        rm -f "$base.asm" "$d/obj/$(basename "$base").o64"
        if ! mmixc "${files[@]}" || [[ ! -f "$base.asm" ]]; then
            bad "$name" "translation failed: $(head -3 "$TMP/cerr")"
            continue
        fi
        "$ROOT/assemble.sh" "$base" > "$TMP/asmlog" 2>&1
        if [[ ! -f "$d/obj/$(basename "$base").o64" ]]; then
            bad "$name" "generated assembly failed: $(grep -i error "$TMP/asmlog" | head -3)"
            continue
        fi
        if ! srcfiles_ok "$base.asm" "${files[@]}"; then
            bad "$name" "source directives: $(cat "$TMP/srcmap_err")"
            continue
        fi
        ( cd "$d" && SIM_STDIN="" sim "$d/obj/$(basename "$base").o64" )
        expexit=0
        [[ -e "$d/exit" ]] && read -r expexit < "$d/exit"
        res="$(cat "$TMP/result")"
        [[ "$res" =~ ^-?[0-9]+$ ]] && res=$(( res & 255 ))
        if ! diff -q "$TMP/out" "$d/expected" > /dev/null; then
            bad "$name" "output differs"
            diff "$d/expected" "$TMP/out" | head -20
            head -5 "$TMP/err"
            continue
        elif [[ "$res" != "$expexit" ]]; then
            bad "$name" "exit code $res, expected $expexit; $(head -3 "$TMP/err")"
            continue
        fi
    fi
    ok "$name"
done

echo "----------------------------------------"
echo "passed: $pass  failed: $fail"
for f in "${failed[@]}"; do echo "  failed: $f"; done
[[ $fail -eq 0 ]]
