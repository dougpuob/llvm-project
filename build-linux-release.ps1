if (-Not (Test-Path 'build-release')) {
	New-Item -Path 'build-release' -ItemType Directory
}

Push-Location build-release

cmake -DLLVM_ENABLE_ABI_BREAKING_CHECKS=0       `
      -DLLVM_BUILD_TOOLS=ON                     `
      -DLLVM_BUILD_EXAMPLES=ON                  `
      -DLLVM_INCLUDE_TESTS=ON                   `
      -DLLVM_TARGETS_TO_BUILD="X86"             `
      -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra"  `
      -DCMAKE_BUILD_TYPE=Release                `
      -G "Ninja"                                `
      ..\llvm                                   `
      | tee ..\build-cmake-linux-clang-config.log


cmake --build . --config Release  | tee ..\build-cmake-linux-clang-make.log

Pop-Location
