#ifndef _COMMON_H_
#define _COMMON_H_


#include <android/log.h>

#define TAG "NDK1"

#define DEBUG
#ifdef DEBUG
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#else
#define LOGD(...)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#define FIFO_AUDIO "/data/ffmpeg/zunix_audio"
#define FIFO_VIDEO "/data/ffmpeg/zunix_video"

int initPipe(int *fd, const char *fname);
void closePipe(int *fd);

#endif

