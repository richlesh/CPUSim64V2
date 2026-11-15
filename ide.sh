#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
CLASSPATH="$SCRIPT_DIR"/lib/CPUSim64IDE-*.jar

java -Dfile.encoding=UTF8 -cp $CLASSPATH com.glowingcat.cpusim64ide.IDEApp "$@"
