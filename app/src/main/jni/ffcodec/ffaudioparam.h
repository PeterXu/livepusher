#ifndef _FFAUDIOPARAM_H_
#define _FFAUDIOPARAM_H_

#include "ffheader.h"

class FF_EXPORT FFAudioParam
{
public:
    FFAudioParam() : 
        sampleFormat(AV_SAMPLE_FMT_NONE), sampleRate(0), channels(0), bitRate(0), codecName("") {
    }

    FFAudioParam(int sampleRate, int channels, int bitRate, std::string codecName) : 
        sampleRate(sampleRate), channels(channels), bitRate(bitRate), codecName(codecName) {
    }

    bool isValid() {
        return !(sampleFormat == AV_SAMPLE_FMT_NONE || sampleRate < 1024 || channels < 1 ||
                bitRate < 1024 || codecName.empty());
    }

public:
    AVSampleFormat sampleFormat;
    int sampleRate;             ///< The sample rate of the audio
    int channels;               ///< The number of audio channels
    int bitRate;                ///< The bit rate of the audio
    std::string codecName;      ///< The name of the audio codec
};

#endif
