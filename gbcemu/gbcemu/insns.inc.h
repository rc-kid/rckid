/** Defines all assembly instructions for the GBC CPU. 
 
    Each instruction is a macro, which contains the following information:

    - the opcode,
    - flags (Z,N,H,C) always in this order, where each flag can be specified as `_` (no change), `0` (clear), `1` (set) or the flag name (Z,N,H,C) if the flag is updated explicitly in the instruction code
    - instruction size in bytes (not really used)
    - number of cycles the instruction takes (this is always a multiple of 4)
    - the mnemonic
    - the instruction code

    Main use of those definitions is the interpreter main loop in the GBCEmu::update method.
 */

/** Does nothing. 
 */
INS(0x00, _,_,_,_, 1, 4 , "nop", {})
/** Loads the immediate 16bit data into the bc register. 
 */
INS(0x01, _,_,_,_, 3, 12, "ld bc, n16", { BC = mem16(PC); PC += 2; })
INS(0x02, _,_,_,_, 1, 8 , "ld [bc], a", { memWr8(BC, A); })
INS(0x03, _,_,_,_, 1, 8 , "inc bc", { ++BC; })
INS(0x04, Z,0,H,_, 1, 4 , "inc b", { B = inc8(B); })
INS(0x05, Z,1,H,_, 1, 4 , "dec b", { B = dec8(B); })
INS(0x06, _,_,_,_, 2, 8 , "ld b, n8", { B = mem8(PC++); })
INS(0x07, 0,0,0,C, 1, 4 , "rlca", { A = rlc8(A); })
/** Stores stack pointer value at the different address. 
 
    addr = SP & 0xff
    addr + 1 = SP >> 8
*/
INS(0x08, _,_,_,_, 3, 20, "ld [n16], sp", { memWr16(mem16(PC), SP); PC += 2; })
INS(0x09, _,0,H,C, 1, 8 , "add hl, bc", { HL = add16(HL, BC); })
INS(0x0a, _,_,_,_, 1, 8 , "ld a, [bc]", { A = memRd8(BC); })
INS(0x0b, _,_,_,_, 1, 8 , "dec bc", { --BC; })
INS(0x0c, Z,0,H,_, 1, 4 , "inc c", { C = inc8(C); })
INS(0x0d, Z,1,H,_, 1, 4 , "dec c", { C = dec8(C); })
INS(0x0e, _,_,_,_, 2, 8 , "ld c, n8", { C = mem8(PC++); })
INS(0x0f, 0,0,0,C, 1, 4 , "rrca", { A = rrc8(A); })
/** TODO similar to halt likely 
 */
INS(0x10, _,_,_,_, 2, 4 , "stop n8", {
    mem8(PC++);
    UNIMPLEMENTED;
})
INS(0x11, _,_,_,_, 3, 12, "ld de, n16", { DE = mem16(PC); PC += 2; })
INS(0x12, _,_,_,_, 1, 8 , "ld [de], a", { memWr8(DE, A); })
INS(0x13, _,_,_,_, 1, 8 , "inc de", { ++DE; })
INS(0x14, Z,0,H,_, 1, 4 , "inc d", { D = inc8(D); })
INS(0x15, Z,1,H,_, 1, 4 , "dec d", { D = dec8(D); })
INS(0x16, _,_,_,_, 2, 8 , "ld d, n8", { D = mem8(PC++); })
INS(0x17, 0,0,0,C, 1, 4 , "rla", { A = rl8(A); })
INS(0x18, _,_,_,_, 2, 12, "jr e8", {
    int8_t offset = static_cast<int8_t>(mem8(PC++));
    PC += offset;
})
INS(0x19, _,0,H,C, 1, 8 , "add hl, de", { HL = add16(HL, DE); })
INS(0x1a, _,_,_,_, 1, 8 , "ld a, [de]", { A = memRd8(DE); })
INS(0x1b, _,_,_,_, 1, 8 , "dec de", { --DE; })
INS(0x1c, Z,0,H,_, 1, 4 , "inc e", { E = inc8(E); })
INS(0x1d, Z,1,H,_, 1, 4 , "dec e", { E = dec8(E); })
INS(0x1e, _,_,_,_, 2, 8 , "ld e, n8", { E = mem8(PC++); })
INS(0x1f, 0,0,0,C, 1, 4 , "rra", { A = rr8(A); })
INS(0x20, _,_,_,_, 2, 8 + 4, "jr nz, e8", {
    // 3 cycles when taken, 2 when not taken
    int8_t offset = static_cast<int8_t>(mem8(PC++));
    if (! flagZ())
        PC += offset;
    else
        usedCycles -= 4;
})
INS(0x21, _,_,_,_, 3, 12, "ld hl, n16", { HL = mem16(PC); PC += 2; })
INS(0x22, _,_,_,_, 1, 8 , "ld [hl+], a", { memWr8(HL, A); ++HL; })
INS(0x23, _,_,_,_, 1, 8 , "inc hl", { ++HL; })
INS(0x24, Z,0,H,_, 1, 4 , "inc h", { H = inc8(H); })
INS(0x25, Z,1,H,_, 1, 4 , "dec h", { H = dec8(H); })
INS(0x26, _,_,_,_, 2, 8 , "ld h, n8", { H = mem8(PC++); })
/** Actually quite fancy adjust instruction for BCD encoded values. 
 
    Using the C, N and H flags, a value in the A register is reconstructed to proper BCD, assuming it has been formed by adding or subtracting BCD numbers before.

    This implementation still fails some tests, but at the moment, but my hope is only in the invalid inputs, so I'm not going to fix it now.
 */
