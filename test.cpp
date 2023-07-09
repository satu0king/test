#include<iostream>
#include<thread>
#include<fstream>
#include<mutex>
#include <condition_variable>
#include<future>
#include <functional>
#include<atomic>
using namespace std;

// g++ -pthread test.cpp && ./a.out 

/*

1. parameters to a thread is always passed by value, use std::ref() wrapper to pass by reference
2. a thread has to be either detached or joined
3. A thread object can only be moved and not copied
4. Oversubscription - more threads than number of cores, use std::thread::hardware_concurrency() to find best concurrency level
4. Mutex - mutual exclusion
5. Hold single mutex lock for reading data + modifying data as an atomic operation

Avoiding data race:
1. Use mutex to sync data access
2. Never leak a handle of data to outside (call a user provided function or returning the handle)
3. Design interface properly (stack - pop and top should be one operation)

Avoiding deadlock
1. Prefer using a single mutex
2. Avoid locking a mutex and then calling a user provided funciton
3. Use std::lock(m1, m2) to lock more than one mutex
4. Lock the mutex in same order

ways to get a future
1. promise::get_future()
2. packaged_task::get_future()
3. async() returns a future

Locking granularity
 - fine-grained lock: protects small amount of data
 - coarse-grained lock: protects large amount of data
*/


std::mutex mu1;
std::mutex mu2;
std::condition_variable cv1;
std::condition_variable cv2;

int capacity = 10;
int count = 0;
void producer() {
    while (true) {
        std::unique_lock<mutex> locker(mu1);
        while (count >= capacity) {
            cv1.wait(locker);
        }
        cout << "Producing: " << count << endl;
        count++;
        cv1.notify_one();
        locker.unlock();
        std::this_thread::sleep_for(chrono::seconds(1));
    }
}

void consumer() {
    while (true) {
        std::unique_lock<mutex> locker(mu1);
        cv1.wait(locker, []{return count > 0;});
        cout << "Consuming: " << count << endl;
        count--;
        cv1.notify_one();
        locker.unlock();
        std::this_thread::sleep_for(chrono::seconds(2));
    }
}

void producer_consumer_test() {
    std::thread t1(producer);
    std::thread t2(consumer);
    t1.join();
    t2.join();
}

void shared_print(string msg, int id) {
    std::lock_guard<std::mutex> guard(mu1);
    cout << msg << id << endl;
}

void fn1() {
    for(int i = 0; i < 100; i++) {
        shared_print("from fn1: ", i);
        std::this_thread::yield();
    }
}

class Fctor {
    public:
    void operator()() {
        for(int i = 0; i < 100; i++) {
            shared_print("from fctor: ", i);
            std::this_thread::yield();
        }
    }
};

void mutex_test() {
    std::thread t2((Fctor()));
    std::thread t1(fn1);
    t1.join();
    t2.join();
}


int main() {
    mutex_test();
    // producer_consumer_test();
}

class LogFile {
    std::mutex _mu;
    std::mutex _mu_open;
    std::once_flag _flag;
    ofstream _f;
public:
    LogFile() {
    }

    void print(string id, string value) {
        // if (!_f.is_open()) {
        //     std::unique_lock<mutex> locker(_mu_open);
        //     _f.open("log.txt");
        // }
        // Wrong solution ^

        std::call_once(_flag, [&](){ _f.open("log.txt");}); // File will be opned only ocne by one thread
        // Correct solution ^

        std::unique_lock<mutex> locker(_mu, std::defer_lock);
        _f << "From " << id <<": "<< value << endl;
    }


};

 int factorial(int n) {
    int x = 1;
    for(int i = 1; i <= n; i++)
        x * i;
    return x;
 }

 void factorial1(int n, int &x) {
    x = factorial(n);
    cout << x << endl;
 }

int factorial2(std::future<int> &f) {
    int x = 1;
    int n = f.get();
    for(int i = 1; i <= n; i++)
        x * i;
    return x;
 }

 void async_test () {
    int x;
    std::future<int> fu1 = std::async(factorial, 4);
    std::future<int> fu2 = std::async(std::launch::deferred, factorial, 4);  // Not called in a separate thread
    std::future<int> fu3 = std::async(std::launch::async, factorial, 4); // Start in a separate thread

    std::promise<int> p;
    std::future<int> f = p.get_future();
    std::future<int> f4 = std::async(std::launch::async, factorial2, std::ref(f));

    p.set_value(4);

    x = f4.get();
}

class A {
    public: 
    void f(int x, char c) {}
    long g(double x) {return 0;}
    int operator()(int N) {return 0;}
};

void packaged_task() {
    std::packaged_task<int(int)> t(factorial);


    t(6); // call in a different context
    int x = t.get_future().get(); 
}

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