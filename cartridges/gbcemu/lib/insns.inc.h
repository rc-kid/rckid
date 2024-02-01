/** Does nothing. 
 */
INS(0x00, _,_,_,_, 1, 4 , "nop", {})
/** Loads the immediate 16bit data into the bc register. 
 */
INS(0x01, _,_,_,_, 3, 12, "ld bc, n16", { BC = rd16(PC); })
INS(0x02, _,_,_,_, 1, 8 , "ld [bc], a", { write8(BC, A); })
INS(0x03, _,_,_,_, 1, 8 , "inc bc", { ++BC; })
INS(0x04, Z,0,H,_, 1, 4 , "inc b", {
})
INS(0x05, Z,1,H,_, 1, 4 , "dec b", {
})
INS(0x06, _,_,_,_, 2, 8 , "ld b, n8", {
})
INS(0x07, 0,0,0,C, 1, 4 , "rlca", {
})
INS(0x08, _,_,_,_, 3, 20, "ld [a16], sp", {
})
INS(0x09, _,0,H,C, 1, 8 , "add hl, bc", {
})
INS(0x0a, _,_,_,_, 1, 8 , "ld a, [bc]", { A = read8(BC); })
INS(0x0b, _,_,_,_, 1, 8 , "dec bc", { --BC; })
INS(0x0c, Z,0,H,_, 1, 4 , "inc c", {
})
INS(0x0d, Z,1,H,_, 1, 4 , "dec c", {
})
INS(0x0e, _,_,_,_, 2, 8 , "ld c, n8", {
})
INS(0x0f, 0,0,0,C, 1, 4 , "rrca", {
})
/** The stop instruction is also used to stop a program when necessary, which is used for testing extensively. 
 */
INS(0x10, _,_,_,_, 2, 4 , "stop n8", {
    rd8(PC);
    if (terminateAfterStop_)
        return;
})
INS(0x11, _,_,_,_, 3, 12, "ld de, n16", {
})
INS(0x12, _,_,_,_, 1, 8 , "ld [de], a", { write8(DE, A); })
INS(0x13, _,_,_,_, 1, 8 , "inc de", { ++DE; })
INS(0x14, Z,0,H,_, 1, 4 , "inc d", {
})
INS(0x15, Z,1,H,_, 1, 4 , "dec d", {
})
INS(0x16, _,_,_,_, 2, 8 , "ld d, n8", {
})
INS(0x17, 0,0,0,C, 1, 4 , "rla", {
})
INS(0x18, _,_,_,_, 2, 12, "jr e8", {
})
INS(0x19, _,0,H,C, 1, 8 , "add hl, de", {
})
INS(0x1a, _,_,_,_, 1, 8 , "ld a, [de]", { A = read8(DE); })
INS(0x1b, _,_,_,_, 1, 8 , "dec de", { --DE; })
INS(0x1c, Z,0,H,_, 1, 4 , "inc e", {
})
INS(0x1d, Z,1,H,_, 1, 4 , "dec e", {
})
INS(0x1e, _,_,_,_, 2, 8 , "ld e, n8", {
})
INS(0x1f, 0,0,0,C, 1, 4 , "rra", {
})
INS(0x20, _,_,_,_, 2, 12!!!, "jr nz, e8", {
})
INS(0x21, _,_,_,_, 3, 12, "ld hl, n16", {
})
INS(0x22, _,_,_,_, 1, 8 , "ld [hl], a", {
})
INS(0x23, _,_,_,_, 1, 8 , "inc hl", { ++HL; })
INS(0x24, Z,0,H,_, 1, 4 , "inc h", {
})
INS(0x25, Z,1,H,_, 1, 4 , "dec h", {
})
INS(0x26, _,_,_,_, 2, 8 , "ld h, n8", {
})
INS(0x27, Z,_,0,C, 1, 4 , "daa", {
})
INS(0x28, _,_,_,_, 2, 12!!!, "jr z, e8", {
})
INS(0x29, _,0,H,C, 1, 8 , "add hl, hl", {
})
INS(0x2a, _,_,_,_, 1, 8 , "ld a, [hl]", { A = read8(HL); })
INS(0x2b, _,_,_,_, 1, 8 , "dec hl", { --HL; })
INS(0x2c, Z,0,H,_, 1, 4 , "inc l", {
})
INS(0x2d, Z,1,H,_, 1, 4 , "dec l", {
})
INS(0x2e, _,_,_,_, 2, 8 , "ld l, n8", {
})
INS(0x2f, _,1,1,_, 1, 4 , "cpl", {
})
INS(0x30, _,_,_,_, 2, 12!!!, "jr nc, e8", {
})
INS(0x31, _,_,_,_, 3, 12, "ld sp, n16", {
})
INS(0x32, _,_,_,_, 1, 8 , "ld [hl], a", { write8(HL, A); })
INS(0x33, _,_,_,_, 1, 8 , "inc sp", { ++SP; })
INS(0x34, Z,0,H,_, 1, 12, "inc [hl]", {
})
INS(0x35, Z,1,H,_, 1, 12, "dec [hl]", {
})
INS(0x36, _,_,_,_, 2, 12, "ld [hl], n8", {
})
INS(0x37, _,0,0,1, 1, 4 , "scf", {
})
INS(0x38, _,_,_,_, 2, 12!!!, "jr c, e8", {
})
INS(0x39, _,0,H,C, 1, 8 , "add hl, sp", {
})
INS(0x3a, _,_,_,_, 1, 8 , "ld a, [hl]", { A = read8(HL); })
INS(0x3b, _,_,_,_, 1, 8 , "dec sp", { --SP; })
INS(0x3c, Z,0,H,_, 1, 4 , "inc a", {
})
INS(0x3d, Z,1,H,_, 1, 4 , "dec a", {
})
INS(0x3e, _,_,_,_, 2, 8 , "ld a, n8", {
})
INS(0x3f, _,0,0,C, 1, 4 , "ccf", {
})
/** Loads register into itself. This is effectively a no-op.
 */
