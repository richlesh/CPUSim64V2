#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

MEMSIZE=1M
STACKSIZE=8K

if [[ -e "$1.obj.gz" ]]; then
	rm $1.obj.gz
fi
java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/* Assembler $1.asm --DEBUG
if [[ -e "$1.obj.gz" ]] ; then
	java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/* Simulation $1.obj.gz --debug --mem=$MEMSIZE --stack=$STACKSIZE $2 $3 $4 $5 $6 $7 $8 $9
fi