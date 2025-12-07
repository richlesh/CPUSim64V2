#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

java -cp "$SCRIPT_DIR"/lib/* Disassembler $1.obj.gz --mem=2048
