
#pragma once

#include <iostream>
#include <complex>
#include <fftw3.h>
#include <vector>
#include "signalProcessingLibrary.h"
#include "streamBuffer.h"
#include "myMutex.h"
#include "gui.h"
#include "SlideBlock.hpp"


// -----------------------------------------------------------------------------------------
template<typename T>
class myVec{
 public:
	myVec(){}
	~myVec(){}
	void alloc(int size){
		data.reserve(size);
		for(int i = 0; i < size; i++) data.push_back(0);
	}
	T& operator[](int x){return data[x];}
 private:
	std::vector<T> data;
};

// -----------------------------------------------------------------------------------------
typedef myVec<double> Window;
typedef myVec<float> SignalBuffer;
typedef myVec<fftw_plan> FFTW_plans;

// -----------------------------------------------------------------------------------------
template<typename T>
class TwoDimArray{
 public:
	TwoDimArray(){}
	~TwoDimArray(){}
	void init(int Time, int LengthOfOneFrame_){
		LengthOfOneFrame = LengthOfOneFrame_;
		BlockSize = Time;
		data.reserve(Time * LengthOfOneFrame);
		for(int i = 0; i < Time * LengthOfOneFrame; i++) data.push_back(0);
	}
	T* operator[](int n){
		return &data[LengthOfOneFrame * n];
	}
 private:
	std::vector<T> data;
	int BlockSize;
	int LengthOfOneFrame;
};

// -----------------------------------------------------------------------------------------
typedef TwoDimArray<std::complex<double> > ComplexSpectrogram;
typedef TwoDimArray<double> SignalFragment;

// -----------------------------------------------------------------------------------------
class phaseRecov{
 public:
	phaseRecov();
	virtual ~phaseRecov();

	virtual void SpecModify(void){};

	void init(int frame, int shift, int ch, int bs);

	void setBuffer(StreamBuffer<float>*in , StreamBuffer<float>* out){
		input = in;
		output = out;
	}

	void start(void){
		pthread_attr_init(&attr_FIFO);
		pthread_attr_setschedpolicy(&attr_FIFO, SCHED_FIFO);
		pthread_create(&tempopitch_thread, &attr_FIFO, (void*(*)(void*))this->callback, this);
	}

	static void* callback(void* arg){reinterpret_cast<phaseRecov*>(arg)->callback_();return 0;}	
	void callback_(void);

 private:
	void read_data_from_the_input(int);
	void push_data_to_the_output(void);

	void inverseFFT(void);
	void iFFTaddition(void);

	void lastIteration(void);
	void iteration2(void);
	void update(void);

	void update_cframe2(void);
	void FFTWalloc(void);
	void generateWindow(void);

	StreamBuffer<float> *input;
	StreamBuffer<float> *output;

	int frame;
	int shift;
	int bs;

	int cframe2, tframe, iframe, frame_count;//(=current_frame,terminal_frame,initial_frame)
	double *ampL, *sigL, *bufL, *refL, *stateL, *iinL, *inL;
	fftw_plan iniL, iiniL, iniL2;
	fftw_complex *outL, *ioutL;
	FFTW_plans plans, inv_plans;

	Window w, iw, a;
	TwoDimArray<double> iiinnn;

//	ComplexSpectrogram cmpSpec;
	SignalFragment sigBuf;
	SlideBlock<std::complex<double> >*cmpSpec;

	SignalBuffer read_buffer;
	SignalBuffer write_buffer;

	pthread_t tempopitch_thread;
	pthread_attr_t attr_FIFO;
};