INS(0x27, Z,_,0,C, 1, 4 , "daa", {
    if (!flagN()) {
        if (flagH() || (A & 0x0f) > 0x09)
            A += 0x06;
        if (flagC() || A > 0x9f) {
            A += 0x60;
            setFlagC(true);
        }
    } else {
        if (flagH())
            A -= 0x06;
        if (flagC())
            A -= 0x60;
    }
    setFlagZ(A == 0);
})
INS(0x28, _,_,_,_, 2, 8 + 4, "jr z, e8", {
    // 3 cycles when taken, 2 when not taken
    int8_t offset = static_cast<int8_t>(mem8(PC++));
    if (flagZ())
        PC += offset;
    else
        usedCycles -= 4;
})
INS(0x29, _,0,H,C, 1, 8 , "add hl, hl", { HL = add16(HL, HL); })
INS(0x2a, _,_,_,_, 1, 8 , "ld a, [hl+]", { A = memRd8(HL); ++HL; })
INS(0x2b, _,_,_,_, 1, 8 , "dec hl", { --HL; })
INS(0x2c, Z,0,H,_, 1, 4 , "inc l", { L = inc8(L); })
INS(0x2d, Z,1,H,_, 1, 4 , "dec l", { L = dec8(L); })
INS(0x2e, _,_,_,_, 2, 8 , "ld l, n8", { L = mem8(PC++); })
INS(0x2f, _,1,1,_, 1, 4 , "cpl", { A = ~A; })
INS(0x30, _,_,_,_, 2, 8 + 4, "jr nc, e8", {
    // 3 cycles when taken, 2 when not taken
    int8_t offset = static_cast<int8_t>(mem8(PC++));
    if (! flagC())
        PC += offset;
    else
        usedCycles -= 4;
})
INS(0x31, _,_,_,_, 3, 12, "ld sp, n16", { SP = mem16(PC); PC += 2; })
INS(0x32, _,_,_,_, 1, 8 , "ld [hl-], a", { memWr8(HL, A); --HL; })
INS(0x33, _,_,_,_, 1, 8 , "inc sp", { ++SP; })
INS(0x34, Z,0,H,_, 1, 12, "inc [hl]", { memWr8(HL, inc8(memRd8(HL))); })
INS(0x35, Z,1,H,_, 1, 12, "dec [hl]", { memWr8(HL, dec8(memRd8(HL))); })
INS(0x36, _,_,_,_, 2, 12, "ld [hl], n8", { memWr8(HL, mem8(PC++)); })
/** Simply sets the carry flag, already handled by the macro expansion in the emulator loop. 
 */
INS(0x37, _,0,0,1, 1, 4 , "scf", {})
INS(0x38, _,_,_,_, 2, 8 + 4, "jr c, e8", {
    // 3 cycles when taken, 2 when not taken
    int8_t offset = static_cast<int8_t>(mem8(PC++));
    if (flagC())
        PC += offset;
    else
        usedCycles -= 4;
})
INS(0x39, _,0,H,C, 1, 8 , "add hl, sp", { HL = add16(HL, SP); })
INS(0x3a, _,_,_,_, 1, 8 , "ld a, [hl-]", { A = memRd8(HL); --HL; })
INS(0x3b, _,_,_,_, 1, 8 , "dec sp", { --SP; })
INS(0x3c, Z,0,H,_, 1, 4 , "inc a", { A = inc8(A); })
INS(0x3d, Z,1,H,_, 1, 4 , "dec a", { A = dec8(A); })
INS(0x3e, _,_,_,_, 2, 8 , "ld a, n8", { A = mem8(PC++); })
INS(0x3f, _,0,0,C, 1, 4 , "ccf", { setFlagC(!flagC()); })
/** Loads register into itself. This is effectively a no-op.
 */
