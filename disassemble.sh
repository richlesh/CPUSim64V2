#!env bash
java -cp target/CPUSim64V2-1.0-SNAPSHOT.jar Disassembler $1.obj.gz --mem=2048
