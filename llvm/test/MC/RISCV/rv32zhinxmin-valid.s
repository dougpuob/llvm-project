# RUN: llvm-mc %s -triple=riscv32 -mattr=+experimental-zhinxmin -riscv-no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc %s -triple=riscv64 -mattr=+experimental-zhinxmin -riscv-no-aliases -show-encoding \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM,CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc -filetype=obj -triple=riscv32 -mattr=+experimental-zhinxmin < %s \
# RUN:     | llvm-objdump --mattr=+experimental-zhinxmin -M no-aliases -d -r - \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM-AND-OBJ %s
# RUN: llvm-mc -filetype=obj -triple=riscv64 -mattr=+experimental-zhinxmin < %s \
# RUN:     | llvm-objdump --mattr=+experimental-zhinxmin -M no-aliases -d -r - \
# RUN:     | FileCheck -check-prefixes=CHECK-ASM-AND-OBJ %s

# CHECK-ASM-AND-OBJ: lh s0, 12(a0)
# CHECK-ASM: encoding: [0x03,0x14,0xc5,0x00]
lh s0, 12(a0)
# CHECK-ASM-AND-OBJ: lh s1, 4(ra)
# CHECK-ASM: encoding: [0x83,0x94,0x40,0x00]
lh s1, +4(ra)
# CHECK-ASM-AND-OBJ: lh s2, -2048(a3)
# CHECK-ASM: encoding: [0x03,0x99,0x06,0x80]
lh s2, -2048(x13)
# CHECK-ASM-AND-OBJ: lh s3, -2048(s1)
# CHECK-ASM: encoding: [0x83,0x99,0x04,0x80]
lh s3, %lo(2048)(s1)
# CHECK-ASM-AND-OBJ: lh s4, 2047(s2)
# CHECK-ASM: encoding: [0x03,0x1a,0xf9,0x7f]
lh s4, 2047(s2)
# CHECK-ASM-AND-OBJ: lh s5, 0(s3)
# CHECK-ASM: encoding: [0x83,0x9a,0x09,0x00]
lh s5, 0(s3)

# CHECK-ASM-AND-OBJ: sh s6, 2047(s4)
# CHECK-ASM: encoding: [0xa3,0x1f,0x6a,0x7f]
sh s6, 2047(s4)
# CHECK-ASM-AND-OBJ: sh s7, -2048(s5)
# CHECK-ASM: encoding: [0x23,0x90,0x7a,0x81]
sh s7, -2048(s5)
# CHECK-ASM-AND-OBJ: sh s0, -2048(s6)
# CHECK-ASM: encoding: [0x23,0x10,0x8b,0x80]
sh x8, %lo(2048)(s6)
# CHECK-ASM-AND-OBJ: sh s1, 999(s7)
# CHECK-ASM: encoding: [0xa3,0x93,0x9b,0x3e]
sh x9, 999(s7)

# CHECK-ASM-AND-OBJ: fcvt.s.h a0, a1
# CHECK-ASM: encoding: [0x53,0x85,0x25,0x40]
fcvt.s.h a0, a1

# CHECK-ASM-AND-OBJ: fcvt.h.s a0, a1, dyn
# CHECK-ASM: encoding: [0x53,0xf5,0x05,0x44]
fcvt.h.s a0, a1
