#pragma once

#include "streamBuffer.h"
#include "HPSS_Idiv.hpp"
#include "fftw_traits.h"
#include "scalar.h"
#include <pthread.h>
#include <complex>
#include <algorithm>
#include <vector>
#include "HPSS_pipe.h"

class HPSS_pipe_long : public HPSS_pipe{
 public:
	HPSS_pipe_long(int a, int length, int shift, Scalar d, Scalar e)
	:		HPSS_pipe(a, length, shift, d, e),
		H_buffer_double_spec(length / 2 + 1),
		P_buffer_double_spec(length / 2 + 1),
		window(length),
		idou(length)
	{
		int i = 0;
		std::generate(window.begin(), window.end(),
			[&]{ return std::sqrt(Scalar(0.5) - Scalar(0.5) * std::cos(Scalar(2.0 * M_PI) * i++ / length)); });

		forward   = Fftw::plan_dft_r2c_1d(length, H_buffer_double.data(), reinterpret_cast<Fftw::complex*>(H_buffer_double_spec.data()), FFTW_ESTIMATE);
		inverse   = Fftw::plan_dft_c2r_1d(length, reinterpret_cast<Fftw::complex*>(H_buffer_double_spec.data()), H_buffer_double.data(), FFTW_ESTIMATE);
		forward_p = Fftw::plan_dft_r2c_1d(length, P_buffer_double.data(), reinterpret_cast<Fftw::complex*>(P_buffer_double_spec.data()), FFTW_ESTIMATE);
		inverse_p = Fftw::plan_dft_c2r_1d(length, reinterpret_cast<Fftw::complex*>(P_buffer_double_spec.data()), P_buffer_double.data(), FFTW_ESTIMATE);
	}

	virtual ~HPSS_pipe_long(){
		Fftw::destroy(forward);
		Fftw::destroy(inverse);
		Fftw::destroy(forward_p);
		Fftw::destroy(inverse_p);
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

//			MUTEX.lock();
			this->pop(H_buffer_double.data(), P_buffer_double.data());
//			MUTEX.unlock();

//			filtering_H();
			filtering_H();
//			filtering_P();

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

	void set_filter_a(Scalar a){this->a = a;}

	// ---------------------------------------------

	void filtering_H(void){
		Fftw::execute( forward );
		auto resolution = Scalar(16000) / length; // 16000 samp rate

		for(int i = 0; i < length /2 + 1; i++){
			H_buffer_double_spec[i] *=
			  std::abs(resolution * i - a) < Scalar(100) ? Scalar(1.5)
			: std::abs(resolution * i - a) < Scalar(200) ? Scalar(1.2)
			: std::abs(resolution * i - a) < Scalar(300) ? Scalar(1.1)
			: Scalar(1);
		}

		Fftw::execute( inverse );

		const auto inv_len = Scalar(1) / length;
		std::for_each(H_buffer_double.begin(), H_buffer_double.end(),
			[inv_len](Scalar& v){ v *= inv_len; });
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

		for(int i = LOW; i < 3 * LOW ; i++){
			if(max < abs(P_buffer_double_spec[i]) + abs(P_buffer_double_spec[2 * i]) + abs(P_buffer_double_spec[i * 3])){
				max = abs(P_buffer_double_spec[i]) + abs(P_buffer_double_spec[2 * i]) + abs(P_buffer_double_spec[i * 3]);
				pitch = i;
			}
		}

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
	std::vector<std::complex<Scalar>> H_buffer_double_spec;
	std::vector<std::complex<Scalar>> P_buffer_double_spec;
	std::vector<Scalar> window;
	std::vector<Scalar> idou;

	Fftw::plan forward{nullptr}, inverse{nullptr};
	Fftw::plan forward_p{nullptr}, inverse_p{nullptr};

	Scalar a;

};
