#!/usr/bin/env bash
# Links relocatable modules (.ro64 + .rel, made with assemble.sh -r) into a runnable .o64.
# Usage: link.sh [-o name] [--DEBUG] [-Dsymbol[=value]] module.ro64 [more.ro64 ...]
# -o is required when more than one module is given. Modules that a module
# #includes (e.g. <system/io.asm>) are found or assembled automatically.

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
CLASSPATH="$SCRIPT_DIR"/lib/cpusim64-*.jar

exec java -Dfile.encoding=UTF8 -cp $CLASSPATH cloud.lesh.CPUSim64.Linker "$@"
