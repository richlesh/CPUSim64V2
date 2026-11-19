#!/usr/bin/env bash

USE_NATIVE=false

MEMSIZE=16M
STACKSIZE=256K

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
# the jar (not a copy such as "cpusim64-2.12.0 2.jar" that iCloud or Finder makes)
for j in "$SCRIPT_DIR"/cpusim64-*.jar; do case "${j##*/}" in *" "*) ;; *) CLASSPATH="$j" ;; esac; done

# Syntax: run.sh [--mem=size_in_GiW] [--stack=size_in_kiW] prog [args...]
# Only options before prog are read here; everything after prog goes to the program.
while [[ $# -gt 0 ]]; do
	case "$1" in
		--mem=*) MEMSIZE="${1#--mem=}"; shift ;;
		--stack=*) STACKSIZE="${1#--stack=}"; shift ;;
		*) break ;;
	esac
done
if [[ $# -eq 0 ]]; then
	echo "Usage: $(basename "$0") [--mem=size_in_GiW] [--stack=size_in_kiW] prog [args...]"
	exit 2
fi
PROG="$1"
shift

# Determine the directory containing the .asm file and the base name
ASM_DIR="$(dirname "$PROG")"
BASE_NAME="$(basename "$PROG")"
OBJ_DIR="${ASM_DIR}/obj"

"$SCRIPT_DIR"/assemble.sh "$PROG"

# Look for .o64 in obj directory, fall back to same directory as .asm
if [[ -e "$OBJ_DIR/${BASE_NAME}.o64" ]]; then
	O64_FILE="$OBJ_DIR/${BASE_NAME}.o64"
elif [[ -e "$PROG.o64" ]]; then
	O64_FILE="$PROG.o64"
else
	O64_FILE=""
fi

if [[ -n "$O64_FILE" ]]; then
	if [[ "$USE_NATIVE" == "true" ]]; then
		"$SCRIPT_DIR"/cpusim64_native "$O64_FILE" --verbose --mem=$MEMSIZE --stack=$STACKSIZE "$@"
	else
		java -Dfile.encoding=UTF8 -cp $CLASSPATH cloud.lesh.CPUSim64.Simulation "$O64_FILE" --verbose --mem=$MEMSIZE --stack=$STACKSIZE "$@"
	fi
fi
