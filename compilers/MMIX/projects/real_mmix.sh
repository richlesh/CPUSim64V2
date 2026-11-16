#!/usr/bin/env bash
###############################################################################
# real_mmix.sh - runs a project on Knuth's mmixal and mmix (needs both, for
# example `brew install mmix`), to compare with the translated program.
#
#   ./real_mmix.sh NAME [args ...]    prints the program's output
#
# NAME.mms and the library files of its "% Options:" line are joined into one
# file (the libraries end with PREFIX ":"), assembled and run in this
# directory, so that the arguments (data/Lincoln.txt ...) are found.
###############################################################################
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
name="$1"; shift
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
files=("$HERE/$name.mms")
for o in $(grep '^% Options:' "$HERE/$name.mms" | sed 's/^% Options://'); do files+=("$HERE/$o"); done
cat "${files[@]}" > "$TMP/$name.mms"
mmixal -o "$TMP/$name.mmo" "$TMP/$name.mms" || exit 1
cd "$HERE" && mmix "$TMP/$name" "$@"
