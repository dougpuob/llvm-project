if (-Not (Test-Path 'build-relwithdebinfo')) {
	New-Item -Path 'build-relwithdebinfo' -ItemType Directory
}

Push-Location build-relwithdebinfo

cmake -DLLVM_ENABLE_ABI_BREAKING_CHECKS=0       `
      -DLLVM_BUILD_TOOLS=ON                     `
      -DLLVM_BUILD_EXAMPLES=ON                  `
      -DLLVM_INCLUDE_TESTS=ON                   `
      -DLLVM_TARGETS_TO_BUILD="X86"             `
      -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra"  `
      -DCMAKE_BUILD_TYPE=RelWithDebInfo         `
      -G "Visual Studio 16 2019"                `
      ..\llvm                                   `
      | tee ..\build-cmake-msvc2019-config.log


#cmake --build . --config RelWithDebInfo  | tee ..\build-cmake-msvc2019-make.log

Pop-Location
