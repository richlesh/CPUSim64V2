#!/usr/bin/env bash
###############################################################################
# ide_check.sh - reproduces, step by step and without a GUI, what the CPUSim64
# IDE does for a Rust window (Run and Debug), and checks every step.
#
# Usage: tests/ide_check.sh [--no-big]
#
# What the IDE does (ide/.../IDEApp.java compileSource / compileWith /
# runCompiler / assemblerArgs / assembleForSourceDebug, CompilerPlugin.java):
#
#  1. CompilerPlugin.discover() syncs compilers/<name> to ~/.cpusim64/compilers
#     (syncBuiltIns/syncTree: not tests/, obj/, output/, hidden files).  This
#     script calls the real CompilerPlugin.syncBuiltIns from the IDE jar.
#  2. The translator rustc.asm is assembled when obj/rustc.o64 is missing or
#     older (Assembler.run: `rustc.asm [--hasMain]`), then moved to obj/.
#  3. Translation (Rust plug-in, translate_to: C11):
#        <sim> .../Rust/obj/rustc.o64 [--verbose] --mem=<compiler_mem> \
#              --stack=<compiler_stack> /abs/prog.rs [Options]
#     native simulator: working directory = folder of prog.rs, env PWD set to
#     it.  The translator writes /abs/prog.gen.c and finds lib/ from argv[0].
#  4. The C11 plug-in compiles prog.gen.c with ITS OWN config (C11 has no
#     compiler_mem/compiler_stack: the defaults 16M / 256K), writing
#     prog.gen.asm, which the IDE moves to prog.asm.
#  5. Run:   prog.asm [--hasMain] -> obj/prog.o64, then the simulator with
#            --verbose --mem=<Heap MiW * 1048576> --stack=<Stack KiB * 1024>
#            (defaults 1 MiW / 8 KiB, AppSettings).
#     Debug: the same with --DEBUG; the debugger shows prog.rs only if
#            IDEApp.srcmapMentions(obj/prog.srcmap, prog.rs) is true.
#
# Environment:
#   CPUSIM64_HOME  install with a current cpusim64-*.jar (and bin/cpusim64_native)
#                  (default: the root of this repository)
#   SIM            native (default) or java
#   IDE_JAR        the IDE jar (default: newest ide/target/CPUSim64IDE-*.jar)
#   HEAP_MIW, STACK_KIB   the IDE's Heap / Stack settings (default 1 and 8)
#   IDE_CHECK_WORK work directory (default: a new /tmp/rust_ide_check.XXXXXX;
#                  it is not deleted)
#
# Exit status: 0 if every check passes.
###############################################################################

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUST_DIR="$(dirname "$HERE")"
ROOT="$(cd "$RUST_DIR/../.." && pwd)"
HOME_DIR="${CPUSIM64_HOME:-$ROOT}"
SIM="${SIM:-native}"
HEAP_MIW="${HEAP_MIW:-1}"
STACK_KIB="${STACK_KIB:-8}"
BIG=1
[[ "$1" == "--no-big" ]] && BIG=0

JAR="$(ls "$HOME_DIR"/lib/cpusim64-*.jar 2>/dev/null | grep -v ' ' | head -1)"
NATIVE="$HOME_DIR/bin/cpusim64_native"
IDE_JAR="${IDE_JAR:-$(ls -t "$ROOT"/ide/target/CPUSim64IDE-*.jar 2>/dev/null | grep -v ' ' | head -1)}"
WORK="${IDE_CHECK_WORK:-$(mktemp -d /tmp/rust_ide_check.XXXXXX)}"
COMP="$WORK/home/.cpusim64/compilers"     # stands for ~/.cpusim64/compilers
PROJ="$WORK/proj"                         # the user's project folder
mkdir -p "$COMP" "$PROJ" "$WORK/entry"
PROJ="$(cd "$PROJ" && pwd -P)"

pass=0; fail=0; warn=0; failed=()
ok()   { pass=$((pass + 1)); echo "PASS  $1"; }
bad()  { fail=$((fail + 1)); failed+=("$1"); echo "FAIL  $1${2:+: $2}"; }
note() { warn=$((warn + 1)); echo "WARN  $1${2:+: $2}"; }
info() { echo "      $*"; }
to()   { local s="$1"; shift; perl -e 'alarm shift; exec @ARGV' -- "$s" "$@"; }

