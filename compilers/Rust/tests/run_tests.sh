#!/usr/bin/env bash
###############################################################################
# run_tests.sh - regression tests for the Rust translator (compilers/Rust)
#
# Usage: tests/run_tests.sh [filter]
#
# Test layout (relative to this directory):
#   lex/NAME.rs + NAME.tokens   `rustc --tokens lex/NAME.rs` must print exactly
#              [+ NAME.tokens-flags]  the .tokens file (after the banner); with
#                               a .tokens-flags file, `--tokens-flags` must
#                               print that too (see lex/README.md)
#   parse/NAME.rs + NAME.ast    `rustc --ast parse/NAME.rs` must print exactly
#                               the .ast file (the AST dump, see src/ast.h)
#   expand/NAME.rs + NAME.expand  `rustc --expand`: the AST after macro expansion
#   resolve/NAME.rs + NAME.resolve `rustc --resolve`: the AST with ` ->path` symbols
#   check/NAME.rs + NAME.types     `rustc --types`: the AST with ` :type` and ` ->path`
#   errors/NAME.rs + NAME.err   translation must fail and stderr must contain
#                               the line in NAME.err
#   run/NAME.rs                 translated to NAME.gen.c, compiled by the C11
#              + NAME.expected  compiler, assembled and run on CPUSim64;
#                               program stdout must equal NAME.expected
#              [+ NAME.exit]    expected exit code (default 0)
#              [+ NAME.argv]    command line arguments for the program
#              [+ NAME.stdin]   standard input for the program
#              [+ NAME.stderr]  a line that the program's stderr must contain
#
# Environment:
#   HOST=1       run the translator natively (build.sh --native) instead of
#                rustc.asm on the simulator.  The generated C of run/ tests is
#                still compiled by the C11 compiler and run on the simulator,
#                because it only works on CPUSim64 (byte addresses).
#   CPUSIM64_HOME  the CPUSim64 install to use for assemble.sh, the simulator
#                and the C11 compiler (default: the root of this repository;
#                the checked-in lib/cpusim64-*.jar may be older than the
#                sources, then point this at a current install)
#   SIM=java     run on the Java simulator (default: the native cpusim64_native)
###############################################################################

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUST_DIR="$(dirname "$HERE")"
ROOT="$(cd "$RUST_DIR/../.." && pwd)"
C11="$ROOT/compilers/C11"
HOME_DIR="${CPUSIM64_HOME:-$ROOT}"
JAR="$(ls "$HOME_DIR"/lib/cpusim64-*.jar 2>/dev/null | grep -v ' ' | head -1)"
NATIVE="$HOME_DIR/bin/cpusim64_native"
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

# ---------------------------------------------------------------- translator
# rustc ARGS... : runs the translator; $TMP/cout = stdout without the banner,
# $TMP/cerr = stderr; the status is 0 on success
if [[ -n "$HOST" ]]; then
    "$RUST_DIR/build.sh" --native > /dev/null || { echo "FATAL: native build failed"; exit 1; }
    rustc_raw() { "$RUST_DIR/obj/rustc-native" "$@" > "$TMP/craw" 2> "$TMP/cerr"; }
else
    if [[ ! -f "$RUST_DIR/obj/rustc.o64" || "$RUST_DIR/rustc.asm" -nt "$RUST_DIR/obj/rustc.o64" ]]; then
        "$HOME_DIR/assemble.sh" "$RUST_DIR/rustc" > "$TMP/asmlog" 2>&1
        [[ -f "$RUST_DIR/obj/rustc.o64" ]] || { cat "$TMP/asmlog"; echo "FATAL: rustc.asm does not assemble"; exit 1; }
    fi
    rustc_raw() {
        "$NATIVE" "$RUST_DIR/obj/rustc.o64" --verbose --mem=64M --stack=1M "$@" > "$TMP/craw2" 2> "$TMP/cerr"
        perl -e '
            local $/; my $s = <STDIN>;
            $s =~ s/\A.*?^Read \d+ words from [^\n]*\n//ms;
            my $res = 1;
            if ($s =~ /\A(.*)Result: (-?\d+)\n.*\z/s) { $s = $1; $res = $2; }
            print $s; exit($res ? 1 : 0);
        ' < "$TMP/craw2" > "$TMP/craw"
    }
