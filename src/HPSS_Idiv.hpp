
#pragma once

#include<complex>
#include<iostream>
#include"fft.hpp"
#include"SlideBlock.hpp"

typedef double FLOAT;
//typedef float FLOAT;


class HPSS_Idiv{
public:
	HPSS_Idiv(int a, int b, int c, FLOAT d, FLOAT e);
	~HPSS_Idiv();

	void push_new_data(FLOAT *x){
		if(count_flag == false){
			push_pop_count++;
		}

		for(int i = 0 ; i < frame_length; i++){
			signal[i] = x[i] * sin(M_PI * i/ frame_length);
		}

		FFT::forward(&(signal[0]), &(fft_ed[0]), frame_length);

		for(int i = 0; i < frame_length/2; i++){
			amplitude[i] = abs(fft_ed[i]);
			phase[i]     = arg(fft_ed[i]);
			mask_def[i]  = 0.5;
		}

		W         -> push(&(amplitude[0]));
		maskH     -> push(&(mask_def[0]));
		maskP     -> push(&(mask_def[0]));
		phaseSpec -> push(&(phase[0]));

		for(int i = 0; i < frame_length/2; i++){
			amplitude[i] = abs(fft_ed[i])/2;
		}

		H->push(&(amplitude[0]));
		P->push(&(amplitude[0]));
	}