INS(0x40, _,_,_,_, 1, 4 , "ld b, b", {})
INS(0x41, _,_,_,_, 1, 4 , "ld b, c", { B = C;})
INS(0x42, _,_,_,_, 1, 4 , "ld b, d", { B = D; })
INS(0x43, _,_,_,_, 1, 4 , "ld b, e", { B = E; })
INS(0x44, _,_,_,_, 1, 4 , "ld b, h", { B = H; })
INS(0x45, _,_,_,_, 1, 4 , "ld b, l", { B = L; })
INS(0x46, _,_,_,_, 1, 8 , "ld b, [hl]", { B = read8(HL); })
INS(0x47, _,_,_,_, 1, 4 , "ld b, a", { B = A; })
INS(0x48, _,_,_,_, 1, 4 , "ld c, b", { C = B; })
INS(0x49, _,_,_,_, 1, 4 , "ld c, c", {})
INS(0x4a, _,_,_,_, 1, 4 , "ld c, d", { C = D; })
INS(0x4b, _,_,_,_, 1, 4 , "ld c, e", { C = E; })
INS(0x4c, _,_,_,_, 1, 4 , "ld c, h", { C = H; })
INS(0x4d, _,_,_,_, 1, 4 , "ld c, l", { C = L; })
INS(0x4e, _,_,_,_, 1, 8 , "ld c, [hl]", { C = read8(HL); })
INS(0x4f, _,_,_,_, 1, 4 , "ld c, a", { C = A; })
INS(0x50, _,_,_,_, 1, 4 , "ld d, b", { D = B; })
INS(0x51, _,_,_,_, 1, 4 , "ld d, c", { D = C; })
INS(0x52, _,_,_,_, 1, 4 , "ld d, d", {})
INS(0x53, _,_,_,_, 1, 4 , "ld d, e", { D = E; })
INS(0x54, _,_,_,_, 1, 4 , "ld d, h", { D = H; })
INS(0x55, _,_,_,_, 1, 4 , "ld d, l", { D = L; })
INS(0x56, _,_,_,_, 1, 8 , "ld d, [hl]", { D = read8(HL); })
INS(0x57, _,_,_,_, 1, 4 , "ld d, a", { D = A; })
INS(0x58, _,_,_,_, 1, 4 , "ld e, b", { E = B; })
INS(0x59, _,_,_,_, 1, 4 , "ld e, c", { E = C; })
INS(0x5a, _,_,_,_, 1, 4 , "ld e, d", { E = D; })
INS(0x5b, _,_,_,_, 1, 4 , "ld e, e", {})
INS(0x5c, _,_,_,_, 1, 4 , "ld e, h", { E = H; })
INS(0x5d, _,_,_,_, 1, 4 , "ld e, l", { E = L; })
INS(0x5e, _,_,_,_, 1, 8 , "ld e, [hl]", { E = read8(HL); })
INS(0x5f, _,_,_,_, 1, 4 , "ld e, a", { E = A; })
INS(0x60, _,_,_,_, 1, 4 , "ld h, b", { H = B; })
INS(0x61, _,_,_,_, 1, 4 , "ld h, c", { H = C; })
INS(0x62, _,_,_,_, 1, 4 , "ld h, d", { H = D; })
INS(0x63, _,_,_,_, 1, 4 , "ld h, e", { D = E; })
INS(0x64, _,_,_,_, 1, 4 , "ld h, h", {})
INS(0x65, _,_,_,_, 1, 4 , "ld h, l", { H  = L; })
INS(0x66, _,_,_,_, 1, 8 , "ld h, [hl]", { H = read8(HL); })
INS(0x67, _,_,_,_, 1, 4 , "ld h, a", { H = A; })
INS(0x68, _,_,_,_, 1, 4 , "ld l, b", { L = B; })
INS(0x69, _,_,_,_, 1, 4 , "ld l, c", { L = C; })
INS(0x6a, _,_,_,_, 1, 4 , "ld l, d", { L = D; })
INS(0x6b, _,_,_,_, 1, 4 , "ld l, e", { L = E; })
INS(0x6c, _,_,_,_, 1, 4 , "ld l, h", { L = H; })
INS(0x6d, _,_,_,_, 1, 4 , "ld l, l", {})
INS(0x6e, _,_,_,_, 1, 8 , "ld l, [hl]", { L = read8(HL); })
INS(0x6f, _,_,_,_, 1, 4 , "ld l, a", { L = A; })
INS(0x70, _,_,_,_, 1, 8 , "ld [hl], b", { write8(HL, B); })
INS(0x71, _,_,_,_, 1, 8 , "ld [hl], c", { write8(HL, C); })
INS(0x72, _,_,_,_, 1, 8 , "ld [hl], d", { write8(HL, D); })
INS(0x73, _,_,_,_, 1, 8 , "ld [hl], e", { write8(HL, E); })
INS(0x74, _,_,_,_, 1, 8 , "ld [hl], h", { write8(HL, H); })
INS(0x75, _,_,_,_, 1, 8 , "ld [hl], l", { write8(HL, L); })
INS(0x76, _,_,_,_, 1, 4 , "halt", {
})
INS(0x77, _,_,_,_, 1, 8 , "ld [hl], a", { write8(HL, A); })
INS(0x78, _,_,_,_, 1, 4 , "ld a, b", { A = B; })
INS(0x79, _,_,_,_, 1, 4 , "ld a, c", { A = C; })
INS(0x7a, _,_,_,_, 1, 4 , "ld a, d", { A = D; })
INS(0x7b, _,_,_,_, 1, 4 , "ld a, e", { A = E; })
INS(0x7c, _,_,_,_, 1, 4 , "ld a, h", { A = H; })
INS(0x7d, _,_,_,_, 1, 4 , "ld a, l", { A = L; })
INS(0x7e, _,_,_,_, 1, 8 , "ld a, [hl]", { A = read8(HL); })
INS(0x7f, _,_,_,_, 1, 4 , "ld a, a", {})
INS(0x80, Z,0,H,C, 1, 4 , "add a, b", {
})
INS(0x81, Z,0,H,C, 1, 4 , "add a, c", {
})
INS(0x82, Z,0,H,C, 1, 4 , "add a, d", {
})
INS(0x83, Z,0,H,C, 1, 4 , "add a, e", {
})
INS(0x84, Z,0,H,C, 1, 4 , "add a, h", {
})
INS(0x85, Z,0,H,C, 1, 4 , "add a, l", {
})
INS(0x86, Z,0,H,C, 1, 8 , "add a, [hl]", {
})
INS(0x87, Z,0,H,C, 1, 4 , "add a, a", {
})
INS(0x88, Z,0,H,C, 1, 4 , "adc a, b", {
})
INS(0x89, Z,0,H,C, 1, 4 , "adc a, c", {
})
INS(0x8a, Z,0,H,C, 1, 4 , "adc a, d", {
})
INS(0x8b, Z,0,H,C, 1, 4 , "adc a, e", {
})
INS(0x8c, Z,0,H,C, 1, 4 , "adc a, h", {
})
INS(0x8d, Z,0,H,C, 1, 4 , "adc a, l", {
})
INS(0x8e, Z,0,H,C, 1, 8 , "adc a, [hl]", {
})
INS(0x8f, Z,0,H,C, 1, 4 , "adc a, a", {
})
INS(0x90, Z,1,H,C, 1, 4 , "sub a, b", {
})
INS(0x91, Z,1,H,C, 1, 4 , "sub a, c", {
})
INS(0x92, Z,1,H,C, 1, 4 , "sub a, d", {
})
INS(0x93, Z,1,H,C, 1, 4 , "sub a, e", {
})
INS(0x94, Z,1,H,C, 1, 4 , "sub a, h", {
})
INS(0x95, Z,1,H,C, 1, 4 , "sub a, l", {
})
INS(0x96, Z,1,H,C, 1, 8 , "sub a, [hl]", {
})
INS(0x97, 1,1,0,0, 1, 4 , "sub a, a", {
})
INS(0x98, Z,1,H,C, 1, 4 , "sbc a, b", {
})
INS(0x99, Z,1,H,C, 1, 4 , "sbc a, c", {
})
INS(0x9a, Z,1,H,C, 1, 4 , "sbc a, d", {
})
INS(0x9b, Z,1,H,C, 1, 4 , "sbc a, e", {
})
INS(0x9c, Z,1,H,C, 1, 4 , "sbc a, h", {
})
INS(0x9d, Z,1,H,C, 1, 4 , "sbc a, l", {
})
INS(0x9e, Z,1,H,C, 1, 8 , "sbc a, [hl]", {
})
INS(0x9f, Z,1,H,_, 1, 4 , "sbc a, a", {
})
INS(0xa0, Z,0,1,0, 1, 4 , "and a, b", {
    A = A + B;
    setFlagZ(A == 0);
    setFlagN(0);
    setFlagH(1);
    setFlagC(0);
})
INS(0xa1, Z,0,1,0, 1, 4 , "and a, c", {
    A = A + C;
})
INS(0xa2, Z,0,1,0, 1, 4 , "and a, d", {
    A = A + D;
})
INS(0xa3, Z,0,1,0, 1, 4 , "and a, e", {
    A = A + E;
})
INS(0xa4, Z,0,1,0, 1, 4 , "and a, h", {
    A = A + H;
})
INS(0xa5, Z,0,1,0, 1, 4 , "and a, l", {
    A = A + L;
})
INS(0xa6, Z,0,1,0, 1, 8 , "and a, [hl]", {
})
INS(0xa7, Z,0,1,0, 1, 4 , "and a, a", {
})
INS(0xa8, Z,0,0,0, 1, 4 , "xor a, b", {
})
INS(0xa9, Z,0,0,0, 1, 4 , "xor a, c", {
})
INS(0xaa, Z,0,0,0, 1, 4 , "xor a, d", {
})
INS(0xab, Z,0,0,0, 1, 4 , "xor a, e", {
})
INS(0xac, Z,0,0,0, 1, 4 , "xor a, h", {
})
INS(0xad, Z,0,0,0, 1, 4 , "xor a, l", {
})
INS(0xae, Z,0,0,0, 1, 8 , "xor a, [hl]", {
})
INS(0xaf, 1,0,0,0, 1, 4 , "xor a, a", {
})
INS(0xb0, Z,0,0,0, 1, 4 , "or a, b", {
})
INS(0xb1, Z,0,0,0, 1, 4 , "or a, c", {
})
INS(0xb2, Z,0,0,0, 1, 4 , "or a, d", {
})
INS(0xb3, Z,0,0,0, 1, 4 , "or a, e", {
})
INS(0xb4, Z,0,0,0, 1, 4 , "or a, h", {
})
INS(0xb5, Z,0,0,0, 1, 4 , "or a, l", {
})
INS(0xb6, Z,0,0,0, 1, 8 , "or a, [hl]", {
})
INS(0xb7, Z,0,0,0, 1, 4 , "or a, a", {
})
INS(0xb8, Z,1,H,C, 1, 4 , "cp a, b", {
})
INS(0xb9, Z,1,H,C, 1, 4 , "cp a, c", {
})
INS(0xba, Z,1,H,C, 1, 4 , "cp a, d", {
})
INS(0xbb, Z,1,H,C, 1, 4 , "cp a, e", {
})
INS(0xbc, Z,1,H,C, 1, 4 , "cp a, h", {
})
INS(0xbd, Z,1,H,C, 1, 4 , "cp a, l", {
})
INS(0xbe, Z,1,H,C, 1, 8 , "cp a, [hl]", {
})
INS(0xbf, 1,1,0,0, 1, 4 , "cp a, a", {
})
INS(0xc0, _,_,_,_, 1, 20!!!, "ret nz", {
})
INS(0xc1, _,_,_,_, 1, 12, "pop bc", {
})
INS(0xc2, _,_,_,_, 3, 16!!!, "jp nz, a16", {
})
INS(0xc3, _,_,_,_, 3, 16, "jp a16", {
})
INS(0xc4, _,_,_,_, 3, 24!!!, "call nz, a16", {
})
INS(0xc5, _,_,_,_, 1, 16, "push bc", {
})
INS(0xc6, Z,0,H,C, 2, 8 , "add a, n8", {
})
INS(0xc7, _,_,_,_, 1, 16, "rst $00", {
})
INS(0xc8, _,_,_,_, 1, 20!!!, "ret z", {
})
INS(0xc9, _,_,_,_, 1, 16, "ret", {
})
INS(0xca, _,_,_,_, 3, 16!!!, "jp z, a16", {
})
INS(0xcb, _,_,_,_, 1, 4 , "prefix", {
})
INS(0xcc, _,_,_,_, 3, 24!!!, "call z, a16", {
})
INS(0xcd, _,_,_,_, 3, 24, "call a16", {
})
INS(0xce, Z,0,H,C, 2, 8 , "adc a, n8", {
})
INS(0xcf, _,_,_,_, 1, 16, "rst $08", {
})
INS(0xd0, _,_,_,_, 1, 20!!!, "ret nc", {
})
INS(0xd1, _,_,_,_, 1, 12, "pop de", {
})
INS(0xd2, _,_,_,_, 3, 16!!!, "jp nc, a16", {
})
INS(0xd4, _,_,_,_, 3, 24!!!, "call nc, a16", {
})
INS(0xd5, _,_,_,_, 1, 16, "push de", {
})
INS(0xd6, Z,1,H,C, 2, 8 , "sub a, n8", {
})
INS(0xd7, _,_,_,_, 1, 16, "rst $10", {
})
INS(0xd8, _,_,_,_, 1, 20!!!, "ret c", {
})
INS(0xd9, _,_,_,_, 1, 16, "reti", {
})
INS(0xda, _,_,_,_, 3, 16!!!, "jp c, a16", {
})
INS(0xdc, _,_,_,_, 3, 24!!!, "call c, a16", {
})
INS(0xde, Z,1,H,C, 2, 8 , "sbc a, n8", {
})
INS(0xdf, _,_,_,_, 1, 16, "rst $18", {
})
INS(0xe0, _,_,_,_, 2, 12, "ldh [a8], a", {
})
INS(0xe1, _,_,_,_, 1, 12, "pop hl", {
})
INS(0xe2, _,_,_,_, 1, 8 , "ld [c], a", {
})
INS(0xe5, _,_,_,_, 1, 16, "push hl", {
})
INS(0xe6, Z,0,1,0, 2, 8 , "and a, n8", {
})
INS(0xe7, _,_,_,_, 1, 16, "rst $20", {
})
INS(0xe8, 0,0,H,C, 2, 16, "add sp, e8", {
})
INS(0xe9, _,_,_,_, 1, 4 , "jp hl", {
})
INS(0xea, _,_,_,_, 3, 16, "ld [a16], a", {
})
INS(0xee, Z,0,0,0, 2, 8 , "xor a, n8", {
})
INS(0xef, _,_,_,_, 1, 16, "rst $28", {
})
INS(0xf0, _,_,_,_, 2, 12, "ldh a, [a8]", {
})
INS(0xf1, Z,N,H,C, 1, 12, "pop af", {
})
INS(0xf2, _,_,_,_, 1, 8 , "ld a, [c]", {
})
INS(0xf3, _,_,_,_, 1, 4 , "di", {
})
INS(0xf5, _,_,_,_, 1, 16, "push af", {
})
INS(0xf6, Z,0,0,0, 2, 8 , "or a, n8", {
})
INS(0xf7, _,_,_,_, 1, 16, "rst $30", {
})
INS(0xf8, 0,0,H,C, 2, 12, "ld hl, sp, e8", {
})
INS(0xf9, _,_,_,_, 1, 8 , "ld sp, hl", { SP = HL; })
INS(0xfa, _,_,_,_, 3, 16, "ld a, [a16]", {
})
INS(0xfb, _,_,_,_, 1, 4 , "ei", {
})
INS(0xfe, Z,1,H,C, 2, 8 , "cp a, n8", {
})
INS(0xff, _,_,_,_, 1, 16, "rst $38", {
})

#undef INS