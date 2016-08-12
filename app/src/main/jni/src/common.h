#ifndef __COMMON_H_
#define __COMMON_H_

#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <queue.h>
#include <pthread.h>
#include <cpu-features.h>

#define HAVE_X264

#include "librtmp/rtmp.h"
#include "librtmp/log.h"

#include "faac.h"

#ifdef HAVE_X264
#include "x264.h"
#include "common/common.h"
#endif

#define DEBUG
#ifdef DEBUG
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "NDK", __VA_ARGS__)
#else
#define LOGD(...)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "NDK", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  "NDK", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "NDK", __VA_ARGS__)

enum {
    E_START     = 100,
    E_STOP      = 101,

    E_AUDIO_ENCODER     = -102,
    E_VIDEO_ENCODER     = -103,

    E_RTMP_ALLOC        = -104,
    E_RTMP_URL          = -105,
    E_RTMP_CONNECT      = -106,
    E_RTMP_STREAM       = -107,
    E_RTMP_SEND         = -108,
};


#ifndef ULONG
typedef unsigned long ULONG;
#endif

typedef struct jni_t {
    JavaVM *jvm;
    jobject pusher_obj;
    jmethodID errorId;
    jmethodID stateId;
}jni_t;

// audio
typedef struct audio_enc_t {
    faacEncHandle handle;
    ULONG nMaxOutputBytes;
    ULONG nInputSamples;
}audio_enc_t;

// video
typedef struct video_enc_t {
    x264_t *handle;
    x264_picture_t *pic_in;
    x264_picture_t *pic_out;
    int y_len;
    int u_v_len;

    int width;
    int height;
    int bitrate;
    int fps;
}video_enc_t;

// rtmp
#define _RTMP_Free(_rtmp)  if(_rtmp) {RTMP_Free(_rtmp); _rtmp = NULL;}
#define _RTMP_Close(_rtmp)  if(_rtmp && RTMP_IsConnected(_rtmp)) RTMP_Close(_rtmp);

typedef struct proto_net_t {
    RTMP *rtmp;
    char* rtmp_path;
}proto_net_t;


typedef struct pusher_t {
    int status;
    int publishing;
    int readyRtmp;
    ULONG start_time;
    ULONG timeoffset;

    audio_enc_t audio;
    video_enc_t video;
    proto_net_t proto;
}pusher_t;


#endif
