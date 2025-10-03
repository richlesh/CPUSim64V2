#!env bash

java -cp target/CPUSim64V2-1.0-SNAPSHOT.jar Assembler $1.asm
java -cp target/CPUSim64V2-1.0-SNAPSHOT.jar Simulation $1.obj.gz --debug --mem="${2:-2048}"
