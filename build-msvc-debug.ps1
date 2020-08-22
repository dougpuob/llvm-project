if (-Not (Test-Path 'build-debug')) {
	New-Item -Path 'build-debug' -ItemType Directory
}

Push-Location build-debug

cmake -DLLVM_ENABLE_ABI_BREAKING_CHECKS=0       `
      -DLLVM_BUILD_TOOLS=ON                     `
      -DLLVM_BUILD_EXAMPLES=ON                  `
      -DLLVM_INCLUDE_TESTS=ON                   `
      -DLLVM_TARGETS_TO_BUILD="X86"             `
      -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra"  `
      -DCMAKE_BUILD_TYPE=Debug                  `
      -G "Visual Studio 16 2019"                `
      ..\llvm                                   `
      | tee ..\build-cmake-msvc2019-config-debug.log


#cmake --build . --config Debug  | tee ..\build-cmake-msvc2019-make-debug.log

Pop-Location
