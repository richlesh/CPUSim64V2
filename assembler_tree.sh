#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

if [[ -e "$1.pp.asm" ]]; then
	rm $1.pp.asm
fi
./preprocess.sh $1 $2 $3 $4 $5 $6 $7 $8 $9
java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/* org.antlr.v4.gui.TestRig cloud.lesh.CPUSim64.CPUSim64 program -tokens $1.pp.asm -tokens -gui
