// RUN: %clang_cc1 -triple riscv32 -moverlay -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple riscv64 -moverlay -fsyntax-only -verify %s

int notAFunction __attribute__((overlay));
// expected-warning@-1 {{'overlay' attribute only applies to functions and global constants}}

void incompatForwardDecl(int x);
void __attribute__((overlay)) incompatForwardDecl(int x) {}
// expected-error@-1 {{redeclaration of 'incompatForwardDecl' must not have the 'overlay' attribute}}
// expected-note@-3 {{previous definition is here}}

static void staticcall() __attribute__((overlay)) {}
// expected-error@-1 {{functions marked with 'overlay' attribute must have external linkage}}

static void __attribute__((overlay)) staticcall2() {}
// expected-error@-1 {{functions marked with 'overlay' attribute must have external linkage}}
