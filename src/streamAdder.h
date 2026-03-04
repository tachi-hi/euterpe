/***************************************************
 * streamAdder
 *
 * (c) 2010 Aug. Hideyuki Tachibana
 * tachibana@hil.t.u-tokyo.ac.jp
 **************************************************/

#pragma once

#include "streamBuffer.h"
#include <vector>
#include <thread>
#include "debug.h"
#include "guiBase.h"

// クロスプラットフォームのスレッド優先度設定（スレッド自身から呼び出す）
#if defined(_WIN32)
#  include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#  include <pthread.h>
#endif

inline void set_audio_thread_priority_self() {
#if defined(_WIN32)
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#elif defined(__APPLE__)
    // macOS: QoS クラスを USER_INTERACTIVE に設定（root 権限不要）
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#elif defined(__linux__)
    // SCHED_RR には CAP_SYS_NICE が必要。失敗しても続行する。
    sched_param sp{};
    sp.sched_priority = sched_get_priority_max(SCHED_RR) / 2;
    pthread_setschedparam(pthread_self(), SCHED_RR, &sp);
#endif
}


class StreamAdder{
 public:
  StreamAdder(int n, GUIBase *panel_)
    : cycle{n}, buffer_read(n), buffer_added(n), panel{panel_} {}

  StreamAdder(const StreamAdder&) = delete;
  StreamAdder& operator=(const StreamAdder&) = delete;

  void add_input_stream(StreamBuffer<float>* tmp){
    streams.push_back(tmp);
    multiply.push_back(1.0);
  }

  void set_multiply(int n, double value){
    multiply.at(n) = value;
  }

  void set_output_stream(StreamBuffer<float>* tmp){
    output = tmp;
  }

  void start(void){
    thread_ = std::jthread([this](std::stop_token st){ callback_(st); });
  }

 private:
  void callback_(std::stop_token st){
    set_audio_thread_priority_self();
    while(!st.stop_requested()){
      double tmp = (double)(panel->volume.get())/200.0;
      multiply[0] = tmp < 0.5 ? 1 + 0.5 - 1.0 * tmp : 4 * (1.0 - tmp) * (1.0 - tmp);
      multiply[1] = tmp < 0.5 ? 1 + 3.0 - 2.0 * tmp : 4 * (1.0 - tmp) * (1.0 - tmp); // harmonic louder
      multiply[2] = tmp < 0.5 ? 4 * tmp * tmp : 1 + 4 * (tmp - 0.5);

      std::fill(buffer_added.begin(), buffer_added.end(), 0.0f);

      for(int i = 0; i < (int)streams.size(); i++){
        // データが来るまで CV でブロック。100ms ごとに stop フラグを確認。
        while(!streams[i]->wait_and_read(buffer_read.data(), cycle)){
          if(st.stop_requested()) return;
        }
        const float m = static_cast<float>(multiply[i]);
        std::transform(buffer_added.begin(), buffer_added.end(),
          buffer_read.begin(), buffer_added.begin(),
          [m](float a, float r){ return a + m * r; });
      }
      output->push_data(buffer_added.data(), cycle);
    }
  }

  std::vector<StreamBuffer<float>*> streams;
  std::vector<float> multiply;
  StreamBuffer<float> *output{nullptr};
  int cycle;
  std::vector<float> buffer_read;
  std::vector<float> buffer_added;
  GUIBase *panel;

  std::jthread thread_;
};
