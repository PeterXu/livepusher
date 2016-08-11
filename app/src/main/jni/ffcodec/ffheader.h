#ifndef _FFHEADER_H_
#define _FFHEADER_H_

#include <string>

#ifdef __cplusplus
extern "C" {
#endif
#define __STDC_LIMIT_MACROS
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#ifdef __cplusplus
}
#endif


// define the max audio packet size as 128 KB
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)

#define PIX_FMT_NONE    AV_PIX_FMT_NONE
#define PixelFormat     AVPixelFormat

#ifndef snprintf
#define snprintf _snprintf
#endif


//> for win32 dll
#ifdef FF_DLL 
#ifdef DLL_FILE
#   define FF_EXPORT _declspec(dllexport)
#else
#   define FF_EXPORT _declspec(dllimport)
#endif
#else
#   define FF_EXPORT
#endif


#endif // _FFHEADER_H_

