// audioPlay.h
//
// PortAudio wrapper (Singleton).
// Supports full-duplex (line-in + out) and output-only mode (for file input).
// Pass nullptr for inBuffer to init() to enable output-only mode.
//
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp

#pragma once
#include <portaudio.h>
#include "streamBuffer.h"

class AudioDevice {
private: // Singleton Pattern (Meyers' Singleton)
    AudioDevice();
    ~AudioDevice();
    AudioDevice(const AudioDevice&)            = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;

public:
    static AudioDevice* getInstance() {
        static AudioDevice instance;
        return &instance;
    }

    // Pass inBuffer=nullptr to enable output-only mode (e.g., for file input).
    void init(int n_ch, int sr, int spb,
              StreamBuffer<float>* inBuffer,
              StreamBuffer<float>* outBuffer);
    void start();
    void stop();
    void kill();

    int get_exec_count() { int tmp = trial_count_store; trial_count = 0; trial_count_store = 0; return tmp; }

private:
    void PortAudioErrorCheck(const PaError&, const char*);

    static int CallBack(const void* is, void* os,
                        unsigned long fpb,
                        const PaStreamCallbackTimeInfo* t,
                        PaStreamCallbackFlags f, void* ud) {
        reinterpret_cast<AudioDevice*>(ud)->CallBack_(is, os, fpb, t, f);
        return 0;
    }
    int CallBack_(const void*, void*, unsigned long,
                  const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags);

    int  trial_count{0};
    int  trial_count_store{0};
    int  n_channel{1};
    int  sampling_rate{16000};
    int  samples_per_buffer{1024};
    bool input_enabled_{true};  // false = output-only mode

    StreamBuffer<float>* inBuffer{nullptr};
    StreamBuffer<float>* outBuffer{nullptr};

    PaStream*           stream{nullptr};
    PaStreamParameters  inputParameters{};
    PaStreamParameters  outputParameters{};
};
