//===--- AsyncBlockingCheck.h - clang-tidy ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CONCURRENCY_ASYNCBLOCKINGCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CONCURRENCY_ASYNCBLOCKINGCHECK_H

#include "../ClangTidyCheck.h"

namespace clang {
namespace tidy {
namespace concurrency {

/// Checks that non-coroutine-safe functions are not used.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/concurrency-async-blocking.html
class AsyncBlockingCheck : public ClangTidyCheck {
public:
  AsyncBlockingCheck(StringRef Name, ClangTidyContext *Context);

  void storeOptions(ClangTidyOptions::OptionMap &Opts) override;

  void registerMatchers(ast_matchers::MatchFinder *Finder) override;

  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;

private:
  const std::string LockableExtra;
  const std::string WaitableExtra;
  const bool AtomicNonLockFree;

  const std::string FunctionsExtra;
  const std::string TypesExtra;
};

} // namespace concurrency
} // namespace tidy
} // namespace clang

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_CONCURRENCY_ASYNCBLOCKINGCHECK_H
