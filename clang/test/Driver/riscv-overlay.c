// Check that ComRV Driver Arguments

// RUN: not %clang -target riscv32 -moverlay %s -o %t.o -mabi=ilp32f 2>&1 \
// RUN:   | FileCheck -check-prefix=INVALID-ABI %s
// INVALID-ABI: invalid ABI 'ilp32f' when using '-moverlay'
