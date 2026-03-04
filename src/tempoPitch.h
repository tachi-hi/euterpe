#pragma once

#include <atomic>
#include <iostream>
#include <complex>
#include <vector>
#include <mutex>
#include "fftw_traits.h"
#include "scalar.h"
#include "signalProcessingLibrary.h"
#include "streamBuffer.h"
#include "guiBase.h"
#include "phaseRecov.hpp"

class TempoPitch: public phaseRecov{
 public:
	TempoPitch();
	virtual ~TempoPitch();

	void init(int frame, int shift, int ch, int bs, int coeff, int freq, GUIBase *panel);

	void setBuffer(StreamBuffer<float>*in , StreamBuffer<float>* out){
		input = in;
		output = out;
	}

	void start(void){
			pthread_create(&update_thread,     &attr_RR,   (void*(*)(void*))TempoPitch::update_callback, this);
			pthread_create(&tempopitch_thread, &attr_FIFO, (void*(*)(void*))TempoPitch::callback, this);
	}

	static void* callback(void* arg){
		reinterpret_cast<TempoPitch*>(arg)->callback_();
		return 0;}
	void callback_(void);

	static void* update_callback(void* arg){
		reinterpret_cast<TempoPitch*>(arg)->update_callback_();
		return 0;}
	void update_callback_(void);

	int get_exec_count_diff(){
		int tmp = exec_count.exchange(0);  // atomic swap
		second++;
		return tmp;
	}
	int get_exec_count_ave(){
		return exec_count_total.load()/second;
	}

 private:
	void read_data_from_the_input(int);
	void pitch_modify(void);
	void inverseFFT(void);
	void iFFTaddition(void);
	void push_data_to_the_output(void);
	void lastIteration(void);
	void iteration2(void);

	void update_cframe2(void);
	void FFTWalloc(void);
	void generateWindow(void);

	StreamBuffer<float> *input;
	StreamBuffer<float> *output;

	int frame;
	int shift;
	int bs;
	int coeff;
	GUIBase *panel;

	int cframe2{0}, tframe_{0}, iframe_{0};
	std::vector<Scalar> ampL, sigL, bufL, refL, stateL, iinL, inL;
	Fftw::plan iniL{nullptr}, iiniL{nullptr}, iniL2{nullptr};
	FftwBuf<Fftw::complex> outL, ioutL;
	FFTW_plans plans, inv_plans;

	Window w, iw, a;
	TwoDimArray<Scalar> iiinnn;

	ComplexSpectrogram cmpSpec;
	SignalFragment sigBuf;

	SignalBuffer read_buffer;
	SignalBuffer write_buffer;

	pthread_t tempopitch_thread;
	pthread_t update_thread;
	pthread_attr_t attr_FIFO;
	pthread_attr_t attr_RR;

	std::mutex MUTEX;
	std::atomic<int> exec_count{0};
	std::atomic<int> exec_count_total{0};
	int second;
};


