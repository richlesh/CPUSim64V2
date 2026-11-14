#!/usr/bin/env python3
"""Writes random MMIX test programs for differential testing.

Usage: gen_random.py SEED [COUNT] > prog.mms

The program sets $0..$15 to random values (integers, floating-point bit
patterns, special values), runs COUNT random instructions, and after each
one records the destination register and rA (and rH or rR where they
change) in a result area.  At the end it prints the result area in hex.
Running it on the real mmix and on the translated program must give the
same output.  Arithmetic exceptions only set event bits in rA (no trip
handlers are enabled); rounding modes change through PUT rA.
"""
import random
import sys

seed = int(sys.argv[1])
count = int(sys.argv[2]) if len(sys.argv) > 2 else 200
rnd = random.Random(seed)

SPECIAL = [0, 1, 0x8000000000000000, 0x7fffffffffffffff, 0xffffffffffffffff,
           0x7ff0000000000000, 0xfff0000000000000, 0x7ff8000000000000,
           0x7ff4000000000000, 0x3ff0000000000000, 0xbff0000000000000,
           0x0000000000000001, 0x000fffffffffffff, 0x0010000000000000,
           0x7fefffffffffffff, 0x4340000000000000, 0x43e0000000000000,
           0xc3e0000000000000, 0x3fe0000000000000, 0x4000000000000000]


def rval():
    k = rnd.randrange(10)
    if k == 0:
        return rnd.choice(SPECIAL)
    if k == 1:
        return rnd.randrange(-300, 300) & (2**64 - 1)
    if k in (2, 3):     # normal double near 1
        e = 1023 + rnd.randrange(-40, 60)
        return (rnd.randrange(2) << 63) | (e << 52) | rnd.getrandbits(52)
    if k == 4:          # double with few mantissa bits
        e = 1023 + rnd.randrange(0, 70)
        return (rnd.randrange(2) << 63) | (e << 52) | (rnd.getrandbits(12) << 40)
    if k == 5:          # tiny / huge exponents
        e = rnd.choice([0, 1, 2, 2045, 2046])
        return (rnd.randrange(2) << 63) | (e << 52) | rnd.getrandbits(52)
    return rnd.getrandbits(64)


def reg():
    return rnd.randrange(16)


INT3 = ["ADD", "ADDU", "SUB", "SUBU", "2ADDU", "4ADDU", "8ADDU", "16ADDU",
        "CMP", "CMPU", "MUL", "MULU", "DIV", "DIVU", "SL", "SLU", "SR", "SRU",
        "OR", "ORN", "NOR", "XOR", "AND", "ANDN", "NAND", "NXOR",
        "BDIF", "WDIF", "TDIF", "ODIF", "MUX", "SADD", "MOR", "MXOR",
        "CSN", "CSZ", "CSP", "CSOD", "CSNN", "CSNZ", "CSNP", "CSEV",
        "ZSN", "ZSZ", "ZSP", "ZSOD", "ZSNN", "ZSNZ", "ZSNP", "ZSEV"]
FLT3 = ["FADD", "FSUB", "FMUL", "FDIV", "FREM", "FCMP", "FUN", "FEQL",
        "FCMPE", "FUNE", "FEQLE"]
FLT2 = ["FSQRT", "FINT", "FIX", "FIXU"]
CONV = ["FLOT", "FLOTU", "SFLOT", "SFLOTU"]
WYDE = ["SETH", "SETMH", "SETML", "SETL", "INCH", "INCMH", "INCML", "INCL",
        "ORH", "ORMH", "ORML", "ORL", "ANDNH", "ANDNMH", "ANDNML", "ANDNL"]
LOADS = ["LDB", "LDBU", "LDW", "LDWU", "LDT", "LDTU", "LDO", "LDOU", "LDHT",
         "LDSF", "LDUNC"]
STORES = ["STB", "STBU", "STW", "STWU", "STT", "STTU", "STO", "STOU", "STHT",
          "STSF", "STUNC"]

out = []
emit = out.append
emit("%% random differential test, seed %d" % seed)
emit("res      GREG")
emit("scr      GREG")
emit("         LOC   Data_Segment")
emit("         GREG  @")
emit("Digits   BYTE  \"0123456789abcdef\"")
emit("Hex      BYTE  \"................\",#a,0")
emit("         LOC   (@+7)&-8")
emit("Init     OCTA  #%x" % rval())
for _ in range(15):
    emit("         OCTA  #%x" % rval())
