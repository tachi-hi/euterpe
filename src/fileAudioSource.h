// fileAudioSource.h
//
// AudioSource implementation for WAV file input.
// Reads an audio file via libsndfile and feeds inBuffer at the target
// sampling rate in real time using std::jthread.
// Mono conversion and linear resampling are applied as needed.

#pragma once
#include "audioSource.h"
#include "audioPlay.h"
#include <filesystem>
#include <thread>
#include <string>

class FileAudioSource : public AudioSource {
public:
    // outBuffer is used for PortAudio output-only setup.
    FileAudioSource(std::filesystem::path  path,
                    int                    target_rate,
                    int                    samples_per_buffer,
                    StreamBuffer<float>&   outBuffer);

    void start(StreamBuffer<float>& inBuffer) override;
    void stop() override;

private:
    void feed_loop(std::stop_token st, StreamBuffer<float>* inBuf);

    std::filesystem::path  path_;
    int                    target_rate_;
    int                    spb_;
    StreamBuffer<float>&   outBuffer_;
    std::jthread           thread_;
};
