// copied from Mizuno's codes
// refactored a little

typedef double FLOAT;

#include "phaseRecov.hpp"
#include <cmath>
#include <unistd.h>

phaseRecov::phaseRecov(){
}

phaseRecov::~phaseRecov(){
	// delete
	delete cmpSpec;
}

// ------------------------------------------------------------------------------------
// FFTW setting
void phaseRecov::FFTWalloc(){
	inL   = new FLOAT[frame * bs];
	sigBuf.init(bs, frame); // inL
	outL  = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*(frame/2+1)*bs);
	cmpSpec = new SlideBlock<std::complex<double> >(bs, frame / 2 + 1); //outL

	plans.alloc(bs);
	inv_plans.alloc(bs);
	// strategy should not be "estimate" but should be "evaluated" or something
	for (int j=0; j<bs; j++){
		plans[j]     = fftw_plan_dft_r2c_1d(frame, inL+j*frame, outL+j*(frame/2+1), FFTW_ESTIMATE );
//		plans[j]     = fftw_plan_dft_r2c_1d(frame, &(sigBuf[j][0]), reinterpret_cast<fftw_complex*>(cmpSpec[j]), FFTW_ESTIMATE );
		inv_plans[j] = fftw_plan_dft_c2r_1d(frame, outL+j*(frame/2+1), inL+j*frame, FFTW_ESTIMATE );
//		inv_plans[j] = fftw_plan_dft_c2r_1d(frame, reinterpret_cast<fftw_complex*>(cmpSpec[j]), &(sigBuf[j][0]), FFTW_ESTIMATE );
	}


	// pitch conv
	iinL  = new FLOAT[frame * 4];
	ioutL = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*(frame*2+1));

	iniL  = fftw_plan_dft_r2c_1d(frame, iinL, ioutL, FFTW_ESTIMATE);
	iiniL = fftw_plan_dft_c2r_1d(frame, ioutL, iinL, FFTW_ESTIMATE);
	iniL2 = fftw_plan_dft_r2c_1d(frame, iinL, ioutL, FFTW_ESTIMATE);

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
///	refL   = new FLOAT[coeff+1];
//	stateL = new FLOAT[coeff];
	ampL   = new FLOAT[(frame/2+1)*bs];
	sigL   = new FLOAT[frame+(bs-1)*shift]; // buffer sufficient?
	bufL   = new FLOAT[frame];

	read_buffer.alloc(frame * 2);

	FFTWalloc();
	generateWindow();

	iframe=0;
	tframe=1;

	cframe2=frame;
//	update_cframe2();
}


// ------------------------------------------------------------------------------------
void phaseRecov::generateWindow(void){
	w. alloc(frame);
	iw.alloc(frame * 4);
	a. alloc(frame);
	for (int h=0; h<frame; h++){
		w[h]  = 0.5 - 0.5 * cos(2.0 * M_PI * h / frame);
		iw[h] = w[h];
		a[h]  = w[h] / frame / (frame/shift) * 8 / 3;  // devide by frame because of the FFTW's design
	}
}

// ------------------------------------------------------------------------------------
void phaseRecov::read_data_from_the_input(int modified_frame_length){
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
void phaseRecov::callback_(void){
	while(1){
//		update_cframe2();

		//////// block shift
		if (++iframe == bs) iframe = 0; //bs may be blcok size
		if (++tframe == bs) tframe = 0;

		read_data_from_the_input(cframe2);

		if(cframe2 != frame){
		}

		fftw_execute(iniL);

		int s = (frame < cframe2 ? frame : cframe2) / 2 + 1;

		for (int h = 0; h < s; h++){
			// rotate
			outL[iframe * (frame / 2 + 1) + h][0] = ioutL[h][0];
			outL[iframe * (frame / 2 + 1) + h][1] = ioutL[h][1];
			ampL[iframe * (frame / 2 + 1) + h] = sqrt(ioutL[h][0] * ioutL[h][0] + ioutL[h][1] * ioutL[h][1]);
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
	for(;;){
		inverseFFT();
		iFFTaddition();
		iteration2();

		inverseFFT();
		iFFTaddition();
		iteration2();

		inverseFFT();
		iFFTaddition();

		if(output->size() < 16000 * 0.5){ // if less than 0.5[s] output
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
		fftw_execute( inv_plans[j] );
		for (int h = 0; h < frame; h++){
			inL[j * frame + h] *= a[h];
		}
	}
}

// ------------------------------------------------------------------------------------
// add the iFFT segments to create a single signal
void phaseRecov::iFFTaddition(void){
	for (int i = 0; i < frame + (bs - 1) * shift; i++){
		sigL[i] = i < frame - shift ? bufL[i] : 0.;
	}
	for (int j = 0; j < bs; j++){
		int offset1 = (tframe + j >= bs) ? (tframe + j - bs) * frame : (tframe + j) * frame;
		for (int h = 0; h < frame; h++){
			sigL[j * shift + h] += inL[offset1 + h];
		}
	}
}

// ------------------------------------------------------------------------------------
void phaseRecov::push_data_to_the_output(void){
		float *tmp = new float[shift];
		for(int i = 0; i < shift; i++){
			tmp[i] = static_cast<float>(bufL[i]);
		}
		output->push_data(tmp, shift);
		delete[] tmp; //kokode error ga okiteru?
}







void phaseRecov::lastIteration(void){
	for (int h = 0; h < frame; h++){
		bufL[h] += inL[tframe * frame + h];
	}
	push_data_to_the_output();
	// shift
	for(int h = 0; h < frame; h++){
		bufL[h] = h < frame - shift ? bufL[h + shift] : 0;
	}
}

void phaseRecov::iteration2(void){
	for (int j = 0; j < bs; j++){
		int offset2 = (j - tframe) * shift + ((j < tframe) ? bs * shift : 0 );
		for (int h = 0; h < frame; h++){
			inL[j * frame + h] = w[h] * sigL[h + offset2];  //window
		}
		fftw_execute(plans[j]);
		int offset1 = j * (frame / 2 + 1);

		for (int h = 0; h < frame / 2 + 1; h++){
			double tmp = sqrt(outL[offset1+h][0]*outL[offset1+h][0]+outL[offset1+h][1]*outL[offset1+h][1]);
 			// update power
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
