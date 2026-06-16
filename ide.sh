#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

java -Dfile.encoding=UTF8 -cp "$SCRIPT_DIR"/lib/* CPUSim64App $1 $2 $3 $4 $5 $6 $7 $8 $9
