// fileAudioSource.cpp
//
// Loads an audio file via libsndfile, resamples to target_rate (mono),
// and feeds inBuffer directly from the PortAudio callback.
// Clock source: PortAudio hardware — no separate thread, no timing drift.

#include "fileAudioSource.h"
#include <sndfile.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

FileAudioSource::FileAudioSource(std::filesystem::path path,
                                 int                   target_rate,
                                 int                   samples_per_buffer,
                                 StreamBuffer<float>&  outBuffer)
    : path_(std::move(path))
    , target_rate_(target_rate)
    , spb_(samples_per_buffer)
    , outBuffer_(outBuffer) {}

bool FileAudioSource::load() {
    SF_INFO info{};
    SNDFILE* sf = sf_open(path_.c_str(), SFM_READ, &info);
    if (!sf) {
        std::cerr << "FileAudioSource: cannot open " << path_
                  << " (" << sf_strerror(nullptr) << ")" << std::endl;
        return false;
    }

    const int       ch        = info.channels;
    const sf_count_t n_frames = info.frames;

    // Read all frames at original sample rate
    std::vector<float> raw(static_cast<size_t>(n_frames) * ch);
    sf_readf_float(sf, raw.data(), n_frames);
    sf_close(sf);

    // Interleaved → mono
    std::vector<float> mono(n_frames);
    for (sf_count_t i = 0; i < n_frames; i++) {
        float s = 0.f;
        for (int c = 0; c < ch; c++) s += raw[i * ch + c];
        mono[i] = s / ch;
    }

    // Linear resample: info.samplerate → target_rate_
    const double ratio     = static_cast<double>(info.samplerate) / target_rate_;
    const int    out_len   = static_cast<int>(n_frames / ratio);
    samples_.resize(out_len);
    for (int n = 0; n < out_len; n++) {
        const double pos = n * ratio;
        const int    i0  = static_cast<int>(pos);
        const double f   = pos - i0;
        const int    i1  = std::min(i0 + 1, static_cast<int>(n_frames) - 1);
        samples_[n] = static_cast<float>(mono[i0] * (1.0 - f) + mono[i1] * f);
    }

    std::cerr << "FileAudioSource: loaded " << path_.filename()
              << "  " << info.samplerate << " Hz " << ch << "ch → "
              << out_len / target_rate_ << "s at " << target_rate_ << " Hz" << std::endl;
    return true;
}

void FileAudioSource::start(StreamBuffer<float>& inBuffer) {
    if (!load()) return;
    read_pos_ = 0;

    auto* dev = AudioDevice::getInstance();

    // Register a synthetic input provider that the PortAudio callback calls
    // instead of reading from the hardware microphone.
    // This lambda runs on the PortAudio callback thread, so it must be fast
    // (only pointer arithmetic + push_data, no I/O or sleep).
    dev->set_input_provider([this, &inBuffer](int n) {
        const size_t pos   = read_pos_.fetch_add(n);
        const size_t total = samples_.size();
        if (pos >= total) {
            eof_.store(true);
            return;  // EOF — stop feeding; main loop handles drain + quit
        }

        const int avail = static_cast<int>(std::min<size_t>(n, total - pos));
        inBuffer.push_data(samples_.data() + pos, avail);

        if (avail < n) {
            // Last partial chunk: pad with zeros and mark EOF
            std::vector<float> pad(n - avail, 0.f);
            inBuffer.push_data(pad.data(), n - avail);
            eof_.store(true);
        }
    });

    dev->init(1, target_rate_, spb_, nullptr, &outBuffer_);
    dev->start();
}

void FileAudioSource::stop() {
    // Stop PortAudio first — Pa_StopStream waits for the last callback to return,
    // so after this the input_provider_ lambda will never be called again.
    AudioDevice::getInstance()->stop();
    AudioDevice::getInstance()->set_input_provider(nullptr);
}
