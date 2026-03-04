// audioPlay.cpp
//
// PortAudio wrapper (Singleton) implementation.
//
// (c) 2010 Aug. Hideyuki Tachibana. tachibana@hil.t.u-tokyo.ac.jp

#include "audioPlay.h"
#include "debug.h"
#include <iostream>
#include <cstdlib>
#include <unistd.h>

AudioDevice::AudioDevice() {}
AudioDevice::~AudioDevice() {}

void AudioDevice::PortAudioErrorCheck(const PaError& err, const char* msg) {
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        std::cerr << msg << std::endl;
        exit(1);
    }
}

void AudioDevice::init(int n_ch, int sr, int spb,
                       StreamBuffer<float>* inBuf,
                       StreamBuffer<float>* outBuf) {
    n_channel          = n_ch;
    sampling_rate      = sr;
    samples_per_buffer = spb;
    inBuffer           = inBuf;
    outBuffer          = outBuf;
    input_enabled_     = (inBuf != nullptr);

    PaError err = Pa_Initialize();
    PortAudioErrorCheck(err, "in initialization.");

    if (input_enabled_) {
        inputParameters.device            = Pa_GetDefaultInputDevice();
        inputParameters.channelCount      = n_channel;
        inputParameters.sampleFormat      = paFloat32;
        inputParameters.suggestedLatency  =
            Pa_GetDeviceInfo(inputParameters.device)->defaultHighInputLatency;
        inputParameters.hostApiSpecificStreamInfo = nullptr;
    }

    outputParameters.device           = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount     = n_channel;
    outputParameters.sampleFormat     = paFloat32;
    outputParameters.suggestedLatency =
        Pa_GetDeviceInfo(outputParameters.device)->defaultHighOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;

    std::cerr << "Audio Parameters Initialized"
              << (input_enabled_ ? " (full-duplex)" : " (output-only)") << std::endl;
}

void AudioDevice::start() {
    stream = nullptr;
    PaError err = Pa_OpenStream(
        &stream,
        input_enabled_ ? &inputParameters : nullptr,
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

void AudioDevice::stop() {
    PaError err = Pa_StopStream(stream);
    PortAudioErrorCheck(err, "in Pa_StopStream");
}

void AudioDevice::kill() {
    PaError err = Pa_CloseStream(stream);
    PortAudioErrorCheck(err, "in Pa_CloseStream");
    err = Pa_Terminate();
    PortAudioErrorCheck(err, "in Pa_Terminate");
}

int AudioDevice::CallBack_(const void* inputBuffer, void* outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo*,
                           PaStreamCallbackFlags) {
    trial_count_store = trial_count++;
    const auto n = static_cast<int>(n_channel * framesPerBuffer);

    // Feed pipeline input: file provider takes priority over hardware mic
    if (input_provider_) {
        input_provider_(n);
    } else if (input_enabled_ && inBuffer) {
        inBuffer->push_data(reinterpret_cast<const float*>(inputBuffer), n);
    }

    // Fill output from pipeline
    auto* out = reinterpret_cast<float*>(outputBuffer);
    for (int i = 0; i < n; i++) out[i] = 0.0f;

    if (!outBuffer->has_not_received_any_data()) {
        // CV で待機（最大 30ms）。タイムアウト時はゼロ埋めのまま返す。
        if (!outBuffer->wait_and_read(out, n, std::chrono::milliseconds(30))) {
            std::cerr << "Underflow." << std::endl;
        }
    }
    return 0;
}