echo "work dir:  $WORK"
echo "simulator: $SIM   jar: $JAR"
echo "ide jar:   $IDE_JAR"
[[ -f "$JAR" ]] || { echo "FATAL: no $HOME_DIR/lib/cpusim64-*.jar"; exit 1; }
[[ "$SIM" == "java" || -x "$NATIVE" ]] || { echo "FATAL: no $NATIVE (use SIM=java)"; exit 1; }
[[ -f "$IDE_JAR" ]] || { echo "FATAL: no IDE jar (build ide, or set IDE_JAR)"; exit 1; }

# ---------------------------------------------------------------- Java probe
# runs the IDE's own classes (CompilerPlugin, IDEApp.definesFunction and
# IDEApp.srcmapMentions through reflection)
cat > "$WORK/IdeProbe.java" <<'EOF'
import com.glowingcat.cpusim64ide.CompilerPlugin;
import java.lang.reflect.Method;
import java.nio.file.*;
import java.util.regex.Pattern;

public class IdeProbe {
    public static void main(String[] a) throws Exception {
        switch (a[0]) {
        case "sync": {
            // sync <builtIn> <userDir> <outTsv>: CompilerPlugin.discover() first syncs the built-in plug-ins
            CompilerPlugin.syncBuiltIns(Path.of(a[1]), Path.of(a[2]));
            StringBuilder sb = new StringBuilder();
            for (CompilerPlugin p : CompilerPlugin.discover(Path.of(a[2]))) {
                boolean known = org.fife.ui.rsyntaxtextarea.TokenMakerFactory.getDefaultInstance()
                        .keySet().contains(p.getSyntaxLanguage());
                sb.append(p.getName()).append('\t').append(p.getSyntaxLanguage()).append('\t')
                  .append(String.join(",", p.getFileExtensions())).append('\t')
                  .append(p.getCompilerMem()).append('\t').append(p.getCompilerStack()).append('\t')
                  .append(p.getTranslateTo()).append('\t').append(p.getMainFunction()).append('\t')
                  .append(p.getEntryPattern()).append('\t').append(known).append('\t')
                  .append(p.getCompilerSource()).append('\t')
                  .append(p.acceptsFile("prog.rs")).append('\n');
            }
            Files.writeString(Path.of(a[3]), sb.toString());
            break;
        }
        case "entry": {
            // entry <userDir> <pluginName> <sampleDir>: IDEApp.isRunnable for each sample
            CompilerPlugin plugin = CompilerPlugin.find(CompilerPlugin.discover(Path.of(a[1])), a[2]);
            Method df = Class.forName("com.glowingcat.cpusim64ide.IDEApp")
                    .getDeclaredMethod("definesFunction", String.class, String.class);
            df.setAccessible(true);
            try (var files = Files.list(Path.of(a[3]))) {
                for (Path f : (Iterable<Path>) files.sorted()::iterator) {
                    String text = Files.readString(f);
                    boolean r;
                    if (plugin.getEntryPattern() != null) r = plugin.getEntryPattern().matcher(text).find();
                    else if (plugin.getEntryLabel() != null) r = false;
                    else r = (Boolean) df.invoke(null, text, plugin.getMainFunction());
                    System.out.println(f.getFileName() + "\t" + r);
                }
            }
            break;
        }
        case "srcmap": {
            // srcmap <file.srcmap> <source>: does the debugger show the source?
            Method m = Class.forName("com.glowingcat.cpusim64ide.IDEApp")
                    .getDeclaredMethod("srcmapMentions", Path.class, Path.class);
            m.setAccessible(true);
            System.out.println(m.invoke(null, Path.of(a[1]), Path.of(a[2])));
            break;
        }
        default: throw new IllegalArgumentException(a[0]);
        }
    }
}
EOF
probe() { ( cd "$WORK" && to 300 java -Djava.awt.headless=true -cp "$IDE_JAR" IdeProbe.java "$@" < /dev/null ); }

# ---------------------------------------------------------------- 1. plug-in sync
echo "== 1. plug-in discovery and copy to ~/.cpusim64/compilers"
if ! probe sync "$ROOT/compilers" "$COMP" "$WORK/plugins.tsv" > "$WORK/probe.log" 2>&1; then
    cat "$WORK/probe.log"; echo "FATAL: the IDE probe failed"; exit 1
