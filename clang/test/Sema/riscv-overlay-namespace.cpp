// RUN: %clang_cc1 %s -triple riscv32-unknown-elf -verify -fsyntax-only -moverlay
// RUN: %clang_cc1 %s -triple riscv64-unknown-elf -verify -fsyntax-only -moverlay

namespace {
class foo {
public:
  static int X() __attribute__((overlay)) { return 0; } // expected-error {{functions marked with 'overlay' attribute must have external linkage}}
};
} // end of anonymous namespace

namespace X {
class bar {
public:
  static int X() __attribute__((overlay)) { return 1; }
};
} // end of namespace X

extern "C" {
int main(void) { return foo::X() + X::bar::X(); }
}
