#ifndef _FFHEADER_H_
#define _FFHEADER_H_

#include <string>

extern "C"
{
//#define INT64_C
#define __STDC_LIMIT_MACROS
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

#define PIX_FMT_NONE AV_PIX_FMT_NONE
#define PixelFormat AVPixelFormat
#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO

#define av_new_stream           avformat_new_stream
#define avcodec_open            avcodec_open2
#define avcodec_decode_audio3   avcodec_decode_audio4
#define avcodec_encode_audio    avcodec_encode_audio2
#define avcodec_frame_alloc     av_frame_alloc
#define avcodec_alloc_frame     av_alloc_frame
#define avcodec_encode_video    avcodec_encode_video2

#ifndef snprintf
#define snprintf _snprintf
#endif

#ifdef FF_DLL // for win32 dll
#ifdef DLL_FILE
#   define FF_EXPORT _declspec(dllexport)
#else
#   define FF_EXPORT _declspec(dllimport)
#endif
#else
#   define FF_EXPORT
#endif


#endif // _FFHEADER_H_