fi
field() { awk -F'\t' -v n="$1" -v c="$2" '$1 == n { print $c }' "$WORK/plugins.tsv"; }
R=Rust
[[ -n "$(field $R 1)" ]] || { echo "FATAL: the Rust plug-in is not discovered"; exit 1; }
RSYN="$(field $R 2)"; RMEM="$(field $R 4)"; RSTACK="$(field $R 5)"; RTO="$(field $R 6)"
RMAIN="$(field $R 7)"; RENTRY="$(field $R 8)"; RKNOWN="$(field $R 9)"; RSRC="$(field $R 10)"; RACC="$(field $R 11)"
info "Rust: syntax=$RSYN mem=$RMEM stack=$RSTACK translate_to=$RTO main_function=$RMAIN entry_pattern=$RENTRY source=$RSRC"
[[ "$RKNOWN" == "true" ]] && ok "syntax_language $RSYN is supported by RSyntaxTextArea" \
    || bad "syntax_language $RSYN is not supported by RSyntaxTextArea"
[[ "$RACC" == "true" ]] && ok "prog.rs is accepted by file_extensions" || bad "prog.rs is not accepted by file_extensions"
[[ "$RTO" == "C11" ]] && ok "translate_to is C11" || bad "translate_to is '$RTO'"
CC_MEM="$(field C11 4)"; CC_STACK="$(field C11 5)"; CC_EXT="$(field C11 3 | cut -d, -f1)"; CC_SRC="$(field C11 10)"
info "C11: compiler_mem=$CC_MEM compiler_stack=$CC_STACK default extension=$CC_EXT (IDE: <base>.gen.$CC_EXT)"

RC="$COMP/Rust"
[[ ! -e "$RC/tests" ]] && ok "tests/ is not copied" || bad "tests/ was copied"
[[ ! -e "$RC/obj" ]] && ok "obj/ is not copied" || bad "obj/ was copied"
miss=""
for f in lib/core.rs lib/collections.rs lib/rc.rs lib/rust_rt.c rustc.asm config.properties; do
    [[ -f "$RC/$f" ]] || miss="$miss $f"
done
[[ -z "$miss" ]] && ok "lib/ (core.rs collections.rs rc.rs rust_rt.c) and rustc.asm are copied" || bad "missing in the copy:$miss"
if diff -rq "$RUST_DIR/src" "$RC/src" > /dev/null && diff -rq "$RUST_DIR/lib" "$RC/lib" > /dev/null; then
    ok "src/ and lib/ are identical in the copy"
else
    bad "src/ or lib/ differ in the copy" "$(diff -rq "$RUST_DIR/lib" "$RC/lib" | head -3)"
fi
skipped="$(cd "$RUST_DIR" && find . -path ./tests -prune -o -path ./obj -prune -o \( -name obj -o -name output -o -name '.*' \) -print | grep -v -e '^\.$' -e '\.DS_Store$' -e '\.gitignore$' | head -5)"
[[ -z "$skipped" ]] && ok "no file needed by the translator is in a skipped folder (obj/, output/, hidden)" \
    || note "these paths are skipped by syncTree (obj/, output/, hidden names)" "$skipped"

# ---------------------------------------------------------------- 2. entry detection
echo "== 2. Run/Debug enabled? (main_function detection, IDEApp.isRunnable)"
E="$WORK/entry"
printf 'fn main() {\n    println!("x");\n}\n' > "$E/yes_plain.rs"
printf 'pub fn main() {\n}\n' > "$E/yes_pub.rs"
printf 'fn main()\n{\n}\n' > "$E/yes_brace_next_line.rs"
printf 'fn helper<'"'"'a>(s: &'"'"'a str) -> &'"'"'a str { s }\n\nfn main() {\n}\n' > "$E/yes_after_lifetimes.rs"
printf 'fn main() -> () {\n}\n' > "$E/yes_unit_return.rs"
printf 'fn main() -> Result<(), String> {\n    Ok(())\n}\n' > "$E/yes_result_return.rs"
printf 'fn main() -> i32 {\n    0\n}\n' > "$E/yes_int_return.rs"
printf '// fn main() {}\nfn other() {}\n' > "$E/no_commented.rs"
printf 'fn helper() {\n}\n' > "$E/no_library.rs"
probe entry "$COMP" "$R" "$E" > "$WORK/entry.out" 2>&1
while IFS=$'\t' read -r name res; do
    want=true; [[ "$name" == no_* ]] && want=false
    if [[ "$res" == "$want" ]]; then ok "entry: $name -> runnable=$res"
    else bad "entry: $name -> runnable=$res, expected $want" "main_function matching is a C-style regex: name\\s*(args)\\s*{ (IDEApp.definesFunction); a return type before { is not recognised, Run/Debug stay disabled; fix: entry_pattern in config.properties"; fi
