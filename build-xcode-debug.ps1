if (-Not (Test-Path 'build-debug')) {
      New-Item -Path 'build-debug' -ItemType Directory
}

Push-Location build

cmake -DLLVM_ENABLE_RTTI=1                      `
      -DLLVM_BUILD_TOOLS=ON                     `
      -DLLVM_BUILD_EXAMPLES=ON                  `
      -DLLVM_INCLUDE_TESTS=ON                   `
      -DLLVM_TARGETS_TO_BUILD="X86"             `
      -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra"  `
      -DCMAKE_BUILD_TYPE=Debug                  `
      -G "Xcode"                                `
      ..\llvm                                   `
| tee ..\build-cmake-xcode-config.log


cmake --build . --config Debug  | tee ..\build-cmake-xcode-make.log

Pop-Location