fi
rustc() {
    rustc_raw "$@"
    local st=$?
    # drop the banner: everything up to and including the "Arguments:" line
    perl -e 'local $/; my $s = <STDIN>; $s =~ s/\A.*?^Arguments:[^\n]*\n//ms; print $s;' \
        < "$TMP/craw" > "$TMP/cout"
    return $st
}

# the C11 compiler (run/ tests need it in both modes)
if true; then
    if [[ ! -f "$C11/obj/cc.o64" || -n "$(find "$C11" -maxdepth 1 \( -name '*.asm' -o -name '*.def' \) -newer "$C11/obj/cc.o64")" ]]; then
        "$HOME_DIR/assemble.sh" "$C11/cc" > "$TMP/asmlog" 2>&1
        [[ -f "$C11/obj/cc.o64" ]] || { cat "$TMP/asmlog"; echo "FATAL: the C11 compiler does not assemble"; exit 1; }
    fi
fi
# cc FILE.c : the C11 compiler writes FILE.asm; $TMP/ccerr = its stderr
cc11() {
    "$NATIVE" "$C11/obj/cc.o64" --verbose --mem=64M --stack=1M "$@" > "$TMP/ccraw" 2> "$TMP/ccerr"
    grep -q "^Result: 0" "$TMP/ccraw"
}

