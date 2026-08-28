#!/usr/bin/env python3

import glob
import os
import subprocess

os.chdir(os.path.dirname(os.path.abspath(__file__)))

# Remove all .o64 and .sym* files
for f in glob.glob("*.bin") + glob.glob("*.out") + glob.glob("*.o64") + glob.glob("*.sym*") + glob.glob("*.pp.*"):
    os.remove(f)

# Run all .asm files
asm_files = sorted(glob.glob("*.asm"))
for asm in asm_files:
    name = asm[:-4]
    if name == "T105_SYNC":
        with open(f"{name}.out", "w") as out:
            subprocess.run(["../../run.sh", name, "--mem=1M"], stdout=out, stderr=out)
    else:
        with open(f"{name}.out", "w") as out:
            subprocess.run(["../../debug.sh", name, "--mem=1M"], stdout=out, stderr=out)

def extract_relevant(filepath, name):
    lines = []
    capturing = False
    with open(filepath) as f:
        for line in f:
            if not capturing and line.startswith(f"Read ") and f"{name}.o64" in line:
                capturing = True
                continue
            if capturing:
                if line.startswith("User CPU Cycles"):
                    break
                lines.append(line)
    return lines

# Compare outputs to reference
for asm in asm_files:
    name = asm[:-4]
    out_file = f"{name}.out"
    obj_file = f"obj/{name}.o64"
    ref_file = f"reference/{name}.out"
    if not os.path.exists(obj_file):
        print(f"{name}: COMPILE ERROR")
        continue
    if not os.path.exists(ref_file):
        print(f"{name}: NO REFERENCE")
        continue
    result = "PASS" if extract_relevant(out_file, name) == extract_relevant(ref_file, name) else "FAIL"
    print(f"{name}: {result}")

# Compare .bin files to reference
for bin_file in sorted(glob.glob("*.bin")):
    ref_file = f"reference/{bin_file}"
    name = bin_file[:-4]
    if not os.path.exists(ref_file):
        print(f"{name}.bin: NO REFERENCE")
        continue
    with open(bin_file, "rb") as f1, open(ref_file, "rb") as f2:
        result = "PASS" if f1.read() == f2.read() else "FAIL"
    print(f"{name}.bin: {result}")
