#!/usr/bin/env bash

USE_NATIVE=false

MEMSIZE=16M
STACKSIZE=8K

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
CLASSPATH="$SCRIPT_DIR"/lib/cpusim64-*.jar

# Determine the directory containing the .asm file and the base name
ASM_DIR="$(dirname "$1")"
BASE_NAME="$(basename "$1")"
OBJ_DIR="${ASM_DIR}/obj"

"$SCRIPT_DIR"/assemble.sh "$1"

# Look for .o64 in obj directory, fall back to same directory as .asm
if [[ -e "$OBJ_DIR/${BASE_NAME}.o64" ]]; then
	O64_FILE="$OBJ_DIR/${BASE_NAME}.o64"
elif [[ -e "$1.o64" ]]; then
	O64_FILE="$1.o64"
else
	O64_FILE=""
fi

if [[ -n "$O64_FILE" ]]; then
	if [[ "$USE_NATIVE" == "true" ]]; then
		"$SCRIPT_DIR"/bin/cpusim64_native "$O64_FILE" --verbose --mem=$MEMSIZE --stack=$STACKSIZE $2 $3 $4 $5 $6 $7 $8 $9
	else
		java -Dfile.encoding=UTF8 -cp $CLASSPATH cloud.lesh.CPUSim64.Simulation "$O64_FILE" --verbose --mem=$MEMSIZE --stack=$STACKSIZE $2 $3 $4 $5 $6 $7 $8 $9
	fi
fi
