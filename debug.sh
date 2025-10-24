#!/usr/bin/env bash

java -cp target/CPUSim64V2-1.0-SNAPSHOT.jar Assembler $1.asm --DEBUG
java -cp target/CPUSim64V2-1.0-SNAPSHOT.jar Simulation $1.obj.gz --debug
