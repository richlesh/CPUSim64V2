#!/usr/bin/env bash

java -cp target/CPUSim64V2-1.0-SNAPSHOT.jar Assembler $1.asm $2 $3 $4 $5 $6 $7 $8 $9
java -cp target/CPUSim64V2-1.0-SNAPSHOT.jar Simulation $1.obj.gz --verbose
