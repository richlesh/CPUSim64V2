#!/usr/bin/env bash
###############################################################################
# random_diff.sh - randomised differential test against the real mmix
#
# Usage: tests/random_diff.sh FIRST_SEED COUNT [INSTRUCTIONS]
#
# For each seed, gen_random.py writes a program of random instructions; it is
# run on the real mmix and, translated, on the native CPUSim64 simulator, and
# the outputs must be equal.  Uses the native build of the translator
# (build.sh --native).  Failing programs are kept in /tmp/mmix_random_fail/.
###############################################################################
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MMIX_DIR="$(dirname "$HERE")"
ROOT="$(cd "$MMIX_DIR/../.." && pwd)"
first="${1:-1}"; n="${2:-20}"; len="${3:-200}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
FAIL=/tmp/mmix_random_fail
mkdir -p "$FAIL"
"$MMIX_DIR/build.sh" --native > /dev/null || exit 1
bad=0
for ((s = first; s < first + n; s++)); do
    p="$TMP/r$s"
    python3 "$HERE/gen_random.py" "$s" "$len" > "$p.mms"
    mmixal -o "$p.mmo" "$p.mms" > /dev/null 2>&1 || { echo "seed $s: mmixal failed"; bad=$((bad+1)); continue; }
    mmix -q "$p.mmo" > "$p.want" 2>&1
    "$MMIX_DIR/obj/mmixc-native" "$p.mms" > /dev/null 2> "$p.err" || { echo "seed $s: translation failed: $(head -2 "$p.err")"; bad=$((bad+1)); continue; }
    "$ROOT/assemble.sh" "$p" > "$p.alog" 2>&1
    if [[ ! -f "$TMP/obj/r$s.o64" ]]; then
        echo "seed $s: assembly failed: $(grep -i error "$p.alog" | head -2)"; bad=$((bad+1)); continue
    fi
    "$ROOT/bin/cpusim64_native" "$TMP/obj/r$s.o64" --mem=16M --stack=256K > "$p.got" 2>&1
    if ! cmp -s "$p.want" "$p.got"; then
        echo "seed $s: output differs"
        cp "$p.mms" "$p.want" "$p.got" "$FAIL/"
        bad=$((bad+1))
    fi
done
echo "random_diff: $n programs of $len instructions from seed $first, $bad failed"
[[ $bad -eq 0 ]]
