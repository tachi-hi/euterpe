// fileAudioSource.cpp
//
// Reads an audio file via libsndfile and feeds inBuffer at target_rate
// in real time. Supports mono conversion and linear resampling.

#include "fileAudioSource.h"
#include <sndfile.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>

FileAudioSource::FileAudioSource(std::filesystem::path path,
                                 int                   target_rate,
                                 int                   samples_per_buffer,
                                 StreamBuffer<float>&  outBuffer)
    : path_(std::move(path))
    , target_rate_(target_rate)
    , spb_(samples_per_buffer)
    , outBuffer_(outBuffer) {}

void FileAudioSource::start(StreamBuffer<float>& inBuffer) {
    // Init PortAudio in output-only mode (nullptr = no hardware input)
    auto* dev = AudioDevice::getInstance();
    dev->init(1, target_rate_, spb_, nullptr, &outBuffer_);
    dev->start();

    // Start file feeder thread
    thread_ = std::jthread([this, &inBuffer](std::stop_token st) {
        feed_loop(st, &inBuffer);
    });
}

void FileAudioSource::stop() {
    thread_.request_stop();
    thread_.join();
    AudioDevice::getInstance()->stop();
}

void FileAudioSource::feed_loop(std::stop_token st, StreamBuffer<float>* inBuf) {
    SF_INFO info{};
    SNDFILE* sf = sf_open(path_.c_str(), SFM_READ, &info);
    if (!sf) {
        std::cerr << "FileAudioSource: cannot open " << path_ << std::endl;
        return;
    }

    const int    ch          = info.channels;
    const double src_rate    = static_cast<double>(info.samplerate);
    const double ratio       = src_rate / static_cast<double>(target_rate_);
    const int    chunk       = spb_;                          // output samples per tick
    const auto   chunk_dur   = std::chrono::duration<double>(
                                   static_cast<double>(chunk) / target_rate_);

    // Read buffer: interleaved samples from file
    const int read_max = static_cast<int>(std::ceil(chunk * ratio)) + 2;
    std::vector<float> file_buf(read_max * ch);
    std::vector<float> mono_buf(read_max);
    std::vector<float> out_chunk(chunk);

    double src_pos = 0.0;   // fractional read position in current mono_buf

    std::cerr << "FileAudioSource: playing " << path_.filename()
              << " (" << info.samplerate << " Hz, " << ch << "ch)" << std::endl;

    while (!st.stop_requested()) {
        auto tick_start = std::chrono::steady_clock::now();

        // Number of source samples needed for this output chunk
        const int needed = static_cast<int>(std::ceil(src_pos + chunk * ratio)) + 1;
        const int to_read = std::min(needed, read_max);

        sf_count_t got = sf_readf_float(sf, file_buf.data(), to_read);
        if (got == 0) break;  // EOF

        // Interleaved → mono
        for (int i = 0; i < static_cast<int>(got); i++) {
            float s = 0.f;
            for (int c = 0; c < ch; c++) s += file_buf[i * ch + c];
            mono_buf[i] = s / ch;
        }

        // Linear resample: src_rate → target_rate
        for (int n = 0; n < chunk; n++) {
            const int    i0 = static_cast<int>(src_pos);
            const double f  = src_pos - i0;
            const int    i1 = std::min(i0 + 1, static_cast<int>(got) - 1);
            out_chunk[n] = static_cast<float>(mono_buf[i0] * (1.0 - f) + mono_buf[i1] * f);
            src_pos += ratio;
        }
        // Carry over fractional position relative to next read
        src_pos -= static_cast<int>(got);
        if (src_pos < 0.0) src_pos = 0.0;

        inBuf->push_data(out_chunk.data(), chunk);

        // Sleep for remaining time in this tick
        auto elapsed   = std::chrono::steady_clock::now() - tick_start;
        auto remaining = chunk_dur - elapsed;
        if (remaining > std::chrono::duration<double>(0))
            std::this_thread::sleep_for(remaining);
    }

    sf_close(sf);
    std::cerr << "FileAudioSource: playback finished." << std::endl;
}
