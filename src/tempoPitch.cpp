// 面倒なのでとりあえずコピペで済ます。簡単に整理はする。

typedef double FLOAT;

#include "tempoPitch.h"
#include <cmath>
#include <unistd.h>
//bool flag;

TempoPitch::TempoPitch(){
		exec_count = 0;
		exec_count_total = 0;
		second = 0;
		pthread_attr_init(&attr_FIFO);
		pthread_attr_init(&attr_RR);
		pthread_attr_setschedpolicy(&attr_FIFO, SCHED_FIFO);
		pthread_attr_setschedpolicy(&attr_RR,   SCHED_RR);
}

TempoPitch::~TempoPitch(){
	//	delete  etc
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

		fftw_destroy_plan(iniL2);
		iniL2 = fftw_plan_dft_r2c_1d(cframe2, iinL, ioutL, FFTW_ESTIMATE);//
		for (int h=0; h<cframe2; h++){
			iw[h] = (0.5 - 0.5 * cos( 2.0 * M_PI * h / cframe2));
		}
	}
}

// ------------------------------------------------------------------------------------
// FFTW setting
void TempoPitch::FFTWalloc(){
	inL   = new FLOAT[frame * bs];
	sigBuf.init(bs, frame); // inL
	outL  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*(frame/2+1)*bs);
	cmpSpec.init(bs, frame / 2 + 1); //outL

	plans.alloc(bs);
	inv_plans.alloc(bs);
	for (int j=0; j<bs; j++){
		plans[j]     = fftw_plan_dft_r2c_1d(frame, inL+j*frame, outL+j*(frame/2+1), FFTW_ESTIMATE );
		inv_plans[j] = fftw_plan_dft_c2r_1d(frame, outL+j*(frame/2+1), inL+j*frame, FFTW_ESTIMATE );
	}


	iinL  = new FLOAT[frame * 4];
	ioutL = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*(frame*2+1));

	iniL  = fftw_plan_dft_r2c_1d(frame, iinL, ioutL, FFTW_ESTIMATE);
	iiniL = fftw_plan_dft_c2r_1d(frame, ioutL, iinL, FFTW_ESTIMATE);
	iniL2 = fftw_plan_dft_r2c_1d(frame, iinL, ioutL, FFTW_ESTIMATE);

}

// ------------------------------------------------------------------------------------
void TempoPitch::init(int frame_, int shift_, int ch_, int bs_, int coeff_, int freq_, GUI *panel_){
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
	refL   = new FLOAT[coeff+1];
	stateL = new FLOAT[coeff];
	ampL   = new FLOAT[(frame/2+1)*bs];
	sigL   = new FLOAT[frame+(bs-1)*shift]; // sufficient?
	bufL   = new FLOAT[frame];

	read_buffer.alloc(frame * 2);

	FFTWalloc();
	generateWindow();

	iframe_=0;
	tframe_=1;

	cframe2=frame;
	update_cframe2();
}


// ------------------------------------------------------------------------------------
void TempoPitch::generateWindow(void){
	w. alloc(frame);
	iw.alloc(frame * 4);
	a. alloc(frame);
	for (int h=0; h<frame; h++){
		w[h]  = 0.5 - 0.5 * cos(2.0 * M_PI * h / frame);
		iw[h] = w[h];
		a[h]  = w[h] / frame / (frame/shift) * 8 / 3;
	}
}

// ------------------------------------------------------------------------------------
// timbre fixed
void TempoPitch::pitch_modify(void){
	SigRef2(iinL,cframe2,coeff,refL); // envelope and residual

	fftw_execute(iniL2); // FFT residual
	if (frame > cframe2){
		for (int h = cframe2 / 2 + 1; h < frame / 2 + 1; h++){
			ioutL[h][0] = ioutL[h][1] = 0.0;
		}
	}

	fftw_execute(iiniL);// iFFT resampled residual
	for(int h = 0; h < coeff; h++){
		stateL[h]=0;
	}
	for(int h = 0; h < frame; h++){
		iinL[h] = refsig(coeff, refL, stateL, iinL[h]) / frame; // resynthesize the signal
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
		fftw_execute( inv_plans[j] );
		for (int h = 0; h < frame; h++){
			inL[j * frame + h] *= a[h];
		}
	}
}

