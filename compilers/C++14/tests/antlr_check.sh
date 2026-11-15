#!/usr/bin/env bash
###############################################################################
# antlr_check.sh - checks the translator's parser against the ANTLR grammar
#
# Usage: tests/antlr_check.sh [filter]
#
# Builds grammar/CPP14Lexer.g4 + grammar/CPP14Parser.g4 (from antlr/grammars-v4)
# with ANTLR 4.13.2 (from the local Maven repository, as used by the cpusim64
# module) and the driver in tests/antlr/, then checks that ANTLR and
# cppc --parse-only both accept
#   tests/grammar/ok/*.cpp and tests/dump/ast_*.cpp
# and both reject
#   tests/grammar/bad/*.cpp and tests/errors/parse_*.cpp
# A file that one parser accepts and the other rejects is a failure, unless
# it is listed in tests/grammar/allow.txt (one name per line, as printed, with
# a comment saying why: the grammar is not semantic, so it accepts some
# programs that the translator rejects, such as a use of an undeclared name
# as a type).
#
# The input files must not use #include or macros (ANTLR does not preprocess).
#
# Environment:
#   CPPC=native   use the native build of the translator (faster); default is
#                 the CPUSim64 build in obj/cppc.o64
###############################################################################

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CPP_DIR="$(dirname "$HERE")"
ROOT="$(cd "$CPP_DIR/../.." && pwd)"
GRAMMAR="$CPP_DIR/grammar"
OUT="$GRAMMAR/antlr-out"
FILTER="${1:-}"
M2="${M2_REPO:-$HOME/.m2/repository}"

# ANTLR tool class path from the Maven repository
CP=""
for j in "$M2"/org/antlr/antlr4/4.13.2/antlr4-4.13.2.jar \
         "$M2"/org/antlr/antlr4-runtime/4.13.2/antlr4-runtime-4.13.2.jar \
         "$M2"/org/antlr/ST4/*/ST4-*.jar \
         "$M2"/org/antlr/antlr-runtime/*/antlr-runtime-*.jar \
         "$M2"/org/abego/treelayout/*/*/org.abego.treelayout.core-*.jar \
         "$M2"/com/ibm/icu/icu4j/*/icu4j-*.jar; do
    [[ -f "$j" && "$j" != *sources* ]] && CP="$CP:$j"
done
if [[ "$CP" != *antlr4-4.13.2.jar* ]]; then
    echo "FATAL: ANTLR 4.13.2 not found in $M2 (build the cpusim64 module with Maven first)"
    exit 1
fi
CP="${CP#:}"

# (re)build the recognizer when a grammar or driver file is newer
stamp="$OUT/.built"
if [[ ! -f "$stamp" ]] || [[ -n "$(find "$GRAMMAR"/*.g4 "$HERE"/antlr/*.java -newer "$stamp" 2>/dev/null)" ]]; then
    rm -rf "$OUT"
    mkdir -p "$OUT/src" "$OUT/classes"
    ( cd "$GRAMMAR" && java -cp "$CP" org.antlr.v4.Tool -o "$OUT/src" CPP14Lexer.g4 CPP14Parser.g4 ) \
        || { echo "FATAL: ANTLR rejects the grammar"; exit 1; }
    cp "$HERE"/antlr/CPP14ParserBase.java "$OUT/src/"
    javac -nowarn -cp "$CP" -d "$OUT/classes" "$OUT"/src/*.java "$HERE"/antlr/CPP14Check.java \
        || { echo "FATAL: the recognizer does not compile"; exit 1; }
    touch "$stamp"
fi

# the translator
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
if [[ "$CPPC" == "native" ]]; then
    "$CPP_DIR/build.sh" --native > /dev/null || { echo "FATAL: native build failed"; exit 1; }
    cppc_ok() { "$CPP_DIR/obj/cppc-native" --parse-only "$1" > /dev/null 2> "$TMP/cerr"; }
else
    if [[ ! -f "$CPP_DIR/obj/cppc.o64" || "$CPP_DIR/cppc.asm" -nt "$CPP_DIR/obj/cppc.o64" ]]; then
        "$ROOT/assemble.sh" "$CPP_DIR/cppc" > /dev/null 2>&1
    fi
    cppc_ok() {
        "$ROOT/bin/cpusim64_native" "$CPP_DIR/obj/cppc.o64" --verbose --mem=64M --stack=1M \
            --parse-only "$1" > "$TMP/cout" 2> "$TMP/cerr"
        grep -q "^Result: 0$" "$TMP/cout"
    }
fi

allowed() {
    [[ -f "$HERE/grammar/allow.txt" ]] && grep -qxF "$1" <(sed 's/[[:space:]]*#.*//' "$HERE/grammar/allow.txt")
}

pass=0
fail=0
failed=()
for kind in ok bad; do
    if [[ $kind == ok ]]; then set -- "$HERE"/grammar/ok/*.cpp "$HERE"/dump/ast_*.cpp
    else set -- "$HERE"/grammar/bad/*.cpp "$HERE"/errors/parse_*.cpp; fi
    for t in "$@"; do
        [[ -e "$t" ]] || continue
        name="${t#$HERE/}"; name="${name%.cpp}"
        [[ -z "$FILTER" || "$name" == *"$FILTER"* ]] || continue
        java -cp "$CP:$OUT/classes" CPP14Check "$t" > "$TMP/aout" 2>&1
        a=$?
        cppc_ok "$t"
        v=$?
        want=0
        [[ $kind == bad ]] && want=1
        if [[ $a -ne 0 ]]; then a=1; fi
        if [[ $v -ne 0 ]]; then v=1; fi
        if [[ $a -eq $want && $v -eq $want ]]; then
            pass=$((pass + 1)); echo "PASS  $name"
        elif allowed "$name"; then
            pass=$((pass + 1)); echo "ALLOW $name (ANTLR $([[ $a -eq 0 ]] && echo accepts || echo rejects), cppc $([[ $v -eq 0 ]] && echo accepts || echo rejects))"
        else
            fail=$((fail + 1)); failed+=("$name")
            echo "FAIL  $name: ANTLR $([[ $a -eq 0 ]] && echo accepts || echo rejects), cppc $([[ $v -eq 0 ]] && echo accepts || echo rejects)"
            head -3 "$TMP/aout"; head -3 "$TMP/cerr"
        fi
    done
done
echo "----------------------------------------"
echo "passed: $pass  failed: $fail"
for f in "${failed[@]}"; do echo "  failed: $f"; done
[[ $fail -eq 0 ]]
