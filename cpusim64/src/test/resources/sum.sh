#!/usr/bin/env bash
# Adds two integers and returns the result as the script's exit code

# ensure exactly 2 args
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <int1> <int2>" >&2
  exit 1
fi

sum=$(( $1 + $2 ))

# exit codes can only be 0–255, so limit result
if [ "$sum" -lt 0 ] || [ "$sum" -gt 255 ]; then
  echo "Warning: result $sum truncated to 8-bit exit code" >&2
  sum=$(( sum & 0xFF ))
fi

exit "$sum"
