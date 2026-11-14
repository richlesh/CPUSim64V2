#!/usr/bin/env bash
###############################################################################
# oracle.sh - makes expected results with the real MMIX tools (mmixal, mmix)
#
# Usage: tests/oracle.sh dump FILE.mms...   writes FILE.dump (from mmixal's .mmo)
#        tests/oracle.sh run  FILE.mms...   writes FILE.expected and FILE.exit
#                                           from running the real mmix
# For run, FILE.argv (arguments) and FILE.stdin (standard input, given to mmix
# with -f) are used if they exist.  The program runs in a scratch directory
# as `mmix NAME args`, so that argv[0] is NAME (the translated program uses
# the source file name), with copies of the files listed in FILE.files.
###############################################################################
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mode="$1"; shift
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
for f in "$@"; do
    base="${f%.mms}"
    name="$(basename "$base")"
    mmixal -o "$TMP/$name.mmo" "$f" > "$TMP/al.log" 2>&1 || { echo "mmixal rejected $f"; cat "$TMP/al.log"; continue; }
    case "$mode" in
    dump)
        python3 "$HERE/mmodump.py" "$TMP/$name.mmo" > "$base.dump"
        ;;
    run)
        argv=""; [[ -e "$base.argv" ]] && read -r argv < "$base.argv"
        opt=""; [[ -e "$base.stdin" ]] && { cp "$base.stdin" "$TMP/stdin.txt"; opt="-fstdin.txt"; }
        if [[ -e "$base.files" ]]; then
            for g in $(cat "$base.files"); do cp "$(dirname "$f")/$g" "$TMP/"; done
        fi
        ( cd "$TMP" && mmix $opt "$name" $argv < /dev/null > out.txt 2> /dev/null; echo $? > exit.txt )
        cp "$TMP/out.txt" "$base.expected"
        if [[ "$(cat "$TMP/exit.txt")" == 0 ]]; then rm -f "$base.exit"; else cp "$TMP/exit.txt" "$base.exit"; fi
        ;;
    esac
done