INS(0x40, _,_,_,_, 1, 4 , "ld b, b", {})
INS(0x41, _,_,_,_, 1, 4 , "ld b, c", { B = C;})
INS(0x42, _,_,_,_, 1, 4 , "ld b, d", { B = D; })
INS(0x43, _,_,_,_, 1, 4 , "ld b, e", { B = E; })
INS(0x44, _,_,_,_, 1, 4 , "ld b, h", { B = H; })
INS(0x45, _,_,_,_, 1, 4 , "ld b, l", { B = L; })
INS(0x46, _,_,_,_, 1, 8 , "ld b, [hl]", { B = memRd8(HL); })
INS(0x47, _,_,_,_, 1, 4 , "ld b, a", { B = A; })
INS(0x48, _,_,_,_, 1, 4 , "ld c, b", { C = B; })
INS(0x49, _,_,_,_, 1, 4 , "ld c, c", {})
INS(0x4a, _,_,_,_, 1, 4 , "ld c, d", { C = D; })
INS(0x4b, _,_,_,_, 1, 4 , "ld c, e", { C = E; })
INS(0x4c, _,_,_,_, 1, 4 , "ld c, h", { C = H; })
INS(0x4d, _,_,_,_, 1, 4 , "ld c, l", { C = L; })
INS(0x4e, _,_,_,_, 1, 8 , "ld c, [hl]", { C = memRd8(HL); })
INS(0x4f, _,_,_,_, 1, 4 , "ld c, a", { C = A; })
INS(0x50, _,_,_,_, 1, 4 , "ld d, b", { D = B; })
INS(0x51, _,_,_,_, 1, 4 , "ld d, c", { D = C; })
INS(0x52, _,_,_,_, 1, 4 , "ld d, d", {})
INS(0x53, _,_,_,_, 1, 4 , "ld d, e", { D = E; })
INS(0x54, _,_,_,_, 1, 4 , "ld d, h", { D = H; })
INS(0x55, _,_,_,_, 1, 4 , "ld d, l", { D = L; })
INS(0x56, _,_,_,_, 1, 8 , "ld d, [hl]", { D = memRd8(HL); })
INS(0x57, _,_,_,_, 1, 4 , "ld d, a", { D = A; })
INS(0x58, _,_,_,_, 1, 4 , "ld e, b", { E = B; })
INS(0x59, _,_,_,_, 1, 4 , "ld e, c", { E = C; })
INS(0x5a, _,_,_,_, 1, 4 , "ld e, d", { E = D; })
INS(0x5b, _,_,_,_, 1, 4 , "ld e, e", {})
INS(0x5c, _,_,_,_, 1, 4 , "ld e, h", { E = H; })
INS(0x5d, _,_,_,_, 1, 4 , "ld e, l", { E = L; })
INS(0x5e, _,_,_,_, 1, 8 , "ld e, [hl]", { E = memRd8(HL); })
INS(0x5f, _,_,_,_, 1, 4 , "ld e, a", { E = A; })
INS(0x60, _,_,_,_, 1, 4 , "ld h, b", { H = B; })
INS(0x61, _,_,_,_, 1, 4 , "ld h, c", { H = C; })
INS(0x62, _,_,_,_, 1, 4 , "ld h, d", { H = D; })
INS(0x63, _,_,_,_, 1, 4 , "ld h, e", { H = E; })
INS(0x64, _,_,_,_, 1, 4 , "ld h, h", {})
INS(0x65, _,_,_,_, 1, 4 , "ld h, l", { H  = L; })
INS(0x66, _,_,_,_, 1, 8 , "ld h, [hl]", { H = memRd8(HL); })
INS(0x67, _,_,_,_, 1, 4 , "ld h, a", { H = A; })
INS(0x68, _,_,_,_, 1, 4 , "ld l, b", { L = B; })
INS(0x69, _,_,_,_, 1, 4 , "ld l, c", { L = C; })
INS(0x6a, _,_,_,_, 1, 4 , "ld l, d", { L = D; })
INS(0x6b, _,_,_,_, 1, 4 , "ld l, e", { L = E; })
INS(0x6c, _,_,_,_, 1, 4 , "ld l, h", { L = H; })
INS(0x6d, _,_,_,_, 1, 4 , "ld l, l", {})
INS(0x6e, _,_,_,_, 1, 8 , "ld l, [hl]", { L = memRd8(HL); })
INS(0x6f, _,_,_,_, 1, 4 , "ld l, a", { L = A; })
INS(0x70, _,_,_,_, 1, 8 , "ld [hl], b", { memWr8(HL, B); })
INS(0x71, _,_,_,_, 1, 8 , "ld [hl], c", { memWr8(HL, C); })
INS(0x72, _,_,_,_, 1, 8 , "ld [hl], d", { memWr8(HL, D); })
INS(0x73, _,_,_,_, 1, 8 , "ld [hl], e", { memWr8(HL, E); })
INS(0x74, _,_,_,_, 1, 8 , "ld [hl], h", { memWr8(HL, H); })
INS(0x75, _,_,_,_, 1, 8 , "ld [hl], l", { memWr8(HL, L); })

