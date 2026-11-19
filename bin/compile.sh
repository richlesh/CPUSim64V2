#!/usr/bin/env bash
###############################################################################
# compile.sh - compile a program of one or more source files with a compiler
#              plug-in from the compilers/ folder and link it into an .o64
#
# Usage: compile.sh LANGUAGE [options] source [more sources ...]
#
#   LANGUAGE  the name of a folder in compilers/ (C11, C++14, Rust, VBNET,
#             MMIX, ...; case does not matter)
#   options   words starting with '-' are passed to the LANGUAGE compiler
#             (e.g. -lstd, -DNAME, --lib=DIR)
#   sources   the first source is the main file; the program is written to
#             obj/<first source without extension>.o64 next to it
#
# Steps, following compilers/<LANGUAGE>/config.properties (as the IDE does):
#   1. The compiler (compiler_source) is assembled into its obj/ folder when
#      the object file is missing or older than any .asm or .def file of the
#      plug-in.  It runs with --mem=compiler_mem and --stack=compiler_stack
#      (default 16M and 256K).
#   2. Without translate_to, the compiler runs once on all the sources and
#      writes <source>.asm (one module per file for compilers that make
#      modules, such as C11; one program named after the first file for others).
#   3. With translate_to: TARGET, the compiler is a translator.  The first
#      source is translated with the others named after it, and each other
#      source with --unit, to <source>.gen.<ext> (ext = the first file
#      extension of TARGET).  TARGET's compiler then compiles all of them
#      (with -lstd when there are several), and each <source>.gen.asm is
#      renamed to <source>.asm.
#   4. When the generated code is made of modules (#export / #extern), every
#      generated .asm is assembled with assemble.sh -r into
#      obj/<source>.ro64, and link.sh links the modules into the .o64.  A
#      complete program (no #export / #extern, e.g. from VBNET or MMIX) is
#      assembled in one piece, as the IDE does: its run-time files only
#      assemble together with it.
#
# Header files (.h, .hh, .hpp, .hxx) named as sources are skipped: they are
# found through #include.
###############################################################################

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
COMPILERS_DIR="$SCRIPT_DIR/compilers"
# the jar (not a copy such as "cpusim64-2.12.0 2.jar" that iCloud or Finder makes)
for j in "$SCRIPT_DIR"/lib/cpusim64-*.jar; do case "${j##*/}" in *" "*) ;; *) CLASSPATH="$j" ;; esac; done
NATIVE="$SCRIPT_DIR/bin/cpusim64_native"

usage() {
    echo "Usage: $(basename "$0") LANGUAGE [options] source [more sources ...]"
    echo "Languages:"
    for d in "$COMPILERS_DIR"/*/; do
        [[ -f "$d/config.properties" ]] && echo "  $(basename "$d")"
    done
    exit 2
}

die() { echo "compile.sh: $*" >&2; exit 1; }

# plugin_dir NAME : prints the plug-in folder whose name matches NAME (ignoring case)
plugin_dir() {
    local want d n
    want="$(printf '%s' "$1" | tr '[:upper:]' '[:lower:]')"
    for d in "$COMPILERS_DIR"/*/; do
        d="${d%/}"
        n="$(basename "$d" | tr '[:upper:]' '[:lower:]')"
        if [[ "$n" == "$want" && -f "$d/config.properties" ]]; then
            printf '%s\n' "$d"
            return 0
        fi
    done
    return 1
}

# prop DIR KEY [DEFAULT] : value of KEY in DIR/config.properties ("key: value" or "key=value")
prop() {
    local v
    v="$(sed -n -E "s/^[[:space:]]*$2[[:space:]]*[:=][[:space:]]*(.*[^[:space:]])[[:space:]]*\$/\\1/p" \
        "$1/config.properties" | head -1)"
    printf '%s\n' "${v:-$3}"
}

# first_ext DIR : the plug-in's first source file extension
first_ext() {
    prop "$1" file_extensions | tr ',' ' ' | awk '{ sub(/^\./, "", $1); print tolower($1) }'
}

# accepts DIR FILE : true if FILE has one of the plug-in's extensions (or it names none)
accepts() {
    local exts e lower
    exts="$(prop "$1" file_extensions | tr ',' ' ')"
    [[ -z "$exts" ]] && return 0
    lower="$(printf '%s' "$2" | tr '[:upper:]' '[:lower:]')"
    for e in $exts; do
        e="${e#.}"
        [[ "$lower" == *."$(printf '%s' "$e" | tr '[:upper:]' '[:lower:]')" ]] && return 0
    done
    return 1
}

strip_ext() { printf '%s\n' "${1%.*}"; }

