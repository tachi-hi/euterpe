
#pragma once

#include <atomic>
#include <complex>
#include <iostream>
#include <memory>
#include <vector>
#include "fft.hpp"
#include "SlideBlock.hpp"
#include "scalar.h"

class HPSS_Idiv{
public:
	HPSS_Idiv(int a, int b, int c, Scalar d, Scalar e);
	~HPSS_Idiv();

	void push_new_data(Scalar *x){
		if(count_flag == false){
			push_pop_count++;
		}

		for(int i = 0 ; i < frame_length; i++){
			signal[i] = x[i] * sin(M_PI * i/ frame_length);
		}

		FFT::forward(signal.data(), fft_ed.data(), frame_length);

		for(int i = 0; i < frame_length/2; i++){
			amplitude[i] = abs(fft_ed[i]);
			phase[i]     = arg(fft_ed[i]);
			mask_def[i]  = Scalar(0.5);
		}

		W         -> push(amplitude.data());
		maskH     -> push(mask_def.data());
		maskP     -> push(mask_def.data());
		phaseSpec -> push(phase.data());

		for(int i = 0; i < frame_length/2; i++){
			amplitude[i] = abs(fft_ed[i])/2;
		}

		H->push(amplitude.data());
		P->push(amplitude.data());
	}

	void update(int n){
		const int KKK = 10;
		for(int iter = 0; iter < n; iter++){
			this -> exec_count++;
			this -> exec_count_total++;
			for(int t = 1; t < block_size-1; t++){
				for(int k = 1; k < KKK; k++){

					(*H)[t][k] = 0;
					(*P)[t][k] = (*W)[t][k];// * sqrt(3 + (Scalar)KKK/(Scalar)k); // enhance bass? -> x
				}
			}
			for(int t = 1; t < block_size-1; t++){
				for(int k = KKK; k < frame_length/2-1; k++){

					auto w_h_tmp = w_h;// * k; // proportional to freq? -> x
					auto w_p_tmp = w_p;// / sqrt(k); // proportional to freq? -> x

					auto H_A = Scalar(2) * w_h_tmp + Scalar(2);
					auto P_A = Scalar(2) * w_p_tmp + Scalar(2);
					auto H_B = w_h_tmp * ((*H)[t+1][k] + (*H)[t-1][k]);
					auto P_B = w_p_tmp * ((*P)[t][k+1] + (*P)[t][k-1]);
					auto H_C = Scalar(2) * (*maskH)[t][k] * (*W)[t][k] * (*W)[t][k];
					auto P_C = Scalar(2) * (*maskP)[t][k] * (*W)[t][k] * (*W)[t][k];
					(*H)[t][k] = (H_B + sqrt(H_B*H_B + Scalar(4) * H_A * H_C))/(Scalar(2)*H_A);
					(*P)[t][k] = (P_B + sqrt(P_B*P_B + Scalar(4) * P_A * P_C))/(Scalar(2)*P_A);

					auto tmp = (*H)[t][k] * (*H)[t][k] + (*P)[t][k] * (*P)[t][k];
					if(tmp > Scalar(0)){
						(*maskH)[t][k] = (*H)[t][k] * (*H)[t][k] /tmp;
						(*maskP)[t][k] = (*P)[t][k] * (*P)[t][k] /tmp;
					}else{
						(*maskH)[t][k] = (*maskP)[t][k] = Scalar(0.5);
					}
				}
			}
		}
	}
	//pop
	void pop(Scalar *Hout, Scalar *Pout){
		push_pop_count--;
		H->pop(&(Hout[0]));
		P->pop(&(Pout[0]));
		maskH->pop(&(Hout[0]));
		maskP->pop(&(Pout[0]));

		W->pop(amplitude.data());
		phaseSpec->pop(phase.data());

		for(int j = 0; j < frame_length/2; j++){
			if(mask == binary){
				Hout[j] = Hout[j] > Pout[j] ? Scalar(1) : Scalar(0);
				Pout[j] = Scalar(1) - Hout[j];
				Hout[j] = (amplitude[j]*(Hout[j]));
				Pout[j] = (amplitude[j]*(Pout[j]));
			}else if(mask == wiener){
				Hout[j] = (amplitude[j]*sqrt(Hout[j]));
				Pout[j] = (amplitude[j]*sqrt(Pout[j]));
			}
		}
		for(int j = frame_length/2+1; j < frame_length; j++){
			Hout[j] = Hout[frame_length - j];
			Pout[j] = Pout[frame_length - j];
		}

		for(int j = 0; j < frame_length/2; j++){
			auto isou = std::complex<Scalar>(cos(phase[j]),  sin(phase[j]));
			auto conj = std::complex<Scalar>(cos(phase[j]), -sin(phase[j]));
			Hcompspec[j] = Hout[j] * isou;
			Pcompspec[j] = Pout[j] * isou;
			if(j == 0) continue;
			Hcompspec[frame_length - j] = Hout[j] * conj;
			Pcompspec[frame_length - j] = Pout[j] * conj;
		}

		FFT::inverse(Hcompspec.data(), Hout, frame_length);
		FFT::inverse(Pcompspec.data(), Pout, frame_length);
		for(int i = 0; i < frame_length; i++){
			Hout[i] *= sin(M_PI * i / frame_length);
			Pout[i] *= sin(M_PI * i / frame_length);
		}
	}
	bool eof(){return push_pop_count < 0 ? true : false;}
	void flag_on(){count_flag = true;}
	bool filled(){return push_pop_count >= block_size ? true : false;}
	int get_exec_count_diff(){
		int tmp = exec_count.exchange(0);  // atomic swap
		second++;
		return tmp;
	}

