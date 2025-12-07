#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

java -cp "$SCRIPT_DIR"/lib/* org.antlr.v4.gui.TestRig cloud.lesh.CPUSim64.Preprocessor preproc -tokens $1.asm -tokens -gui
