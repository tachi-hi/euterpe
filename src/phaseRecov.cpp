// copied from Mizuno's codes
// refactored a little

#include "phaseRecov.hpp"
#include <cmath>
#include <algorithm>

phaseRecov::phaseRecov() {}

phaseRecov::~phaseRecov(){
	// Destroy FFTW plans (only those that were actually created)
	for (int j = 0; j < bs; j++) {
		if (plans[j])     Fftw::destroy(plans[j]);
		if (inv_plans[j]) Fftw::destroy(inv_plans[j]);
	}
	if (iniL)  Fftw::destroy(iniL);
	if (iiniL) Fftw::destroy(iiniL);
	if (iniL2) Fftw::destroy(iniL2);
	// vector/smart pointer members are cleaned up automatically.
}

// ------------------------------------------------------------------------------------
// FFTW setting
void phaseRecov::FFTWalloc(){
	inL.assign(frame * bs, 0.0);
	sigBuf.init(bs, frame);
	outL.reset((Fftw::complex*) Fftw::alloc(sizeof(Fftw::complex)*(frame/2+1)*bs));
	cmpSpec = std::make_unique<SlideBlock<std::complex<Scalar>>>(bs, frame / 2 + 1);

	plans.assign(bs, nullptr);
	inv_plans.assign(bs, nullptr);
	for (int j=0; j<bs; j++){
		plans[j]     = Fftw::plan_dft_r2c_1d(frame, inL.data()+j*frame, outL.get()+j*(frame/2+1), FFTW_ESTIMATE);
		inv_plans[j] = Fftw::plan_dft_c2r_1d(frame, outL.get()+j*(frame/2+1), inL.data()+j*frame, FFTW_ESTIMATE);
	}

	// pitch conv
	iinL.assign(frame * 4, 0.0);
	ioutL.reset((Fftw::complex*) Fftw::alloc(sizeof(Fftw::complex)*(frame*2+1)));

	iniL  = Fftw::plan_dft_r2c_1d(frame, iinL.data(), ioutL.get(), FFTW_ESTIMATE);
	iiniL = Fftw::plan_dft_c2r_1d(frame, ioutL.get(), iinL.data(), FFTW_ESTIMATE);
	iniL2 = Fftw::plan_dft_r2c_1d(frame, iinL.data(), ioutL.get(), FFTW_ESTIMATE);
}

// ------------------------------------------------------------------------------------
void phaseRecov::init(int frame_, int shift_, int ch_, int bs_){
	frame = frame_;
	shift = shift_;
	bs = bs_;

	if(ch_ != 1){
		DBGMSG("channel must be monaural");
		exit(1);
	}

	//////// memory allocation
	ampL.assign((frame/2+1)*bs, 0.0);
	sigL.assign(frame+(bs-1)*shift, 0.0);
	bufL.assign(frame, 0.0);

	read_buffer.assign(frame * 2, 0.0f);

	FFTWalloc();
	generateWindow();

	iframe=0;
	tframe=1;

	cframe2=frame;
}


// ------------------------------------------------------------------------------------
void phaseRecov::generateWindow(void){
	w.assign(frame, 0.0);
	iw.assign(frame * 4, 0.0);
	a.assign(frame, 0.0);

	int h = 0;
	std::generate(w.begin(), w.end(),
		[&]{ return Scalar(0.5) - Scalar(0.5) * std::cos(Scalar(2.0 * M_PI) * h++ / frame); });
	std::copy(w.begin(), w.end(), iw.begin()); // iw の後半 3*frame 要素はゼロのまま
	const auto ca = Scalar(1.0) / frame / (frame / shift) * Scalar(8.0 / 3.0);
	std::transform(w.begin(), w.end(), a.begin(), [ca](Scalar v){ return v * ca; });
}

// ------------------------------------------------------------------------------------
void phaseRecov::read_data_from_the_input(int modified_frame_length){
	while(!input->wait_and_read(read_buffer.data(), modified_frame_length))
		; // ブロッキング読み出し（100ms タイムアウト後リトライ）
	std::transform(read_buffer.begin(), read_buffer.begin() + modified_frame_length,
		iw.begin(), iinL.begin(),
		[](float r, Scalar w) -> Scalar { return r * w; });
	input->rewind_stream_a_little((int)( (modified_frame_length - shift * 1 /*tempo*/) ));
}


