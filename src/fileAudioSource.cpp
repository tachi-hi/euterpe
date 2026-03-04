// fileAudioSource.cpp
//
// Reads an audio file via libsndfile and feeds inBuffer at target_rate
// in real time. Supports mono conversion and linear resampling.
//
// Timing: sleep_until on an absolute clock to prevent cumulative drift
// versus the PortAudio hardware clock.

#include "fileAudioSource.h"
#include <sndfile.h>
#include <iostream>
#include <vector>
#include <chrono>
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
        std::cerr << "FileAudioSource: cannot open " << path_
                  << " (" << sf_strerror(nullptr) << ")" << std::endl;
        return;
    }

    const int    ch        = info.channels;
    const double src_rate  = static_cast<double>(info.samplerate);
    const double ratio     = src_rate / static_cast<double>(target_rate_);
    const int    chunk     = spb_;
    const auto   chunk_dur = std::chrono::duration<double>(
                                 static_cast<double>(chunk) / target_rate_);

    // mono_buf holds carry-over from the previous tick plus newly read samples.
    // Worst case per tick: ceil(chunk * ratio) + carry(≤2) + 2 headroom
    const int buf_max = static_cast<int>(std::ceil(chunk * ratio)) + 8;
    std::vector<float> file_buf(buf_max * ch);
    std::vector<float> mono_buf(buf_max);
    std::vector<float> out_chunk(chunk);

    int    carry_n = 0;   // valid samples already in mono_buf[0..carry_n-1]
    double frac    = 0.0; // fractional source-position carry-over [0, 1)

    std::cerr << "FileAudioSource: playing " << path_.filename()
              << " (" << info.samplerate << " Hz, " << ch << "ch)" << std::endl;

    // Absolute timeline — prevents cumulative drift vs PortAudio hardware clock.
    // Each tick advances by exactly chunk_dur regardless of processing time.
    auto next_tick = std::chrono::steady_clock::now() + chunk_dur;

    while (!st.stop_requested()) {
        // How many new source samples are needed this tick?
        // We already have carry_n samples in mono_buf[0..carry_n-1].
        const int total_needed = static_cast<int>(frac + chunk * ratio) + 2;
        const int new_needed   = std::max(0, total_needed - carry_n);
        const int to_read      = std::min(new_needed, buf_max - carry_n);

        sf_count_t got = 0;
        if (to_read > 0) {
            got = sf_readf_float(sf, file_buf.data(), to_read);
            if (got == 0) break;  // EOF

            // Interleaved → mono (carry_n samples already occupy mono_buf[0..carry_n-1])
            for (int i = 0; i < static_cast<int>(got); i++) {
                float s = 0.f;
                for (int c = 0; c < ch; c++) s += file_buf[i * ch + c];
                mono_buf[carry_n + i] = s / ch;
            }
        }
        const int total_avail = carry_n + static_cast<int>(got);

        // Linear resample: src_rate → target_rate
        double pos = frac;
        for (int n = 0; n < chunk; n++) {
            const int    i0 = static_cast<int>(pos);
            const double f  = pos - i0;
            const int    i1 = std::min(i0 + 1, total_avail - 1);
            out_chunk[n] = static_cast<float>(mono_buf[i0] * (1.0 - f) + mono_buf[i1] * f);
            pos += ratio;
        }

        // Carry-over: keep unconsumed whole samples and the fractional offset.
        // consumed = number of source samples fully indexed by the resampler.
        const int consumed = static_cast<int>(pos);
        frac    = pos - consumed;
        carry_n = std::max(0, total_avail - consumed);
        if (carry_n > 0)
            std::copy_n(mono_buf.data() + consumed, carry_n, mono_buf.data());

        inBuf->push_data(out_chunk.data(), chunk);

        // Sleep until the next absolute tick.
        // Unlike sleep_for, this does not accumulate drift when a tick runs long.
        std::this_thread::sleep_until(next_tick);
        next_tick += chunk_dur;
    }

    sf_close(sf);
    std::cerr << "FileAudioSource: playback finished." << std::endl;
}
