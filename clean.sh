find . -iname \*.o64 -exec rm "{}" \;
find . -iname \*.pp.asm -exec rm "{}" \;
find . -iname \*.sym\* -exec rm "{}" \;
find . -iname \*~.\* -exec rm "{}" \;
find . -iname \*~\ \* -exec rm "{}" \;
