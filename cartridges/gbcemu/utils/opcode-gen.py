#!/bin/python3

# op tables taken from https://gbdev.io/gb-opcodes/optables/

import json
import sys

def convertFlag(flag):
    if (flag == "-"):
        return "_"
    return flag

filename = sys.argv[1]

with open(filename, "r") as file:
    data = json.load(file)
    for (opcode, data) in data["unprefixed"].items():
        mnemonic = data["mnemonic"].lower()
        if (mnemonic.startswith("illegal_")):
            continue
        opcode = opcode.lower()
        size = data["bytes"]
        cycles = data["cycles"][0]
        if (len(data["cycles"]) != 1):
            cycles = f"{cycles}!!!"
        Z = convertFlag(data["flags"]["Z"])
        N = convertFlag(data["flags"]["N"])
        H = convertFlag(data["flags"]["H"])
        C = convertFlag(data["flags"]["C"])
        args = ", ".join(map(lambda x : x["name"] if x["immediate"] else "[" + x["name"] + "]", data["operands"])).lower()
        if args != "":
            mnemonic = mnemonic + " " + args

        print(f"INS({opcode}, {Z},{N},{H},{C}, {size}, {cycles:<2}, \"{mnemonic}\", {{\n}})")