done < "$WORK/entry.out"

# ---------------------------------------------------------------- simulator helpers
# simrun OBJ MEM STACK ARGS...: output in $WORK/sim.out, "Result: N" in $WORK/sim.res
simrun() {
    local obj="$1" mem="$2" stack="$3" dir="$4"; shift 4
    : > "$WORK/sim.raw"
    if [[ "$SIM" == "java" ]]; then
        ( cd "$WORK" && to 900 java -Dfile.encoding=UTF8 -cp "$JAR" cloud.lesh.CPUSim64.Simulation \
            "$obj" --verbose --mem="$mem" --stack="$stack" "$@" < /dev/null > "$WORK/sim.raw" 2> "$WORK/sim.err" )
    else
        ( cd "$dir" && PWD="$dir" to 900 "$NATIVE" "$obj" --verbose --mem="$mem" --stack="$stack" "$@" \
            < /dev/null > "$WORK/sim.raw" 2> "$WORK/sim.err" )
    fi
    perl -e '
        local $/; my $s = <STDIN>;
        $s =~ s/\A.*?^Read \d+ words from [^\n]*\n//ms;
        my $res = "none";
        if ($s =~ /\A(.*?)^Result: (-?\d+)\n.*\z/ms) { $s = $1; $res = $2; }
        open(my $o, ">", $ARGV[0]); print $o $s; close $o;
        open(my $r, ">", $ARGV[1]); print $r "$res\n"; close $r;
    ' "$WORK/sim.out" "$WORK/sim.res" < "$WORK/sim.raw"
}
# assemble ASM [--DEBUG]: Assembler.run(assemblerArgs(...)) then moveAssemblerOutputs
assemble() {
    local asm="$1" dbg="$2" dir base args
    dir="$(dirname "$asm")"; base="$(basename "$asm" .asm)"
    args=("$asm")
    [[ -n "$dbg" ]] && args+=("--DEBUG")
    if grep -Eqi '^[[:space:]]*#(export|extern)\b' "$asm"; then
        note "$base.asm is a module (#export/#extern): the IDE would assemble it with -r and link it" "not reproduced"
    fi
    grep -qi '^[[:space:]]*__MAIN__:' "$asm" && args+=("--hasMain")
    to 600 java -Dfile.encoding=UTF8 -cp "$JAR" cloud.lesh.CPUSim64.Assembler "${args[@]}" < /dev/null > "$WORK/asm.log" 2>&1
    mkdir -p "$dir/obj"
    for e in .o64 .sym .sym1 .sym2 .srcmap; do
        [[ -f "$dir/$base$e" ]] && mv -f "$dir/$base$e" "$dir/obj/$base$e"
    done
    [[ -f "$dir/obj/$base.o64" ]]
}

# ---------------------------------------------------------------- 3. build the compilers
echo "== 3. assemble the plug-in compilers (needsBuild: obj/<compiler_source>.o64 is missing)"
if assemble "$RC/$RSRC.asm" ""; then ok "$RSRC.asm assembles -> obj/$RSRC.o64"; else cat "$WORK/asm.log" | tail -5; bad "$RSRC.asm does not assemble"; echo "FATAL"; exit 1; fi
CC="$COMP/C11"
if assemble "$CC/$CC_SRC.asm" ""; then ok "C11 $CC_SRC.asm assembles -> obj/$CC_SRC.o64"; else tail -5 "$WORK/asm.log"; bad "C11 compiler does not assemble"; echo "FATAL"; exit 1; fi

