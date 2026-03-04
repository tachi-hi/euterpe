// lineAudioSource.h
//
// AudioSource implementation for real-time microphone/line-in input via PortAudio.
// Wraps AudioDevice (full-duplex mode).

#pragma once
#include "audioSource.h"
#include "audioPlay.h"

class LineAudioSource : public AudioSource {
public:
    LineAudioSource(int sampling_rate, int samples_per_buffer,
                    StreamBuffer<float>& outBuffer)
        : device_(AudioDevice::getInstance())
        , sr_(sampling_rate)
        , spb_(samples_per_buffer)
        , outBuffer_(outBuffer) {}

    void start(StreamBuffer<float>& inBuffer) override {
        device_->init(1, sr_, spb_, &inBuffer, &outBuffer_);
        device_->start();
    }

    void stop() override {
        device_->stop();
    }

private:
    AudioDevice*          device_;
    int                   sr_;
    int                   spb_;
    StreamBuffer<float>&  outBuffer_;
};
