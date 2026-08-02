#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

MEMSIZE=1M
STACKSIZE=8K

if [[ -e "$1.o64" ]]; then
	rm $1.o64
fi
java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/../../lib/* Assembler $1.asm --DEBUG > /dev/null 2>&1
if [[ -e "$1.o64" ]] ; then
	echo "> debug.sh" $@
	java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/../../lib/* Simulation $1.o64 --debug --mem=$MEMSIZE --stack=$STACKSIZE $2 $3 $4 $5 $6 $7 $8 $9
fi
