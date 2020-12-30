// RUN: %check_clang_tidy %s concurrency-async-blocking %t

void test_c11() {
  mtx_lock(0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function mtx_lock may sleep and is not coroutine-safe [concurrency-async-blocking]
  mtx_timedlock(0, 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function mtx_timedlock may sleep and is not coroutine-safe [concurrency-async-blocking]

  thrd_sleep(0, 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function thrd_sleep may sleep and is not coroutine-safe [concurrency-async-blocking]
  thrd_join(0, 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function thrd_join may sleep and is not coroutine-safe [concurrency-async-blocking]

  cnd_wait(0, 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function cnd_wait may sleep and is not coroutine-safe [concurrency-async-blocking]
  cnd_timedwait(0, 0, 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function cnd_timedwait may sleep and is not coroutine-safe [concurrency-async-blocking]
}

void test_posix() {
  sleep(1);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function sleep may sleep and is not coroutine-safe [concurrency-async-blocking]

  xsleep(1);
  sleepx(1);

  system("ls");
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function system may sleep and is not coroutine-safe [concurrency-async-blocking]

  wait(0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function wait may sleep and is not coroutine-safe [concurrency-async-blocking]
  waitpid(0, 0, 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function waitpid may sleep and is not coroutine-safe [concurrency-async-blocking]
  wait3(0, 0, 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function wait3 may sleep and is not coroutine-safe [concurrency-async-blocking]
  wait4(0, 0, 0, 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function wait4 may sleep and is not coroutine-safe [concurrency-async-blocking]
}