# ---------------------------------------------------------------- the pipeline
# pipeline NAME: PROJ/NAME.rs -> gen.c -> gen.asm -> NAME.asm -> obj/NAME.o64
pipeline() {
    local name="$1" src="$PROJ/$1.rs" base="$PROJ/$1"
    rm -f "$base.gen.c" "$base.gen.asm" "$base.asm"           # compileSource deletes stale results
    # --- translator
    simrun "$RC/obj/$RSRC.o64" "$RMEM" "$RSTACK" "$PROJ" "$src"
    cp "$WORK/sim.out" "$WORK/translate.out"
    if [[ "$(cat "$WORK/sim.res")" != 0 ]]; then
        bad "$name: translation failed (Result: $(cat "$WORK/sim.res"))" "$(head -3 "$WORK/sim.err") $(tail -3 "$WORK/sim.out")"; return 1
    fi
    [[ -f "$base.gen.c" ]] && ok "$name: translator wrote $name.gen.c (--mem=$RMEM --stack=$RSTACK)" \
        || { bad "$name: translator did not write $name.gen.c"; return 1; }
    # --- C11 plug-in, with ITS config
    simrun "$CC/obj/$CC_SRC.o64" "$CC_MEM" "$CC_STACK" "$PROJ" "$base.gen.c"
    if [[ "$(cat "$WORK/sim.res")" != 0 || ! -f "$base.gen.asm" ]]; then
        bad "$name: C11 compilation failed with the C11 plug-in's compiler_mem=$CC_MEM compiler_stack=$CC_STACK (Result: $(cat "$WORK/sim.res"))" "$(head -3 "$WORK/sim.out")"; return 1
    fi
    ok "$name: C11 compiled $name.gen.c -> $name.gen.asm (--mem=$CC_MEM --stack=$CC_STACK)"
    mv -f "$base.gen.asm" "$base.asm"
    return 0
}

# ---------------------------------------------------------------- 4. sample program
echo "== 4. sample program: translate, compile, assemble, run"
cat > "$PROJ/prog.rs" <<'EOF'
// sample
fn square(x: i32) -> i32 {
    x * x
}

