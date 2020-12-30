//===--- AsyncBlockingCheck.cpp - clang-tidy ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AsyncBlockingCheck.h"
#include "../utils/OptionsUtils.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

static const auto kName = "name";
static const auto kType = "type";
static const auto kFunction = "function";
static const auto kMethod = "method";
static const auto kAtomic = "atomic";
static const auto kLockfree = "lockfree";

static std::vector<clang::StringRef>
toVector(const std::vector<clang::StringRef> Base, clang::StringRef Extra) {
  llvm::SmallVector<clang::StringRef, 4> Tmp{Base.begin(), Base.end()};
  if (!Extra.empty()) {
    Extra.split(Tmp, ";");
  }

  return {Tmp.begin(), Tmp.end()};
}

static const std::vector<clang::StringRef> LockableBase = {
    /* C++ std */
    "std::mutex",                 //
    "std::timed_mutex",           //
    "std::recursive_mutex",       //
    "std::recursive_timed_mutex", //
    "std::shared_mutex",          //
    "std::shared_timed_mutex",    //

    /* Boost.Thread */
    "boost::mutex",                 //
    "boost::timed_mutex",           //
    "boost::recursive_mutex",       //
    "boost::recursive_timed_mutex", //
    "boost::shared_mutex",          //
    "boost::upgrade_mutex",         //
};

static const std::vector<clang::StringRef> LockableMethods = {
    "lock",                  //
    "try_lock_for",          //
    "try_lock_until",        //
    "lock_shared",           //
    "try_lock_shared_for",   //
    "try_lock_shared_until", //
};

static const std::vector<clang::StringRef> LockGuardTemplates = {
    /* C++ std */
    "std::lock_guard",  //
    "std::scoped_lock", //
    "std::unique_lock", //
    "std::shared_lock", //

    /* Boost.Thread */
    "boost::unique_lock",            //
    "boost::shared_lock",            //
    "boost::upgrade_lock",           //
    "boost::upgrade_to_unique_lock", //
};

static const std::vector<clang::StringRef> AtomicNames = {
    "std::atomic",
    "boost::atomic",
};

static const std::vector<clang::StringRef> WaitableBase = {
    /* C++ std */
    "std::condition_variable", //
    "std::latch",         //
    "std::barrier",       //
    "std::future",        //
    "std::shared_future", //
    // TODO: std::condition_variable_any?

    /* Boost.Thread */
    "boost::condition_variable", //
    "boost::latch",              //
    "boost::barrier",            //
    "boost::future",             //
    "boost::shared_future",      //
};

static const std::vector<clang::StringRef> WaitableMethods = {
    "get",             //
    "wait",            //
    "wait_for",        //
    "wait_until",      //
    "arrive_and_wait", //
};

static const std::vector<clang::StringRef> BlockingFunctions = {
    /* C++ std */
    "std::this_thread::sleep_for", //
    "std::this_thread::sleep_until",
    // skip std::this_thread::yield()

    "std::thread::join",     //
    "std::jthread::jthread", //
    "std::jthread::join",    //

    // std::atomic::wait has a custom matcher
    "std::atomic_flag::wait", //

    "std::atomic_wait",               //
    "std::atomic_wait_explicit",      //
    "std::atomic_flag_wait",          //
    "std::atomic_flag_wait_explicit", //

    /* C11 */
    "::thrd_sleep", //
    "::thrd_join",  //
    // skip thrd_yield()

    "::mtx_lock",      //
    "::mtx_timedlock", //

    "::cnd_wait",      //
    "::cnd_timedwait", //

    /* POSIX (pthread) */
    "::pthread_barrier_wait",   //
    "::pthread_cond_timedwait", //
    "::pthread_cond_wait",      //
    "::pthread_join",           //

    "::pthread_mutex_lock",      //
    "::pthread_mutex_timedlock", //

    "::pthread_rwlock_timedrdlock", //
    "::pthread_rwlock_timedwrlock", //
    "::pthread_rwlock_rdlock",      //
    "::pthread_rwlock_wrlock",      //

    "::pthread_spin_lock",    //
    "::pthread_timedjoin_np", //

    /* POSIX */
    "::wait",            //
    "::waitpid",         //
    "::system",          //
    "::sleep",           //
    "::usleep",          //
    "::nanosleep",       //
    "::clock_nanosleep", //
    "::accept",          //
    "::mq_receive",      //
    "::mq_timedreceive", //
    "::msgsnd",          //
    "::msgrcv",          //
    "::poll",            //
    "::pselect",         //
    "::select",          //
    "::recv",            //
    "::recvmsg",         //
    "::recvfrom",        //
    "::semop",           //
    "::semtimedop",      //
    "::openlog",         //
    "::syslog",          //
    "::vsyslog",         //
    "::waitid",          //

    /* Linux syscalls */
    "::wait3",       //
    "::wait4",       //
    "::epoll_wait",  //
    "::epoll_pwait", //
    "::ppoll",       //

    /* Boost.Thread */
    "boost::this_thread::sleep",     //
    "boost::this_thread::sleep_for", //
    "boost::this_thread::sleep_until",

    "boost::thread::join",       //
    "boost::thread::timed_join", //
};

static const std::vector<clang::StringRef> TypesBase = {
    /* C++ std */
    "std::counting_semaphore", //
    "std::binary_semaphore",   //
    "std::jthread",            // due to dtr
};

