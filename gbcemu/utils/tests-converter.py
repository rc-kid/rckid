# Takes JSON tests from https://github.com/raddad772/jsmoo/tree/gbc/misc/tests/GeneratedTests/sm83 (which are also listed in this repo as a zip file) and converts them to a format that can be used by the test suite directly.
#
# Generates one test in dedicated inc.h file for each instruction. Those files can then be included into a CPP file to actually run the tests. This is because keeping the CPP files around confuses the tooling and it takes ages to analyze the project with that many large cpp files, even if they are disabled. 

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
    print(f"        EXPECT(0x{x:x}, gbc.a());", file = f)
    x = test["final"]["b"]
    print(f"        EXPECT(0x{x:x}, gbc.b());", file = f)
    x = test["final"]["c"]
    print(f"        EXPECT(0x{x:x}, gbc.c());", file = f)
    x = test["final"]["d"]
    print(f"        EXPECT(0x{x:x}, gbc.d());", file = f)
    x = test["final"]["e"]
    print(f"        EXPECT(0x{x:x}, gbc.e());", file = f)
    x = test["final"]["h"]
    print(f"        EXPECT(0x{x:x}, gbc.h());", file = f)
    x = test["final"]["l"]
    print(f"        EXPECT(0x{x:x}, gbc.l());", file = f)
    x = test["final"]["sp"]
    print(f"        EXPECT(0x{x:x}, gbc.sp());", file = f)
    x = test["final"]["pc"]
    print(f"        EXPECT(0x{x:x}, gbc.pc());", file = f)
    x = test["final"]["ime"]
    print(f"        EXPECT(0x{x:x}, gbc.ime());", file = f)
    try:
        x = test["final"]["ie"]
        print(f"        EXPECT(0x{x:x}, gbc.ie());", file = f)
    except:
        pass
    # check flags
    x = test["final"]["f"]
    print(f"        EXPECT({'true' if (x & 0x80) else 'false'}, gbc.flagZ());", file = f)
    print(f"        EXPECT({'true' if (x & 0x40) else 'false'}, gbc.flagN());", file = f)
    print(f"        EXPECT({'true' if (x & 0x20) else 'false'}, gbc.flagH());", file = f)
    print(f"        EXPECT({'true' if (x & 0x10) else 'false'}, gbc.flagC());", file = f)
    # check ram
    for (addr, value) in test["final"]["ram"]:
        if (addr >= 0x8000):
            print(f"        EXPECT(0x{value:x}, gbc.readMem(0x{addr:x}));", file = f)

def processTests(tests, f, opcode):
    print(f"    TEST(gbcemu, opcode_{opcode}) {{", file = f)
    print( "        GBCEmu gbc{\"\", nullptr};", file = f)
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

for opcodeId in range(256):
    opcode = f"{opcodeId:02x}"
    convertTests(opcode)

for opcodeId in range(256):
    opcode = f"cb {opcodeId:02x}"
    convertTests(opcode)
