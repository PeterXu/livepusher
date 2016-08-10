#ifndef _FFVIDEOPARAM_H_
#define _FFVIDEOPARAM_H_

#include "ffheader.h"

class FF_EXPORT FFVideoParam
{
public:
    FFVideoParam() : 
        width(0), height(0), pixelFormat(PIX_FMT_NONE), bitRate(0), frameRate(0), codecName("")
    {
    }

    FFVideoParam(int width, int height, PixelFormat pixelFormat, 
            int bitRate, int frameRate, std::string codecName) : 
        width(width), height(height), pixelFormat(pixelFormat), 
        bitRate(bitRate), frameRate(frameRate), codecName(codecName)
    {
    }

    bool isValid() {
	    return !(width < 4 || height < 4 || pixelFormat == PIX_FMT_NONE || bitRate < 1024 || frameRate < 5);
    }

public:
    int width;                  ///< The width of the video
    int height;                 ///< The height of the video
    PixelFormat pixelFormat;    ///< The pixel format of the video
    int bitRate;                ///< The bit rate of the video
    int frameRate;              ///< The frame rate of the video
    std::string codecName;      ///< The name of the video codec
};

#endif