namespace clang {
namespace tidy {
namespace concurrency {

AsyncBlockingCheck::AsyncBlockingCheck(StringRef Name,
                                       ClangTidyContext *Context)
    : ClangTidyCheck(Name, Context),
      LockableExtra(Options.get("LockableExtra", "")),
      WaitableExtra(Options.get("WaitableExtra", "")),
      AtomicNonLockFree(Options.get("AtomicNonLockFree", true)),
      FunctionsExtra(Options.get("FunctionsExtra", "")),
      TypesExtra(Options.get("TypesExtra", "")) {}

void AsyncBlockingCheck::storeOptions(ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "LockableExtra", LockableExtra);
  Options.store(Opts, "WaitableExtra", WaitableExtra);
  Options.store(Opts, "AtomicNonLockFree", AtomicNonLockFree);
  Options.store(Opts, "FunctionsExtra", FunctionsExtra);
  Options.store(Opts, "TypesExtra", TypesExtra);
}

void AsyncBlockingCheck::registerMatchers(MatchFinder *Finder) {
  // std::mutex
  auto Lockable = toVector(LockableBase, StringRef(LockableExtra));
  Finder->addMatcher(
      valueDecl(hasType(cxxRecordDecl(hasAnyName(Lockable)))).bind(kType),
      this);

  // User code may use already created std::mutex, catch its usage
  // std::mutex::lock()
  // Note: exception for std::atomic as its type is handled separately
  Finder->addMatcher(
      cxxMemberCallExpr(
          callee(functionDecl(hasAnyName(LockableMethods)).bind(kName)),
          on(hasType(cxxRecordDecl(hasAnyName(Lockable)))))
          .bind(kMethod),
      this);

  // std::future<T>
  auto Waitable = toVector(WaitableBase, StringRef(WaitableExtra));
  Finder->addMatcher(
      valueDecl(hasType(cxxRecordDecl(hasAnyName(Waitable)))).bind(kType),
      this);

  // std::future<T>::wait()
  Finder->addMatcher(
      cxxMemberCallExpr(
          callee(functionDecl(hasAnyName(WaitableMethods)).bind(kName)),
          on(hasType(cxxRecordDecl(
              anyOf(hasAnyName(Waitable), hasAnyName(AtomicNames))))))
          .bind(kMethod),
      this);

  // sleep()
  auto SleepingFunctionNames =
      toVector(BlockingFunctions, StringRef(FunctionsExtra));
  Finder->addMatcher(
      callExpr(
          callee(functionDecl(hasAnyName(SleepingFunctionNames)).bind(kName)))
          .bind(kFunction),
      this);

  // std::jthread
  auto TypeNames = toVector(TypesBase, StringRef(TypesExtra));
  Finder->addMatcher(
      valueDecl(hasType(cxxRecordDecl(hasAnyName(TypeNames)))).bind(kType),
      this);

  // std::atomic<T> which is !is_always_lock_free
  if (AtomicNonLockFree) {
    Finder->addMatcher(
        valueDecl(
            hasType(cxxRecordDecl(
                has(varDecl(hasName("is_always_lock_free")).bind(kLockfree)),
                hasAnyName(AtomicNames))))
            .bind(kAtomic),
        this);
  }

  // std::unique_lock<std::mutex>
  Finder->addMatcher(
      valueDecl(hasType(qualType(hasDeclaration(classTemplateSpecializationDecl(
                    hasAnyName(LockGuardTemplates),
                    hasTemplateArgument(
                        0, refersToType(qualType(hasDeclaration(
                               cxxRecordDecl(hasAnyName(Lockable)))))))))))
          .bind(kType),
      this);
}

void AsyncBlockingCheck::check(const MatchFinder::MatchResult &Result) {
  auto *Name = Result.Nodes.getNodeAs<NamedDecl>(kName);

  const auto *D = Result.Nodes.getNodeAs<ValueDecl>(kType);
  if (D) {
    diag(D->getBeginLoc(), "type " + D->getType().getAsString() +
                               " may sleep and is not coroutine-safe")
        << SourceRange(D->getBeginLoc(), D->getEndLoc());
  }

  const auto *CE = Result.Nodes.getNodeAs<CXXMemberCallExpr>(kMethod);
  if (CE) {
    if (Name) {
      diag(CE->getBeginLoc(), "method " + Name->getNameAsString() +
                                  " may sleep and is not coroutine-safe")
          << SourceRange(CE->getBeginLoc(), CE->getEndLoc());
    }
  }

  const auto *E = Result.Nodes.getNodeAs<CallExpr>(kFunction);
  if (E) {
    if (Name) {
      diag(E->getBeginLoc(), "function " + Name->getNameAsString() +
                                 " may sleep and is not coroutine-safe")
          << SourceRange(E->getBeginLoc(), E->getEndLoc());
    }
  }

  const auto *Atomic = Result.Nodes.getNodeAs<ValueDecl>(kAtomic);
  if (Atomic) {
    const auto *Lockfree = Result.Nodes.getNodeAs<VarDecl>(kLockfree);
    if (Lockfree) {
      const auto *EV = Lockfree->ensureEvaluatedStmt();
      if (EV && EV->Evaluated.getKind() == APValue::ValueKind::Int &&
          EV->Evaluated.getInt() == 0) {
        diag(Atomic->getBeginLoc(), "atomic is not always lockfree, may sleep "
                                    "and is not coroutine-safe")
            << SourceRange(Atomic->getBeginLoc(), Atomic->getEndLoc());
      }
    }
  }
}

} // namespace concurrency
} // namespace tidy
} // namespace clang
