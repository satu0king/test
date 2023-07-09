#include<iostream>
#include<thread>
#include<fstream>
#include<mutex>
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
3. Lock the mutex in same order

Locking granularity
 - fine-grained lock: protects small amount of data
 - coarse-grained lock: protects large amount of data
*/


std::mutex mu1;
std::mutex mu2;
std:condition_variable cv1;
std:condition_variable cv2;

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


void thread_functions() {
    std::thread t1(thread_functions); // create a thread
    t1.join(); // block on t1 completion
    t1.detach(); // convert to daemon thread, no parent
    t1.joinable(); // returns true if joinable
    t1.get_id(); // get id


    std::thread::hardware_concurrency(); // get number of cores

    std::thread t1_2 = std::move(t1);

    Fctor fct;
    std::thread t2(fct);
    std::thread t3((Fctor())); // extra brackets needed

    std::thread t4([](){
        cout <<"hello" << endl;
    });

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

    std::this_thread::sleep_for(chrono::seconds(1)); // sleep for 1 second
}

// Detaching a thread -> daemon thread
// t1.detach()
// Threads that are not joined or detached will throw exception when destructor is called
// if ()
