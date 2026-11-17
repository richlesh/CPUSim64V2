#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
# the jar (not a copy such as "cpusim64-2.12.0 2.jar" that iCloud or Finder makes)
for j in "$SCRIPT_DIR"/lib/cpusim64-*.jar; do case "${j##*/}" in *" "*) ;; *) CLASSPATH="$j" ;; esac; done

# Determine the directory containing the .asm file and the base name
ASM_DIR="$(dirname "$1")"
BASE_NAME="$(basename "$1")"

if [[ -e "$1.pp.asm" ]]; then
	rm $1.pp.asm
fi
if grep -qi '^[[:space:]]*__MAIN__:' $1.asm; then
    hasMain="--hasMain"
else
    hasMain=""
fi
java -Dfile.encoding=UTF8 -cp $CLASSPATH cloud.lesh.CPUSim64.Preprocessor $1.asm $hasMain $2 $3 $4 $5 $6 $7 $8 $9
