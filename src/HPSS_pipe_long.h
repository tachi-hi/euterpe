#pragma once

#include "streamBuffer.h"
#include "HPSS_Idiv.hpp"
#include <unistd.h> //usleep
#include <pthread.h>
#include "myMutex.h"
#include "HPSS_pipe.h"

class HPSS_pipe_long : public HPSS_pipe{
 public:
	HPSS_pipe_long(int a, int length, int shift, double d, double e)
	:		HPSS_pipe(a,length,shift,d,e)
	{
		H_buffer_double_spec = new std::complex<double>[length / 2 + 1];
		P_buffer_double_spec = new std::complex<double>[length / 2 + 1];
		window = new double[length];
		idou = new double[length];
		for(int i = 0; i < length; i++){
			window[i] = sqrt(0.5 - 0.5 * cos(2. * M_PI * i / length));
		}

		forward = fftw_plan_dft_r2c_1d(length, H_buffer_double, reinterpret_cast<fftw_complex*>(H_buffer_double_spec), FFTW_ESTIMATE);
		inverse = fftw_plan_dft_c2r_1d(length, reinterpret_cast<fftw_complex*>(H_buffer_double_spec), H_buffer_double, FFTW_ESTIMATE);
		forward_p = fftw_plan_dft_r2c_1d(length, P_buffer_double, reinterpret_cast<fftw_complex*>(P_buffer_double_spec), FFTW_ESTIMATE);
		inverse_p = fftw_plan_dft_c2r_1d(length, reinterpret_cast<fftw_complex*>(P_buffer_double_spec), P_buffer_double, FFTW_ESTIMATE);

	}
	virtual ~HPSS_pipe_long(){
		delete[] H_buffer_double_spec;
		delete[] P_buffer_double_spec;
		delete[] window;
		delete[] idou;
	}


	void update_callback_(void){
		if(loop_flag){
			while(1){
				MUTEX.lock();
				this->update(1);
				MUTEX.unlock();
			}
		}
	}

	static void *callback(void* arg){reinterpret_cast<HPSS_pipe_long*>(arg)->callback_();return 0;}

	void callback_(void){
		while(1){
			// ここを出力側の都合に合うように書き換える？
				// 待つのではなく
			while(!inBuffer->read_data(local_buffer_float, length)){
				usleep(1000); //1[ms]
			}

			inBuffer->rewind_stream_a_little(shift);

			for(int i = 0; i < length; i++){
				local_buffer_double[i] = static_cast<double>(local_buffer_float[i]);
			}

			this->push_new_data(local_buffer_double);

			if(!loop_flag){
				this->update(5);
			}

			if(!this->filled()){
				continue;
			}

//			MUTEX.lock();
			this->pop(H_buffer_double, P_buffer_double);
//			MUTEX.unlock();

//			filtering_H();
			filtering_H();
//			filtering_P();


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

	void set_filter_a(double a){this->a = a;}

	// ---------------------------------------------

	void filtering_H(void){
		fftw_execute( forward );
		double resolution = 16000. / length; // 16000はサンプリングレート

		for(int i = 0; i < length /2 + 1; i++){
			H_buffer_double_spec[i] *= // 1.5 / (1 + abs(resolution * i - a) );
			  abs(resolution * i - a) < 100 ? 1.5
			: abs(resolution * i - a) < 200 ? 1.2
			: abs(resolution * i - a) < 300 ? 1.1
			: 1.;
		}

		fftw_execute( inverse );

		for(int i = 0; i < length; i++){
			H_buffer_double[i] *= 1. / length;
		}
	}

	// ---------------------------------------------

	// ---------------------------------------------
#if 0
	void filtering_P(void){ このやり方では全然だめ
		const int LOW = 100;
		const int N = 5;
		int pitch;
		double max = 0;
		fftw_execute( forward_p );
/*
		for(int i = 0; i < length; i++){
			idou[i] = 0;
		}
		for(int i = LOW; i < length - N ; i++){
			for(int j = -N; j < N; j++){
				idou[i] = abs(P_buffer_double_spec[i + j]) / 2 / N;
			}
		}
*/
		for(int i = LOW; i < 3 * LOW ; i++){
			if(max < abs(P_buffer_double_spec[i]) + abs(P_buffer_double_spec[2 * i]) + abs(P_buffer_double_spec[i * 3])){
				max = abs(P_buffer_double_spec[i]) + abs(P_buffer_double_spec[2 * i]) + abs(P_buffer_double_spec[i * 3]);
				pitch = i;
			}
		}

/*
		for(int i = 0; i < LOW; i++){
			P_buffer_double_spec[i] = 0.;
		}
		for(int i = 5; i < length - N; i++){
			double tmp = abs(P_buffer_double_spec[i]);
			P_buffer_double_spec[i] *= tmp > idou[i] + 0.1 ?
				LOW / sqrt((double)(i)) / tmp :
				0.;
		}
*/

		for(int i = 1; i < length ; i++){
			double tmp = abs(P_buffer_double_spec[i]);
			P_buffer_double_spec[i] *= i % pitch == 0 ?
				LOW / sqrt((double)(i)) / tmp :
				0.;
		}

		fftw_execute( inverse_p );

		for(int i = 0; i < length; i++){
			P_buffer_double[i] *= 1. / length;
		}
	}
#endif
	// ---------------------------------------------

 private:
	double *window;
	double *idou;
	std::complex<double> *H_buffer_double_spec;
	std::complex<double> *P_buffer_double_spec;

	fftw_plan forward, inverse;
	fftw_plan forward_p, inverse_p;

//	double *H_buffer_double_spec;
//	double *P_buffer_double_spec;

	double a;

};
