#!/usr/bin/env bash

SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"

java -cp "$SCRIPT_DIR"/lib/cpusim64-*.jar org.antlr.v4.gui.TestRig cloud.lesh.CPUSim64.Preprocessor preproc "$1.asm" -tokens -gui

# Generate top-down parse tree as SVG via Graphviz
TREE_OUTPUT=$(java -cp "$SCRIPT_DIR"/lib/cpusim64-*.jar org.antlr.v4.gui.TestRig cloud.lesh.CPUSim64.Preprocessor preproc "$1.asm" -tree)

DOT_FILE="$1_tree.dot"
SVG_FILE="$1_tree.svg"

python3 -c "
import sys
import re

tree_str = sys.argv[1]
node_id = [0]
edges = []
labels = []

def next_id():
    node_id[0] += 1
    return node_id[0]

def parse_tree(s, pos=0):
    \"\"\"Parse LISP-style tree string into a dot graph.\"\"\"
    result_edges = []
    result_labels = []

    if pos >= len(s):
        return pos, None, result_edges, result_labels

    if s[pos] == '(':
        pos += 1  # skip '('
        # Read rule name
        name_start = pos
        while pos < len(s) and s[pos] not in (' ', ')', '('):
            pos += 1
        name = s[name_start:pos]
        my_id = next_id()
        result_labels.append((my_id, name, True))

        # Parse children
        while pos < len(s) and s[pos] != ')':
            if s[pos] == ' ':
                pos += 1
                continue
            if s[pos] == '(':
                pos, child_id, child_edges, child_labels = parse_tree(s, pos)
                if child_id is not None:
                    result_edges.append((my_id, child_id))
                    result_edges.extend(child_edges)
                    result_labels.extend(child_labels)
            else:
                # Terminal token
                token_start = pos
                while pos < len(s) and s[pos] not in (' ', ')'):
                    pos += 1
                token = s[token_start:pos]
                child_id = next_id()
                result_labels.append((child_id, token, False))
                result_edges.append((my_id, child_id))

        if pos < len(s) and s[pos] == ')':
            pos += 1  # skip ')'

        return pos, my_id, result_edges, result_labels
    else:
        # Bare token (shouldn't happen at top level)
        token_start = pos
        while pos < len(s) and s[pos] not in (' ', ')'):
            pos += 1
        token = s[token_start:pos]
        my_id = next_id()
        result_labels.append((my_id, token, False))
        return pos, my_id, result_edges, result_labels

_, root_id, edges, labels = parse_tree(tree_str.strip())

dot_lines = ['digraph ParseTree {', '    rankdir=LR;', '    ordering=out;', '    nodesep=0.2;', '    ranksep=0.3;', '    node [shape=none, fontname=\"Helvetica\", fontsize=10];', '    edge [arrowsize=0.4];', '']

for nid, label, is_rule in labels:
    escaped = label.replace('\\\\', '\\\\\\\\').replace('\"', '\\\\\"')
    if is_rule:
        dot_lines.append(f'    n{nid} [label=\"{escaped}\", fontcolor=\"blue\"];')
    else:
        dot_lines.append(f'    n{nid} [label=\"{escaped}\", fontcolor=\"darkgreen\"];')

dot_lines.append('')
for parent, child in edges:
    dot_lines.append(f'    n{parent} -> n{child};')

dot_lines.append('}')
print('\\n'.join(dot_lines))
" "$TREE_OUTPUT" > "$DOT_FILE"

if command -v dot &> /dev/null; then
    dot -Tsvg -o "$SVG_FILE" "$DOT_FILE"
    echo "Generated: $DOT_FILE and $SVG_FILE"
    open "$SVG_FILE" 2>/dev/null || xdg-open "$SVG_FILE" 2>/dev/null || echo "Open $SVG_FILE to view the tree"
else
    echo "Generated: $DOT_FILE"
    echo "Install Graphviz to generate SVG: brew install graphviz"
fi
