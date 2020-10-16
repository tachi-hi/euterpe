/*******************************************************************/
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp
/*******************************************************************/


#pragma once

#include <iostream>
#include <vector>
#include <cstdlib>
#include <pthread.h>
#include <limits.h>

#include"debug.h"

template<typename T>
class StreamBuffer{
 public:
  StreamBuffer();
  virtual ~StreamBuffer();

  void push_data(const T* x, int size);
  bool read_data(T* y, int size);
  void rewind_stream_a_little(int size);
  int size(void){return current_write_index - current_read_index;};
  bool has_not_received_any_data(void){return flag_not_received;}

 private:
  std::vector<T> data_stream;

  long current_read_index;
  long current_write_index;
  void overrun_check();

  bool overrun_flag;
  bool flag_not_received;

  // Mutex
  pthread_mutex_t mutex;
  void LOCK(){pthread_mutex_lock(&mutex);};
  void UNLOCK(){pthread_mutex_unlock(&mutex);};

  int MAX_SIZE;
};

template<typename T>
StreamBuffer<T>::StreamBuffer(){
	MAX_SIZE = 16000 * 10;  //10 seconds
  current_read_index	= 0;
  current_write_index = 0;
  overrun_flag = false;
  flag_not_received = true;
  data_stream.assign(MAX_SIZE, 0.0);
  pthread_mutex_init(&mutex, NULL);
}


template<typename T>
StreamBuffer<T>::~StreamBuffer(){
}


template<typename T>
void StreamBuffer<T>::push_data(const T *x, int length){
  LOCK();
  flag_not_received = false;
    for(int i = 0; i < length; i++){
      data_stream[(current_write_index + i) % MAX_SIZE] = x[i];
    }
    current_write_index += length;
  UNLOCK();
}


template<typename T>
bool StreamBuffer<T>::read_data(T *y, int length){
  LOCK();
  overrun_check();
  bool flag_ret=false;

  if(size() >= length){
    flag_ret = true;
    for(int i = 0; i < length; i++){
      y[i] = data_stream[(current_read_index + i) % MAX_SIZE];
    }
    current_read_index += length;
  }else{
    flag_ret = false;
	}
  UNLOCK();
  return flag_ret;
}


template<typename T>
void StreamBuffer<T>::rewind_stream_a_little(int length){
  LOCK();
  current_read_index -= length;
  if(current_read_index < 0){
    DBGMSG("rewinding error!");
    exit(1);
  }
  UNLOCK();
}


template<typename T>
inline void StreamBuffer<T>::overrun_check(void){
  overrun_flag = (size() > MAX_SIZE);
}
