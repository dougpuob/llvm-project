// RUN: %check_clang_tidy %s concurrency-async-blocking %t -- \
// RUN: -config='{CheckOptions: [{key: "concurrency-async-blocking.LockableExtra", value: "my::mutex;my::shared_mutex"}, {key: "concurrency-async-blocking.WaitableExtra", value: "my::Future;my::cv"}, {key: "concurrency-async-blocking.LockableExtra", value: "my::mutex;my::shared_mutex"}, {key: "concurrency-async-blocking.FunctionsExtra", value: "my_sleep;my::sleep"}, {key: "concurrency-async-blocking.TypesExtra", value: "my::big_lock;my::other_lock"}]}'

/* Poor man's declaration of std::mutex and friends */
namespace std {
namespace chrono {
class seconds {
public:
  seconds(int);
};
} // namespace chrono

class mutex {
public:
  void lock();

  // non-std methods
  void lock_suffix();
  void prefix_lock();

  template <typename Duration>
  void try_lock_for(Duration);
};
class recursive_mutex {};
class recursive_timed_mutex {};
class shared_mutex {};
class shared_timed_mutex {};
class mutex_suffix {};
class prefix_mutex {};

template <typename Lock>
class unique_lock {
public:
  unique_lock(Lock &);

  void lock();
  template <typename Duration>
  void try_lock_for(Duration);

  // non-std methods
  void lock_suffix();
  void prefix_lock();
};

} // namespace std

namespace ns {
class mutex {};
} // namespace ns

class mutex {};

template <typename T>
class nonlock {};

namespace my {
class mutex {
public:
  void lock();
};
class shared_mutex {};
class non_mutex {
public:
  void lock();
};
} // namespace my

void test_lockable() {
  std::mutex m;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::mutex may sleep and is not coroutine-safe [concurrency-async-blocking]
  ::std::mutex mns;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type ::std::mutex may sleep and is not coroutine-safe [concurrency-async-blocking]
  std::shared_mutex sm;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::shared_mutex may sleep and is not coroutine-safe [concurrency-async-blocking]
  std::recursive_mutex rm;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::recursive_mutex may sleep and is not coroutine-safe [concurrency-async-blocking]
  std::recursive_timed_mutex rtm;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::recursive_timed_mutex may sleep and is not coroutine-safe [concurrency-async-blocking]
  my::mutex mym;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type my::mutex may sleep and is not coroutine-safe [concurrency-async-blocking]

  std::mutex_suffix m1;
  std::prefix_mutex m2;
  ns::mutex m3;
  mutex m4;
  my::non_mutex myn;

  m.lock();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method lock may sleep and is not coroutine-safe [concurrency-async-blocking]
  mns.lock();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method lock may sleep and is not coroutine-safe [concurrency-async-blocking]
  mym.lock();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method lock may sleep and is not coroutine-safe [concurrency-async-blocking]
  myn.lock();

  m.lock_suffix();
  m.prefix_lock();

  std::unique_lock<std::mutex> lock(m);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::unique_lock<std::mutex> may sleep and is not coroutine-safe [concurrency-async-blocking]

  std::unique_lock<std::mutex_suffix> l1(m1);
  std::unique_lock<std::prefix_mutex> l2(m2);
  std::unique_lock<ns::mutex> l3(m3);
  std::unique_lock<mutex> l4(m4);

  nonlock<std::mutex> nonlock;
}

void sleep(int);
void nanosleep(int);
void usleep(int);
void xsleep(int);
void sleepx(int);
void system(const char *);
int wait(int *);
int waitpid(int, int *, int);
int waitid(int idtype, int id, int *infop, int options);

struct rusage {};
using pid_t = int;
pid_t wait3(int *status, int options,
            struct rusage *rusage);

pid_t wait4(pid_t pid, int *status, int options,
            struct rusage *rusage);

namespace std {
namespace this_thread {
void yield();

template <typename Duration>
void sleep_for(Duration);

template <typename Duration>
void sleep_until(Duration);
} // namespace this_thread
} // namespace std

