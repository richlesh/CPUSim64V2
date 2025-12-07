#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

java -cp "$SCRIPT_DIR"/lib/* Preprocessor $1.asm $2 $3 $4 $5 $6 $7 $8 $9
