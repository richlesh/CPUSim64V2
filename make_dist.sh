#!/bin/bash

set -e

OUTPUT="CPUSimV2.zip"

# Remove existing file if needed
rm -f "$OUTPUT"

zip -r "$OUTPUT" \
    pom.xml \
    *.sh \
    target/CPUSim*.jar \
    src \
    documentation \
    -x "**/.DS_Store"