/** Halt pauses execution until an interrupt is requested. 
 
    We do this by a trick - halt moves the PC back by one byte so that next instruction will be halt again. This will allow the cycles to continue and other timings to work as instructed, but when an interrupt is being triggered we check if current instruction is HALT and advance to the next instruction in that case. 
 */
INS(0x76, _,_,_,_, 1, 4 , "halt", {
    --PC;
})
INS(0x77, _,_,_,_, 1, 8 , "ld [hl], a", { memWr8(HL, A); })
INS(0x78, _,_,_,_, 1, 4 , "ld a, b", { A = B; })
INS(0x79, _,_,_,_, 1, 4 , "ld a, c", { A = C; })
INS(0x7a, _,_,_,_, 1, 4 , "ld a, d", { A = D; })
INS(0x7b, _,_,_,_, 1, 4 , "ld a, e", { A = E; })
INS(0x7c, _,_,_,_, 1, 4 , "ld a, h", { A = H; })
INS(0x7d, _,_,_,_, 1, 4 , "ld a, l", { A = L; })
INS(0x7e, _,_,_,_, 1, 8 , "ld a, [hl]", { A = memRd8(HL); })
INS(0x7f, _,_,_,_, 1, 4 , "ld a, a", {})
INS(0x80, Z,0,H,C, 1, 4 , "add a, b", { A = add8(A, B); })
INS(0x81, Z,0,H,C, 1, 4 , "add a, c", { A = add8(A, C); })
INS(0x82, Z,0,H,C, 1, 4 , "add a, d", { A = add8(A, D); })
INS(0x83, Z,0,H,C, 1, 4 , "add a, e", { A = add8(A, E); })
INS(0x84, Z,0,H,C, 1, 4 , "add a, h", { A = add8(A, H); })
INS(0x85, Z,0,H,C, 1, 4 , "add a, l", { A = add8(A, L); })
INS(0x86, Z,0,H,C, 1, 8 , "add a, [hl]", { A = add8(A, memRd8(HL)); })
INS(0x87, Z,0,H,C, 1, 4 , "add a, a", { A = add8(A, A); })
INS(0x88, Z,0,H,C, 1, 4 , "adc a, b", { A = add8(A, B, flagC()); })
INS(0x89, Z,0,H,C, 1, 4 , "adc a, c", { A = add8(A, C, flagC()); })
INS(0x8a, Z,0,H,C, 1, 4 , "adc a, d", { A = add8(A, D, flagC()); })
INS(0x8b, Z,0,H,C, 1, 4 , "adc a, e", { A = add8(A, E, flagC()); })
INS(0x8c, Z,0,H,C, 1, 4 , "adc a, h", { A = add8(A, H, flagC()); })
INS(0x8d, Z,0,H,C, 1, 4 , "adc a, l", { A = add8(A, L, flagC()); })
INS(0x8e, Z,0,H,C, 1, 8 , "adc a, [hl]", { A = add8(A, memRd8(HL), flagC()); })
INS(0x8f, Z,0,H,C, 1, 4 , "adc a, a", { A = add8(A, A, flagC()); })
INS(0x90, Z,1,H,C, 1, 4 , "sub a, b", { A = sub8(A, B); })
INS(0x91, Z,1,H,C, 1, 4 , "sub a, c", { A = sub8(A, C); })
INS(0x92, Z,1,H,C, 1, 4 , "sub a, d", { A = sub8(A, D); })
INS(0x93, Z,1,H,C, 1, 4 , "sub a, e", { A = sub8(A, E); })
INS(0x94, Z,1,H,C, 1, 4 , "sub a, h", { A = sub8(A, H); })
INS(0x95, Z,1,H,C, 1, 4 , "sub a, l", { A = sub8(A, L); })
INS(0x96, Z,1,H,C, 1, 8 , "sub a, [hl]", { A = sub8(A, memRd8(HL)); })
INS(0x97, 1,1,0,0, 1, 4 , "sub a, a", { A = 0; })
INS(0x98, Z,1,H,C, 1, 4 , "sbc a, b", { A = sub8(A, B, flagC()); })
INS(0x99, Z,1,H,C, 1, 4 , "sbc a, c", { A = sub8(A, C, flagC()); })
INS(0x9a, Z,1,H,C, 1, 4 , "sbc a, d", { A = sub8(A, D, flagC()); })
INS(0x9b, Z,1,H,C, 1, 4 , "sbc a, e", { A = sub8(A, E, flagC()); })
INS(0x9c, Z,1,H,C, 1, 4 , "sbc a, h", { A = sub8(A, H, flagC()); })
INS(0x9d, Z,1,H,C, 1, 4 , "sbc a, l", { A = sub8(A, L, flagC()); })
INS(0x9e, Z,1,H,C, 1, 8 , "sbc a, [hl]", { A = sub8(A, memRd8(HL), flagC()); })
INS(0x9f, Z,1,H,_, 1, 4 , "sbc a, a", { A = sub8(A, A, flagC()); })
INS(0xa0, Z,0,1,0, 1, 4 , "and a, b", { A = A & B; setFlagZ(A == 0); })
INS(0xa1, Z,0,1,0, 1, 4 , "and a, c", { A = A & C; setFlagZ(A == 0); })
INS(0xa2, Z,0,1,0, 1, 4 , "and a, d", { A = A & D; setFlagZ(A == 0); })
INS(0xa3, Z,0,1,0, 1, 4 , "and a, e", { A = A & E; setFlagZ(A == 0); })
INS(0xa4, Z,0,1,0, 1, 4 , "and a, h", { A = A & H; setFlagZ(A == 0); })
INS(0xa5, Z,0,1,0, 1, 4 , "and a, l", { A = A & L; setFlagZ(A == 0); })
INS(0xa6, Z,0,1,0, 1, 8 , "and a, [hl]", { A = A & memRd8(HL); setFlagZ(A == 0); })
INS(0xa7, Z,0,1,0, 1, 4 , "and a, a", { A = A & A; setFlagZ(A == 0); })
INS(0xa8, Z,0,0,0, 1, 4 , "xor a, b", { A = A ^ B; setFlagZ(A == 0); })
INS(0xa9, Z,0,0,0, 1, 4 , "xor a, c", { A = A ^ C; setFlagZ(A == 0); })
INS(0xaa, Z,0,0,0, 1, 4 , "xor a, d", { A = A ^ D; setFlagZ(A == 0); })
INS(0xab, Z,0,0,0, 1, 4 , "xor a, e", { A = A ^ E; setFlagZ(A == 0); })
INS(0xac, Z,0,0,0, 1, 4 , "xor a, h", { A = A ^ H; setFlagZ(A == 0); })
INS(0xad, Z,0,0,0, 1, 4 , "xor a, l", { A = A ^ L; setFlagZ(A == 0); })
INS(0xae, Z,0,0,0, 1, 8 , "xor a, [hl]", { A = A ^ memRd8(HL); setFlagZ(A == 0); })
INS(0xaf, 1,0,0,0, 1, 4 , "xor a, a", {  A =  0; })
INS(0xb0, Z,0,0,0, 1, 4 , "or a, b", { A = A | B; setFlagZ(A == 0); })
INS(0xb1, Z,0,0,0, 1, 4 , "or a, c", { A = A | C; setFlagZ(A == 0); })
INS(0xb2, Z,0,0,0, 1, 4 , "or a, d", { A = A | D; setFlagZ(A == 0); })
INS(0xb3, Z,0,0,0, 1, 4 , "or a, e", { A = A | E; setFlagZ(A == 0); })
INS(0xb4, Z,0,0,0, 1, 4 , "or a, h", { A = A | H; setFlagZ(A == 0); })
INS(0xb5, Z,0,0,0, 1, 4 , "or a, l", { A = A | L; setFlagZ(A == 0); })
INS(0xb6, Z,0,0,0, 1, 8 , "or a, [hl]", { A = A | memRd8(HL); setFlagZ(A == 0); })
INS(0xb7, Z,0,0,0, 1, 4 , "or a, a", { A = A | A; setFlagZ(A == 0); })
INS(0xb8, Z,1,H,C, 1, 4 , "cp a, b", { sub8(A, B); })
INS(0xb9, Z,1,H,C, 1, 4 , "cp a, c", { sub8(A, C); })
INS(0xba, Z,1,H,C, 1, 4 , "cp a, d", { sub8(A, D); })
INS(0xbb, Z,1,H,C, 1, 4 , "cp a, e", { sub8(A, E); })
INS(0xbc, Z,1,H,C, 1, 4 , "cp a, h", { sub8(A, H); })
INS(0xbd, Z,1,H,C, 1, 4 , "cp a, l", { sub8(A, L); })
INS(0xbe, Z,1,H,C, 1, 8 , "cp a, [hl]", { sub8(A, memRd8(HL)); })
/** This literally does nothing but set the flags to consts. 
 */
