#!/usr/bin/env bash
rm $1.pp.asm
./preprocess.sh $1
java -cp antlr-4.13.2-complete.jar:target/CPUSim64V2-1.0-SNAPSHOT.jar org.antlr.v4.gui.TestRig cloud.lesh.CPUSim64v2.CPUSim64v2 program -tokens $1.pp.asm -tokens -gui
