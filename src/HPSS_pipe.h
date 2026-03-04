// HPSSを継承してパイプとして使えるようにする

#pragma once

#include "streamBuffer.h"
#include "HPSS_Idiv.hpp"
#include "scalar.h"
#include <pthread.h>
#include <mutex>
#include <algorithm>
#include <vector>

#include<iostream>

class HPSS_pipe : public HPSS_Idiv{
 public:
 HPSS_pipe(int a, int length, int shift, Scalar d, Scalar e)
	 : HPSS_Idiv(a, length, shift, d, e),
	   shift{shift}, length{length},
	   local_buffer_float(length, 0.0f),
	   H_buffer_float(shift, 0.0f),
	   P_buffer_float(shift, 0.0f),
	   local_buffer_double(length, 0.0),
	   H_buffer_double(length, 0.0),
	   P_buffer_double(length, 0.0),
	   H_prev(shift, 0.0f),
	   P_prev(shift, 0.0f),
	   loop_flag{false}
	{
	  pthread_attr_init(&attr);
	  pthread_attr_setschedpolicy(&attr, SCHED_RR);
	}

	virtual ~HPSS_pipe() = default;

	void setBuffer(StreamBuffer<float> *in,
		 StreamBuffer<float> *H,
		 StreamBuffer<float> *P){
		inBuffer = in;
		outBuffer_H = H;
		outBuffer_P = P;
	}

	// 反復回数の上限を設定する。
	// n >= 1 : 1フレームあたりの最大反復回数（デフォルト 3）
	// n == -1: 次の入力フレームが届くまで無制限に反復（高品質モード）
	void setMaxIterations(int n) { max_outer_loops = n; }

	void start(void){
	  pthread_create(&update_thread, &attr, (void*(*)(void*))update_callback, this);
	  pthread_create(&thread, &attr, (void*(*)(void*))callback, this);
	}

	static void *update_callback(void* arg){reinterpret_cast<HPSS_pipe*>(arg)->update_callback_();return 0;}

	void update_callback_(void){
		if(loop_flag){
			while(1){
				MUTEX.lock();
				this->update(1);
				MUTEX.unlock();
			}
		}
	}

	static void *callback(void* arg){reinterpret_cast<HPSS_pipe*>(arg)->callback_();return 0;}

	virtual void callback_(void){
		while(1){
			while(!inBuffer->wait_and_read(local_buffer_float.data(), length))
				; // ブロッキング読み出し（100ms タイムアウト後リトライ）

			inBuffer->rewind_stream_a_little(shift);

			std::transform(local_buffer_float.begin(), local_buffer_float.end(),
				local_buffer_double.begin(),
				[](float v){ return static_cast<Scalar>(v); });

			this->push_new_data(local_buffer_double.data());

			if(!loop_flag){
				// 次フレームが届くまでの空き時間を追加反復に使う（品質向上）
				int count = 0;
				do {
					this->update(1);
					++count;
				} while(inBuffer->size() < length
				        && (max_outer_loops < 0 || count < max_outer_loops));
			}

			if(!this->filled()){
				continue;
			}

			MUTEX.lock();
			this->pop(H_buffer_double.data(), P_buffer_double.data());
			MUTEX.unlock();

			std::transform(H_buffer_double.begin(), H_buffer_double.begin() + shift,
				H_prev.begin(), H_buffer_float.begin(),
				[](Scalar h, float p){ return static_cast<float>(h) + p; });
			std::transform(P_buffer_double.begin(), P_buffer_double.begin() + shift,
				P_prev.begin(), P_buffer_float.begin(),
				[](Scalar p, float prev){ return static_cast<float>(p) + prev; });
			std::transform(H_buffer_double.begin() + shift, H_buffer_double.end(),
				H_prev.begin(), [](Scalar v){ return static_cast<float>(v); });
			std::transform(P_buffer_double.begin() + shift, P_buffer_double.end(),
				P_prev.begin(), [](Scalar v){ return static_cast<float>(v); });

			outBuffer_H->push_data(H_buffer_float.data(), shift);
			outBuffer_P->push_data(P_buffer_float.data(), shift);
	}
}

 protected:
	int shift;
	int length;

	StreamBuffer<float> *inBuffer;
	StreamBuffer<float> *outBuffer_H;
	StreamBuffer<float> *outBuffer_P;

	std::vector<float>  local_buffer_float;
	std::vector<float>  H_buffer_float;
	std::vector<float>  P_buffer_float;

	std::vector<Scalar> local_buffer_double;
	std::vector<Scalar> H_buffer_double;
	std::vector<Scalar> P_buffer_double;

	std::vector<float>  H_prev;
	std::vector<float>  P_prev;
	bool loop_flag;

	pthread_t update_thread;
	pthread_t thread;
	pthread_attr_t attr;
	std::mutex MUTEX;

	// デフォルト 3 回。-1 で次フレームまで無制限。
	int max_outer_loops{3};
};