fn main() {
    let mut sum = 0;
    for i in 1..4 {
        sum += square(i);
    }
    println!("sum = {}", sum);
}
EOF
if pipeline prog; then
    lines="$(grep '^#line ' "$PROJ/prog.gen.c" | sed 's/^#line [0-9]* "\(.*\)"$/\1/' | sort -u)"
    rel="$(echo "$lines" | grep -v '^/' | head -3)"
    if [[ -z "$rel" ]]; then ok "every #line names an absolute path (srcmapMentions needs one)"
    elif echo "$rel" | grep -q 'prog\.rs$'; then bad "the #line of the user's source is relative" "$rel"
    else
        # the translator names library files with display_path(): relative to $PWD when shorter.  The C11
        # compiler reads a relative name as relative to the .asm, so the library lines of the debugger
        # then name a file that does not exist.  Only with the Java simulator, whose $PWD is the IDE's.
        note "library #line names are relative to the IDE's \$PWD (Java simulator only; stepping into lib/core.rs finds no file)" "$rel"
    fi
    echo "$lines" | grep -qx "$PROJ/prog.rs" && ok "#line names the source $PROJ/prog.rs" || bad "no #line names $PROJ/prog.rs" "$(echo "$lines" | head -3)"
    echo "$lines" | grep -q "Rust/lib/core\.rs$" && ok "lib/core.rs was found through argv[0] (find_lib): $RC/lib" \
        || bad "lib/core.rs of the plug-in copy not named in #line" "$(echo "$lines" | grep -v prog.rs | head -3)"
    if assemble "$PROJ/prog.asm" ""; then
        ok "prog.asm assembles -> obj/prog.o64"
        simrun "$PROJ/obj/prog.o64" "$((HEAP_MIW * 1048576))" "$((STACK_KIB * 1024))" "$PROJ/obj"
        if [[ "$(cat "$WORK/sim.res")" == 0 && "$(cat "$WORK/sim.out")" == "sum = 14" ]]; then
            ok "run (--mem=$((HEAP_MIW * 1048576)) --stack=$((STACK_KIB * 1024)), the IDE defaults): sum = 14"
        else
            bad "run: Result $(cat "$WORK/sim.res"), output '$(head -3 "$WORK/sim.out")'" "$(head -3 "$WORK/sim.err")"
        fi
        # ---------------------------------------------------------- 5. debug
        echo "== 5. Debug: .source directives from #line"
        if assemble "$PROJ/prog.asm" --DEBUG; then
            ok "prog.asm assembles with --DEBUG"
            sm="$PROJ/obj/prog.srcmap"
            [[ "$(probe srcmap "$sm" "$PROJ/prog.rs" | tail -1)" == true ]] \
                && ok "IDEApp.srcmapMentions: the debugger shows prog.rs" \
                || bad "IDEApp.srcmapMentions is false: the debugger would open the generated assembly instead of prog.rs"
            info ".source in prog.asm: $(grep -c '^[[:space:]]*\.source "prog.rs"' "$PROJ/prog.asm") statements name \"prog.rs\" (relative to the .asm; the assembler makes the srcmap absolute)"
            perl -e '
                use utf8; use strict;
                binmode(STDOUT, ":utf8");
                my ($sm, $src) = @ARGV; my %lines; my %max;
                open my $F, "<:encoding(UTF-8)", $sm or die "no srcmap";
                while (<$F>) { if (/^\d+: «(.*)»:(\d+)/) { $lines{$1}{$2} = 1; $max{$1} = $2 if !defined $max{$1} || $2 > $max{$1}; } }
                my $bad = 0;
                for my $f (sort keys %max) {
                    next unless $f =~ m{\.rs$} && $f =~ m{^/} && -f $f;
                    open my $G, "<", $f; my @t = <$G>; close $G;
                    my @over = sort { $a <=> $b } grep { $_ > scalar(@t) } keys %{$lines{$f}};
                    my @blank = sort { $a <=> $b } grep { $_ >= 1 && $_ <= @t && $t[$_ - 1] !~ /\S/ } keys %{$lines{$f}};
                    my $short = $f; $short =~ s{.*/}{};
                    my @all = sort { $a <=> $b } keys %{$lines{$f}};
                    my @shown = @all > 30 ? @all[0 .. 29] : @all;
                    printf "      %s: file has %d lines, %d distinct mapped lines (%s%s), highest %d\n", $short, scalar(@t),
                        scalar(@all), join(",", @shown), (@all > 30 ? ",..." : ""), $max{$f};
                    if (@over || @blank) { $bad = 1; print "LINES-OUT-OF-RANGE $short: ", scalar(@over), " mapped lines beyond the end of the file (first: ", join(",", @over[0 .. ($#over > 4 ? 4 : $#over)]), "), ", scalar(@blank), " on blank lines\n"; }
                }
                exit($bad);
            ' "$sm" "$PROJ/prog.rs" < /dev/null > "$WORK/lines.out" 2>&1
            lrc=$?
            grep -v '^LINES-OUT' "$WORK/lines.out"
            if [[ $lrc == 0 ]]; then ok "every .rs line in the srcmap exists in its file"
            else bad "the srcmap maps code to .rs lines that do not exist (breakpoints and the current-line highlight drift)" "$(grep '^LINES-OUT' "$WORK/lines.out" | head -3 | tr '\n' ' ') -- cause: the translator emits one #line per Rust statement (cg_line, cg_types.c) but the C11 lexer counts every following C line"; fi
        else
            tail -5 "$WORK/asm.log"; bad "prog.asm does not assemble with --DEBUG"
        fi
    else
        tail -5 "$WORK/asm.log"; bad "prog.asm does not assemble"
    fi
fi

# ---------------------------------------------------------------- 6. a bigger program
if [[ $BIG == 1 ]]; then
    echo "== 6. bigger program (tests/run/t5_tokenizer.rs): C11 with compiler_mem=$CC_MEM compiler_stack=$CC_STACK"
    if [[ -f "$HERE/run/t5_tokenizer.rs" ]]; then
        cp "$HERE/run/t5_tokenizer.rs" "$PROJ/big.rs"
        if pipeline big; then
            if assemble "$PROJ/big.asm" ""; then
                simrun "$PROJ/obj/big.o64" "$((HEAP_MIW * 1048576))" "$((STACK_KIB * 1024))" "$PROJ/obj"
                if [[ "$(cat "$WORK/sim.res")" == 0 ]] && diff -q "$WORK/sim.out" "$HERE/run/t5_tokenizer.expected" > /dev/null; then
                    ok "big: output equals t5_tokenizer.expected with the IDE's default Heap/Stack"
                else
                    bad "big: Result $(cat "$WORK/sim.res"); output differs from t5_tokenizer.expected with the IDE's default Heap=$HEAP_MIW MiW Stack=$STACK_KIB KiB" "$(head -3 "$WORK/sim.err")"
                fi
            else
                bad "big.asm does not assemble"
            fi
        fi
    else
        note "tests/run/t5_tokenizer.rs not found" "skipped"
    fi
fi

echo "----------------------------------------"
echo "passed: $pass  failed: $fail  warnings: $warn   (work dir: $WORK)"
for f in "${failed[@]}"; do echo "  failed: $f"; done
[[ $fail -eq 0 ]]
