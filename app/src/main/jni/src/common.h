#ifndef _COMMON_H_
#define _COMMON_H_

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <cpu-features.h>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavformat/url.h"
#include "libavutil/log.h"
#ifdef __cplusplus
}
#endif

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


enum {
    E_AUDIO = 0,
    E_VIDEO = 1,
};

typedef struct packet_t {
    int len;
    int type;
    char data[1];
}packet_t;


#endif

