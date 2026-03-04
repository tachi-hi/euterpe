// audioSource.h
//
// Abstract interface for audio input sources.
// Implementations: LineAudioSource (PortAudio mic), FileAudioSource (WAV file).

#pragma once
#include "streamBuffer.h"

class AudioSource {
public:
    virtual void start(StreamBuffer<float>& inBuffer) = 0;
    virtual void stop() = 0;
    virtual bool is_eof() const { return false; }
    virtual ~AudioSource() = default;
};
