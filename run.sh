#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

MEMSIZE=16M
STACKSIZE=8K

# Determine the directory containing the .asm file and the base name
ASM_DIR="$(dirname "$1")"
BASE_NAME="$(basename "$1")"
OBJ_DIR="${ASM_DIR}/obj"
mkdir -p "$OBJ_DIR"

# Remove old output files
if [[ -e "$OBJ_DIR/${BASE_NAME}.o64" ]]; then
	rm "$OBJ_DIR/${BASE_NAME}.o64"
fi
if [[ -e "$1.o64" ]]; then
	rm "$1.o64"
fi

java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/* Assembler "$1.asm"

# Move output files to obj directory
if [[ -e "$1.o64" ]]; then
	mv "$1.o64" "$OBJ_DIR/${BASE_NAME}.o64"
fi
if [[ -e "$1.sym" ]]; then
	mv "$1.sym" "$OBJ_DIR/${BASE_NAME}.sym"
fi
if [[ -e "$1.srcmap" ]]; then
	mv "$1.srcmap" "$OBJ_DIR/${BASE_NAME}.srcmap"
fi

# Look for .o64 in obj directory, fall back to same directory as .asm
if [[ -e "$OBJ_DIR/${BASE_NAME}.o64" ]]; then
	O64_FILE="$OBJ_DIR/${BASE_NAME}.o64"
elif [[ -e "$1.o64" ]]; then
	O64_FILE="$1.o64"
else
	O64_FILE=""
fi

if [[ -n "$O64_FILE" ]]; then
	java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/* Simulation "$O64_FILE" --verbose --mem=$MEMSIZE --stack=$STACKSIZE $2 $3 $4 $5 $6 $7 $8 $9
fi
