import os

with open('src/mmu.c', 'r') as f:
    content = f.read()

# Fix VTCR_EL2
content = content.replace(
    "(1ULL << 6)  | // ORGN0 = Normal WB\n                        (24ULL << 0);  // T0SZ = 24",
    "(1ULL << 10) | // ORGN0 = Normal WB\n                        (1ULL << 6)  | // SL0 = 1 (Level 1)\n                        (25ULL << 0);  // T0SZ = 25 (39-bit space)"
)

with open('src/mmu.c', 'w') as f:
    f.write(content)

with open('src/boot.S', 'r') as f:
    content = f.read()

# Fix 0x80080000 -> 0x40080000
content = content.replace("0x80080000", "0x40080000")
with open('src/boot.S', 'w') as f:
    f.write(content)

with open('linker.ld', 'r') as f:
    content = f.read()
content = content.replace("0x80080000", "0x40080000")
with open('linker.ld', 'w') as f:
    f.write(content)