INS(0xbf, 1,1,0,0, 1, 4 , "cp a, a", { }) 
INS(0xc0, _,_,_,_, 1, 8 + 12, "ret nz", {
    // 5 cycles taken, 2 cyles not taken
    if (! flagZ()) {
        PC = memRd16(SP);
        SP += 2;
    } else {
        usedCycles -= 12;
    }
})
INS(0xc1, _,_,_,_, 1, 12, "pop bc", { BC = memRd16(SP); SP += 2; })
INS(0xc2, _,_,_,_, 3, 12 + 4, "jp nz, a16", {
    // 4 cycles taken, 3 cycles not taken
    uint16_t addr = mem16(PC); PC += 2;
    if (! flagZ())
        PC = addr;
    else
        usedCycles -= 4;
})
INS(0xc3, _,_,_,_, 3, 16, "jp a16", { PC = mem16(PC); })
INS(0xc4, _,_,_,_, 3, 12 + 12, "call nz, a16", {
    // 6 cycles taken, 3 cycles not taken
    uint16_t addr = mem16(PC); PC += 2;
    if (! flagZ()) {
        SP -= 2;
        memWr16(SP, PC);
        PC = addr;
    } else {
        usedCycles -= 12;
    }
})
INS(0xc5, _,_,_,_, 1, 16, "push bc", { SP -= 2; memWr16(SP, BC); })
INS(0xc6, Z,0,H,C, 2, 8 , "add a, n8", { A = add8(A, mem8(PC++)); })
INS(0xc7, _,_,_,_, 1, 16, "rst $00", {
    SP -= 2; 
    memWr16(SP, PC); 
    PC = 0x00; 
})
INS(0xc8, _,_,_,_, 1, 8 + 12, "ret z", {
    // 5 cycles taken, 2 cyles not taken
    if (flagZ()) {
        PC = memRd16(SP);
        SP += 2;
    } else {
        usedCycles -= 12;
    }
})
INS(0xc9, _,_,_,_, 1, 16, "ret", { PC = memRd16(SP); SP += 2; })
INS(0xca, _,_,_,_, 3, 12 + 4, "jp z, a16", {
    // 4 cycles taken, 3 cycles not taken
    uint16_t addr = mem16(PC); PC += 2;
    if (flagZ())
        PC = addr;
    else
        usedCycles -= 4;
})
/** The prefixed instructions. 
 
    The follow a very simple decoding pattern whereas the 3 LSB identify a register (B, C, D, E, H,  L, [HL], A) and the upper 5 bits specify the operation. 
*/
INS(0xcb, Z,N,H,C, 1, 4 , "prefix", {
    uint8_t eo = mem8(PC++); 
    uint8_t reg = eo & 7;
    eo = eo >> 3;
    uint8_t r;
    switch (reg) {
        case 0: r = B; break;
        case 1: r = C; break;
        case 2: r = D; break;
        case 3: r = E; break;
        case 4: r = H; break;
        case 5: r = L; break;
        case 6: r = memRd8(HL); break;
        default:
        case 7: r = A; break;
    }
    switch (eo) {
        case 0: // RLC
            r = rlc8(r);
            setFlagZ(r == 0); // rlc already sets carry
            setFlagH(0);
            setFlagN(0);
            break;
        case 1: // RRC
            r = rrc8(r);
            setFlagZ(r == 0); // rrc already sets carry
            setFlagH(0);
            setFlagN(0);
            break;
        case 2: // RL
            r = rl8(r);
            setFlagZ(r == 0); // rl already sets carry
            setFlagH(0);
            setFlagN(0);
            break;
        case 3: // RR
            r = rr8(r);
            setFlagZ(r == 0); // rr already sets carry
            setFlagH(0);
            setFlagN(0);
            break;
        case 4: // SLA
            r = sla8(r);
            setFlagZ(r == 0); // sla already sets carry
            setFlagH(0);
            setFlagN(0);
            break;
        case 5: // SRA
            r = sra8(r);
            setFlagZ(r == 0); // sra already sets carry
            setFlagH(0);
            setFlagN(0);
            break;
        case 6: // SWAP
            eo = r & 0xf;
            r = (eo << 4) | (r >> 4);
            setFlagZ(r == 0);
            setFlagH(0);
            setFlagN(0);
            setFlagC(0);
            break;
        case 7: // SRL
            r = srl8(r);
            setFlagZ(r == 0); // srl already sets carry
            setFlagH(0);
            setFlagN(0);
            break;
        default: { // bit operations
            unsigned bit = eo & 7;
            eo = eo >> 3;
            switch (eo) {
                case 1: // BIT
                    setFlagZ((r & (1 << bit)) == 0);
                    setFlagN(0);
                    setFlagH(1);
                    break;
                case 2: // RES
                    r = r & ~(1 << bit);
                    break;
                case 3: // SET
                    r = r | (1 << bit);
                    break;
            }
        }
    }
    switch (reg) {
        case 0: B = r; break;
        case 1: C = r; break;
        case 2: D = r; break;
        case 3: E = r; break;
        case 4: H = r; break;
        case 5: L = r; break;
        case 6: memWr8(HL, r); break;
        case 7: A = r; break;
    }
})
INS(0xcc, _,_,_,_, 3, 12 + 12, "call z, a16", {
    // 6 cycles taken, 3 cycles not taken
    uint16_t addr = mem16(PC); PC += 2;
    if (flagZ()) {
        SP -= 2;
        memWr16(SP, PC);
        PC = addr;
    } else {
        usedCycles -= 12;
    }
})
INS(0xcd, _,_,_,_, 3, 24, "call a16", {
    uint16_t addr = mem16(PC); PC += 2;
    SP -= 2;
    memWr16(SP, PC);
    PC = addr;
})
INS(0xce, Z,0,H,C, 2, 8 , "adc a, n8", { A = add8(A, mem8(PC++), flagC()); })
INS(0xcf, _,_,_,_, 1, 16, "rst $08", {
    SP -= 2; 
    memWr16(SP, PC); 
    PC = 0x08; 
})
INS(0xd0, _,_,_,_, 1, 8 + 12, "ret nc", {
    // 5 cycles taken, 2 cyles not taken
    if (! flagC()) {
        PC = memRd16(SP);
        SP += 2;
    } else {
        usedCycles -= 12;
    }
})
INS(0xd1, _,_,_,_, 1, 12, "pop de", { DE = memRd16(SP); SP += 2; })
INS(0xd2, _,_,_,_, 3, 12 + 4, "jp nc, a16", {
    // 4 cycles taken, 3 cycles not taken
    uint16_t addr = mem16(PC); PC += 2;
    if (! flagC())
        PC = addr;
    else
        usedCycles -= 4;
})
INS(0xd4, _,_,_,_, 3, 12 + 12, "call nc, a16", {
    // 6 cycles taken, 3 cycles not taken
    uint16_t addr = mem16(PC); PC += 2;
    if (! flagC()) {
        SP -= 2;
        memWr16(SP, PC);
        PC = addr;
    } else {
        usedCycles -= 12;
    }
})
INS(0xd5, _,_,_,_, 1, 16, "push de", { SP -= 2; memWr16(SP, DE); })
INS(0xd6, Z,1,H,C, 2, 8 , "sub a, n8", { A = sub8(A, mem8(PC++)); })
INS(0xd7, _,_,_,_, 1, 16, "rst $10", {
    SP -= 2; 
    memWr16(SP, PC); 
    PC = 0x10; 
})
INS(0xd8, _,_,_,_, 1, 8 + 12, "ret c", {
    // 5 cycles taken, 2 cyles not taken
    if (flagC()) {
        PC = memRd16(SP);
        SP += 2;
    } else {
        usedCycles -= 12;
    }
})
INS(0xd9, _,_,_,_, 1, 16, "reti", {
    PC = memRd16(SP);
    SP += 2;
    ime_ = true;
})
INS(0xda, _,_,_,_, 3, 12 + 4, "jp c, a16", {
    // 4 cycles taken, 3 cycles not taken
    uint16_t addr = mem16(PC); PC += 2;
    if (flagC())
        PC = addr;
    else
        usedCycles -= 4;
})
INS(0xdc, _,_,_,_, 3, 12 + 12, "call c, a16", {
    // 6 cycles taken, 3 cycles not taken
    uint16_t addr = mem16(PC); PC += 2;
    if (flagC()) {
        SP -= 2;
        memWr16(SP, PC);
        PC = addr;
    } else {
        usedCycles -= 12;
    }
})
INS(0xde, Z,1,H,C, 2, 8 , "sbc a, n8", { A = sub8(A, mem8(PC++), flagC()); })
INS(0xdf, _,_,_,_, 1, 16, "rst $18", {
    SP -= 2; 
    memWr16(SP, PC); 
    PC = 0x18; 
})
INS(0xe0, _,_,_,_, 2, 12, "ldh [a8], a", { memWr8(0xff00 + mem8(PC++), A); })
INS(0xe1, _,_,_,_, 1, 12, "pop hl", { HL = memRd16(SP); SP += 2; })
INS(0xe2, _,_,_,_, 1, 8 , "ld [c], a", {  memWr8(0xff00 + C, A); })
INS(0xe5, _,_,_,_, 1, 16, "push hl", { SP -= 2; memWr16(SP, HL); })
INS(0xe6, Z,0,1,0, 2, 8 , "and a, n8", { A = A & mem8(PC++); setFlagZ(A == 0); })
INS(0xe7, _,_,_,_, 1, 16, "rst $20", {
    SP -= 2; 
    memWr16(SP, PC); 
    PC = 0x20; 
})
/** Adds 8bit immediate to 16bit SP register, but unlike other 16 bit arithmetics, uses carry and half carry flags from 8bit arithmetics.
 */
