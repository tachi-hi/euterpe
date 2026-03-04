#include "tempoPitch.h"
#include <cmath>
#include <algorithm>
#include <vector>

TempoPitch::TempoPitch(){
		second = 0;
		pthread_attr_init(&attr_FIFO);
		pthread_attr_init(&attr_RR);
		pthread_attr_setschedpolicy(&attr_FIFO, SCHED_FIFO);
		pthread_attr_setschedpolicy(&attr_RR,   SCHED_RR);
}

TempoPitch::~TempoPitch(){
	for (int j = 0; j < bs; j++) {
		if (plans[j])     Fftw::destroy(plans[j]);
		if (inv_plans[j]) Fftw::destroy(inv_plans[j]);
	}
	if (iniL)  Fftw::destroy(iniL);
	if (iiniL) Fftw::destroy(iiniL);
	if (iniL2) Fftw::destroy(iniL2);
}

// ------------------------------------------------------------------------------------
// this function is not actuallty used except in print function
double SetPitch(int key){
	return pow(2.0, static_cast<double>(key)/12.0);
}

int reset_frame(int key, int frame){
	static int bunshi[] = {
		1, //-12
		135, //-11
		9, //-10
		75, //-9
		5, //-8
		27*25, //-7
		45, //-6
		3, //-5
		81*5, //-4
		27, //-3
		9*25, //-2
		15, //-1
		1, // 0
		27*5, //+1
		9, //+2
		243*5, //+3
		5, //+4
		27*25, //+5
		45, //+6
		3, //+7
		81*5, //+8
		27, //+9
		9*25, //+10
		15, //+11
		2 //+12
	};
	static int bunbo[] = {
		1, //-12
		8, //-11
		4, //-10
		7, //-9
		3, //-8
		10, //-7
		6, //-6
		2, //-5
		9, //-4
		5, //-3
		8, //-2
		4, //-1
		0, // 0
		7, //+1
		3, //+2
		10, //+3
		2, //+4
		9, //+5
		5, //+6
		1, //+7
		8, //+8
		4, //+9
		7, //+10
		3, //+11
		0 //+12
	};

	int ret = (frame >> bunbo[key + 12]) * bunshi[key + 12];
	if (ret != 0){
		return ret;
	}else{
		fprintf(stderr, "%d %d %d %d", 	ret, frame, bunbo[key + 12], bunshi[key + 12]);
		std::cerr << "error" << std::endl;
		exit(1);
	}
}

// ------------------------------------------------------------------------------------
void TempoPitch::update_cframe2(){
	int tmp = reset_frame((int)(panel->key.get()), frame);

	if (cframe2 != tmp){
		cframe2 = tmp;
		std::cerr << tmp << " " << (int)(SetPitch(panel->key.get()) * frame) << " " << (int)(panel->key.get()) << " " << frame << " " << std::endl;

		Fftw::destroy(iniL2);
		iniL2 = Fftw::plan_dft_r2c_1d(cframe2, iinL.data(), ioutL.get(), FFTW_ESTIMATE);
		int h = 0;
		std::generate_n(iw.begin(), cframe2,
			[&]{ return Scalar(0.5) - Scalar(0.5) * std::cos(Scalar(2.0 * M_PI) * h++ / cframe2); });
	}
}

