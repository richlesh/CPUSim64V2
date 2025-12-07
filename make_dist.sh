#!/bin/bash

set -e

OUTPUT="CPUSim64.zip"

# Remove existing file if needed
rm -f "$OUTPUT"

zip -r "$OUTPUT" \
    pom.xml \
    *.sh \
    *.bat \
    lib \
    src \
    documentation \
    -x "**/.DS_Store"