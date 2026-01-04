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
    src \
    documentation \
    NOTICE \
    LICENSE \
    README.md \
    -x "**/.DS_Store"