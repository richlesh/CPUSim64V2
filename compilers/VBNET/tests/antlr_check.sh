#!/usr/bin/env bash
###############################################################################
# antlr_check.sh - checks the compiler's parser against the ANTLR grammar
#
# Usage: tests/antlr_check.sh [filter]
#
# Builds the derived grammar grammar/VBLexer.g4 + grammar/VBParser.g4 with
# ANTLR 4.13.2 (from the local Maven repository, as used by the cpusim64
# module) and the test driver in tests/antlr/, then checks that ANTLR and
# vbc --parse-only both accept
#   tests/grammar/ok/*.vb, tests/dump/ast_*.vb and tests/run/*.vb
# and both reject
#   tests/grammar/bad/*.vb and tests/errors/parse_*.vb
# A file that one parser accepts and the other rejects is a failure.
#
# Environment:
#   VBC=native   use the native build of the compiler (faster); default is
#                the CPUSim64 build in obj/vbc.o64
###############################################################################

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VB_DIR="$(dirname "$HERE")"
ROOT="$(cd "$VB_DIR/../.." && pwd)"
GRAMMAR="$VB_DIR/grammar"
OUT="$GRAMMAR/antlr-out"
FILTER="${1:-}"
M2="${M2_REPO:-$HOME/.m2/repository}"

if [[ ! -f "$GRAMMAR/VBLexer.g4" || ! -f "$GRAMMAR/VBParser.g4" || ! -f "$HERE/antlr/VBCheck.java" ]]; then
    echo "SKIP: the derived grammar (grammar/VBLexer.g4, VBParser.g4) or tests/antlr/VBCheck.java is not written yet"
    exit 0
fi

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
    ( cd "$GRAMMAR" && java -cp "$CP" org.antlr.v4.Tool -Werror -o "$OUT/src" VBLexer.g4 VBParser.g4 ) \
        || { echo "FATAL: ANTLR rejects the grammar"; exit 1; }
    javac -nowarn -cp "$CP" -d "$OUT/classes" "$OUT"/src/*.java "$HERE"/antlr/*.java \
        || { echo "FATAL: the recognizer does not compile"; exit 1; }
    touch "$stamp"
fi

# the compiler
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
if [[ "$VBC" == "native" ]]; then
    "$VB_DIR/build.sh" --native > /dev/null || { echo "FATAL: native build failed"; exit 1; }
    vbc_ok() { "$VB_DIR/obj/vbc-native" --parse-only "$1" > /dev/null 2> "$TMP/cerr"; }
else
    [[ -f "$VB_DIR/obj/vbc.o64" ]] || "$ROOT/assemble.sh" "$VB_DIR/vbc" > /dev/null 2>&1
    vbc_ok() {
        "$ROOT/bin/cpusim64_native" "$VB_DIR/obj/vbc.o64" --verbose --mem=16M --stack=256K \
            --parse-only "$1" > "$TMP/cout" 2> "$TMP/cerr"
        grep -q "^Result: 0$" "$TMP/cout"
    }
fi

pass=0
fail=0
failed=()
for kind in ok bad; do
    if [[ $kind == ok ]]; then set -- "$HERE"/grammar/ok/*.vb "$HERE"/dump/ast_*.vb "$HERE"/run/*.vb
    else set -- "$HERE"/grammar/bad/*.vb "$HERE"/errors/parse_*.vb; fi
    for t in "$@"; do
        [[ -e "$t" ]] || continue
        name="${t#$HERE/}"; name="${name%.vb}"
        [[ -z "$FILTER" || "$name" == *"$FILTER"* ]] || continue
        java -cp "$CP:$OUT/classes" VBCheck "$t" > "$TMP/aout" 2>&1
        a=$?
        vbc_ok "$t"
        v=$?
        want=0
        [[ $kind == bad ]] && want=1
        if [[ $a -ne 0 ]]; then a=1; fi
        if [[ $v -ne 0 ]]; then v=1; fi
        if [[ $a -eq $want && $v -eq $want ]]; then
            pass=$((pass + 1)); echo "PASS  $name"
        else
            fail=$((fail + 1)); failed+=("$name")
            echo "FAIL  $name: ANTLR $([[ $a -eq 0 ]] && echo accepts || echo rejects), vbc $([[ $v -eq 0 ]] && echo accepts || echo rejects)"
            head -3 "$TMP/aout"; head -3 "$TMP/cerr"
        fi
    done
done
echo "----------------------------------------"
echo "passed: $pass  failed: $fail"
for f in "${failed[@]}"; do echo "  failed: $f"; done
[[ $fail -eq 0 ]]
