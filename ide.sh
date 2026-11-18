#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
# the jar (not a copy such as "CPUSim64IDE-2.12.0 2.jar" that iCloud or Finder makes)
for j in "$SCRIPT_DIR"/lib/CPUSim64IDE-*.jar; do case "${j##*/}" in *" "*) ;; *) CLASSPATH="$j" ;; esac; done

java -Dfile.encoding=UTF8 -cp $CLASSPATH com.glowingcat.cpusim64ide.IDEApp "$@"
