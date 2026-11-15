#!/usr/bin/env bash
###############################################################################
# run_tests.sh - regression tests for the C++ translator (compilers/cpp)
#
# Usage: tests/run_tests.sh [filter]
#
# Test layout (relative to this directory):
#   dump/NAME.cpp + NAME.args   the translator is run (in this directory) with
#              + NAME.expected  the options in NAME.args (e.g. --tokens); its
#                               stdout, without the banner, must equal
#                               NAME.expected.  With HOST=1 and --preprocess,
#                               the token sequence must also equal that of
#                               g++ -E (ppcmp.py)
#                               (--ast dumps are checked by antlr_check.sh as well)
#   errors/NAME.cpp + NAME.err  translation (or the C11 compilation of its
#              [+ NAME.args]    output) must fail and stderr must contain the
#                               line in NAME.err
#   run/NAME.cpp                translated to NAME.gen.c, compiled by the C11
#              + NAME.expected  compiler, assembled and run on CPUSim64;
#                               program stdout must equal NAME.expected
#              [+ NAME.exit]    expected exit code (default 0)
#              [+ NAME.argv]    command line arguments for the program
#              [+ NAME.stdin]   standard input for the program
#              [+ NAME.cppargs] extra translator arguments
#              [+ NAME.stderr]  a line that the program's stderr must contain
#              [+ NAME.nohost]  the test is not valid for a host C++ compiler
#
# Environment:
#   CPPC=native  use the native development build of the translator
#                (build.sh --native) instead of the CPUSim64 build
#   SIM=java     run on the Java simulator (default: the native cpusim64_native)
#   HOST=1       also compile each run test with the host g++ (-std=c++14, or the
#                standard in NAME.std, e.g. c++17) and
#                check that it prints the .expected output (skips .nohost)
###############################################################################

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPP_DIR="$(dirname "$HERE")"
ROOT="$(cd "$CPP_DIR/../.." && pwd)"
C11="$ROOT/compilers/C11"
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
    if [[ "$SIM" == "java" ]]; then
        java -Dfile.encoding=UTF8 -cp "$JAR" cloud.lesh.CPUSim64.Simulation \
            "$obj" --verbose --mem=64M --stack=1M "$@" < "$input" > "$TMP/raw" 2> "$TMP/err"
    else
        "$NATIVE" "$obj" --verbose --mem=64M --stack=1M "$@" < "$input" > "$TMP/raw" 2> "$TMP/err"
    fi
    strip_run
}

