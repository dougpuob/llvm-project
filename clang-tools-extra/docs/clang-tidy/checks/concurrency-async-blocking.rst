.. title:: clang-tidy - concurrency-async-blocking

concurrency-async-blocking
==========================

Checks for some synchronous functions and types that volunteerly preempt system thread.
Volunteer preemption of a system thread in asynchronous code
(e.g. in coroutines/fibers/green threads) is a bug that prevents the current
thread from executing other coroutines/etc. and negatively affects overall
process performance.

The preemptive functions/types can be separated into the following categories:
 - explicit sleep(3)-like functions
 - sleeping/waiting synchronization primitives
 - io/filesystem stuff (not implemented yet)

The check searches for:
 - C++ synchronization primitives
 - C11 synchronization primitives
 - POSIX synchronization primitives
 - some POSIX blocking functions
 - some blocking Linux syscalls
 - some Boost.Thread synchronization primitives

.. option:: LockableExtra

  Specifies additional lock type names separated with semicolon. Usually they
  implement C++17 BasicLockable, Lockable, TimedLockable, Mutex, or TimedMutex
  requirement or has one or several methods from the following list:
    - `lock`
    - `try_lock_for`
    - `try_lock_until`
    - `lock_shared`
    - `try_lock_shared_for`
    - `try_lock_shared_until`

  The check searches for explicit method calls from the list above and implicit
  locking using std::lock_guard and other RAII sychronization primitive
  ownership wrappers.

  The list of classes which are already handled (and their boost:: twins):
    - `std::mutex`
    - `std::timed_mutex`
    - `std::recursive_mutex`
    - `std::recursive_timed_mutex`
    - `std::shared_mutex`
    - `std::shared_timed_mutex`

.. option:: WaitableExtra

  Specifies additional future-like types separated with semicolon.
  The type must implement one or several methods from the following list:
    - `get`
    - `wait`
    - `wait_for`
    - `wait_until`
    - `arrive_and_wait`

  The list of classes which are already handled (and their boost:: twins):
    - `std::condition_variable`
    - `std::latch`
    - `std::barrier`
    - `std::future<T>`
    - `std::shared_future`

.. option:: AtomicNonLockFree

  Specifies whether search for std::atomic types which are not always lock-free.
  Non-lockfree atomics use std synchronization primitives (e.g. std::mutex), so
  they may block current system thread for a while. If such accesses are
  frequent, too much execution time might be spent on waiting due to mutex
  contention.

  If set to true, checks `std::atomic<T>::is_always_lock_free` and warns about
  `std::atomic<T>`.

.. option:: FunctionsExtra

  Extra functions that may block current system thread, separated with semicolon.
  Usually the function list should be compiled for third-party libraries or
  user-defined known-to-be-blocking functions.

  Already handled:
    - C11 thread/mutex/condition variable functions
    - some POSIX functions
    - some Linux syscalls

.. option:: TypesExtra

  Extra types that may block current system thread, separated with semicolon.
  Usually the type list should be compiled for third-party libraries or
  user-defined known-to-be-blocking types.
