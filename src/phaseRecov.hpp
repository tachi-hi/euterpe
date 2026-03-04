
#pragma once

#include <iostream>
#include <complex>
#include <vector>
#include <memory>
#include <algorithm>
#include <mutex>
#include "fftw_traits.h"
#include "scalar.h"
#include "signalProcessingLibrary.h"
#include "streamBuffer.h"
#include "gui.h"
#include "SlideBlock.hpp"

// Custom deleter for Fftw::alloc allocations
struct FftwDeleter {
    void operator()(void* p) const noexcept { Fftw::free_(p); }
};
template<typename T>
using FftwBuf = std::unique_ptr<T[], FftwDeleter>;


// -----------------------------------------------------------------------------------------
using Window      = std::vector<Scalar>;
using SignalBuffer = std::vector<float>;    // 音声I/O は常に float
using FFTW_plans  = std::vector<Fftw::plan>;

// -----------------------------------------------------------------------------------------
template<typename T>
class TwoDimArray{
 public:
	TwoDimArray(){}
	~TwoDimArray(){}
	void init(int Time, int LengthOfOneFrame_){
		LengthOfOneFrame = LengthOfOneFrame_;
		BlockSize = Time;
		data.assign(Time * LengthOfOneFrame, T{});
	}
	T* operator[](int n){
		return &data[LengthOfOneFrame * n];
	}
 private:
	std::vector<T> data;
	int BlockSize{0};
	int LengthOfOneFrame{0};
};

// -----------------------------------------------------------------------------------------
typedef TwoDimArray<std::complex<Scalar>> ComplexSpectrogram;
typedef TwoDimArray<Scalar> SignalFragment;

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

	// 反復回数の上限を設定する。
	// n >= 1 : 1フレームあたりの外側ループの最大回数（デフォルト 3）
	// n == -1: 次の入力フレームが届くまで無制限に反復（高品質モード）
	void setMaxIterations(int n) { max_outer_loops = n; }

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

	int frame{0};
	int shift{0};
	int bs{0};

	int cframe2{0}, tframe{0}, iframe{0}, frame_count{0};
	std::vector<Scalar> ampL, sigL, bufL, refL, stateL, iinL, inL;
	Fftw::plan iniL{nullptr}, iiniL{nullptr}, iniL2{nullptr};
	FftwBuf<Fftw::complex> outL, ioutL;
	FFTW_plans plans, inv_plans;

	Window w, iw, a;
	TwoDimArray<Scalar> iiinnn;

	SignalFragment sigBuf;
	std::unique_ptr<SlideBlock<std::complex<Scalar>>> cmpSpec;

	SignalBuffer read_buffer;
	SignalBuffer write_buffer;

	pthread_t tempopitch_thread;
	pthread_attr_t attr_FIFO;

protected:
	// デフォルト 3 回（外側ループ）。-1 で次フレームまで無制限。
	int max_outer_loops{3};
};


