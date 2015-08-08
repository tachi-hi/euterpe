/*******************************************************************/
// 入力をしばらくの間保持しておくための機構 
// Euterpeをリアルタイムで処理させるため。
//
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
  
  void push_data(const T* x, int size); // データをブロック単位で書き込み
  bool read_data(T* y, int size); // データをブロック単位で読み込み
  void rewind_stream_a_little(int size); //巻き戻したいサンプル数を指定すると、読み取りポイントを巻き戻す。
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
	MAX_SIZE = 16000 * 10;  //10秒くらいプールできるようにしておく。
  current_read_index	= 0;
  current_write_index = 0;
  overrun_flag = false;
  flag_not_received = true;
  data_stream.assign(MAX_SIZE, 0.0); 
  pthread_mutex_init(&mutex, NULL);
//  std::cerr << "streamBuffer initialized" << std::endl;
}


template<typename T>
StreamBuffer<T>::~StreamBuffer(){
}


// データを挿入する, ロックする。
template<typename T>
void StreamBuffer<T>::push_data(const T *x, int length){
  LOCK();
  flag_not_received = false;
//  overrun_check();
/*  if(overrun_flag){ //オーバーランしている場合
			//;
			//とりあえず同じように処理するが警告が出るように。
    for(int i = 0; i < length; i++){
      data_stream[(current_write_index + i) % MAX_SIZE] = x[i];
    }
    current_write_index += length;
  }else{ //オーバーランしてない場合
*/
    for(int i = 0; i < length; i++){
      data_stream[(current_write_index + i) % MAX_SIZE] = x[i];
    }
    current_write_index += length;
//  }
  UNLOCK();
}


// 読み取るための関数, ロックする。
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
/*
  }else if(overrun_flag == true){
    flag_ret = false;  //外でwhile(!read_data())してほしい。読むのに失敗し続ける限り読みつづける感じに。
    DBGMSG("buffer: failed");
  }
*/
  }else{
    flag_ret = false;  //外でwhile(!read_data())してほしい。読むのに失敗し続ける限り読みつづける感じに。
	}
  UNLOCK();
  return flag_ret;
}


// 巻き戻し関数, ロックする。
template<typename T>
void StreamBuffer<T>::rewind_stream_a_little(int length){
  LOCK();
  current_read_index -= length;
  if(current_read_index < 0){ 
    //		current_read_index += MAX_SIZE;//いみふ
    DBGMSG("rewinding error!");
    exit(1);
  }
  UNLOCK();
}


// リングバッファのバッファが一周分たまってしてしまったらフラグを立てる。ロックしない
template<typename T>
inline void StreamBuffer<T>::overrun_check(void){
  overrun_flag = (size() > MAX_SIZE);
/*
  
  if(current_read_index > LONG_MAX / 2){ //約6時間に相当？しない？314時間相当？
    std::cerr << "Error. The program is running for too long duration."<< std::endl;
    exit(-1);
  }
*/
}

