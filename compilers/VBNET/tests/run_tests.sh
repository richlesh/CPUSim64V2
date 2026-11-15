#!/usr/bin/env bash
###############################################################################
# run_tests.sh - regression tests for the VB.NET compiler
#
# Usage: tests/run_tests.sh [filter]
#
# Test layout (relative to this directory):
#   dump/NAME.vb + NAME.args    the compiler is run with the options in
#              + NAME.expected  NAME.args (e.g. --tokens); its stdout, without
#                               the banner, must equal NAME.expected
#   errors/NAME.vb + NAME.err   compilation must fail and stderr must contain
#                               the line in NAME.err
#   run/NAME.vb                 compiled, assembled and run on CPUSim64;
#              + NAME.expected  program stdout must equal NAME.expected
#              [+ NAME.exit]    expected exit code (default 0)
#              [+ NAME.argv]    command line arguments for the program
#              [+ NAME.stdin]   standard input for the program
#              [+ NAME.vbcargs] extra compiler arguments (more .vb files,
#                               relative to run/, or options)
#              [+ NAME.stderr]  a line that the program's stderr must contain
#              [+ NAME.mem]     the simulator's memory size (default 16M)
#
# Environment:
#   VBC=native   use the native development build of the compiler
#                (build.sh --native) instead of the CPUSim64 build
#   SIM=java     run the compiled programs on the Java simulator
#                (default: the native cpusim64_native)
#   GC_STRESS=1  compile the run tests with --gc-stress (a garbage
#                collection at every allocation)
###############################################################################

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VB_DIR="$(dirname "$HERE")"
ROOT="$(cd "$VB_DIR/../.." && pwd)"
JAR="$(ls "$ROOT"/lib/cpusim64-*.jar | grep -v ' ' | head -1)"
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

# strip_run : removes the simulator's start-up lines and the "Result: N"
# line from $TMP/raw; writes $TMP/out and $TMP/result
strip_run() {
    perl -e '
        local $/; my $s = <STDIN>;
        $s =~ s/\A.*?^Read \d+ words from [^\n]*\n//ms;
        my $res = "none";
        if ($s =~ /\A(.*)Result: (-?\d+)\n.*\z/s) { $s = $1; $res = $2; }
        open(my $o, ">", $ARGV[0]); print $o $s; close $o;
        open(my $r, ">", $ARGV[1]); print $r "$res\n"; close $r;
    ' "$TMP/out" "$TMP/result" < "$TMP/raw"
}

# sim OBJ ARGS... : runs an object file; $TMP/out = program stdout,
# $TMP/err = stderr, $TMP/result = program result
sim() {
    local obj="$1"; shift
    local input="/dev/null"
    if [[ -n "$SIM_STDIN" ]]; then input="$SIM_STDIN"; fi
    local mem="${SIM_MEM:-16M}"
    if [[ "$SIM" == "java" ]]; then
        java -Dfile.encoding=UTF8 -cp "$JAR" cloud.lesh.CPUSim64.Simulation \
            "$obj" --verbose --mem=$mem --stack=256K "$@" < "$input" > "$TMP/raw" 2> "$TMP/err"
    else
        "$NATIVE" "$obj" --verbose --mem=$mem --stack=256K "$@" < "$input" > "$TMP/raw" 2> "$TMP/err"
    fi
    strip_run
}

