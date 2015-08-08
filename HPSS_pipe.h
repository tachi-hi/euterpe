// HPSSを継承してパイプとして使えるようにする

#pragma once

#include "streamBuffer.h"
#include "HPSS_Idiv.hpp"
#include <unistd.h> //usleep
#include <pthread.h>
#include "myMutex.h"

#include<iostream>

class HPSS_pipe : public HPSS_Idiv{
 public:
 HPSS_pipe(int a, int length, int shift, double d, double e)
	 : HPSS_Idiv(a,length,shift,d,e){
		local_buffer_float = new float[length];
		H_buffer_float     = new float[shift];
		P_buffer_float     = new float[shift];
		local_buffer_double = new double[length];
		H_buffer_double     = new double[length];
		P_buffer_double     = new double[length];
		H_prev = new float[shift];
		P_prev = new float[shift];
		this->shift = shift;
		this->length = length;
	  pthread_attr_init(&attr);
	  pthread_attr_setschedpolicy(&attr, SCHED_RR);

//		loop_flag = true; // ここでiteration strategyが変えられる
		loop_flag = false;
	}
	virtual ~HPSS_pipe(){
		delete[] local_buffer_double;
		delete[] H_buffer_double;
		delete[] P_buffer_double;
		delete[] local_buffer_float;
		delete[] H_buffer_float;
		delete[] P_buffer_float;
	}

	void setBuffer(StreamBuffer<float> *in,
		 StreamBuffer<float> *H,
		 StreamBuffer<float> *P){
		inBuffer = in;
		outBuffer_H = H;
		outBuffer_P = P;
	}

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
			// ここを出力側の都合に合うように書き換える？
				// 待つのではなく
			while(!inBuffer->read_data(local_buffer_float, length)){
				usleep(1000); //1[ms]まつ
			}

			inBuffer->rewind_stream_a_little(shift);
	
			for(int i = 0; i < length; i++){
				local_buffer_double[i] = static_cast<double>(local_buffer_float[i]);
			}

			this->push_new_data(local_buffer_double);

			if(!loop_flag){
//				this->update(5); 
				this->update(1); 
			}

			if(!this->filled()){
				continue;
			}

			MUTEX.lock();
			this->pop(H_buffer_double, P_buffer_double); 
			MUTEX.unlock();
	 
			for(int i = 0; i < shift; i++){
				H_buffer_float[i] = static_cast<float>(H_buffer_double[i]) + H_prev[i];
				P_buffer_float[i] = static_cast<float>(P_buffer_double[i]) + P_prev[i];
			}
			for(int i = shift; i < length; i++){
				H_prev[i - shift] = static_cast<float>(H_buffer_double[i]);
				P_prev[i - shift] = static_cast<float>(P_buffer_double[i]);				
			}
			outBuffer_H->push_data(H_buffer_float, shift);
			outBuffer_P->push_data(P_buffer_float, shift);
	}
}

 protected:
	int shift;
	int length;

	StreamBuffer<float> *inBuffer;
	StreamBuffer<float> *outBuffer_H;
	StreamBuffer<float> *outBuffer_P;

	float *local_buffer_float;
	float *H_buffer_float;
	float *P_buffer_float;

	double *local_buffer_double;
	double *H_buffer_double;
	double *P_buffer_double;

	//前フレームの出力を覚えておくためのバッファ
	float *H_prev;
	float *P_prev;
	bool loop_flag;

	pthread_t update_thread;
	pthread_t thread;
	pthread_attr_t attr;
	myMutex<int> MUTEX;
};


