import json
import sys

def verifyApplicable(test):
    if (test["initial"]["pc"] > 0xf000):
        return False
    mem = {}
    for (addr, value) in test["initial"]["ram"]:
        mem[addr] = value
        # check if there are conflicting writes to shadow address
        if (addr >= 0xc00 and addr < 0xddff):
            shadowAddr = addr + 0x2000
            for (a2, v2) in test["initial"]["ram"]:
                if (a2 == shadowAddr and v2 != value):
                    return False
    for (addr, value) in test["final"]["ram"]:
        if (addr < 0x8000 and mem[addr] != value):
            return False
    return True

def processTest(test, f):
    if (not verifyApplicable(test)):
        return
    af = test["initial"]["a"] << 8 | test["initial"]["f"]
    bc = test["initial"]["b"] << 8 | test["initial"]["c"]
    de = test["initial"]["d"] << 8 | test["initial"]["e"]
    hl = test["initial"]["h"] << 8 | test["initial"]["l"]
    sp = test["initial"]["sp"]
    pc = test["initial"]["pc"]
    ime = test["initial"]["ime"]
    ie = test["initial"]["ie"]
    testName = test["name"]
    print(f"        // {testName}", file = f)
    print(f"        gbc.setState(0x{pc:x}, 0x{sp:x}, 0x{af:x}, 0x{bc:x}, 0x{de:x}, 0x{hl:x}, 0x{ime:x}, 0x{ie:x});", file = f)
    # analyze the initial ram settings
    mem = {}
    # check the addresses and make them into consecutive arrays in the mem dict
    for [addr, value] in test["initial"]["ram"]:
        if addr not in mem:
            mem[addr + 1] = [value]
        else:
            mem[addr + 1] = mem[addr] + [value]
            del mem[addr]
    for (addr, values) in mem.items():
        addr = addr - len(values)
        values = ", ".join(map(lambda x : f"0x{x:x}", values))
        print(f"        gbc.writeMem(0x{addr:x}, {{ {values} }});", file = f)
    # step the instruction
    print( "        gbc.step();", file = f)
    # and now check the final state, this will be a bunch of expects
    x = test["final"]["a"]
    print(f"        EXPECT(gbc.a(),  0x{x:x});", file = f)
    x = test["final"]["b"]
    print(f"        EXPECT(gbc.b(),  0x{x:x});", file = f)
    x = test["final"]["c"]
    print(f"        EXPECT(gbc.c(),  0x{x:x});", file = f)
    x = test["final"]["d"]
    print(f"        EXPECT(gbc.d(),  0x{x:x});", file = f)
    x = test["final"]["e"]
    print(f"        EXPECT(gbc.e(),  0x{x:x});", file = f)
    x = test["final"]["h"]
    print(f"        EXPECT(gbc.h(),  0x{x:x});", file = f)
    x = test["final"]["l"]
    print(f"        EXPECT(gbc.l(),  0x{x:x});", file = f)
    x = test["final"]["sp"]
    print(f"        EXPECT(gbc.sp(), 0x{x:x});", file = f)
    x = test["final"]["pc"]
    print(f"        EXPECT(gbc.pc(), 0x{x:x});", file = f)
    x = test["final"]["ime"]
    print(f"        EXPECT(gbc.ime(), 0x{x:x});", file = f)
    try:
        x = test["final"]["ie"]
        print(f"        EXPECT(gbc.ie(), 0x{x:x});", file = f)
    except:
        pass
    # check flags
    x = test["final"]["f"]
    print(f"        EXPECT(gbc.flagZ(),  {'true' if (x & 0x80) else 'false'});", file = f)
    print(f"        EXPECT(gbc.flagN(),  {'true' if (x & 0x40) else 'false'});", file = f)
    print(f"        EXPECT(gbc.flagH(),  {'true' if (x & 0x20) else 'false'});", file = f)
    print(f"        EXPECT(gbc.flagC(),  {'true' if (x & 0x10) else 'false'});", file = f)
    # check ram
    for (addr, value) in test["final"]["ram"]:
        if (addr >= 0x8000):
            print(f"        EXPECT(gbc.readMem(0x{addr:x}), 0x{value:x});", file = f)

def processTests(tests, f, opcode):
    print(f"    TEST(gbcemu, opcode_{opcode}) {{", file = f)
    print( "        GBCEmu gbc{};", file = f)
    print( "        uint8_t cartridge[0x8000];", file = f)
    print( "        cartridge[0x149] = 0x02; // 8kb external RAM", file = f)
    print( "        gbc.loadCartridge(new FlashGamePak(cartridge));", file = f)
    for test in tests:
        processTest(test, f)
    print( "    }", file = f)

# Access the data

inputDir = sys.argv[1]
outputDir = sys.argv[2]

def convertTests(opcode):
    print(f"Checking opcode {opcode}")
    try:
        with open(f"{inputDir}/{opcode}.json", "r") as fIn:
            data = json.load(fIn)
            opcode = opcode.replace(" ", "_")
            with open(f"{outputDir}/{opcode}.inc.h", "w") as fOut:
                processTests(data, fOut, opcode)
    except:
        print(f"No input tests for opcode {opcode} found")

"""

for opcodeId in range(256):
    opcode = f"{opcodeId:02x}"
    convertTests(opcode)

for opcodeId in range(256):
    opcode = f"cb {opcodeId:02x}"
    convertTests(opcode)
"""

convertTests("cb 66")