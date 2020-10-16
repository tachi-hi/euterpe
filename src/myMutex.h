// Mutex

#include <pthread.h>

#ifndef MY_MUTEX_H
#define MY_MUTEX_H

template<typename T>
class myMutex{
 public:
  myMutex(){
    pthread_mutex_init(&mutex, NULL);
    value = 0;
  }
  myMutex(T n){
    pthread_mutex_init(&mutex, NULL);
    value = n;
  }

  //
  void lock(){pthread_mutex_lock(&mutex);};
  void unlock(){pthread_mutex_unlock(&mutex);};

  // not needed
  T get(){
    pthread_mutex_lock(&mutex);
    T tmp = value;
    pthread_mutex_unlock(&mutex);
    return tmp;
  }
  void set(T n){
    pthread_mutex_lock(&mutex);
    value = n;
    pthread_mutex_unlock(&mutex);
  }

 private:
  T value;
  pthread_mutex_t mutex; // avoid locked by someone outside of the class
};

#endif
