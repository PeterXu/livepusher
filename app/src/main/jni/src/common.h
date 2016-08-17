#ifndef __COMMON_H_
#define __COMMON_H_

#include <jni.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"


//> common
#ifndef return_if_fail
#define return_if_fail(p)       {if(!(p)) return;}
#define returnv_if_fail(p, v)   {if(!(p)) return (v);}
#define return_if(p)            {if(p) return;}
#define returnv_if(p, v)        {if(p) return (v);}
#endif

#ifndef free_malloc
#define free_malloc(p) {if(p) {free(p); (p)= NULL;}}
#endif


//> use x264
//#define HAVE_X264

#include "librtmp/rtmp.h"
#include "librtmp/log.h"
#include "faac.h"

#ifdef HAVE_X264
#include "x264.h"
#include "common/common.h"
#else
#include "wels/codec_api.h"
#endif


//> android log
#include <android/log.h>
#include <cpu-features.h>
#define DEBUG
#ifdef DEBUG
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "NDK", __VA_ARGS__)
#else
#define LOGD(...)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "NDK", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  "NDK", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "NDK", __VA_ARGS__)


//> status code
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

//> notify mark
enum {
    LOG_ERROR,
    LOG_STATE,
};


#ifndef ULONG
typedef unsigned long ULONG;
#endif


//> jni struct
typedef struct jni_t {
    JavaVM *jvm;
    jobject pusher_obj;
    jmethodID errorId;
    jmethodID stateId;
}jni_t;


//> audio
typedef struct audio_enc_t {
    faacEncHandle handle;
    ULONG nMaxOutputBytes;
    ULONG nInputSamples;
}audio_enc_t;

//> video
typedef struct video_enc_t {
    void *handle;
#ifdef HAVE_X264
    x264_picture_t *pic_in;
    x264_picture_t *pic_out;
#else
    uint8_t *pic_in;
    int pic_len;
#endif
    int y_len;
    int u_v_len;

    int width;
    int height;
    int bitrate;
    int fps;
}video_enc_t;


//> rtmp proto
#define _RTMP_Free(_rtmp)  if(_rtmp) {RTMP_Free(_rtmp); _rtmp = NULL;}
#define _RTMP_Close(_rtmp)  if(_rtmp && RTMP_IsConnected(_rtmp)) RTMP_Close(_rtmp);

typedef struct proto_net_t {
    RTMP *rtmp;
    char* rtmp_path;
}proto_net_t;


//> main entry struct
typedef struct pusher_t {
    int loop;
    int publishing;
    ULONG start_time;
    ULONG timeoffset;

    audio_enc_t audio;
    video_enc_t video;
    proto_net_t proto;
}pusher_t;


//> video methods
void fireVideo(video_enc_t *video, uint8_t *nv21_buffer);
void releaseVideo(video_enc_t *video);
int setVideoOptions(video_enc_t *video, int width, int height, int bitrate, int fps);
void forceIDR(video_enc_t *video);

//> audio methods
void fireAudio(audio_enc_t *audio, uint8_t* data, int len);
void releaseAudio(audio_enc_t *audio);
int setAudioOptions(audio_enc_t *audio, int sampleRate, int channel, int bitrate);

//> rtmp methods
void add_aac_sequence_header(faacEncHandle handle);
void add_aac_body(unsigned char * buf, int len);
void add_264_sequence_header(unsigned char* pps, unsigned char* sps, int pps_len, int sps_len);
void add_264_body(unsigned char * buf, int len);


//> misc
void notifyNativeInfo(int level, int code);


#endif // __COMMON_H_