# srcmap_ok ASM SRCMAP CPP : the lines that SRCMAP gives for the absolute path of
# CPP are lines named by .source directives of ASM (so the debugger shows the
# .cpp lines)
srcmap_ok() {
    perl -e '
        use Cwd qw(abs_path);
        use File::Basename;
        my ($asm, $map, $src) = @ARGV;
        my $want = abs_path($src);
        my $base = basename($src);
        my (%a, %m);
        open(my $f, "<", $asm) or die;
        while (<$f>) { if (/^\.source\s+"([^"]*)",\s*(\d+)/) { $a{$2} = 1 if basename($1) eq $base; } }
        close $f;
        open($f, "<", $map) or do { print "no $map\n"; exit 1 };
        while (<$f>) { if (/\xc2\xab(.*)\xc2\xbb:(\d+)/) { my ($p, $l) = ($1, $2); $m{$l} = 1 if (abs_path($p) // "") eq $want; } }
        my @a = sort { $a <=> $b } keys %a; my @m = sort { $a <=> $b } keys %m;
        my @x = grep { !$a{$_} } @m;
        if (!@m) { print "no lines of $src in the source map\n"; exit 1 }
        if (@x) { print "lines @x are not .source lines\n"; exit 1 }
    ' "$1" "$2" "$3" > "$TMP/srcmap_err" 2>&1
}

# ---------------------------------------------------------------- translator
# cppc ARGS... : runs the translator; $TMP/cout = stdout without the banner,
# $TMP/cerr = stderr; the status is 0 on success
if [[ "$CPPC" == "native" ]]; then
    "$CPP_DIR/build.sh" --native > /dev/null || { echo "FATAL: native build failed"; exit 1; }
    cppc_raw() { "$CPP_DIR/obj/cppc-native" "$@" > "$TMP/craw" 2> "$TMP/cerr"; }
else
    if [[ ! -f "$CPP_DIR/obj/cppc.o64" || "$CPP_DIR/cppc.asm" -nt "$CPP_DIR/obj/cppc.o64" ]]; then
        "$ROOT/assemble.sh" "$CPP_DIR/cppc" > "$TMP/asmlog" 2>&1
        [[ -f "$CPP_DIR/obj/cppc.o64" ]] || { cat "$TMP/asmlog"; echo "FATAL: cppc.asm does not assemble"; exit 1; }
    fi
    cppc_raw() {
        "$NATIVE" "$CPP_DIR/obj/cppc.o64" --verbose --mem=64M --stack=1M "$@" > "$TMP/craw2" 2> "$TMP/cerr"
        perl -e '
            local $/; my $s = <STDIN>;
            $s =~ s/\A.*?^Read \d+ words from [^\n]*\n//ms;
            my $res = 1;
            if ($s =~ /\A(.*)Result: (-?\d+)\n.*\z/s) { $s = $1; $res = $2; }
            print $s; exit($res ? 1 : 0);
        ' < "$TMP/craw2" > "$TMP/craw"
    }
fi
cppc() {
    cppc_raw "$@"
    local st=$?
    # drop the banner: everything up to and including the "Arguments:" line
    perl -e 'local $/; my $s = <STDIN>; $s =~ s/\A.*?^Arguments:[^\n]*\n//ms; print $s;' \
        < "$TMP/craw" > "$TMP/cout"
    return $st
}

# the C11 compiler
if [[ ! -f "$C11/obj/cc.o64" || -n "$(find "$C11" -maxdepth 1 \( -name '*.asm' -o -name '*.def' \) -newer "$C11/obj/cc.o64")" ]]; then
    "$ROOT/assemble.sh" "$C11/cc" > "$TMP/asmlog" 2>&1
    [[ -f "$C11/obj/cc.o64" ]] || { cat "$TMP/asmlog"; echo "FATAL: the C11 compiler does not assemble"; exit 1; }
fi
# cc FILE.c : the C11 compiler writes FILE.asm; $TMP/ccerr = its stderr
cc11() {
    "$NATIVE" "$C11/obj/cc.o64" --verbose --mem=64M --stack=1M "$@" > "$TMP/ccraw" 2> "$TMP/ccerr"
    grep -q "^Result: 0" "$TMP/ccraw"
}

# ---------------------------------------------------------------- dump tests
for t in "$HERE"/dump/*.cpp; do
    [[ -e "$t" ]] || continue
    name="dump/$(basename "$t" .cpp)"
    want "$name" || continue
    base="${t%.cpp}"
    args=""
    [[ -e "$base.args" ]] && read -r args < "$base.args"
    if ! ( cd "$HERE" && cppc $args "dump/$(basename "$t")" ); then
        bad "$name" "translator failed: $(head -2 "$TMP/cerr")"
    elif ! diff -q "$TMP/cout" "$base.expected" > /dev/null; then
        bad "$name" "output differs"
        diff "$base.expected" "$TMP/cout" | head -20
    elif [[ -n "$HOST" && "$args" == *--preprocess* ]]; then
        # the same token sequence as the host C++ preprocessor
        hostargs="${args//--preprocess/}"
        if ! ( cd "$HERE" && g++ -std=c++14 -x c++ -E -P $hostargs "dump/$(basename "$t")" > "$TMP/host.pp" 2> "$TMP/hosterr" ); then
            bad "$name" "host g++ failed: $(head -3 "$TMP/hosterr")"
        elif ! python3 "$HERE/ppcmp.py" "$TMP/cout" "$TMP/host.pp" > "$TMP/ppcmp.out"; then
            bad "$name" "differs from g++ -E: $(cat "$TMP/ppcmp.out")"
        else
            ok "$name"
        fi
    else
        ok "$name"
    fi
done

# ---------------------------------------------------------------- error tests
for t in "$HERE"/errors/*.cpp; do
    [[ -e "$t" ]] || continue
    name="errors/$(basename "$t" .cpp)"
    want "$name" || continue
    base="${t%.cpp}"
    expect="$(cat "$base.err")"
    args=""
    [[ -e "$base.args" ]] && read -r args < "$base.args"
    if ( cd "$HERE" && cppc $args "errors/$(basename "$t")" ); then
        # the translator accepted it: the C11 compiler must reject the result
        if cc11 "$base.gen.c"; then
            bad "$name" "compilation succeeded but should fail"
            continue
        fi
        cat "$TMP/ccerr" >> "$TMP/cerr"
    fi
    if ! grep -qF -- "$expect" "$TMP/cerr" "$TMP/cout"; then
        bad "$name" "missing '$expect', got: $(head -2 "$TMP/cerr")"
    else
        ok "$name"
    fi
done

# ---------------------------------------------------------------- run tests
for t in "$HERE"/run/*.cpp; do
    [[ -e "$t" ]] || continue
    name="run/$(basename "$t" .cpp)"
    want "$name" || continue
    base="${t%.cpp}"
    [[ -e "$base.expected" ]] || continue          # an extra file of a multi-file test
    rm -f "$base.gen.c" "$base.gen.asm" "$base.asm" "$HERE/run/obj/$(basename "$base").o64"
    cppargs=""
    [[ -e "$base.cppargs" ]] && read -r cppargs < "$base.cppargs"
    # NAME.units: the other source files of a program of several units
    units=""
    [[ -e "$base.units" ]] && read -r units < "$base.units"
    unitpaths=""
    for u in $units; do unitpaths="$unitpaths $HERE/run/$u"; done
    if ! ( cd "$HERE/run" && cppc "$t" $unitpaths $cppargs ) || [[ ! -f "$base.gen.c" ]]; then
        bad "$name" "translation failed: $(head -3 "$TMP/cerr")"
        continue
    fi
    unitfail=""
    gens=""
    for u in $units; do
        ub="$HERE/run/${u%.cpp}"
        rm -f "$ub.gen.c" "$ub.gen.asm" "$ub.asm"
        if ! ( cd "$HERE/run" && cppc --unit "$HERE/run/$u" $cppargs ) || [[ ! -f "$ub.gen.c" ]]; then unitfail="$u"; break; fi
        gens="$gens $ub.gen.c"
    done
    if [[ -n "$unitfail" ]]; then
        bad "$name" "translation of $unitfail failed: $(head -3 "$TMP/cerr")"
        continue
    fi
    cclib=""
    [[ -n "$units" ]] && cclib="-lstd"          # several modules share the C library
    if ! ( cd "$HERE/run" && cc11 "$base.gen.c" $gens $cclib ) || [[ ! -f "$base.gen.asm" ]]; then
        bad "$name" "C11 compilation of the generated C failed: $(head -3 "$TMP/ccerr")"
        continue
    fi
    # as in the IDE: the C11 compiler's result prog.gen.asm becomes prog.asm
    mv "$base.gen.asm" "$base.asm"
    if [[ -n "$units" ]]; then
        # the main module includes the other modules (they are linked with it): they are renamed too
        for u in $units; do
            ub="$HERE/run/${u%.cpp}"
            mv "$ub.gen.asm" "$ub.asm"
            perl -pi -e 's/\Q'"$(basename "$ub")"'\E\.gen\.asm"/'"$(basename "$ub")"'.asm"/' "$base.asm"
        done
        # modules: the first one is assembled with -r and linked with the others (as the IDE does)
        CPUSIM64_CACHE="$TMP/cache" "$ROOT/assemble.sh" -r "$base.asm" -o "$HERE/run/obj/$(basename "$base").o64" > "$TMP/asmlog" 2>&1
    else
        "$ROOT/assemble.sh" "$base" > "$TMP/asmlog" 2>&1
    fi
    if [[ ! -f "$HERE/run/obj/$(basename "$base").o64" ]]; then
        bad "$name" "generated assembly failed: $(grep -i error "$TMP/asmlog" | head -3)"
        continue
    fi
    # the source map gives the .cpp line of the instructions (.source directives)
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
    if ! diff -q "$TMP/out" "$base.expected" > /dev/null; then
        bad "$name" "output differs"
        diff "$base.expected" "$TMP/out" | head -20
        head -5 "$TMP/err"
    elif [[ "$res" != "$expexit" ]]; then
        bad "$name" "exit code $res, expected $expexit; $(head -3 "$TMP/err")"
    elif [[ -e "$base.stderr" ]] && ! grep -qF -- "$(cat "$base.stderr")" "$TMP/err"; then
        bad "$name" "stderr does not contain '$(cat "$base.stderr")': $(head -3 "$TMP/err")"
    elif [[ -n "$HOST" && ! -e "$base.nohost" ]]; then
        # the same program compiled by the host C++ compiler must print the same
        stdin="/dev/null"
        [[ -e "$base.stdin" ]] && stdin="$base.stdin"
        hoststd=c++14
        [[ -e "$base.std" ]] && read -r hoststd < "$base.std"
        if ! ( cd "$HERE/run" && g++ -std=$hoststd -w -o "$TMP/host.bin" "$t" $units ) 2> "$TMP/hosterr"; then
            bad "$name" "host g++ failed: $(head -3 "$TMP/hosterr")"
        else
            ( cd "$HERE/run" && "$TMP/host.bin" $argv < "$stdin" > "$TMP/host.out" 2> /dev/null )
            if diff -q "$TMP/host.out" "$base.expected" > /dev/null; then ok "$name"
            else bad "$name" "host g++ output differs from .expected"; diff "$base.expected" "$TMP/host.out" | head -10; fi
        fi
    else
        ok "$name"
    fi
done

echo "----------------------------------------"
echo "passed: $pass  failed: $fail"
for f in "${failed[@]}"; do echo "  failed: $f"; done
[[ $fail -eq 0 ]]
