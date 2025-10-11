#!/usr/bin/env bash
java -cp antlr-4.13.2-complete.jar:target/CPUSim64V2-1.0-SNAPSHOT.jar org.antlr.v4.gui.TestRig cloud.lesh.CPUSim64v2.Preprocessor preproc -tokens $1.asm -tokens -gui
