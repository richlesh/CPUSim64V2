#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

# Determine the directory containing the .asm file and the base name
ASM_DIR="$(dirname "$1")"
BASE_NAME="$(basename "$1")"

# Create obj directory relative to the .asm file location
OBJ_DIR="${ASM_DIR}/obj"
mkdir -p "$OBJ_DIR"

# Skip assembly if .o64 is newer than .asm source
if [[ -e "$OBJ_DIR/${BASE_NAME}.o64" && "$OBJ_DIR/${BASE_NAME}.o64" -nt "$1.asm" ]]; then
	echo "Up to date: $OBJ_DIR/${BASE_NAME}.o64"
	exit 0
fi

# Remove old output files from obj directory
if [[ -e "$OBJ_DIR/${BASE_NAME}.o64" ]]; then
	rm "$OBJ_DIR/${BASE_NAME}.o64"
	rm "$OBJ_DIR/${BASE_NAME}.sym*"
	rm "$OBJ_DIR/${BASE_NAME}.srcmap"
fi

# Also clean up any old files in the source directory
if [[ -e "$1.o64" ]]; then
	rm "$1.o64"
	rm "$1.sym*"
	rm "$1.srcmap"
fi

if grep -qi '^[[:space:]]*__MAIN__:' "$1.asm"; then
    hasMain="--hasMain"
else
    hasMain=""
fi

java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/cpusim64-*.jar cloud.lesh.CPUSim64.Assembler "$1.asm" $hasMain $2 $3 $4 $5 $6 $7 $8 $9

# Move output files to obj directory
if [[ -e "$1.o64" ]]; then
	mv "$1.o64" "$OBJ_DIR/${BASE_NAME}.o64"
fi
if [[ -e "$1.sym" ]]; then
	mv "$1.sym" "$OBJ_DIR/${BASE_NAME}.sym"
fi
if [[ -e "$1.sym1" ]]; then
	mv "$1.sym1" "$OBJ_DIR/${BASE_NAME}.sym1"
fi
if [[ -e "$1.sym2" ]]; then
	mv "$1.sym2" "$OBJ_DIR/${BASE_NAME}.sym2"
fi
if [[ -e "$1.srcmap" ]]; then
	mv "$1.srcmap" "$OBJ_DIR/${BASE_NAME}.srcmap"
fi
