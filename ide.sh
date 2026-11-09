#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
CLASSPATH="$SCRIPT_DIR"/lib/cpusim64-*.jar

java -Dfile.encoding=UTF8 -cp $CLASSPATH com.glowingcat.cpusim64ide.IDEApp "$@"