	void update(int n){
		const int KKK = 10;
		for(int iter = 0; iter < n; iter++){
			this -> exec_count++;
			this -> exec_count_total++;
			for(int t = 1; t < block_size-1; t++){
				for(int k = 1; k < KKK; k++){

					(*H)[t][k] = 0;
					(*P)[t][k] = (*W)[t][k];// * sqrt(3 + (FLOAT)KKK/(FLOAT)k); // enhance bass? -> x
				}
			}
			for(int t = 1; t < block_size-1; t++){
				for(int k = KKK; k < frame_length/2-1; k++){
//*
					FLOAT w_h_tmp = w_h;// * k; // proportional to freq? -> x
					FLOAT w_p_tmp = w_p;// / sqrt(k); // proportional to freq? -> x

					FLOAT H_A = 2 * w_h_tmp + 2;
					FLOAT P_A = 2 * w_p_tmp + 2;
					FLOAT H_B = w_h_tmp * ((*H)[t+1][k] + (*H)[t-1][k]);
					FLOAT P_B = w_p_tmp * ((*P)[t][k+1] + (*P)[t][k-1]);
					FLOAT H_C = 2 * (*maskH)[t][k] * (*W)[t][k] * (*W)[t][k];
					FLOAT P_C = 2 * (*maskP)[t][k] * (*W)[t][k] * (*W)[t][k];
					(*H)[t][k] = (H_B + sqrt(H_B*H_B + 4 * H_A * H_C))/(2*H_A);
					(*P)[t][k] = (P_B + sqrt(P_B*P_B + 4 * P_A * P_C))/(2*P_A);
//*/
/*
					FLOAT tmp1 = sqrt((*H)[t-1][k]) + sqrt((*H)[t+1][k]);
					FLOAT tmp2 = sqrt((*P)[t][k-1]) + sqrt((*P)[t][k+1]);
					FLOAT bunbo = sqrt(tmp1 * tmp1 + tmp2 * tmp2 + 1e-100);
					(*H)[t][k] = tmp1/bunbo * (*W)[t][k];
					(*P)[t][k] = tmp2/bunbo * (*W)[t][k];
//*/
					FLOAT tmp = (*H)[t][k] * (*H)[t][k] + (*P)[t][k] * (*P)[t][k];
					if(tmp > 0.0){
						(*maskH)[t][k] = (*H)[t][k] * (*H)[t][k] /tmp;
						(*maskP)[t][k] = (*P)[t][k] * (*P)[t][k] /tmp;
					}else{
						(*maskH)[t][k] = (*maskP)[t][k] = 0.5;
					}
				}
			}
		}
	}
	//pop
	void pop(FLOAT *Hout, FLOAT *Pout){
		push_pop_count--;
		H->pop(&(Hout[0]));
		P->pop(&(Pout[0]));
		maskH->pop(&(Hout[0]));
		maskP->pop(&(Pout[0]));

		W->pop(&(amplitude[0]));
		phaseSpec->pop(&(phase[0]));

		for(int j = 0; j < frame_length/2; j++){
/*
			int tmp1, tmp2, tmp3, tmp4, tmp5;
			tmp1 = tmp2 = tmp3 = tmp4 = tmp5 = 0;
			if( j != 0 && j != 1){
				tmp1 	= Hout[j - 2] > Pout[j - 2] ? 1 : 0;
				tmp2 	= Hout[j - 1] > Pout[j - 1] ? 1 : 0;
				tmp3 	= Hout[j + 0] > Pout[j + 0] ? 1 : 0;
				tmp4 	= Hout[j + 1] > Pout[j + 1] ? 1 : 0;
				tmp5 	= Hout[j + 2] > Pout[j + 2] ? 1 : 0;
			}
			FLOAT a = tmp1 + tmp2 + tmp3 + tmp4 + tmp5 > 2 ? 1 : 0;
			Hout[j] = (amplitude[j]*a); //
			Pout[j] = (amplitude[j]*(1 - a));
*/
			if(mask == binary){
				Hout[j] = Hout[j] > Pout[j] ? 1 : 0;
				Pout[j] = 1 - Hout[j];
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
			std::complex<FLOAT> isou(cos(phase[j]), sin(phase[j]));
			std::complex<FLOAT> conj(cos(phase[j]),-sin(phase[j]));
			Hcompspec[j] = Hout[j] * isou;
			Pcompspec[j] = Pout[j] * isou;
			if(j == 0) continue;
			Hcompspec[frame_length - j] = Hout[j] * conj;
			Pcompspec[frame_length - j] = Pout[j] * conj;
		}

		FFT::inverse(&(Hcompspec[0]), &(Hout[0]), frame_length);
		FFT::inverse(&(Pcompspec[0]), &(Pout[0]), frame_length);
		for(int i = 0; i < frame_length; i++){
			Hout[i] *= sin(M_PI * i / frame_length);
			Pout[i] *= sin(M_PI * i / frame_length);
		}
	}
	bool eof(){return push_pop_count < 0 ? true : false;}
	void flag_on(){count_flag = true;}
	bool filled(){return push_pop_count >= block_size ? true : false;}
	int get_exec_count_diff(){
		int tmp = exec_count;
		exec_count = 0;
		second++;
		return tmp;
	}

	int get_exec_count_ave(){
		int tmp = exec_count_total / second;
		return tmp;
	}

private:
	int exec_count, exec_count_total, second;
	int block_size;
	int frame_length;
	int frame_shift;
	int push_pop_count;
	bool count_flag;
	FLOAT w_h;
	FLOAT w_p;
	SlideBlock<FLOAT> *W;
	SlideBlock<FLOAT> *H, *H_buf;
	SlideBlock<FLOAT> *P, *P_buf;
	SlideBlock<FLOAT> *maskH, *maskH_buf;
	SlideBlock<FLOAT> *maskP, *maskP_buf;
	SlideBlock<FLOAT> *phaseSpec;
	FLOAT *signal, *amplitude, *phase, *mask_def;
	std::complex<FLOAT> *fft_ed, *Hcompspec, *Pcompspec;
	enum{binary, wiener} mask;
};

HPSS_Idiv::HPSS_Idiv(int a, int b, int c, FLOAT d, FLOAT e){
	block_size = a;
	frame_length = b;
	frame_shift = c;
	push_pop_count = 0;
	count_flag = false;
	w_h = d;
	w_p = e;
	W =         new SlideBlock<FLOAT>(block_size, frame_length/2);
	H =         new SlideBlock<FLOAT>(block_size, frame_length/2);
	P =         new SlideBlock<FLOAT>(block_size, frame_length/2);
	maskH =     new SlideBlock<FLOAT>(block_size, frame_length/2);
	maskP =     new SlideBlock<FLOAT>(block_size, frame_length/2);
	H_buf =     new SlideBlock<FLOAT>(block_size, frame_length/2);
	P_buf =     new SlideBlock<FLOAT>(block_size, frame_length/2);
	maskH_buf = new SlideBlock<FLOAT>(block_size, frame_length/2);
	maskP_buf = new SlideBlock<FLOAT>(block_size, frame_length/2);
	phaseSpec = new SlideBlock<FLOAT>(block_size, frame_length/2);
	signal    = new FLOAT[frame_length];
	amplitude = new FLOAT[frame_length];
	phase     = new FLOAT[frame_length];
	mask_def  = new FLOAT[frame_length];
	fft_ed    = new std::complex<FLOAT>[frame_length];
	Hcompspec = new std::complex<FLOAT>[frame_length];
	Pcompspec = new std::complex<FLOAT>[frame_length];
	exec_count = 0;
	exec_count_total = 0;
	second = 0;

	mask = binary;
//	mask = wiener;
}

HPSS_Idiv::~HPSS_Idiv(){
	delete W;
	delete H;
	delete P;
	delete maskH;
	delete maskP;
	delete H_buf;
	delete P_buf;
	delete maskH_buf;
	delete maskP_buf;
	delete phaseSpec;
	delete []signal;
	delete []amplitude;
	delete []phase;
	delete []mask_def;
	delete []fft_ed;
	delete []Hcompspec;
	delete []Pcompspec;
}