# build_compiler DIR : assembles the plug-in's compiler when needed; prints the object file
build_compiler() {
    local dir="$1" src obj f
    src="$(prop "$dir" compiler_source)"
    src="${src%.asm}"
    [[ -n "$src" ]] || die "$dir/config.properties has no compiler_source"
    [[ -f "$dir/$src.asm" ]] || die "compiler source not found: $dir/$src.asm"
    obj="$dir/obj/$src.o64"
    local stale=false
    if [[ ! -f "$obj" ]]; then
        stale=true
    else
        for f in "$dir"/*.asm "$dir"/*.def; do
            [[ -e "$f" && "$f" -nt "$obj" ]] && { stale=true; break; }
        done
    fi
    if [[ "$stale" == "true" ]]; then
        echo "> Assembling the $(basename "$dir") compiler ($src.asm)..." >&2
        rm -f "$obj"        # assemble.sh skips a file whose .o64 is newer than its main .asm
        "$SCRIPT_DIR/assemble.sh" "$dir/$src" > "$TMPDIR_C/asmlog" 2>&1
        if [[ ! -f "$obj" ]]; then
            cat "$TMPDIR_C/asmlog" >&2
            die "assembly of the $(basename "$dir") compiler failed"
        fi
    fi
    printf '%s\n' "$obj"
}

# run_compiler DIR ARGS... : runs DIR's compiler on ARGS in the main source's
# folder; shows its output without the simulator's banner and statistics and
# returns the compiler's result
run_compiler() {
    local dir="$1"; shift
    local obj mem stack
    obj="$(build_compiler "$dir")" || exit 1
    mem="$(prop "$dir" compiler_mem 16M)"
    stack="$(prop "$dir" compiler_stack 256K)"
    echo "> $(basename "$dir"): $(basename "$obj" .o64) $*" | sed "s#$WORK_DIR/##g"
    if [[ -x "$NATIVE" ]]; then
        ( cd "$WORK_DIR" && "$NATIVE" "$obj" --verbose --mem="$mem" --stack="$stack" "$@" ) \
            > "$TMPDIR_C/raw" 2>&1 < /dev/null
    else
        ( cd "$WORK_DIR" && java -Dfile.encoding=UTF8 -cp "$CLASSPATH" cloud.lesh.CPUSim64.Simulation \
            "$obj" --verbose --mem="$mem" --stack="$stack" "$@" ) > "$TMPDIR_C/raw" 2>&1 < /dev/null
    fi
    # drop everything up to "Read N words from ..." and from "Result: N" on
    awk '
        !body { if ($0 ~ /^Read [0-9]+ words from /) body = 1; next }
        /^Result: -?[0-9]+$/ { exit }
        { print }
    ' "$TMPDIR_C/raw"
    local res
    res="$(sed -n -E 's/^Result: (-?[0-9]+)$/\1/p' "$TMPDIR_C/raw" | tail -1)"
    if [[ -z "$res" ]]; then
        # no result line: the simulator itself failed (e.g. an exception)
        grep -v '^Read [0-9]* words from' "$TMPDIR_C/raw" | tail -5 >&2
        return 1
    fi
    return "$(( res == 0 ? 0 : 1 ))"
}

# ---------------------------------------------------------------- arguments
[[ $# -ge 2 ]] || usage
LANG_NAME="$1"; shift
PLUGIN="$(plugin_dir "$LANG_NAME")" || { echo "compile.sh: no compiler '$LANG_NAME' in $COMPILERS_DIR" >&2; usage; }

OPTIONS=()
SOURCES=()
for a in "$@"; do
    if [[ "$a" == -* ]]; then
        OPTIONS+=("$a")
        continue
    fi
    [[ -f "$a" ]] || die "source file not found: $a"
    case "$(printf '%s' "$a" | tr '[:upper:]' '[:lower:]')" in
        *.h|*.hh|*.hpp|*.hxx) continue ;;
    esac
    accepts "$PLUGIN" "$a" || die "$a is not a $(basename "$PLUGIN") source ($(prop "$PLUGIN" file_extensions))"
    SOURCES+=("$(realpath "$a")")
done
[[ ${#SOURCES[@]} -ge 1 ]] || die "no source files"

MAIN="${SOURCES[0]}"
WORK_DIR="$(dirname "$MAIN")"
MAIN_BASE="$(strip_ext "$MAIN")"
OUT_NAME="$(basename "$MAIN_BASE")"
TMPDIR_C="$(mktemp -d)"
trap 'rm -rf "$TMPDIR_C"' EXIT

# never use a stale result of an earlier compile
for s in "${SOURCES[@]}"; do
    b="$(strip_ext "$s")"
    rm -f "$b.asm" "$b".gen.* "$(dirname "$b")/obj/$(basename "$b").ro64"
done
rm -f "$WORK_DIR/obj/$OUT_NAME.o64"

# ---------------------------------------------------------------- compile
TARGET_NAME="$(prop "$PLUGIN" translate_to)"
if [[ -z "$TARGET_NAME" ]]; then
    run_compiler "$PLUGIN" "${SOURCES[@]}" ${OPTIONS[@]+"${OPTIONS[@]}"} || die "compilation failed"
else
    TARGET="$(plugin_dir "$TARGET_NAME")" || die "the $(basename "$PLUGIN") compiler needs the $TARGET_NAME compiler, which was not found"
    [[ -z "$(prop "$TARGET" translate_to)" ]] || die "the $TARGET_NAME compiler must not itself use translate_to"
    EXT="$(first_ext "$TARGET")"
    EXT="${EXT:-src}"

    # the main file, with the other units named after it
    run_compiler "$PLUGIN" "${SOURCES[@]}" ${OPTIONS[@]+"${OPTIONS[@]}"} || die "translation of $(basename "$MAIN") failed"
    [[ -f "$MAIN_BASE.gen.$EXT" ]] || die "the translator did not write $(basename "$MAIN_BASE").gen.$EXT"
    GENS=("$MAIN_BASE.gen.$EXT")
    for s in "${SOURCES[@]:1}"; do
        b="$(strip_ext "$s")"
        run_compiler "$PLUGIN" --unit "$s" ${OPTIONS[@]+"${OPTIONS[@]}"} || die "translation of $(basename "$s") failed"
        [[ -f "$b.gen.$EXT" ]] || die "the translator did not write $(basename "$b").gen.$EXT"
        GENS+=("$b.gen.$EXT")
    done
    LSTD=()
    [[ ${#GENS[@]} -gt 1 ]] && LSTD=(-lstd)       # the modules share the target's library
    run_compiler "$TARGET" "${GENS[@]}" ${LSTD[@]+"${LSTD[@]}"} || die "$TARGET_NAME compilation of the generated code failed"

    # <source>.gen.asm becomes <source>.asm; the main module includes the others by name
    for s in "${SOURCES[@]}"; do
        b="$(strip_ext "$s")"
        [[ -f "$b.gen.asm" ]] || continue
        mv -f "$b.gen.asm" "$b.asm"
        if [[ "$s" != "$MAIN" ]]; then
            n="$(basename "$b")"
            N="$n" perl -pi -e 's/\Q$ENV{N}\E\.gen\.asm"/$ENV{N}.asm"/g' "$MAIN_BASE.asm"
        fi
    done
fi
[[ -f "$MAIN_BASE.asm" ]] || die "the compiler did not write $(basename "$MAIN_BASE").asm"

# ---------------------------------------------------------------- assemble
ASMS=()
for s in "${SOURCES[@]}"; do
    b="$(strip_ext "$s")"
    [[ -f "$b.asm" ]] && ASMS+=("$b.asm")        # compilers such as MMIX write one .asm for all files
done

# A complete program (no #export / #extern, e.g. from VBNET or MMIX) may #include
# run-time files that only assemble together with it, so it is assembled in one
# piece, as the IDE does.  Modules are assembled with -r and linked.
IS_MODULE=false
for a in "${ASMS[@]}"; do
    grep -qiE '^[[:space:]]*#(export|extern)\b' "$a" && { IS_MODULE=true; break; }
done
if [[ "$IS_MODULE" == "false" && ${#ASMS[@]} -eq 1 ]]; then
    echo "> Assembling $(basename "${ASMS[0]}") (a complete program, not a module: no linking)"
    "$SCRIPT_DIR/assemble.sh" "$(strip_ext "${ASMS[0]}")" > "$TMPDIR_C/asmlog" 2>&1
    if [[ ! -f "$WORK_DIR/obj/$OUT_NAME.o64" ]]; then
        grep -iE "error|exception" "$TMPDIR_C/asmlog" | head -10 >&2
        die "assembly of $(basename "${ASMS[0]}") failed"
    fi
    echo "Wrote $WORK_DIR/obj/$OUT_NAME.o64"
    exit 0
fi

MODULES=()
for a in "${ASMS[@]}"; do
    b="$(strip_ext "$a")"
    echo "> Assembling $(basename "$b").asm"
    "$SCRIPT_DIR/assemble.sh" -r "$b.asm" > "$TMPDIR_C/asmlog" 2>&1
    ro="$(dirname "$b")/obj/$(basename "$b").ro64"
    if [[ ! -f "$ro" ]]; then
        grep -iE "error|exception" "$TMPDIR_C/asmlog" | head -10 >&2
        die "assembly of $(basename "$b").asm failed"
    fi
    MODULES+=("$ro")
done

# ---------------------------------------------------------------- link
echo "> Linking $WORK_DIR/obj/$OUT_NAME.o64" | sed "s#$WORK_DIR/##"
"$SCRIPT_DIR/link.sh" -o "$WORK_DIR/obj/$OUT_NAME" "${MODULES[@]}" > "$TMPDIR_C/linklog" 2>&1
if [[ ! -f "$WORK_DIR/obj/$OUT_NAME.o64" ]]; then
    grep -iE "error|exception" "$TMPDIR_C/linklog" | head -10 >&2
    die "linking failed"
fi
echo "Wrote $WORK_DIR/obj/$OUT_NAME.o64"
