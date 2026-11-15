#!/usr/bin/env bash
###############################################################################
# build.sh - builds the C++ translator for CPUSim64.
#
#   ./build.sh            cppc.asm (the translator), compiled by the C11
#                         compiler
#   ./build.sh --native   obj/cppc-native, a native development copy of the
#                         translator made with the host C compiler
#
# The translator's C sources in src/ are compiled by the CPUSim64 C11 compiler
# (compilers/C11).  cppc.asm is the program that the IDE assembles and runs
# as this plug-in's compiler; the IDE then compiles its output (prog.gen.c)
# with the C11 plug-in (translate_to in config.properties).
###############################################################################
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
C11="$ROOT/compilers/C11"

if [[ "$1" == "--native" ]]; then
    mkdir -p "$HERE/obj"
    cc -std=c99 -Wall -O2 -o "$HERE/obj/cppc-native" "$HERE/src/cppc.c"
    echo "Wrote $HERE/obj/cppc-native"
    exit 0
fi

LOG="$(mktemp)"
trap 'rm -f "$LOG"' EXIT

# c11 FILE... : compiles with the C11 compiler, fails on an error
c11() {
    ( cd "$C11" && "$ROOT/run.sh" cc "$@" ) > "$LOG" 2>&1 || true
    if ! grep -q "^Result: 0" "$LOG"; then
        grep -i "error" "$LOG" | head -20
        echo "build.sh: compiling $1 failed"
        exit 1
    fi
}

c11 "$HERE/src/cppc.c"
# moved up from src/: the .source file names are relative to the .asm file
sed 's|^\.source "|.source "src/|' "$HERE/src/cppc.asm" > "$HERE/cppc.asm"
rm -f "$HERE/src/cppc.asm"
rm -rf "$HERE/src/obj"
rm -f "$HERE/obj/cppc.o64"
echo "Wrote $HERE/cppc.asm ($(wc -l < "$HERE/cppc.asm" | tr -d ' ') lines)"
