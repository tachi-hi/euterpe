/*******************************************************************/
// PortAudio wrapper
// singleton
//
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp
/*******************************************************************/

//#include "waviostream.hpp"
//wavistream *fp;

#include <iostream>
#include "audioPlay.h"
#include <cstdlib>
#include <cmath>
#include<unistd.h>
#include"debug.h"

// singleton
AudioDevice *AudioDevice::instance = 0;
AudioDevice::AudioDevice(){}
AudioDevice::~AudioDevice(){}

// error checker
void AudioDevice::PortAudioErrorCheck(const PaError& err,
				      const char *debug_message)
{
  if(err != paNoError){
    std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    std::cerr << debug_message << std::endl;
    exit(1);
  }
}


void AudioDevice::init(int n_channel_,
		       int sampling_rate_,
		       int samples_per_buffer_,
		       StreamBuffer<float>* inBuffer_,
		       StreamBuffer<float>* outBuffer_){
  // Parameter initialization
  n_channel = n_channel_;
  sampling_rate = sampling_rate_;
  samples_per_buffer = samples_per_buffer_;
  inBuffer = inBuffer_;
  outBuffer = outBuffer_;

  // Port Audio Initialization
  PaError err = Pa_Initialize();
  PortAudioErrorCheck(err, "in initialization.");

  // Input and Output Initialization
  inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
  inputParameters.channelCount = n_channel;
  inputParameters.sampleFormat = paFloat32;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;

  outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
  outputParameters.channelCount = n_channel;
  outputParameters.sampleFormat = paFloat32;
  //ここでエラーを返してくることがある。
  outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
  outputParameters.hostApiSpecificStreamInfo = NULL;

  std::cerr << "Audio Parameteres Initialized" << std::endl;
}

void AudioDevice::start(){
  stream = NULL;
  PaError err = Pa_OpenStream(&stream,
			      &inputParameters,
			      &outputParameters,
			      sampling_rate,
			      samples_per_buffer,
			      paNoFlag,
			      AudioDevice::CallBack,
			      this);
  PortAudioErrorCheck(err, "in Pa_OpenStream");
  err = Pa_StartStream(stream);
  PortAudioErrorCheck(err, "in Pa_StartStream");
}


void AudioDevice::stop(){
  PaError err = Pa_StopStream( stream );
  PortAudioErrorCheck(err, "in Pa_StopStream");
}


void AudioDevice::kill(){
  PaError err = Pa_CloseStream( stream );
  PortAudioErrorCheck(err, "in Pa_CloseStream");

  err = Pa_Terminate();
  PortAudioErrorCheck(err, "in Pa_Terminate");
}


// is it possible to call call this callback more frequently?
int AudioDevice::CallBack_(const void* inputBuffer,
			   void *outputBuffer,
			   unsigned long framesPerBuffer,
			   const PaStreamCallbackTimeInfo* timeInfo,
			   PaStreamCallbackFlags statisFlags)
{
			trial_count_store=trial_count;

  inBuffer->push_data(reinterpret_cast<const float*>(inputBuffer), n_channel * framesPerBuffer);
    for(int i = 0; i < (int)(n_channel * framesPerBuffer); i++){
      reinterpret_cast<float*>(outputBuffer)[i] = 0.0;
    }
  if( outBuffer->has_not_received_any_data() ){
    for(int i = 0; i < (int)(n_channel * framesPerBuffer); i++){
      reinterpret_cast<float*>(outputBuffer)[i] = 0.0;
    }
  }else{
    // resign if NNN times trials fail
		const int NNN = 30;
		trial_count = 0;
		for(int i = 0; i < NNN; i++){
			trial_count++;
			if(!outBuffer->read_data(reinterpret_cast<float *>(outputBuffer), n_channel * framesPerBuffer)){
				if(i == NNN-1){
					std::cerr << "Underflow." << std::endl;
				}
				usleep(1000); //1 [ms] -> 5[ms]? risky?
				continue;
			}
			else{
				break;
			}
		}

  }
  return 0;
}
