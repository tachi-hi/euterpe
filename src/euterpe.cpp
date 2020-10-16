/*******************************************************************/
// EUTERPE
//
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp
/*******************************************************************/

//typedef double FLOAT;

// todo
//  file io

#include "euterpe.h"

#include <iostream>
#include <cstdio>

#include <vector>
#include <cstdlib>
#include <cmath>

#include <portaudio.h>
#include <pthread.h>

//#include "filter.h"
#include "HPSS_pipe_long.h"

int main(int argc, char **argv){
	const int freq = 16000;

	// declare stream buffer ------------------------------------------------------------
	StreamBuffer<float> inBuffer;
	StreamBuffer<float> Buffer_H_short;
	StreamBuffer<float> Buffer_P_short;
	StreamBuffer<float> Buffer_H_long;
	StreamBuffer<float> Buffer_P_long;
	StreamBuffer<float> Buffer_karaoke;
	StreamBuffer<float> outBuffer;

	// Audio IO -------------------------------------------------------------------------
	AudioDevice* audio_thread = AudioDevice::getInstance();
	audio_thread->init(1, freq, 1024, &inBuffer, &outBuffer);

	// HPSS short ------------------------------------------------------------------------
	HPSS_pipe HPSS_short(7, 512, 256, 10, 10);
	HPSS_short.setBuffer(&Buffer_P_long, &Buffer_H_short, &Buffer_P_short);

	// HPSS long --------------------------------------------------------------------------
	HPSS_pipe HPSS_long(7, 2048, 1024, 10, 10);
	HPSS_long.setBuffer(&inBuffer, &Buffer_H_long,  &Buffer_P_long);

	// GUI --------------------------------------------------------------------------------
	GUI panel(argc, argv);

	// Adder ------------------------------------------------------------------------------
	StreamAdder adder(2048, &panel); 
	adder.add_input_stream(&Buffer_P_short); adder.set_multiply(0, 1.0);
	adder.add_input_stream(&Buffer_H_long);  adder.set_multiply(1, 1.0);
	adder.add_input_stream(&Buffer_H_short);  adder.set_multiply(2, 0.0); // control through GUI (todo)
	adder.set_output_stream(&Buffer_karaoke);

	// PitchConversion ---------------------------------------------------------------------
	TempoPitch converter;
	converter.init(1024 * 2, 128 * 2, 1 /* channel */,  7 /* block */, 15 /* coeff */, freq, &panel);//param? // frame should be very long
	converter.setBuffer(&Buffer_karaoke, &outBuffer);

	// start  ---------------------------------------------------------------------------
	audio_thread->start();
	HPSS_short.start();
	HPSS_long.start();
	adder.start();
	converter.start();
	panel.start();

	// display something every 1[s] -----------------------------------------------------------------
	int sec = 0;
	while(1){
		sec++;
		usleep(1000000);
		fprintf(stderr, "%3d[ms]  %3d[ms]  %3d[ms] \t HPSS: short %d[times/s] long %d[times/s] phase %d[times/s] audio trial %d \n",
			(int)((double)inBuffer.size() / freq * 1000),
			(int)((double)Buffer_karaoke.size() / freq * 1000),
			(int)((double)outBuffer.size() / freq * 1000),
			HPSS_short.get_exec_count_diff(),
			HPSS_long. get_exec_count_diff(),
			converter. get_exec_count_diff(),
			audio_thread->get_exec_count()
		);

		fprintf(stderr, "sec: %d min %d sec         \t ave:  short %d[times/s] long %d[times/s] phase %d[times/s] \n\n",
			sec / 60,
			sec % 60,
			HPSS_short.get_exec_count_ave(),
			HPSS_long. get_exec_count_ave(),
			converter. get_exec_count_ave()
		);

		if(panel.getstop()){audio_thread->stop();}
		if(panel.getquit()){break;}

	}

	audio_thread->kill();
	// GUI kill
	return 0;
}