	int get_exec_count_ave(){
		int tmp = exec_count_total.load() / second;
		return tmp;
	}

private:
	std::atomic<int> exec_count{0};
	std::atomic<int> exec_count_total{0};
	int second;
	int block_size;
	int frame_length;
	int frame_shift;
	int push_pop_count;
	bool count_flag;
	Scalar w_h;
	Scalar w_p;
	std::unique_ptr<SlideBlock<Scalar>> W, H, H_buf, P, P_buf;
	std::unique_ptr<SlideBlock<Scalar>> maskH, maskH_buf, maskP, maskP_buf;
	std::unique_ptr<SlideBlock<Scalar>> phaseSpec;
	std::vector<Scalar> signal, amplitude, phase, mask_def;
	std::vector<std::complex<Scalar>> fft_ed, Hcompspec, Pcompspec;
	enum{binary, wiener} mask;
};

HPSS_Idiv::HPSS_Idiv(int a, int b, int c, Scalar d, Scalar e)
	: exec_count{0}, exec_count_total{0}, second(0),
	  block_size(a), frame_length(b), frame_shift(c),
	  push_pop_count(0), count_flag(false),
	  w_h(d), w_p(e),
	  W        (std::make_unique<SlideBlock<Scalar>>(a, b/2)),
	  H        (std::make_unique<SlideBlock<Scalar>>(a, b/2)),
	  H_buf    (std::make_unique<SlideBlock<Scalar>>(a, b/2)),
	  P        (std::make_unique<SlideBlock<Scalar>>(a, b/2)),
	  P_buf    (std::make_unique<SlideBlock<Scalar>>(a, b/2)),
	  maskH    (std::make_unique<SlideBlock<Scalar>>(a, b/2)),
	  maskH_buf(std::make_unique<SlideBlock<Scalar>>(a, b/2)),
	  maskP    (std::make_unique<SlideBlock<Scalar>>(a, b/2)),
	  maskP_buf(std::make_unique<SlideBlock<Scalar>>(a, b/2)),
	  phaseSpec(std::make_unique<SlideBlock<Scalar>>(a, b/2)),
	  signal   (b), amplitude(b), phase(b), mask_def(b),
	  fft_ed   (b), Hcompspec(b), Pcompspec(b),
	  mask(binary)
{}

HPSS_Idiv::~HPSS_Idiv() = default;
