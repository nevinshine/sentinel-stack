#ifndef TCA_INTRINSICS_H
#define TCA_INTRINSICS_H

// TCA Opcode Mapping (Custom-0 space)
// setintent: [opcode=0x0B | rd=0 | funct3=0x0 | rs1=pointer | rs2=hash | funct7=0x01]
#define TCA_SET_INTENT(ptr, hash) \
    __asm__ volatile (".insn r 0x0B, 0x0, 0x01, x0, %0, %1" : : "r"(ptr), "r"(hash))

// clear: [opcode=0x0B | rd=0 | funct3=0x0 | rs1=0 | rs2=0 | funct7=0x02]
#define TCA_CLEAR() \
    __asm__ volatile (".insn r 0x0B, 0x0, 0x02, x0, x0, x0")

#endif