# ---------------------------------------------------------------- lexer tests
for t in "$HERE"/lex/*.rs; do
    [[ -e "$t" ]] || continue
    name="lex/$(basename "$t" .rs)"
    want "$name" || continue
    base="${t%.rs}"
    [[ -e "$base.tokens" ]] || continue
    if ! ( cd "$HERE" && rustc --tokens "lex/$(basename "$t")" ); then
        bad "$name" "lexing failed: $(head -2 "$TMP/cerr")"
        continue
    fi
    if ! diff -q "$TMP/cout" "$base.tokens" > /dev/null; then
        bad "$name" "tokens differ"
        diff "$base.tokens" "$TMP/cout" | head -10
        continue
    fi
    if [[ -e "$base.tokens-flags" ]]; then
        if ! ( cd "$HERE" && rustc --tokens-flags "lex/$(basename "$t")" ) || ! diff -q "$TMP/cout" "$base.tokens-flags" > /dev/null; then
            bad "$name" "tokens with flags differ"
            diff "$base.tokens-flags" "$TMP/cout" | head -10
            continue
        fi
    fi
    ok "$name"
done

# ---------------------------------------------------------------- parser tests
for t in "$HERE"/parse/*.rs; do
    [[ -e "$t" ]] || continue
    name="parse/$(basename "$t" .rs)"
    want "$name" || continue
    base="${t%.rs}"
    [[ -e "$base.ast" ]] || continue
    if ! ( cd "$HERE" && rustc --ast "parse/$(basename "$t")" ); then
        bad "$name" "parsing failed: $(head -2 "$TMP/cerr")"
        continue
    fi
    if ! diff -q "$TMP/cout" "$base.ast" > /dev/null; then
        bad "$name" "AST differs"
        diff "$base.ast" "$TMP/cout" | head -10
        continue
    fi
    ok "$name"
done

# ---------------------------------------------------------------- expansion tests
for t in "$HERE"/expand/*.rs; do
    [[ -e "$t" ]] || continue
    name="expand/$(basename "$t" .rs)"
    want "$name" || continue
    base="${t%.rs}"
    [[ -e "$base.expand" ]] || continue
    if ! ( cd "$HERE" && rustc --expand "expand/$(basename "$t")" ); then
        bad "$name" "expansion failed: $(head -2 "$TMP/cerr")"
        continue
    fi
    if ! diff -q "$TMP/cout" "$base.expand" > /dev/null; then
        bad "$name" "expansion differs"
        diff "$base.expand" "$TMP/cout" | head -10
        continue
    fi
    ok "$name"
done

# ---------------------------------------------------------------- resolver tests
for t in "$HERE"/resolve/*.rs; do
    [[ -e "$t" ]] || continue
    name="resolve/$(basename "$t" .rs)"
    want "$name" || continue
    base="${t%.rs}"
    [[ -e "$base.resolve" ]] || continue
    if ! ( cd "$HERE" && rustc --resolve "resolve/$(basename "$t")" ); then
        bad "$name" "resolution failed: $(head -2 "$TMP/cerr")"
        continue
    fi
    if ! diff -q "$TMP/cout" "$base.resolve" > /dev/null; then
        bad "$name" "resolution differs"
        diff "$base.resolve" "$TMP/cout" | head -10
        continue
    fi
    ok "$name"
done

# ---------------------------------------------------------------- type checker tests
for t in "$HERE"/check/*.rs; do
    [[ -e "$t" ]] || continue
    name="check/$(basename "$t" .rs)"
    want "$name" || continue
    base="${t%.rs}"
    [[ -e "$base.types" ]] || continue
    if ! ( cd "$HERE" && rustc --types "check/$(basename "$t")" ); then
        bad "$name" "type checking failed: $(head -2 "$TMP/cerr")"
        continue
    fi
    if ! diff -q "$TMP/cout" "$base.types" > /dev/null; then
        bad "$name" "types differ"
        diff "$base.types" "$TMP/cout" | head -10
        continue
    fi
    ok "$name"
done

# ---------------------------------------------------------------- error tests
for t in "$HERE"/errors/*.rs; do
    [[ -e "$t" ]] || continue
    name="errors/$(basename "$t" .rs)"
    want "$name" || continue
    base="${t%.rs}"
    expect="$(cat "$base.err")"
    rm -f "$base.gen.c"
    if ( cd "$HERE" && rustc "errors/$(basename "$t")" ); then
        bad "$name" "translation succeeded but should fail"
        continue
    fi
    if ! grep -qF -- "$expect" "$TMP/cerr" "$TMP/cout"; then
        bad "$name" "missing '$expect', got: $(head -2 "$TMP/cerr")"
    else
        ok "$name"
    fi
done

# ---------------------------------------------------------------- run tests
for t in "$HERE"/run/*.rs; do
    [[ -e "$t" ]] || continue
    name="run/$(basename "$t" .rs)"
    want "$name" || continue
    base="${t%.rs}"
    [[ -e "$base.expected" ]] || continue
    rm -f "$base.gen.c" "$base.gen.asm" "$base.asm" "$HERE/run/obj/$(basename "$base").o64"
    if ! ( cd "$HERE/run" && rustc "$t" ) || [[ ! -f "$base.gen.c" ]]; then
        bad "$name" "translation failed: $(head -3 "$TMP/cerr")"
        continue
    fi
    argv=""
    [[ -e "$base.argv" ]] && read -r argv < "$base.argv"
    stdin=""
    [[ -e "$base.stdin" ]] && stdin="$base.stdin"
    expexit=0
    [[ -e "$base.exit" ]] && read -r expexit < "$base.exit"
    # the generated C only runs on CPUSim64 (byte addresses), so it is always
    # compiled by the C11 compiler and run on the simulator
    if ! ( cd "$HERE/run" && cc11 "$base.gen.c" ) || [[ ! -f "$base.gen.asm" ]]; then
        bad "$name" "C11 compilation of the generated C failed: $(head -3 "$TMP/ccerr")"
        continue
    fi
    # as in the IDE: the C11 compiler's result prog.gen.asm becomes prog.asm
    mv "$base.gen.asm" "$base.asm"
    "$HOME_DIR/assemble.sh" "$base" > "$TMP/asmlog" 2>&1
    if [[ ! -f "$HERE/run/obj/$(basename "$base").o64" ]]; then
        bad "$name" "generated assembly failed: $(grep -i error "$TMP/asmlog" | head -3)"
        continue
    fi
    ( cd "$HERE/run" && SIM_STDIN="$stdin" sim "$HERE/run/obj/$(basename "$base").o64" $argv )
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