// ------------------------------------------------------------------------------------
// FFTW setting
void TempoPitch::FFTWalloc(){
	inL.assign(frame * bs, 0.0);
	sigBuf.init(bs, frame);
	outL.reset((Fftw::complex*) Fftw::alloc(sizeof(Fftw::complex)*(frame/2+1)*bs));
	cmpSpec.init(bs, frame / 2 + 1);

	plans.assign(bs, nullptr);
	inv_plans.assign(bs, nullptr);
	for (int j=0; j<bs; j++){
		plans[j]     = Fftw::plan_dft_r2c_1d(frame, inL.data()+j*frame, outL.get()+j*(frame/2+1), FFTW_ESTIMATE);
		inv_plans[j] = Fftw::plan_dft_c2r_1d(frame, outL.get()+j*(frame/2+1), inL.data()+j*frame, FFTW_ESTIMATE);
	}

	iinL.assign(frame * 4, 0.0);
	ioutL.reset((Fftw::complex*) Fftw::alloc(sizeof(Fftw::complex)*(frame*2+1)));

	iniL  = Fftw::plan_dft_r2c_1d(frame, iinL.data(), ioutL.get(), FFTW_ESTIMATE);
	iiniL = Fftw::plan_dft_c2r_1d(frame, ioutL.get(), iinL.data(), FFTW_ESTIMATE);
	iniL2 = Fftw::plan_dft_r2c_1d(frame, iinL.data(), ioutL.get(), FFTW_ESTIMATE);
}

// ------------------------------------------------------------------------------------
void TempoPitch::init(int frame_, int shift_, int ch_, int bs_, int coeff_, int freq_, GUIBase *panel_){
	frame = frame_;
	shift = shift_;
	bs = bs_;
	coeff = coeff_;
	panel = panel_;

	if(ch_ != 1){
		DBGMSG("channel must be monaural");
		exit(1);
	}

	//////// memory allocation
	refL.assign(coeff+1, 0.0);
	stateL.assign(coeff, 0.0);
	ampL.assign((frame/2+1)*bs, 0.0);
	sigL.assign(frame+(bs-1)*shift, 0.0);
	bufL.assign(frame, 0.0);

	read_buffer.assign(frame * 2, 0.0f);

	FFTWalloc();
	generateWindow();

	iframe_=0;
	tframe_=1;

	cframe2=frame;
	update_cframe2();
}


