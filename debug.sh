#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

rm $1.obj.gz
java -cp "$SCRIPT_DIR"/lib/* Assembler $1.asm --DEBUG
java -cp "$SCRIPT_DIR"/lib/* Simulation $1.obj.gz --debug