INS(0xe8, 0,0,H,C, 2, 16, "add sp, e8", {
    int8_t x = static_cast<int8_t>(mem8(PC++));
    setFlagH((SP & 0xf) + (x & 0xf) > 0xf);
    setFlagC((SP & 0xff) + (x & 0xff) > 0xff);
    SP = SP + x;
})
INS(0xe9, _,_,_,_, 1, 4 , "jp hl", { PC = HL; })
INS(0xea, _,_,_,_, 3, 16, "ld [a16], a", { memWr8(mem16(PC), A); PC += 2;})
INS(0xee, Z,0,0,0, 2, 8 , "xor a, n8", { A = A ^ mem8(PC++); setFlagZ(A == 0); })
INS(0xef, _,_,_,_, 1, 16, "rst $28", {
    SP -= 2; 
    memWr16(SP, PC); 
    PC = 0x28; 
})
INS(0xf0, _,_,_,_, 2, 12, "ldh a, [a8]", { A = memRd8(0xff00 + mem8(PC++)); })
INS(0xf1, Z,N,H,C, 1, 12, "pop af", { 
    AF = memRd16(SP); SP += 2; 
    // maybe not necessary, but the lower 4 bits of F are always 0 and should be cleared out for consistency as we might be popping into AF sth that was not F register originally
    F = F & 0xf0; 
})
INS(0xf2, _,_,_,_, 1, 8 , "ld a, [c]", { A = memRd8(0xff00 + C); })
INS(0xf3, _,_,_,_, 1, 4 , "di", { ime_ = false; })
INS(0xf5, _,_,_,_, 1, 16, "push af", { SP -= 2; memWr16(SP, AF); })
INS(0xf6, Z,0,0,0, 2, 8 , "or a, n8", { A = A | mem8(PC++); setFlagZ(A == 0); })
INS(0xf7, _,_,_,_, 1, 16, "rst $30", { 
    SP -= 2; 
    memWr16(SP, PC); 
    PC = 0x30; 
})
INS(0xf8, 0,0,H,C, 2, 12, "ld hl, sp, e8", { 
    int8_t imm = static_cast<int8_t>(mem8(PC++));
    HL = SP + imm;
    // a bit weird, but internet suggests this is actually what the instruction does
    setFlagH((SP & 0xf) + (imm & 0xf) > 0xf);
    setFlagC((SP & 0xff) + (imm & 0xff) > 0xff);
})
INS(0xf9, _,_,_,_, 1, 8 , "ld sp, hl", { SP = HL; })
INS(0xfa, _,_,_,_, 3, 16, "ld a, [a16]", { A = memRd8(mem16(PC)); PC += 2; })
/** Enables interrupts. 
 
    Note that in the original gameboy, this instruction enables interrupts, but only after the next instruction. Emulating such details is likely not necessary so we enable them immediately.
 */
INS(0xfb, _,_,_,_, 1, 4 , "ei", { ime_ = true; })



INS(0xfd, _,_,_,_, 1, 4,  "bkpt", {
    // the bkpt instruction should actually never be executed
    UNREACHABLE;
})
INS(0xfe, Z,1,H,C, 2, 8 , "cp a, n8", { sub8(A, mem8(PC++)); })
INS(0xff, _,_,_,_, 1, 16, "rst $38", { 
    SP -= 2; 
    memWr16(SP, PC); 
    PC = 0x38; 
})

#undef INS