void TempoPitch::iFFTaddition(void){
	for (int i = 0; i < frame + (bs - 1) * shift; i++){
		sigL[i] = i < frame - shift ? bufL[i] : 0.;
	}
	for (int j = 0; j < bs; j++){
		int offset1 = (tframe_ + j >= bs) ? (tframe_ + j - bs) * frame : (tframe_ + j) * frame;
		for (int h = 0; h < frame; h++){
			sigL[j * shift + h] += inL[offset1 + h];
		}
	}
}


void TempoPitch::iteration2(void){
	for (int j = 0; j < bs; j++){
		int offset2 = (j - tframe_) * shift + ((j < tframe_) ? bs * shift : 0 );
		for (int h = 0; h < frame; h++){
			inL[j * frame + h] = w[h] * sigL[h + offset2];
		}
		fftw_execute(plans[j]);
		int offset1 = j * (frame / 2 + 1);

		for (int h = 0; h < frame / 2 + 1; h++){
			double tmp = sqrt(outL[offset1+h][0]*outL[offset1+h][0]+outL[offset1+h][1]*outL[offset1+h][1]);
			if (tmp < 1e-100){
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

		fftw_execute(iniL);

		int s = (frame < cframe2 ? frame : cframe2) / 2 + 1;
		s = frame / 2 + 1;
		MUTEX.lock();
		for (int h = 0; h < s; h++){
			//回転させるということ？
			outL[iframe_ * (frame / 2 + 1) + h][0] = ioutL[h][0];
			outL[iframe_ * (frame / 2 + 1) + h][1] = ioutL[h][1];
			ampL[iframe_ * (frame / 2 + 1) + h] = sqrt(ioutL[h][0] * ioutL[h][0] + ioutL[h][1] * ioutL[h][1]);
		}
		for (int h = s; h < frame/2 + 1; h++){ 
/*
			outL[iframe_ * (frame / 2 + 1) + h][0] = 0.0;
			outL[iframe_ * (frame / 2 + 1) + h][1] = 0.0;
			ampL[iframe_ * (frame / 2 + 1) + h]=0.0;
*/
/*
			outL[iframe_ * (frame / 2 + 1) + h][0] = ioutL[s-1][0] / (2 * h/frame);
			outL[iframe_ * (frame / 2 + 1) + h][1] = ioutL[s-1][0] / (2 * h/frame);
			ampL[iframe_ * (frame / 2 + 1) + h]=sqrt(ioutL[s-1][0] * ioutL[s-1][0] + ioutL[s-1][1] * ioutL[s-1][1])/  (2 * h/frame); //ここで計算する必要性は？
*/
		}
		MUTEX.unlock();

		MUTEX.lock();

		const int NNN = 1;
		for(int i = 0; i < NNN - 1; ++i){
			exec_count++;
			exec_count_total++;
			inverseFFT();
			iFFTaddition();
			iteration2();
		}
		exec_count++;
		exec_count_total++;
		inverseFFT();
		iFFTaddition();

		if(output->size() < 16000 * 0.5){ // if less than 0.5 sec
			lastIteration();
		}
		iteration2();
		MUTEX.unlock();
	}
}

// ------------------------------------------------------------------------------------
void TempoPitch::read_data_from_the_input(int modified_frame_length){
	while(!input->read_data(&(read_buffer[0]), modified_frame_length)){
			usleep(1000);
	}
	for(int i = 0; i < modified_frame_length; i++){
		iinL[i] = read_buffer[i] * iw[i];
	}
//FLOAT tempo = (float)( panel->tempo.get() )/ 100.0;
	input->rewind_stream_a_little((int)( (modified_frame_length - shift * 1 /*tempo*/) ));
}



// ------------------------------------------------------------------------------------



void TempoPitch::lastIteration(void){
	for (int h = 0; h < frame; h++){
		bufL[h] += inL[tframe_ * frame + h];
	}
	push_data_to_the_output();
	// shift
	for(int h = 0; h < frame; h++){
		bufL[h] = h < frame - shift ? bufL[h + shift] : 0;
	}
}

void TempoPitch::push_data_to_the_output(void){
		float *tmp = new float[shift];
		for(int i = 0; i < shift; i++){
			tmp[i] = static_cast<float>(bufL[i]);
		}
		output->push_data(tmp, shift);
		delete[] tmp; //kokode error ga okiteru?
}