emit("Scratch  OCTA  #%x" % rnd.getrandbits(64))
for _ in range(7):
    emit("         OCTA  #%x" % rnd.getrandbits(64))
emit("Res      OCTA  0")
emit("         LOC   #100")
emit("Main     LDA   res,Res")
emit("         LDA   scr,Scratch")
emit("         LDA   $20,Init")
for k in range(16):
    emit("         LDO   $%d,$20,%d" % (k, 8 * k))


def record(r):
    emit("         STO   %s,res,0" % r)
    emit("         ADDU  res,res,8")


for i in range(count):
    k = rnd.randrange(100)
    x, y, z = reg(), reg(), reg()
    zop = "$%d" % z if rnd.randrange(3) else str(rnd.randrange(256))
    if k < 3:                                   # rounding mode
        emit("         SETML $16,%d" % rnd.randrange(4))
        emit("         PUT   rA,$16")
        continue
    if k < 40:
        op = rnd.choice(INT3)
        if op in ("SL", "SLU", "SR", "SRU") and not zop.startswith("$"):
            zop = str(rnd.choice([0, 1, 3, 31, 32, 63, 64, 65, 200]))
        if op == "DIVU":
            emit("         PUT   rD,$%d" % reg())
        if op == "MUX":
            emit("         PUT   rM,$%d" % reg())
        emit("         %-5s $%d,$%d,%s" % (op, x, y, zop))
        record("$%d" % x)
        if op in ("MULU",):
            emit("         GET   $16,rH")
            record("$16")
        if op in ("DIV", "DIVU"):
            emit("         GET   $16,rR")
            record("$16")
    elif k < 62:
        op = rnd.choice(FLT3)
        if op.endswith("E"):
            emit("         PUT   rE,$%d" % reg())
        emit("         %-5s $%d,$%d,$%d" % (op, x, y, z))
        record("$%d" % x)
    elif k < 70:
        op = rnd.choice(FLT2)
        emit("         %-5s $%d,%d,$%d" % (op, x, rnd.randrange(5), z))
        record("$%d" % x)
    elif k < 76:
        op = rnd.choice(CONV)
        emit("         %-5s $%d,%d,%s" % (op, x, rnd.randrange(5), zop))
        record("$%d" % x)
    elif k < 82:
        op = rnd.choice(WYDE)
        emit("         %-5s $%d,#%x" % (op, x, rnd.getrandbits(16)))
        record("$%d" % x)
    elif k < 88:
        op = rnd.choice(["NEG", "NEGU"])
        emit("         %-5s $%d,%d,%s" % (op, x, rnd.randrange(3), zop))
        record("$%d" % x)
    elif k < 94:
        op = rnd.choice(STORES)
        emit("         %-5s $%d,scr,%d" % (op, x, rnd.randrange(60)))
        emit("         LDO   $16,scr,%d" % (8 * rnd.randrange(8)))
        record("$16")
    else:
        op = rnd.choice(LOADS)
        emit("         %-5s $%d,scr,%d" % (op, x, rnd.randrange(64)))
        record("$%d" % x)
    emit("         GET   $16,rA")
    record("$16")

# print the result area
emit("         LDA   $30,Res")
emit("         LDA   $37,Digits")
emit("2H       CMPU  $32,$30,res")
emit("         BNN   $32,9F")
emit("         LDOU  $33,$30,0")
emit("         SET   $34,16")
emit("         LDA   $35,Hex+16")
emit("3H       SUB   $35,$35,1")
emit("         AND   $36,$33,#f")
emit("         LDBU  $36,$37,$36")
emit("         STBU  $36,$35,0")
emit("         SRU   $33,$33,4")
emit("         SUB   $34,$34,1")
emit("         PBP   $34,3B")
emit("         LDA   $255,Hex")
emit("         TRAP  0,Fputs,StdOut")
emit("         ADDU  $30,$30,8")
emit("         JMP   2B")
emit("9H       TRAP  0,Halt,0")
print("\n".join(out))
