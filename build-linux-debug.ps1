if (-Not (Test-Path 'build-debug')) {
	New-Item -Path 'build-debug' -ItemType Directory
}

Push-Location build-debug

cmake -DLLVM_ENABLE_ABI_BREAKING_CHECKS=0       `
      -DLLVM_BUILD_TOOLS=ON                     `
      -DLLVM_BUILD_EXAMPLES=ON                  `
      -DLLVM_INCLUDE_TESTS=ON                   `
      -DLLVM_USE_LINKER=lld                     `
      -DLLVM_TARGETS_TO_BUILD="X86"             `
      -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra"  `
      -DCMAKE_BUILD_TYPE=Debug                  `
      -DCMAKE_CXX_STANDARD=17                   `
      -DCMAKE_VERBOSE_MAKEFILE=OFF              `
      -G "Ninja"                                `
      ..\llvm                                   `
      | tee ..\build-cmake-linux-clang-config.log


cmake --build . --config Debug   | tee ..\build-cmake-linux-clang-make.log

Pop-Location
