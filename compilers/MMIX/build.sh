#!/usr/bin/env bash
###############################################################################
# build.sh - builds the translator and its run-time library for CPUSim64.
#
#   ./build.sh            mmixc.asm (the translator) and rt/mmix_rt.asm (the
#                         run-time library), both with the C11 compiler
#   ./build.sh --rt       only rt/mmix_rt.asm
#   ./build.sh --native   obj/mmixc-native, a native development copy of the
#                         translator made with the host C compiler
#
# The translator src/mmixc.c and the run-time library rt/mmix_rt.c are
# compiled by the CPUSim64 C11 compiler (compilers/C11).  mmixc.asm is the
# program that the IDE assembles and runs as this plug-in's compiler.
###############################################################################
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
C11="$ROOT/compilers/C11"

if [[ "$1" == "--native" ]]; then
    mkdir -p "$HERE/obj"
    cc -std=c99 -Wall -O2 -o "$HERE/obj/mmixc-native" "$HERE/src/mmixc.c"
    echo "Wrote $HERE/obj/mmixc-native"
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

# The run-time library is compiled as a module together with mmix_stub.c,
# which only declares the assembly functions of mmix_io.asm; the stub module
# and the line that includes it are dropped.
c11 "$HERE/rt/mmix_rt.c" "$HERE/rt/mmix_stub.c"
grep -v '^#include ".*mmix_stub.asm"$' "$HERE/rt/mmix_rt.asm" > "$HERE/rt/mmix_rt.asm.tmp"
mv "$HERE/rt/mmix_rt.asm.tmp" "$HERE/rt/mmix_rt.asm"
rm -f "$HERE/rt/mmix_stub.asm"
rm -rf "$HERE/rt/obj"
echo "Wrote $HERE/rt/mmix_rt.asm ($(wc -l < "$HERE/rt/mmix_rt.asm" | tr -d ' ') lines)"
[[ "$1" == "--rt" ]] && exit 0

c11 "$HERE/src/mmixc.c"
# moved up from src/: the .source file names are relative to the .asm file
sed 's|^\.source "|.source "src/|' "$HERE/src/mmixc.asm" > "$HERE/mmixc.asm"
rm -f "$HERE/src/mmixc.asm"
rm -f "$HERE/obj/mmixc.o64"
echo "Wrote $HERE/mmixc.asm ($(wc -l < "$HERE/mmixc.asm" | tr -d ' ') lines)"
