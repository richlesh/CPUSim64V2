#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

# Determine the directory containing the .asm file and the base name
ASM_DIR="$(dirname "$1")"
BASE_NAME="$(basename "$1")"

if [[ -e "$1.pp.asm" ]]; then
	rm $1.pp.asm
fi
if grep -qi '^[[:space:]]*__MAIN__:' $1.asm; then
    hasMain="--hasMain"
else
    hasMain=""
fi
java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/cpusim64-*.jar cloud.lesh.CPUSim64.Preprocessor $1.asm $hasMain $2 $3 $4 $5 $6 $7 $8 $9
