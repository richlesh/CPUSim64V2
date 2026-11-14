#!/usr/bin/env bash
###############################################################################
# run_tests.sh - regression tests for the CPUSim64 C compiler
#
# Usage: tests/run_tests.sh [filter]
#
# Test layout (all relative to this directory):
#   unit/NAME.asm             stand-alone assembly unit test; must print
#                             "ALL PASS" and no line containing "FAIL"
#   dump/NAME.c + NAME.args   compiler is run with the flags in NAME.args;
#              + NAME.expected  its stdout must equal NAME.expected
#   run/NAME.c                compiled, assembled and executed;
#              + NAME.expected  program stdout must equal NAME.expected
#              [+ NAME.exit]    expected program result (default 0)
#              [+ NAME.argv]    extra command line arguments for the program
#              [+ NAME.stdin]   standard input for the program
#              [+ NAME.ccargs]  extra compiler arguments ($HERE expands to
#                               this directory); with more .c files (kept in
#                               multi/) or -lstd the compiler writes modules,
#                               which are assembled and linked with -r
#              [+ NAME.noasm]   strings (one per line) that must not appear
#                               in the generated assembly
#   errors/NAME.c + NAME.err  compilation must fail (non-zero result) and the
#                             compiler's stderr must contain NAME.err
#              [+ NAME.ccargs]  extra compiler arguments, as for run tests
#
# The simulator always exits with status 0, so the program result is taken
# from the "Result: N" line printed in --verbose mode.
###############################################################################

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export CPUSIM64_TEST_VAR="hello world"   # read by tests/run/lib_getenv.c
CC_DIR="$(dirname "$HERE")"
# CPUSim64 install: $CPUSIM64_HOME, else the nearest ancestor of the compiler
# directory that contains assemble.sh and lib/cpusim64-*.jar
ROOT="${CPUSIM64_HOME:-}"
if [[ -z "$ROOT" ]]; then
    d="$CC_DIR"
    while [[ "$d" != "/" ]]; do
        d="$(dirname "$d")"
        if [[ -x "$d/assemble.sh" ]] && ls "$d"/lib/cpusim64-*.jar > /dev/null 2>&1; then
            ROOT="$d"
            break
        fi
    done
fi
if [[ -z "$ROOT" ]]; then
    echo "FATAL: CPUSim64 install not found; set CPUSIM64_HOME"
    exit 1
fi
JAR="$(ls "$ROOT"/lib/cpusim64-*.jar | head -1)"
MEM=16M
STACK=256K
FILTER="${1:-}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

pass=0
fail=0
failed=()

# sim OBJ ARGS... : run object file; writes $TMP/out (program stdout),
# $TMP/err (stderr) and $TMP/result (program result)
sim() {
    local obj="$1"; shift
    local input="/dev/null"
    if [[ -n "$SIM_STDIN" ]]; then input="$SIM_STDIN"; fi
    java -Dfile.encoding=UTF8 -cp "$JAR" cloud.lesh.CPUSim64.Simulation \
        "$obj" --verbose --mem=$MEM --stack=$STACK "$@" \
        < "$input" > "$TMP/raw" 2> "$TMP/err"
    perl -e '
        local $/; my $s = <STDIN>;
        $s =~ s/\A.*?^Read \d+ words from [^\n]*\n//ms;
        my $res = "none";
        if ($s =~ /\A(.*)Result: (-?\d+)\n.*\z/s) { $s = $1; $res = $2; }
        open(my $o, ">", $ARGV[0]); print $o $s; close $o;
        open(my $r, ">", $ARGV[1]); print $r "$res\n"; close $r;
    ' "$TMP/out" "$TMP/result" < "$TMP/raw"
}