// ------------------------------------------------------------------------------------
void phaseRecov::callback_(void){
	while(1){
		//////// block shift
		if (++iframe == bs) iframe = 0; //bs may be blcok size
		if (++tframe == bs) tframe = 0;

		read_data_from_the_input(cframe2);

		if(cframe2 != frame){
		}

		Fftw::execute(iniL);

		int s = (frame < cframe2 ? frame : cframe2) / 2 + 1;

		for (int h = 0; h < s; h++){
			// rotate
			outL[iframe * (frame / 2 + 1) + h][0] = ioutL[h][0];
			outL[iframe * (frame / 2 + 1) + h][1] = ioutL[h][1];
			ampL[iframe * (frame / 2 + 1) + h] = std::sqrt(ioutL[h][0] * ioutL[h][0] + ioutL[h][1] * ioutL[h][1]);
		}
		for (int h = s; h < frame/2 + 1; h++){
			outL[iframe * (frame / 2 + 1) + h][0] = 0.0;
			outL[iframe * (frame / 2 + 1) + h][1] = 0.0;
			ampL[iframe * (frame / 2 + 1) + h]=0.0;
		}

		update();
	}
}

void phaseRecov::update(void){
	int loop_count = 0;
	for(;;){
		inverseFFT();
		iFFTaddition();
		iteration2();

		inverseFFT();
		iFFTaddition();
		iteration2();

		inverseFFT();
		iFFTaddition();

		++loop_count;
		// 終了条件:
		//   (a) 次フレームが届いた
		//   (b) 出力バッファが危険水域（2フレーム＝32ms未満）
		//   (c) ループ上限に達した（max_outer_loops >= 0 のとき）
		if(input->size() >= cframe2
		   || output->size() < shift * 2
		   || (max_outer_loops >= 0 && loop_count >= max_outer_loops)){
			lastIteration();
			iteration2();
			break;
		}
		iteration2();
	}
}

// ------------------------------------------------------------------------------------
// apply inverse FFT for each frame
void phaseRecov::inverseFFT(void){
	for (int j = 0; j < bs; j++){
		Fftw::execute( inv_plans[j] );
		std::transform(inL.begin() + j*frame, inL.begin() + (j+1)*frame,
			a.begin(), inL.begin() + j*frame,
			[](Scalar s, Scalar w){ return s * w; });
	}
}

// ------------------------------------------------------------------------------------
// add the iFFT segments to create a single signal
void phaseRecov::iFFTaddition(void){
	std::copy_n(bufL.begin(), frame - shift, sigL.begin());
	std::fill(sigL.begin() + (frame - shift), sigL.end(), Scalar(0));

	for (int j = 0; j < bs; j++){
		int offset1 = (tframe + j >= bs) ? (tframe + j - bs) * frame : (tframe + j) * frame;
		std::transform(sigL.begin() + j*shift, sigL.begin() + j*shift + frame,
			inL.begin() + offset1, sigL.begin() + j*shift,
			std::plus<Scalar>{});
	}
}

// ------------------------------------------------------------------------------------
void phaseRecov::push_data_to_the_output(void){
	std::vector<float> tmp(shift);
	std::transform(bufL.begin(), bufL.begin() + shift, tmp.begin(),
		[](Scalar v){ return static_cast<float>(v); });
	output->push_data(tmp.data(), shift);
}

void phaseRecov::lastIteration(void){
	std::transform(bufL.begin(), bufL.end(),
		inL.begin() + tframe*frame, bufL.begin(), std::plus<Scalar>{});
	push_data_to_the_output();
	// shift buffer
	std::copy_n(bufL.begin() + shift, frame - shift, bufL.begin());
	std::fill(bufL.begin() + (frame - shift), bufL.end(), Scalar(0));
}

void phaseRecov::iteration2(void){
	for (int j = 0; j < bs; j++){
		int offset2 = (j - tframe) * shift + ((j < tframe) ? bs * shift : 0 );
		std::transform(w.begin(), w.end(),
			sigL.begin() + offset2, inL.begin() + j*frame,
			[](Scalar w, Scalar s){ return w * s; }); //window
		Fftw::execute(plans[j]);
		int offset1 = j * (frame / 2 + 1);

		for (int h = 0; h < frame / 2 + 1; h++){
			auto tmp = std::sqrt(outL[offset1+h][0]*outL[offset1+h][0]+outL[offset1+h][1]*outL[offset1+h][1]);
			// update power
			if (tmp < Scalar(1e-100)){
				outL[offset1 + h][0] = 0.0;
				outL[offset1 + h][1] = 0.0;
			}else{
				tmp = ampL[offset1+h]/tmp;
				outL[offset1 + h][0] *= tmp;
				outL[offset1 + h][1] *= tmp;
			}
		}
	}
}
