/*******************************************************************/
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp
/*******************************************************************/

#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>

#include "debug.h"

template<typename T>
class StreamBuffer{
 public:
  StreamBuffer();
  virtual ~StreamBuffer() = default;

  StreamBuffer(const StreamBuffer&) = delete;
  StreamBuffer& operator=(const StreamBuffer&) = delete;

  void push_data(const T* x, int length);
  bool read_data(T* y, int length);

  // ブロッキング版: データが length サンプル貯まるまで待つ。
  // timeout 以内にデータが来なければ false を返す（停止フラグのチェックに使う）。
  bool wait_and_read(T* y, int length,
                     std::chrono::milliseconds timeout = std::chrono::milliseconds(100));

  void rewind_stream_a_little(int length);
  int  size() const { return current_write_index - current_read_index; }
  bool has_not_received_any_data() const { return flag_not_received; }

 private:
  std::vector<T> data_stream;

  long current_read_index{0};
  long current_write_index{0};
  bool overrun_flag{false};
  bool flag_not_received{true};

  std::mutex               mutex_;
  std::condition_variable  cv_;   // push_data が notify → 待機スレッドが起床

  int MAX_SIZE;

  void overrun_check();
};


template<typename T>
StreamBuffer<T>::StreamBuffer(){
  MAX_SIZE = 16000 * 10;  // 10 seconds
  data_stream.assign(MAX_SIZE, T{});
}


// push: ロック解放後に notify（ロック保持中の notify は冗長なウェイクアップを招く）
template<typename T>
void StreamBuffer<T>::push_data(const T *x, int length){
  {
    std::scoped_lock lk(mutex_);
    flag_not_received = false;
    for(int i = 0; i < length; i++)
      data_stream[(current_write_index + i) % MAX_SIZE] = x[i];
    current_write_index += length;
  }
  cv_.notify_all();
}


// ノンブロッキング読み出し（既存コードとの互換用）
template<typename T>
bool StreamBuffer<T>::read_data(T *y, int length){
  std::scoped_lock lk(mutex_);
  overrun_check();
  if(size() < length) return false;
  for(int i = 0; i < length; i++)
    y[i] = data_stream[(current_read_index + i) % MAX_SIZE];
  current_read_index += length;
  return true;
}


// ブロッキング読み出し: データが来るまで CV で待機
// timeout 内にデータが揃わなければ false（呼び元で stop フラグを確認する）
template<typename T>
bool StreamBuffer<T>::wait_and_read(T *y, int length, std::chrono::milliseconds timeout){
  std::unique_lock lk(mutex_);
  bool ready = cv_.wait_for(lk, timeout, [&]{ return size() >= length; });
  if(!ready) return false;
  overrun_check();
  for(int i = 0; i < length; i++)
    y[i] = data_stream[(current_read_index + i) % MAX_SIZE];
  current_read_index += length;
  return true;
}


template<typename T>
void StreamBuffer<T>::rewind_stream_a_little(int length){
  std::scoped_lock lk(mutex_);
  current_read_index -= length;
  if(current_read_index < 0){
    DBGMSG("rewinding error!");
    exit(1);
  }
}


template<typename T>
inline void StreamBuffer<T>::overrun_check(){
  overrun_flag = (size() > MAX_SIZE);
}
