#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

if [[ -e "$1.o64" ]]; then
	rm $1.o64
fi
if grep -qi '^[[:space:]]*__MAIN__:' $1.asm; then
    hasMain="--hasMain"
else
    hasMain=""
fi
java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/* Assembler $1.asm $hasMain $2 $3 $4 $5 $6 $7 $8 $9
