#!/usr/bin/env bash
###############################################################################
# build.sh - builds the VB.NET compiler for CPUSim64.
#
#   ./build.sh            vbc.asm (the compiler) and rt/vb_rt.asm (the
#                         run-time library), both with the C11 compiler
#   ./build.sh --rt       only rt/vb_rt.asm
#   ./build.sh --native   obj/vbc-native, a native development copy of the
#                         compiler made with the host C compiler
#
# The compiler's C sources in src/ are compiled by the CPUSim64 C11 compiler
# (compilers/C11).  vbc.asm is the program that the IDE assembles and runs
# as this plug-in's compiler.
###############################################################################
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
C11="$ROOT/compilers/C11"

if [[ "$1" == "--native" ]]; then
    mkdir -p "$HERE/obj"
    cc -std=c99 -Wall -O2 -o "$HERE/obj/vbc-native" "$HERE/src/vbc.c"
    echo "Wrote $HERE/obj/vbc-native"
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

# The run-time library is compiled as a module together with vb_stub.c,
# which only stands for the generated vb_program(); the stub module and the
# line that includes it are dropped.
c11 "$HERE/rt/vb_rt.c" "$HERE/rt/vb_stub.c"
grep -v '^#include ".*vb_stub.asm"$' "$HERE/rt/vb_rt.asm" > "$HERE/rt/vb_rt.asm.tmp"
mv "$HERE/rt/vb_rt.asm.tmp" "$HERE/rt/vb_rt.asm"
rm -f "$HERE/rt/vb_stub.asm"
rm -rf "$HERE/rt/obj"
echo "Wrote $HERE/rt/vb_rt.asm ($(wc -l < "$HERE/rt/vb_rt.asm" | tr -d ' ') lines)"
[[ "$1" == "--rt" ]] && exit 0

c11 "$HERE/src/vbc.c"
# moved up from src/: the .source file names are relative to the .asm file
sed 's|^\.source "|.source "src/|' "$HERE/src/vbc.asm" > "$HERE/vbc.asm"
rm -f "$HERE/src/vbc.asm"
rm -rf "$HERE/src/obj"
rm -f "$HERE/obj/vbc.o64"
echo "Wrote $HERE/vbc.asm ($(wc -l < "$HERE/vbc.asm" | tr -d ' ') lines)"
