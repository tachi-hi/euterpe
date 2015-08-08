// HPSSを継承してパイプとして使えるようにするクラス。
// 結局これは必要ない可能性

#pragma once

#include "streamBuffer.h"
#include "HPSS_Idiv.hpp"
#include <unistd.h> //usleep
#include <pthread.h>
#include "myMutex.h"
#include <iostream>

using namespace std;

class Filter{
 public:
	Filter(int length, int shift){
		this->length = length;
		this->shift = shift;

		local_buffer_float = new float[length];
		local_buffer_float_old = new float[length];
		local_buffer_double = new double[length];
		local_buffer_double_spec = new std::complex<double>[length / 2 + 1];
		window = new double[length];
		for(int i = 0; i < length; i++){
			local_buffer_float_old[i] = 0.;
			window[i] = sqrt(0.5 - 0.5 * cos(2. * M_PI * i / length));
		}
		forward = fftw_plan_dft_r2c_1d(length, local_buffer_double, reinterpret_cast<fftw_complex*>(local_buffer_double_spec), FFTW_ESTIMATE);
		inverse = fftw_plan_dft_c2r_1d(length, reinterpret_cast<fftw_complex*>(local_buffer_double_spec), local_buffer_double, FFTW_ESTIMATE);

	}

	virtual ~Filter(){
//		delete spec;
		delete[] local_buffer_float;
		delete[] local_buffer_float_old;
		delete[] local_buffer_double;
		delete[] local_buffer_double_spec;
		delete[] window;
	}

	void set_filter_a(double a){this->a = a;}
	void set_filter_b(double b){this->b = b;}

	void setBuffer(StreamBuffer<float> *in, StreamBuffer<float> *out){
		inBuffer = in;
		outBuffer = out;
	}

	void start(void){
		pthread_attr_init(&attr);
		pthread_attr_setschedpolicy(&attr,   SCHED_RR);
	  pthread_create(&thread, &attr, (void*(*)(void*))callback, this);
	}

	static void *callback(void* arg){reinterpret_cast<Filter*>(arg)->callback_();return 0;}

	void callback_(void){
		while(1){
			while(!inBuffer->read_data(local_buffer_float, length)){
				usleep(1000);
			}
			inBuffer->rewind_stream_a_little(shift);

			for(int i = 0; i < length; i++){
				local_buffer_double[i] = local_buffer_float[i];
			}

			filtering();
	 
			for(int i = 0; i < shift; i++){
				local_buffer_float[i] = local_buffer_double[i] + local_buffer_float_old[i];
			}
			for(int i = shift; i < length; i++){
				local_buffer_float_old[i - shift] = local_buffer_float[i];
			}
			outBuffer->push_data(local_buffer_float, shift);
		}
	}
	// ---------------------------------------------
	void filtering(void){
		for(int i = 0; i < length; i++){
			local_buffer_double[i] *= window[i];
		}

		fftw_execute( forward );
		double resolution = 16000. / length; // 16000はサンプリングレート
		for(int i = 0; i < length /2 + 1; i++){
			local_buffer_double_spec[i] *= 1.2 / (1 + abs(resolution * i - a) +  abs(resolution * i - b));
		}
		fftw_execute( inverse );

		for(int i = 0; i < length; i++){
			local_buffer_double[i] *= window[i] / length;
		}
	}
	// ---------------------------------------------

 private:
	int shift;
	int length;

	StreamBuffer<float> *inBuffer;
	StreamBuffer<float> *outBuffer;

	double *local_buffer_double;
	double *window;
	std::complex<double> *local_buffer_double_spec;
	float *local_buffer_float;
	float *local_buffer_float_old;

	fftw_plan forward, inverse;

	double a, b;

	pthread_t thread;
	pthread_attr_t attr;
};


