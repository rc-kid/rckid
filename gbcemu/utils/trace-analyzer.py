import sys

def main():
    if len(sys.argv) != 3:
        print("Usage: python trace-analyzer.py <reference-file> <trace-file>")
        sys.exit(1)

    filename = sys.argv[1]
    filename2 = sys.argv[2]
    pcs = set()
    opcodes = set()
    print("Reading reference file...")
    with open(filename, 'r') as file:
        for line in file:
            line = line.strip()
            if not line:
                continue
            pc, opcode = line.split(':')
            pc = pc.strip()
            opcode = opcode.strip()
            pc = int(pc, 16)
            opcode = int(opcode, 16)
            pcs.add(pc)
            opcodes.add(opcode)
    print(f"Read {len(pcs)} pcs and {len(opcodes)} opcodes")

    print("Reading trace file...")
    invalidPcs = set()
    invalidOpcodes = set()
    with open(filename2, 'r') as file:
        for line in file:
            line = line.strip()
            if not line:
                continue
            try: 
                pc, opcode = line.split(':')
                pc = pc.strip()
                opcode = opcode.strip()
                pc = int(pc, 16)
                opcode = int(opcode, 16)
                if (pc not in pcs):
                    if (pc not in invalidPcs):
                        invalidPcs.add(pc)
                        print(f"Error: pc {pc:x} not found in reference file")
                    #sys.exit(1);
                if (opcode not in opcodes):
                    if (opcode not in invalidOpcodes):
                        invalidOpcodes.add(opcode)
                        print(f"Error: opcode {opcode:x} not found in reference file")
                    #sys.exit(1)
            except:
                continue
    print("INVALID OPCODES:")
    for opcode in invalidOpcodes:
        print(f"{opcode:x}")
    print("INVALID PCS:")
    for pc in invalidPcs:
        print(f"{pc:x}")
    print(f"Found {len(invalidPcs)} invalid pcs and {len(invalidOpcodes)} invalid opcodes")

if __name__ == "__main__":
    main()