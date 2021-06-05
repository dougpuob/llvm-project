// RUN: %clang -target riscv32-unknown-elf %s -mno-div -S -o - 2>&1 \
// RUN:   | not FileCheck -check-prefix=CHECK-DIV %s

// RUN: %clang -target riscv64-unknown-elf %s -mno-div -S -o - 2>&1 \
// RUN:   | not FileCheck -check-prefix=CHECK-DIV %s

// RUN: %clang -target riscv32-unknown-elf %s -mno-div -S -o - 2>&1 \
// RUN:   | not FileCheck -check-prefix=CHECK-REM %s

// RUN: %clang -target riscv64-unknown-elf %s -mno-div -S -o - 2>&1 \
// RUN:   | not FileCheck -check-prefix=CHECK-REM %s

// RUN: %clang -target riscv32-unknown-elf %s -mno-div -S -o - 2>&1 \
// RUN:   | FileCheck -check-prefix=CHECK-MUL %s

// RUN: %clang -target riscv64-unknown-elf %s -mno-div -S -o - 2>&1 \
// RUN:   | FileCheck -check-prefix=CHECK-MUL %s

// RUN: %clang -target riscv32-unknown-elf %s -S -o - 2>&1 \
// RUN:   | FileCheck -check-prefixes=CHECK-MUL,CHECK-DIV,CHECK-REM %s

// RUN: %clang -target riscv64-unknown-elf %s -S -o - 2>&1 \
// RUN:   | FileCheck -check-prefixes=CHECK-MUL,CHECK-DIV,CHECK-REM %s

int foo(int x, int y) {
  // CHECK-DIV: div{{w?}} a{{[0-9]}}, a{{[0-9]}}, a{{[0-9]}}
  // CHECK-REM: rem{{w?}} a{{[0-9]}}, a{{[0-9]}}, a{{[0-9]}}
  // CHECK-MUL: mul{{w?}} a{{[0-9]}}, a{{[0-9]}}, a{{[0-9]}}
  return (x / y) * (x % y);
}
