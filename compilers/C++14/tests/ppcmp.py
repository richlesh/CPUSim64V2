#!/usr/bin/env python3
"""ppcmp.py - compares two preprocessed C++ outputs token by token.

Usage: ppcmp.py OURS THEIRS

Both files are split into preprocessing tokens (white space and line markers
are ignored), so differences in spacing and line breaks do not matter.
Exit status 0 if the token sequences are equal; otherwise the first
difference is printed.
"""
import re
import sys

TOKEN = re.compile(r'''
    (?:u8|u|U|L)?R"(?P<d>[^()\\ ]{0,16})\((?:.|\n)*?\)(?P=d)"   # raw string
  | (?:u8|u|U|L)?"(?:[^"\\\n]|\\.)*"                              # string
  | (?:u8|u|U|L)?'(?:[^'\\\n]|\\.)*'                              # char
  | \.?[0-9](?:[eEpP][+-]|[0-9a-zA-Z_.']|\\.)*                    # pp-number
  | [A-Za-z_$][A-Za-z_0-9$]*                                      # identifier
  | %:%:|%:|<%|%>|<:|:>|\.\.\.|->\*|<<=|>>=|\#\#|::|\.\*|->|\+\+|--|<<|>>
  | <=|>=|==|!=|&&|\|\||\+=|-=|\*=|/=|%=|\^=|&=|\|=
  | \S
''', re.X)

ALT = {'%:%:': '##', '%:': '#', '<%': '{', '%>': '}', '<:': '[', ':>': ']',
       'and': '&&', 'or': '||', 'not': '!', 'bitand': '&', 'bitor': '|',
       'xor': '^', 'compl': '~', 'and_eq': '&=', 'or_eq': '|=', 'xor_eq': '^=',
       'not_eq': '!='}

def tokens(path):
    text = open(path, encoding='utf-8', errors='replace').read()
    out = []
    for line in text.split('\n'):
        if re.match(r'\s*#\s*\d+\s', line):      # a line marker of g++ -E
            continue
        out.extend(m.group(0) for m in TOKEN.finditer(line))
    return [ALT.get(t, t) for t in out]

a, b = tokens(sys.argv[1]), tokens(sys.argv[2])
if a == b:
    sys.exit(0)
for i in range(min(len(a), len(b))):
    if a[i] != b[i]:
        print("token %d: ours %r, theirs %r; ours: %s | theirs: %s" % (
            i, a[i], b[i], ' '.join(a[max(0, i - 4):i + 4]), ' '.join(b[max(0, i - 4):i + 4])))
        break
else:
    print("one output is longer: ours %d tokens, theirs %d tokens" % (len(a), len(b)))
sys.exit(1)
