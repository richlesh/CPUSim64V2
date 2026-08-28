#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

# Determine the directory containing the .asm file and the base name
ASM_DIR="$(dirname "$1")"
BASE_NAME="$(basename "$1")"

# Determine the directory containing the .asm file and the base name
OBJ_DIR="${ASM_DIR}/obj"

java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/cpusim64-*.jar cloud.lesh.CPUSim64.Disassembler "$OBJ_DIR/$BASE_NAME.o64" $2
