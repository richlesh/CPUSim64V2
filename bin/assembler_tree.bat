@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"

call "%SCRIPT_DIR%preprocess.bat" %1 %2 %3 %4 %5 %6 %7 %8 %9
java -Dfile.encoding=UTF8 -cp "%SCRIPT_DIR%lib\*" org.antlr.v4.gui.TestRig cloud.lesh.CPUSim64.CPUSim64 program -tokens "%~1.pp.asm" -tokens -gui

:: Generate LR parse tree as SVG via Graphviz
set "DOT_FILE=%~1_tree.dot"
set "SVG_FILE=%~1_tree.svg"

:: Get tree output
java -Dfile.encoding=UTF8 -cp "%SCRIPT_DIR%lib\*" org.antlr.v4.gui.TestRig cloud.lesh.CPUSim64.CPUSim64 program "%~1.pp.asm" -tree > "%TEMP%\tree_output.txt"
set /p TREE_OUTPUT=<"%TEMP%\tree_output.txt"

python -c "import sys; tree_str = open(sys.argv[1]).read().strip(); node_id = [0]; exec('''^ndef next_id():^n    node_id[0] += 1^n    return node_id[0]^ndef parse_tree(s, pos=0):^n    result_edges = []^n    result_labels = []^n    if pos >= len(s):^n        return pos, None, result_edges, result_labels^n    if s[pos] == '(':^n        pos += 1^n        name_start = pos^n        while pos < len(s) and s[pos] not in (' ', ')', '('):^n            pos += 1^n        name = s[name_start:pos]^n        my_id = next_id()^n        result_labels.append((my_id, name, True))^n        while pos < len(s) and s[pos] != ')':^n            if s[pos] == ' ':^n                pos += 1^n                continue^n            if s[pos] == '(':^n                pos, child_id, child_edges, child_labels = parse_tree(s, pos)^n                if child_id is not None:^n                    result_edges.append((my_id, child_id))^n                    result_edges.extend(child_edges)^n                    result_labels.extend(child_labels)^n            else:^n                token_start = pos^n                while pos < len(s) and s[pos] not in (' ', ')'):^n                    pos += 1^n                token = s[token_start:pos]^n                child_id = next_id()^n                result_labels.append((child_id, token, False))^n                result_edges.append((my_id, child_id))^n        if pos < len(s) and s[pos] == ')':^n            pos += 1^n        return pos, my_id, result_edges, result_labels^n    else:^n        token_start = pos^n        while pos < len(s) and s[pos] not in (' ', ')'):^n            pos += 1^n        token = s[token_start:pos]^n        my_id = next_id()^n        result_labels.append((my_id, token, False))^n        return pos, my_id, result_edges, result_labels^n'''.replace('^n', chr(10))); _, root_id, edges, labels = parse_tree(tree_str); dot_lines = ['digraph ParseTree {', '    rankdir=LR;', '    ordering=out;', '    nodesep=0.2;', '    ranksep=0.3;', '    node [shape=none, fontname=\"Helvetica\", fontsize=10];', '    edge [arrowsize=0.4];', '']; [dot_lines.append(f'    n{nid} [label=\"{label.replace(chr(92), chr(92)+chr(92)).replace(chr(34), chr(92)+chr(34))}\", fontcolor=\"{\"blue\" if is_rule else \"darkgreen\"}\"];') for nid, label, is_rule in labels]; dot_lines.append(''); [dot_lines.append(f'    n{p} -> n{c};') for p, c in edges]; dot_lines.append('}'); print(chr(10).join(dot_lines))" "%TEMP%\tree_output.txt" > "%DOT_FILE%"

where dot >nul 2>&1
if not errorlevel 1 (
    dot -Tsvg -o "%SVG_FILE%" "%DOT_FILE%"
    echo Generated: %DOT_FILE% and %SVG_FILE%
    start "" "%SVG_FILE%"
) else (
    echo Generated: %DOT_FILE%
    echo Install Graphviz to generate SVG: https://graphviz.org/download/
)