void test_std_thread() {
  std::this_thread::yield();

  std::chrono::seconds s(1);

  std::this_thread::sleep_for(s);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function sleep_for may sleep and is not coroutine-safe [concurrency-async-blocking]
  std::this_thread::sleep_until(s);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function sleep_until may sleep and is not coroutine-safe [concurrency-async-blocking]
}

void my_sleep();

namespace my {
void sleep();
}

void test_posix() {
  sleep(1);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function sleep may sleep and is not coroutine-safe [concurrency-async-blocking]
  ::sleep(1);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function sleep may sleep and is not coroutine-safe [concurrency-async-blocking]

  ::xsleep(1);
  xsleep(1);
  ::sleepx(1);
  sleepx(1);

  my_sleep();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function my_sleep may sleep and is not coroutine-safe [concurrency-async-blocking]
  my::sleep();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function sleep may sleep and is not coroutine-safe [concurrency-async-blocking]

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

namespace std {
template <typename T>
class atomic {
public:
  void wait(const T &);

  static constexpr bool is_always_lock_free = false;
};

class atomic_flag {
public:
  void wait(bool);
};

template <>
class atomic<short> {
public:
  void wait(const short &);

  static constexpr bool is_always_lock_free = false;
};

template <>
class atomic<char> {
public:
  void wait(const char &);

  static constexpr bool is_always_lock_free = true;
};

template <>
class atomic<long> {
public:
  void wait(const long &);

  static constexpr bool is_always_lock_free{true};
};

template <>
class atomic<long long> {
public:
  void wait(const char &);

  static constexpr bool is_always_lock_free{false};
};

template <typename T, typename U>
void atomic_wait(T *, U);

template <typename T, typename U, typename V>
void atomic_wait_explicit(T *, U, V);

template <typename T>
void atomic_flag_wait(T *, bool);

template <typename T, typename V>
void atomic_flag_wait_explicit(T *, bool, V);
} // namespace std

namespace boost {
template <typename T>
class atomic {
public:
  void wait(const T &);

  static constexpr bool is_always_lock_free = true;
};
} // namespace boost

void test_atomic() {
  // TODO: std::atomic<int> ai;
  // CHECKT-MESSAGES: :[[@LINE-1]]:3: warning: atomic is not always lockfree, may sleep and is not coroutine-safe [concurrency-async-blocking]
  std::atomic<short> as;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: atomic is not always lockfree, may sleep and is not coroutine-safe [concurrency-async-blocking]
  std::atomic<char> ac;
  std::atomic<long> al;
  std::atomic<long long> all;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: atomic is not always lockfree, may sleep and is not coroutine-safe [concurrency-async-blocking]

  // TODO: ai.wait(0);
  // CHECKT-MESSAGES: :[[@LINE-1]]:3: warning: method wait may sleep and is not coroutine-safe [concurrency-async-blocking]

  ac.wait(0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method wait may sleep and is not coroutine-safe [concurrency-async-blocking]

  std::atomic_flag flag;
  flag.wait(false);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function wait may sleep and is not coroutine-safe [concurrency-async-blocking]
  std::atomic_flag_wait(&flag, false);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function atomic_flag_wait may sleep and is not coroutine-safe [concurrency-async-blocking]
  std::atomic_flag_wait_explicit(&flag, false, 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function atomic_flag_wait_explicit may sleep and is not coroutine-safe [concurrency-async-blocking]

  std::atomic_wait(&ac, 1);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function atomic_wait may sleep and is not coroutine-safe [concurrency-async-blocking]
  std::atomic_wait_explicit(&ac, 1, 0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function atomic_wait_explicit may sleep and is not coroutine-safe [concurrency-async-blocking]

  boost::atomic<int> ba;
  ba.wait(0);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method wait may sleep and is not coroutine-safe [concurrency-async-blocking]
}

namespace std {
class thread {
public:
  void join();
};

class jthread {
public:
  ~jthread();
  void join();
};
} // namespace std

void test_thread() {
  std::thread t;
  t.join();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function join may sleep and is not coroutine-safe [concurrency-async-blocking]

  std::jthread j;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::jthread may sleep and is not coroutine-safe [concurrency-async-blocking]
  j.join();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: function join may sleep and is not coroutine-safe [concurrency-async-blocking]
}

namespace std {
class condition_variable {
public:
  void wait(unique_lock<std::mutex> &);

  template <typename Duration>
  void wait_for(unique_lock<std::mutex> &, Duration);

  template <typename Duration>
  void wait_until(unique_lock<std::mutex> &, Duration);
};
class barrier {
public:
  void wait();
  void arrive_and_wait();
};
class latch {
public:
  void wait();
  void arrive_and_wait();
};

template <typename T>
class future {
public:
  void wait();

  void get();
};
} // namespace std

namespace my {
class Future {
public:
  void wait();
  void get();
};
class Cv {
  void wait();
};
} // namespace my

void test_waitable() {
  std::mutex m;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::mutex may sleep and is not coroutine-safe [concurrency-async-blocking]
  std::unique_lock<std::mutex> lock(m);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::unique_lock<std::mutex> may sleep and is not coroutine-safe [concurrency-async-blocking]

  std::condition_variable cv;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::condition_variable may sleep and is not coroutine-safe [concurrency-async-blocking]
  cv.wait(lock);
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method wait may sleep and is not coroutine-safe [concurrency-async-blocking]
  cv.wait_for(lock, std::chrono::seconds(1));
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method wait_for may sleep and is not coroutine-safe [concurrency-async-blocking]
  cv.wait_until(lock, std::chrono::seconds(1));
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method wait_until may sleep and is not coroutine-safe [concurrency-async-blocking]

  my::Future myf;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type my::Future may sleep and is not coroutine-safe [concurrency-async-blocking]
  myf.wait();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method wait may sleep and is not coroutine-safe [concurrency-async-blocking]
  myf.get();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method get may sleep and is not coroutine-safe [concurrency-async-blocking]

  std::mutex_suffix ms;
  std::unique_lock<std::mutex_suffix> mslock(ms);

  std::latch l;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::latch may sleep and is not coroutine-safe [concurrency-async-blocking]
  l.wait();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method wait may sleep and is not coroutine-safe [concurrency-async-blocking]
  l.arrive_and_wait();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method arrive_and_wait may sleep and is not coroutine-safe [concurrency-async-blocking]

  std::barrier b;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::barrier may sleep and is not coroutine-safe [concurrency-async-blocking]
  b.wait();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method wait may sleep and is not coroutine-safe [concurrency-async-blocking]
  b.arrive_and_wait();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method arrive_and_wait may sleep and is not coroutine-safe [concurrency-async-blocking]

  std::future<int> f;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::future<int> may sleep and is not coroutine-safe [concurrency-async-blocking]
  f.wait();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method wait may sleep and is not coroutine-safe [concurrency-async-blocking]
  f.get();
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: method get may sleep and is not coroutine-safe [concurrency-async-blocking]
}

class X {
  std::mutex m;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::mutex may sleep and is not coroutine-safe [concurrency-async-blocking]

  std::unique_lock<std::mutex> lock;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type std::unique_lock<std::mutex> may sleep and is not coroutine-safe [concurrency-async-blocking]
};

void mtx_lock(void *);
void mtx_timedlock(void *, void *);
int thrd_sleep(void *, void *);
int thrd_join(void *, int *);
int cnd_wait(void *, void *);
int cnd_timedwait(void *, void *, void *);

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

namespace my {
class big_lock {};

class other_lock {};

class other {};
} // namespace my

void test_types() {
  my::big_lock lock;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type my::big_lock may sleep and is not coroutine-safe [concurrency-async-blocking]
  my::other_lock other_lock;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: type my::other_lock may sleep and is not coroutine-safe [concurrency-async-blocking]
  my::other other;
}

// TODO: remove CHECKT-MESSAGES
