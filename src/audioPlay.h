/*******************************************************************/
// PortAudioのラッパー
// シングルトン
//
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp
/*******************************************************************/


#include <portaudio.h>
#include "streamBuffer.h"

#ifndef AUDIO_PLAY_H
#define AUDIO_PLAY_H

class AudioDevice{
 private: // Singleton Pattern
  AudioDevice();
  virtual ~AudioDevice();
  AudioDevice(const AudioDevice& tmp);
  AudioDevice& operator=(AudioDevice& tmp);
  static AudioDevice *instance;
  
 public:
  //Singleton pattern
  static AudioDevice* getInstance(void){
    if(!instance)
      instance = new AudioDevice;
    return instance;
  }
  
  void init(int, int, int, StreamBuffer<float>*, StreamBuffer<float>*);
  void start();
  void stop();
  void kill();
  
	int get_exec_count(void){int tmp = trial_count_store; return tmp;}
 private:
  // Methods which are used in this class only.
  void PortAudioErrorCheck(const PaError&, const char*);		
  
  // Callback
  static int CallBack(const void* is,
		      void* os,
		      unsigned long fpb,
		      const PaStreamCallbackTimeInfo* t,
		      PaStreamCallbackFlags f,
		      void* usrData){
    reinterpret_cast<AudioDevice*>(usrData)->CallBack_(is, os, fpb, t, f);
    return 0;
  }
  int CallBack_(const void*,
		void*,
		unsigned long,
		const PaStreamCallbackTimeInfo*,
		PaStreamCallbackFlags);
  
  // members
	int trial_count;
	int trial_count_store;
  int n_channel;
  int sampling_rate;
  int samples_per_buffer;
  
  StreamBuffer<float>* inBuffer;
  StreamBuffer<float>* outBuffer;
  
  PaStream *stream;
  PaStreamParameters inputParameters;
  PaStreamParameters outputParameters;
};

#endif
