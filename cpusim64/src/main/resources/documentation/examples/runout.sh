#!/usr/bin/env bash

USE_NATIVE=true

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
CLASSPATH="$SCRIPT_DIR"/../../../../../../lib/cpusim64-*.jar

MEMSIZE=1M
STACKSIZE=8K

if [[ -e "$1.o64" ]]; then
	rm $1.o64
fi
java -Dfile.encoding=UTF8 -cp $CLASSPATH cloud.lesh.CPUSim64.Assembler $1.asm > /dev/null 2>&1
echo "> run.sh" $@
if [[ "$USE_NATIVE" == "true" ]]; then
	"$SCRIPT_DIR"/../../../../../../bin/cpusim64_native $1.o64 --mem=$MEMSIZE --stack=$STACKSIZE $2 $3 $4 $5 $6 $7 $8 $9
else
	java -Dfile.encoding=UTF8 -cp $CLASSPATH cloud.lesh.CPUSim64.Simulation $1.o64 --mem=$MEMSIZE --stack=$STACKSIZE $2 $3 $4 $5 $6 $7 $8 $9
fi
