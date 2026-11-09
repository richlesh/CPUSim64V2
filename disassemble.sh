#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
CLASSPATH="$SCRIPT_DIR"/lib/cpusim64-*.jar

# Determine the directory containing the .asm file and the base name
ASM_DIR="$(dirname "$1")"
BASE_NAME="$(basename "$1")"
OBJ_DIR="${ASM_DIR}/obj"

# Look for .o64 in obj directory, fall back to same directory as .asm
if [[ -e "$OBJ_DIR/${BASE_NAME}.o64" ]]; then
	O64_FILE="$OBJ_DIR/${BASE_NAME}.o64"
elif [[ -e "$1.o64" ]]; then
	O64_FILE="$1.o64"
else
	O64_FILE=""
fi

if [[ -n "$O64_FILE" ]]; then
    java -Dfile.encoding=UTF8 -cp $CLASSPATH cloud.lesh.CPUSim64.Disassembler "$O64_FILE" $2
fi