# srcmap_ok ASM SRCMAP C : the source map has lines of C (by absolute path),
# and each of them is a line named by a .source directive of ASM
srcmap_ok() {
    perl -e '
        use Cwd qw(abs_path);
        use File::Basename qw(dirname);
        my ($asm, $map, $c) = @ARGV;
        my $want = abs_path($c);
        my $dir = dirname($asm);
        my (%a, %m);
        open(my $f, "<", $asm) or die;
        while (<$f>) {
            next unless /^\.source\s+"((?:[^"\\]|\\.)*)",\s*(\d+)/;
            my ($p, $l) = ($1, $2); $p =~ s/\\(.)/$1/g;
            $p = "$dir/$p" unless $p =~ m{^/};
            $a{$l} = 1 if (abs_path($p) // "") eq $want;
        }
        close $f;
        open($f, "<", $map) or do { print "no $map\n"; exit 1 };
        while (<$f>) { if (/\xc2\xab(.*)\xc2\xbb:(\d+)/) { $m{$2} = 1 if (abs_path($1) // "") eq $want; } }
        my @m = sort { $a <=> $b } keys %m;
        my @x = grep { !$a{$_} } @m;
        if (!@m) { print "no lines of $c\n"; exit 1 }
        if (@x) { print "lines @x are not .source lines\n"; exit 1 }
    ' "$1" "$2" "$3" > "$TMP/srcmap_err" 2>&1
}

# assemble BASE_WITHOUT_EXT : returns 0 if the object file was produced
assemble() {
    local base="$1"
    local dir; dir="$(dirname "$base")"
    rm -f "$dir/obj/$(basename "$base").o64"
    "$ROOT/assemble.sh" "$base" > "$TMP/asmlog" 2>&1
    [[ -e "$dir/obj/$(basename "$base").o64" ]]
}

ok()   { pass=$((pass+1)); echo "PASS  $1"; }
bad()  { fail=$((fail+1)); failed+=("$1"); echo "FAIL  $1: $2"; }

want() { [[ -z "$FILTER" || "$1" == *"$FILTER"* ]]; }

# ---------------------------------------------------------------- build cc
if ! assemble "$CC_DIR/cc"; then
    cat "$TMP/asmlog"
    echo "FATAL: compiler failed to assemble"
    exit 1
fi
CC_OBJ="$CC_DIR/obj/cc.o64"

# ---------------------------------------------------------------- cli tests
# cli_case NAME EXPECTED_RESULT STDERR_SUBSTRING ARGS...
cli_case() {
    local name="cli/$1" eres="$2" estr="$3"; shift 3
    want "$name" || return
    SIM_STDIN="" sim "$CC_OBJ" "$@"
    local res; res="$(cat "$TMP/result")"
    if [[ "$res" != "$eres" ]]; then
        bad "$name" "result $res, expected $eres"
    elif ! grep -qF -- "$estr" "$TMP/err"; then
        bad "$name" "stderr missing '$estr'"
    else
        ok "$name"
    fi
}
cli_case missing_arg  2 "usage: cc input.c"
cli_case missing_file 1 "cannot open file: $HERE/no_such_file.c" "$HERE/no_such_file.c"
cli_case bad_option   1 "unknown option: --bogus" "$HERE/cli/empty.c" --bogus
cli_case empty_file   1 "no 'main' function" "$HERE/cli/empty.c"

# ---------------------------------------------------------------- unit tests
for t in "$HERE"/unit/*.asm; do
    [[ -e "$t" ]] || continue
    name="unit/$(basename "$t" .asm)"
    want "$name" || continue
    base="${t%.asm}"
    if ! assemble "$base"; then bad "$name" "assembly failed"; continue; fi
    sim "$HERE/unit/obj/$(basename "$base").o64"
    if grep -q "FAIL" "$TMP/out" || ! grep -q "ALL PASS" "$TMP/out"; then
        bad "$name" "$(grep FAIL "$TMP/out" | head -3)"
    else
        ok "$name"
    fi
done

# ---------------------------------------------------------------- dump tests
for t in "$HERE"/dump/*.c; do
    [[ -e "$t" ]] || continue
    name="dump/$(basename "$t" .c)"
    want "$name" || continue
    base="${t%.c}"
    read -r flags < "$base.args"
    SIM_STDIN="" sim "$CC_OBJ" "$t" $flags
    if diff -q "$TMP/out" "$base.expected" > /dev/null; then
        ok "$name"
    else
        bad "$name" "output differs"
        diff "$base.expected" "$TMP/out" | head -20
        cat "$TMP/err" | head -5
    fi
done

# ---------------------------------------------------------------- error tests
for t in "$HERE"/errors/*.c; do
    [[ -e "$t" ]] || continue
    name="errors/$(basename "$t" .c)"
    want "$name" || continue
    base="${t%.c}"
    ccargs=""
    [[ -e "$base.ccargs" ]] && read -r ccargs < "$base.ccargs"
    ccargs="${ccargs//\$HERE/$HERE}"
    SIM_STDIN="" sim "$CC_OBJ" "$t" $ccargs
    res="$(cat "$TMP/result")"
    expect="$(cat "$base.err")"
    if [[ "$res" == "0" ]]; then
        bad "$name" "compiler succeeded but should fail"
    elif ! grep -qF -- "$expect" "$TMP/err"; then
        bad "$name" "stderr missing '$expect', got: $(head -2 "$TMP/err")"
    else
        ok "$name"
    fi
done

# ---------------------------------------------------------------- run tests
for t in "$HERE"/run/*.c; do
    [[ -e "$t" ]] || continue
    name="run/$(basename "$t" .c)"
    want "$name" || continue
    base="${t%.c}"
    rm -f "$base.asm"
    ccargs=""
    [[ -e "$base.ccargs" ]] && read -r ccargs < "$base.ccargs"
    ccargs="${ccargs//\$HERE/$HERE}"
    SIM_STDIN="" sim "$CC_OBJ" "$t" $ccargs
    if [[ "$(cat "$TMP/result")" != "0" || ! -e "$base.asm" ]]; then
        bad "$name" "compile failed: $(head -3 "$TMP/err")"
        continue
    fi
    if [[ -e "$base.noasm" ]]; then
        hit=""
        while IFS= read -r pat; do
            [[ -z "$pat" ]] && continue
            if grep -qF -- "$pat" "$base.asm"; then hit="$pat"; break; fi
        done < "$base.noasm"
        if [[ -n "$hit" ]]; then
            bad "$name" "generated assembly contains '$hit'"
            continue
        fi
    fi
    if [[ "$ccargs" == *.c* || "$ccargs" == *-lstd* ]]; then
        # modules: assemble the first one with -r and link it with the others
        rm -f "$HERE/run/obj/$(basename "$base").o64"
        CPUSIM64_CACHE="$TMP/cache" "$ROOT/assemble.sh" -r "$base.asm" \
            -o "$HERE/run/obj/$(basename "$base").o64" > "$TMP/asmlog" 2>&1
        if [[ ! -e "$HERE/run/obj/$(basename "$base").o64" ]]; then
            bad "$name" "linking failed: $(grep -i error "$TMP/asmlog" | head -3)"
            continue
        fi
    elif ! assemble "$base"; then
        bad "$name" "generated assembly failed: $(grep -i error "$TMP/asmlog" | head -3)"
        continue
    fi
    # the source map gives the .c lines of the code (.source directives)
    if ! srcmap_ok "$base.asm" "$HERE/run/obj/$(basename "$base").srcmap" "$t"; then
        bad "$name" "source map: $(cat "$TMP/srcmap_err")"
        continue
    fi
    argv=""
    [[ -e "$base.argv" ]] && read -r argv < "$base.argv"
    stdin=""
    [[ -e "$base.stdin" ]] && stdin="$base.stdin"
    SIM_STDIN="$stdin" sim "$HERE/run/obj/$(basename "$base").o64" $argv
    expexit=0
    [[ -e "$base.exit" ]] && read -r expexit < "$base.exit"
    res="$(cat "$TMP/result")"
    if ! diff -q "$TMP/out" "$base.expected" > /dev/null; then
        bad "$name" "output differs"
        diff "$base.expected" "$TMP/out" | head -20
        head -5 "$TMP/err"
    elif [[ "$res" != "$expexit" ]]; then
        bad "$name" "result $res, expected $expexit; $(head -3 "$TMP/err")"
    else
        ok "$name"
    fi
done

echo "----------------------------------------"
echo "passed: $pass  failed: $fail"
for f in "${failed[@]}"; do echo "  failed: $f"; done
[[ $fail -eq 0 ]]
