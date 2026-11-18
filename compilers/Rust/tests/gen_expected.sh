#!/usr/bin/env bash
###############################################################################
# gen_expected.sh NAME... - (re)generate the expected files of run/ tests with
# the host rustc (debug build, so integer overflow panics like our translator).
#
# For each run/NAME.rs this writes
#   run/NAME.expected  the program's stdout
#   run/NAME.exit      the exit status, only when it is not 0
#   run/NAME.stderr    for a panic (status 101): the message line, i.e. the
#                      line after "thread 'main' panicked at run/NAME.rs:L:C:"
# The program is run from this directory (so panic locations read
# `run/NAME.rs:L:C`) with run/NAME.argv as arguments and run/NAME.stdin as
# standard input when they exist.
#
# Usage: tests/gen_expected.sh t1_arith p_overflow
#        tests/gen_expected.sh        (no arguments: every run/*.rs)
###############################################################################
cd "$(dirname "$0")" || exit 1

names=("$@")
if [[ ${#names[@]} -eq 0 ]]; then
    for f in run/*.rs; do names+=("$(basename "$f" .rs)"); done
fi

status=0
for n in "${names[@]}"; do
    n="${n%.rs}"
    n="${n#run/}"
    src="run/$n.rs"
    if [[ ! -f "$src" ]]; then
        echo "missing: $src" >&2
        status=1
        continue
    fi
    bin="/tmp/rt_$n"
    if ! rustc --edition 2021 -C debuginfo=0 -A warnings -o "$bin" "$src" 2> "/tmp/rt_$n.cerr"; then
        echo "host rustc failed for $n:" >&2
        head -20 "/tmp/rt_$n.cerr" >&2
        status=1
        continue
    fi
    argv=""
    [[ -f "run/$n.argv" ]] && read -r argv < "run/$n.argv"
    stdin=/dev/null
    [[ -f "run/$n.stdin" ]] && stdin="run/$n.stdin"
    # shellcheck disable=SC2086
    "$bin" $argv < "$stdin" > "run/$n.expected" 2> "/tmp/rt_$n.err"
    code=$?
    rm -f "run/$n.exit" "run/$n.stderr"
    if [[ $code -ne 0 ]]; then
        echo "$code" > "run/$n.exit"
    fi
    if [[ $code -eq 101 ]]; then
        # the message is the line after "thread 'main' panicked at FILE:L:C:"
        awk '/panicked at /{getline; print; exit}' "/tmp/rt_$n.err" > "run/$n.stderr"
    fi
    echo "$n: exit $code, $(wc -l < "run/$n.expected" | tr -d ' ') lines"
done
exit $status
