#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
CLASSPATH="$SCRIPT_DIR"/lib/cpusim64-*.jar

# Relocatable modules and linking: assemble.sh -r name [-o out] [options]
#                                   assemble.sh -o out a b ... [options]
# (inputs may be given with or without .asm; .ro64 modules are linked as they are).
# A single -r input without -o is written to obj/<name>.ro64 next to the source.
NEEDS_LINKER=false
INPUTS=0
for a in "$@"; do
	case "$a" in
		-r|-o) NEEDS_LINKER=true ;;
		-*) ;;
		*) INPUTS=$((INPUTS + 1)) ;;
	esac
done
if [[ $INPUTS -gt 1 ]]; then NEEDS_LINKER=true; fi
if [[ "$NEEDS_LINKER" == "true" ]]; then
	ARGS=()
	RMODE=false
	HAVE_OUT=false
	FIRST=""
	PREV=""
	for a in "$@"; do
		if [[ "$PREV" == "-o" ]]; then
			ARGS+=("$a"); PREV=""; HAVE_OUT=true; continue
		fi
		case "$a" in
			-o) ARGS+=("$a"); PREV="-o" ;;
			-r) ARGS+=("$a"); RMODE=true ;;
			-*) ARGS+=("$a") ;;
			*.asm|*.ro64) ARGS+=("$a"); [[ -z "$FIRST" ]] && FIRST="$a" ;;
			*) ARGS+=("$a.asm"); [[ -z "$FIRST" ]] && FIRST="$a.asm" ;;
		esac
	done
	if [[ "$RMODE" == "true" && "$HAVE_OUT" == "false" && $INPUTS -eq 1 ]]; then
		D="$(dirname "$FIRST")"; B="$(basename "$FIRST" .asm)"
		mkdir -p "$D/obj"
		ARGS+=("-o" "$D/obj/$B")
	fi
	exec java -Dfile.encoding=UTF8 -cp $CLASSPATH cloud.lesh.CPUSim64.Assembler "${ARGS[@]}"
fi

# Determine the directory containing the .asm file and the base name
ASM_DIR="$(dirname "$1")"
BASE_NAME="$(basename "$1")"

# Create obj directory relative to the .asm file location
OBJ_DIR="${ASM_DIR}/obj"
mkdir -p "$OBJ_DIR"

# A program without .asm source (e.g. linked with link.sh) is left alone
if [[ ! -e "$1.asm" ]]; then
	if [[ -e "$OBJ_DIR/${BASE_NAME}.o64" || -e "$1.o64" ]]; then
		echo "No source $1.asm: using the existing ${BASE_NAME}.o64"
		exit 0
	fi
	echo "Can't find file: $1.asm"
	exit 3
fi

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

java -Dfile.encoding=UTF8 -cp $CLASSPATH cloud.lesh.CPUSim64.Assembler "$1.asm" $hasMain $2 $3 $4 $5 $6 $7 $8 $9

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
