#include <chrono>
#include <iostream>
#include <memory>
#include <semaphore>
#include <string>
#include <thread>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <type_traits>
#include <bits/stdc++.h>

using namespace std;

mutex print_mutex;
void print(string msg) {
  lock_guard<mutex> lock(print_mutex);
  cout << msg << endl;
}

void sleep(int milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

uint64_t get_unix_time() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

char print_buffer[10000];
template<class... Args>
void print(const char* const format, Args&&... args) {
  lock_guard<mutex> lock(print_mutex);
  sprintf(print_buffer, format, std::forward<Args>(args)...);
  cout << print_buffer << endl;
}

struct semaphore {
  int count = 0;
  int wake_ups = 0;
  mutex m;
  condition_variable cv;

  semaphore(int value = 0): count(value) {}

  void wait() {
    unique_lock<std::mutex> lock(m);
    count--;
    if (count < 0) {
      cv.wait(lock, [&]{return wake_ups > 0;});
      wake_ups--;
    }
  }

  void signal() {
    unique_lock<std::mutex> lock(m);
    count++;
    if (count <= 0) {
      wake_ups++;
    }
    lock.unlock();
    cv.notify_one();
  }
};



std::mutex mu1;
std::mutex mu2;
std::condition_variable cv1;
std::condition_variable cv2;

class Fctor {
    public:
    void operator()() {}
};

void thread_functions() {
    std::thread t1(thread_functions); // create a thread
    t1.join(); // block on t1 completion
    t1.detach(); // convert to daemon thread, no parent
    t1.joinable(); // returns true if joinable
    t1.get_id(); // get id
    std::this_thread::yield();


    std::thread::hardware_concurrency(); // get number of cores

    std::thread t1_2 = std::move(t1);

    Fctor fct;
    std::thread t2(fct);
    std::thread t3((Fctor())); // extra brackets needed

    std::thread t4([](){
        cout <<"hello" << endl;
    });

    int res;
    std::thread t5(factorial1, 5, std::ref(res));

    mu1.lock(); // not recommended, use lock guard instead
    mu1.unlock(); // not recommended

    lock_guard<std::mutex> lock1(mu1); // ensures lock is released in destructor

    unique_lock<std::mutex> lock2(mu1); // lock_guard + more
    lock2.unlock(); 
    lock2.lock(); 
    unique_lock<std::mutex> lock3(mu1, std::defer_lock); // Don't lock for nowm will lock
    unique_lock<std::mutex> lock4 = std::move(lock3); // unique_lock can be moved

    // Deadlock avoidance
    std::lock(mu1, mu2);
    std::lock_guard<std::mutex> lg1(mu1, std::adopt_lock); // mutex is already locked, just take ownership to handle destructor stuff
    std::lock_guard<std::mutex> lg2(mu2, std::adopt_lock); // mutex is already locked, just take ownership to handle destructor stuff

    std::once_flag _flag;
    std::call_once(_flag, [&](){ cout <<"Called only once\n";}); // Will be called only once and by one thread

    // Futures
    std::future<int> fu1 = std::async(std::launch::async | std::launch::deferred, factorial, 4);
    // std::launch::async --> force create a thread
    // std::launch::deferred --> do not create a thread
    // both --> let implementation decide
    // fu1.get()

    // Promises
    std::promise<int> p1;
    std::future<int> p_f = p1.get_future();
    p1.set_value(3);
    p1.set_exception(std::make_exception_ptr(std::runtime_error("To err is human")));

    // Shared future
    std::shared_future<int> sf = p_f.share(); // convert future to shareable --> get() can be called multiple times, used for broadcasts

    // std::bind
    factorial(3);
    auto f_3 = std::bind(factorial, 3);

    {
        A a;
        std::thread t1(a, 6); // copy_of_a() in a different thread
        std::thread t2(std::ref(a), 6); // a() in a different thread
        std::thread t3(std::move(a), 6); //  a() in a different thread, a is no longer usable
        std::thread t4(A(), 6); // temp A
        std::thread t5(&A::f, a, 8, 'w'); // copy_of_a.f(8, 'w')
        std::thread t6(&A::f, &a, 8, 'w'); // a.f(8, 'w')
    }

    // Time stuff
    std::this_thread::sleep_for(chrono::seconds(1)); // sleep for 1 second
    chrono::steady_clock::time_point tp = chrono::steady_clock::now() + chrono::microseconds(4);
    auto tp2 = chrono::steady_clock::now() + chrono::microseconds(4);
    std::this_thread::sleep_until(tp);
    std::this_thread::sleep_until(tp2);
    lock4.try_lock(); 
    // lock4.try_lock_for(chrono::seconds(1));
    // lock4.try_lock_until(tp);
    // fu1.wait_for(chrono::seconds(1))
    // fu1.wait_until(tp)
    // fu1.wait();


    atomic<unsigned> counter123 = {0};
    atomic<bool> counter_bool= {0};
}
