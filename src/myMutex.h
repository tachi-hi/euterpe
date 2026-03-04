// Mutex

#pragma once

#include <mutex>

template<typename T>
class myMutex{
 public:
  myMutex() : value{} {}
  explicit myMutex(T n) : value{n} {}

  myMutex(const myMutex&) = delete;
  myMutex& operator=(const myMutex&) = delete;

  void lock()  { mutex_.lock();   }
  void unlock(){ mutex_.unlock(); }

  T get(){
    std::scoped_lock lk(mutex_);
    return value;
  }
  void set(T n){
    std::scoped_lock lk(mutex_);
    value = n;
  }

 private:
  T value;
  std::mutex mutex_;
};
