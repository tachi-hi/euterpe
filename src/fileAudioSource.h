// fileAudioSource.h
//
// AudioSource implementation for audio file input.
// Loads the file at start() (with mono-mix and linear resampling to target_rate),
// then feeds inBuffer directly from the PortAudio callback — no separate thread,
// no sleep-based timing.  Synchronized to the hardware audio clock.

#pragma once
#include "audioSource.h"
#include "audioPlay.h"
#include <filesystem>
#include <vector>
#include <atomic>
#include <string>

class FileAudioSource : public AudioSource {
public:
    FileAudioSource(std::filesystem::path  path,
                    int                    target_rate,
                    int                    samples_per_buffer,
                    StreamBuffer<float>&   outBuffer);

    void start(StreamBuffer<float>& inBuffer) override;
    void stop() override;

private:
    bool load();

    std::filesystem::path  path_;
    int                    target_rate_;
    int                    spb_;
    StreamBuffer<float>&   outBuffer_;

    std::vector<float>     samples_;      // whole file, mono, resampled to target_rate_
    std::atomic<size_t>    read_pos_{0};
};
