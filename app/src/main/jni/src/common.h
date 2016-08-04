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

#include "libavutil/log.h"
#include "libavformat/url.h"

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

#define FIFO_AUDIO "/sdcard/ffmpeg/zunix_audio"
#define FIFO_VIDEO "/sdcard/ffmpeg/zunix_video"

extern int      create_queue();
extern int      destroy_queue();

extern int      queue_size();
extern int      queue_append_last(void *pval);
extern void*    queue_get_first();
extern int      queue_delete_first();

typedef struct packet_t {
    int len;
    char data[1];
}packet_t;

extern void ffmpeg_cleanup(int ret);

#endif

