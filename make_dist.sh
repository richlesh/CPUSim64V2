#!/bin/bash

set -e

OUTPUT="CPUSim64V2.zip"

# Remove existing file if needed
rm -f "$OUTPUT"

zip -r "$OUTPUT" \
    pom.xml \
    *.sh \
    *.bat \
    lib \
    compilers \
    aichat/pom.xml \
    aichat/src \
    cpusim64/pom.xml \
    cpusim64/src \
    cpusim64/validation \
    cpusim64_native/pom.xml \
    cpusim64_native/Cargo.toml \
    cpusim64_native/src \
    NOTICE \
    LICENSE \
    README.md \
    -x "**/.DS_Store" \
    -x "**/*.o64" \
    -x "**/*.o64" \
    -x "**/*.out*" \
    -x "**/*.srcmap*" \
    -x "**/*.sym*" \
    -x "**/__pycache__/*" \
    -x "**/obj/*" \
    -x "compilers/*/tests/*" \
    -x "**/bad/*"
