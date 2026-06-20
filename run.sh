#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

MEMSIZE=16M
STACKSIZE=8K

if [[ -e "$1.o64" ]]; then
	rm $1.o64
fi
java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/* Assembler $1.asm
if [[ -e "$1.o64" ]] ; then
	java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/* Simulation $1.o64 --verbose --mem=$MEMSIZE --stack=$STACKSIZE $2 $3 $4 $5 $6 $7 $8 $9
fi