# srcmap_ok ASM SRCMAP VB : the lines named by the .source directives of ASM
# for VB are the lines that SRCMAP gives for the absolute path of VB (so the
# debugger shows the .vb lines)
srcmap_ok() {
    perl -e '
        use Cwd qw(abs_path);
        use File::Basename;
        my ($asm, $map, $vb) = @ARGV;
        my $want = abs_path($vb);
        my $base = basename($vb);
        my (%a, %m);
        open(my $f, "<", $asm) or die; while (<$f>) { $a{$1} = 1 if /^\.source\s+"([^"]*)",\s*(\d+)/ && (basename($1) eq $base) && ($a{$2} = 1) && 0; } close $f;
        open($f, "<", $asm) or die; while (<$f>) { if (/^\.source\s+"([^"]*)",\s*(\d+)/) { $a{$2} = 1 if basename($1) eq $base; } } close $f;
        open($f, "<", $map) or do { print "no $map\n"; exit 1 };
        while (<$f>) { if (/\xc2\xab(.*)\xc2\xbb:(\d+)/) { my ($p, $l) = ($1, $2); $m{$l} = 1 if (abs_path($p) // "") eq $want; } }
        my @a = sort { $a <=> $b } keys %a; my @m = sort { $a <=> $b } keys %m;
        if (!@a || "@a" ne "@m") { print "asm lines @a[0..($#a<9?$#a:9)], srcmap lines @m[0..($#m<9?$#m:9)]\n"; exit 1 }
    ' "$1" "$2" "$3" > "$TMP/srcmap_err" 2>&1
}

# ---------------------------------------------------------------- compiler
# vbc ARGS... : runs the compiler; $TMP/cout = stdout without the banner,
# $TMP/cerr = stderr; the status is 0 on success
if [[ "$VBC" == "native" ]]; then
    "$VB_DIR/build.sh" --native > /dev/null || { echo "FATAL: native build failed"; exit 1; }
    vbc_raw() { "$VB_DIR/obj/vbc-native" "$@" > "$TMP/craw" 2> "$TMP/cerr"; }
else
    if [[ ! -f "$VB_DIR/obj/vbc.o64" || "$VB_DIR/vbc.asm" -nt "$VB_DIR/obj/vbc.o64" ]]; then
        "$ROOT/assemble.sh" "$VB_DIR/vbc" > "$TMP/asmlog" 2>&1
        [[ -f "$VB_DIR/obj/vbc.o64" ]] || { cat "$TMP/asmlog"; echo "FATAL: vbc.asm does not assemble"; exit 1; }
    fi
    vbc_raw() {
        "$NATIVE" "$VB_DIR/obj/vbc.o64" --verbose --mem=16M --stack=256K "$@" > "$TMP/craw2" 2> "$TMP/cerr"
        perl -e '
            local $/; my $s = <STDIN>;
            $s =~ s/\A.*?^Read \d+ words from [^\n]*\n//ms;
            my $res = 1;
            if ($s =~ /\A(.*)Result: (-?\d+)\n.*\z/s) { $s = $1; $res = $2; }
            print $s; exit($res ? 1 : 0);
        ' < "$TMP/craw2" > "$TMP/craw"
    }
fi
vbc() {
    vbc_raw "$@"
    local st=$?
    # drop the banner: everything up to and including the "Arguments:" line
    perl -e 'local $/; my $s = <STDIN>; $s =~ s/\A.*?^Arguments:[^\n]*\n//ms; print $s;' \
        < "$TMP/craw" > "$TMP/cout"
    return $st
}

# ---------------------------------------------------------------- dump tests
for t in "$HERE"/dump/*.vb; do
    [[ -e "$t" ]] || continue
    name="dump/$(basename "$t" .vb)"
    want "$name" || continue
    base="${t%.vb}"
    args=""
    [[ -e "$base.args" ]] && read -r args < "$base.args"
    if ! vbc $args "$t"; then
        bad "$name" "compiler failed: $(head -2 "$TMP/cerr")"
    elif ! diff -q "$TMP/cout" "$base.expected" > /dev/null; then
        bad "$name" "output differs"
        diff "$base.expected" "$TMP/cout" | head -20
    else
        ok "$name"
    fi
done

# ---------------------------------------------------------------- error tests
for t in "$HERE"/errors/*.vb; do
    [[ -e "$t" ]] || continue
    name="errors/$(basename "$t" .vb)"
    want "$name" || continue
    base="${t%.vb}"
    expect="$(cat "$base.err")"
    if vbc "$t"; then
        bad "$name" "compiler succeeded but should fail"
    elif ! grep -qF -- "$expect" "$TMP/cerr" "$TMP/cout"; then
        bad "$name" "missing '$expect', got: $(head -2 "$TMP/cerr")"
    else
        ok "$name"
    fi
done

# ---------------------------------------------------------------- run tests
for t in "$HERE"/run/*.vb; do
    [[ -e "$t" ]] || continue
    name="run/$(basename "$t" .vb)"
    want "$name" || continue
    base="${t%.vb}"
    [[ -e "$base.expected" ]] || continue          # an extra file of a multi-file test
    rm -f "$base.asm" "$HERE/run/obj/$(basename "$base").o64"
    vbcargs=""
    [[ -e "$base.vbcargs" ]] && read -r vbcargs < "$base.vbcargs"
    stress=""
    [[ -n "$GC_STRESS" ]] && stress="--gc-stress"
    if ! ( cd "$HERE/run" && vbc "$t" $vbcargs $stress ) || [[ ! -f "$base.asm" ]]; then
        bad "$name" "compilation failed: $(head -3 "$TMP/cerr")"
        continue
    fi
    "$ROOT/assemble.sh" "$base" > "$TMP/asmlog" 2>&1
    if [[ ! -f "$HERE/run/obj/$(basename "$base").o64" ]]; then
        bad "$name" "generated assembly failed: $(grep -i error "$TMP/asmlog" | head -3)"
        continue
    fi
    # the source map gives the .vb line of the instructions (.source directives)
    if ! srcmap_ok "$base.asm" "$HERE/run/obj/$(basename "$base").srcmap" "$t"; then
        bad "$name" "source map: $(cat "$TMP/srcmap_err")"
        continue
    fi
    argv=""
    [[ -e "$base.argv" ]] && read -r argv < "$base.argv"
    stdin=""
    [[ -e "$base.stdin" ]] && stdin="$base.stdin"
    mem=""
    [[ -e "$base.mem" ]] && read -r mem < "$base.mem"
    ( cd "$HERE/run" && SIM_STDIN="$stdin" SIM_MEM="$mem" sim "$HERE/run/obj/$(basename "$base").o64" $argv )
    expexit=0
    [[ -e "$base.exit" ]] && read -r expexit < "$base.exit"
    res="$(cat "$TMP/result")"
    if ! diff -q "$TMP/out" "$base.expected" > /dev/null; then
        bad "$name" "output differs"
        diff "$base.expected" "$TMP/out" | head -20
        head -5 "$TMP/err"
    elif [[ "$res" != "$expexit" ]]; then
        bad "$name" "exit code $res, expected $expexit; $(head -3 "$TMP/err")"
    elif [[ -e "$base.stderr" ]] && ! grep -qF -- "$(cat "$base.stderr")" "$TMP/err"; then
        bad "$name" "stderr does not contain '$(cat "$base.stderr")': $(head -3 "$TMP/err")"
    else
        ok "$name"
    fi
done

echo "----------------------------------------"
echo "passed: $pass  failed: $fail"
for f in "${failed[@]}"; do echo "  failed: $f"; done
[[ $fail -eq 0 ]]