// ------------------------------------------------------------------------------------
void TempoPitch::generateWindow(void){
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
// timbre fixed
void TempoPitch::pitch_modify(void){
	SigRef2(iinL.data(), cframe2, coeff, refL.data()); // envelope and residual

	Fftw::execute(iniL2); // FFT residual
	if (frame > cframe2){
		for (int h = cframe2 / 2 + 1; h < frame / 2 + 1; h++){
			ioutL[h][0] = ioutL[h][1] = 0.0;
		}
	}

	Fftw::execute(iiniL);// iFFT resampled residual
	std::fill(stateL.begin(), stateL.end(), Scalar(0));
	for(int h = 0; h < frame; h++){
		iinL[h] = refsig(coeff, refL.data(), stateL.data(), iinL[h]) / frame;
	}
}

// ------------------------------------------------------------------------------------

void TempoPitch::update_callback_(void){
	if(false){
		while(1){
			MUTEX.lock();
			exec_count++;
			exec_count_total++;
			inverseFFT();
			iFFTaddition();
			iteration2();
			MUTEX.unlock();
		}
	}
}

// ------------------------------------------------------------------------------------
// apply inverse FFT for each frame
void TempoPitch::inverseFFT(void){
	for (int j = 0; j < bs; j++){
		Fftw::execute( inv_plans[j] );
		std::transform(inL.begin() + j*frame, inL.begin() + (j+1)*frame,
			a.begin(), inL.begin() + j*frame,
			[](Scalar s, Scalar w){ return s * w; });
	}
}

void TempoPitch::iFFTaddition(void){
	std::copy_n(bufL.begin(), frame - shift, sigL.begin());
	std::fill(sigL.begin() + (frame - shift), sigL.end(), Scalar(0));

	for (int j = 0; j < bs; j++){
		int offset1 = (tframe_ + j >= bs) ? (tframe_ + j - bs) * frame : (tframe_ + j) * frame;
		std::transform(sigL.begin() + j*shift, sigL.begin() + j*shift + frame,
			inL.begin() + offset1, sigL.begin() + j*shift,
			std::plus<Scalar>{});
	}
}


void TempoPitch::iteration2(void){
	for (int j = 0; j < bs; j++){
		int offset2 = (j - tframe_) * shift + ((j < tframe_) ? bs * shift : 0 );
		std::transform(w.begin(), w.end(),
			sigL.begin() + offset2, inL.begin() + j*frame,
			[](Scalar w, Scalar s){ return w * s; }); //window
		Fftw::execute(plans[j]);
		int offset1 = j * (frame / 2 + 1);

		for (int h = 0; h < frame / 2 + 1; h++){
			auto tmp = std::sqrt(outL[offset1+h][0]*outL[offset1+h][0]+outL[offset1+h][1]*outL[offset1+h][1]);
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

// -------------------------------------------------------------------------------------------------------------
void TempoPitch::callback_(void){
	while(1){
		MUTEX.lock();
		update_cframe2();

		//////// block shift
		if (++iframe_ == bs) iframe_ = 0;
		if (++tframe_ == bs) tframe_ = 0;
		MUTEX.unlock();

		read_data_from_the_input(cframe2);

		if(cframe2 != frame){
			pitch_modify();
		}

		Fftw::execute(iniL);

		int s = (frame < cframe2 ? frame : cframe2) / 2 + 1;
		s = frame / 2 + 1;
		MUTEX.lock();
		for (int h = 0; h < s; h++){
			outL[iframe_ * (frame / 2 + 1) + h][0] = ioutL[h][0];
			outL[iframe_ * (frame / 2 + 1) + h][1] = ioutL[h][1];
			ampL[iframe_ * (frame / 2 + 1) + h] = std::sqrt(ioutL[h][0] * ioutL[h][0] + ioutL[h][1] * ioutL[h][1]);
		}
		MUTEX.unlock();

		MUTEX.lock();
		// 次フレームが届くまでの空き時間を追加反復に使う（品質向上）
		// 出力バッファが枯渇しそうな場合は早めに出力して音飛びを防ぐ
		{
			int loop_count = 0;
			do {
				inverseFFT(); iFFTaddition(); iteration2(); exec_count++; exec_count_total++;
				inverseFFT(); iFFTaddition(); iteration2(); exec_count++; exec_count_total++;
				inverseFFT(); iFFTaddition();
				++loop_count;
				// 終了条件:
				//   (a) 次フレームが届いた
				//   (b) 出力バッファが危険水域（2フレーム＝32ms未満）
				//   (c) ループ上限に達した（max_outer_loops >= 0 のとき）
				if(input->size() >= cframe2
				   || output->size() < shift * 2
				   || (max_outer_loops >= 0 && loop_count >= max_outer_loops)){
					lastIteration();
					iteration2(); exec_count++; exec_count_total++;
					break;
				}
				iteration2(); exec_count++; exec_count_total++;
			} while(true);
		}
		MUTEX.unlock();
	}
}

// ------------------------------------------------------------------------------------
void TempoPitch::read_data_from_the_input(int modified_frame_length){
	while(!input->wait_and_read(read_buffer.data(), modified_frame_length))
		; // ブロッキング読み出し（100ms タイムアウト後リトライ）
	std::transform(read_buffer.begin(), read_buffer.begin() + modified_frame_length,
		iw.begin(), iinL.begin(),
		[](float r, Scalar w) -> Scalar { return r * w; });
	input->rewind_stream_a_little((int)( (modified_frame_length - shift * 1 /*tempo*/) ));
}



// ------------------------------------------------------------------------------------



void TempoPitch::lastIteration(void){
	std::transform(bufL.begin(), bufL.end(),
		inL.begin() + tframe_*frame, bufL.begin(), std::plus<Scalar>{});
	push_data_to_the_output();
	// shift
	std::copy_n(bufL.begin() + shift, frame - shift, bufL.begin());
	std::fill(bufL.begin() + (frame - shift), bufL.end(), Scalar(0));
}

void TempoPitch::push_data_to_the_output(void){
	std::vector<float> tmp(shift);
	std::transform(bufL.begin(), bufL.begin() + shift, tmp.begin(),
		[](Scalar v){ return static_cast<float>(v); });
	output->push_data(tmp.data(), shift);
}
