#include <jni.h>
#include <queue>

#include "common.h"

#ifndef DEFINE_API
#define DEFINE_API(v, f) JNIEXPORT v JNICALL Java_com_zenvv_live_jni_PusherNative_##f
#endif


enum {
    E_NONE = 0,
    E_INIT,
    E_START,
    E_STOP,
};


static volatile int s_status = E_NONE;
static std::queue<packet_t *> s_queue;

static pthread_cond_t s_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;


static void setOptions(char *str, char oopts[][MAX_PATH]) {
    int argc = 0;
    const char delim[] = "\t ";
    char *argv = strtok(str, delim);
    while(argv != NULL && argc <= 31) {
        if (strlen(argv) < MAX_PATH)
            strcpy(oopts[argc++], argv);
        argv = strtok(NULL, delim);
    }
}
static void setMediaOptions(JNIEnv *env, jstring opts, char oopts[][MAX_PATH]) {
    const char *str = env->GetStringUTFChars(opts, 0);
    setOptions((char *)str, oopts);
    env->ReleaseStringUTFChars(opts, str);
}
static void setArgvOptions(char *argv[], int *argc, char oopts[][MAX_PATH]) {
    for (int k=0; k < 32; k++) {
        if (strlen(oopts[k]) <= 0) 
            break;

        int pos = *argc;
        argv[pos] = strdup(oopts[k]);
        *argc = pos + 1;
    }
}

DEFINE_API(void, setVideoOptions)(JNIEnv *env, jobject thiz, jstring opts) {
}
DEFINE_API(void, setAudioOptions)(JNIEnv *env, jobject thiz, jstring opts) {
}

void *main_loop(void *param) {
    s_status = E_START;

    LOGI("main_loop begin");

    do {
        pthread_mutex_lock(&s_mutex);
        pthread_cond_wait(&s_cond, &s_mutex);
        if (s_status != E_START) {
            pthread_mutex_unlock(&s_mutex);
            break;
        }

        if (s_queue.empty()) {
            pthread_mutex_unlock(&s_mutex);
            continue;
        }

        packet_t *pkt = s_queue.front();
        s_queue.pop();
        pthread_mutex_unlock(&s_mutex);

        if (pkt->type == E_VIDEO) {
        }else if (pkt->type == E_AUDIO) {
        }

        free(pkt);
    }while(s_status == E_START);

    // free queue resource
    pthread_mutex_lock(&s_mutex);
    while(!s_queue.empty()) {
        packet_t *pkt = s_queue.front();
        free(pkt);
        s_queue.pop();
    }
    pthread_mutex_unlock(&s_mutex);

    LOGI("main_loop exit");
    pthread_exit(0);
}

DEFINE_API(void, startPusher)(JNIEnv *env, jobject thiz, jstring url) {
    if (s_status == E_START || s_status == E_INIT) 
        return;

    s_status = E_INIT;

    static char stream_url[1024] = {0};
    const char *purl = env->GetStringUTFChars(url, 0);
    strcpy(stream_url, purl);
    env->ReleaseStringUTFChars(url, purl);

    strcpy(stream_url, "/sdcard/ztest_out2.flv");
    LOGI("startPusher, url: %s", stream_url);

    pthread_t tid;
    pthread_create(&tid, NULL, main_loop, stream_url);
}

DEFINE_API(void, stopPusher)(JNIEnv *env, jobject thiz) {
    if (s_status != E_START)
        return;

    LOGI("stopPusher, begin");
    pthread_mutex_lock(&s_mutex);
    s_status = E_STOP;
    pthread_cond_signal(&s_cond);
    pthread_mutex_unlock(&s_mutex);
    LOGI("stopPusher, end");
}

void fireData(JNIEnv *env, jbyteArray data, jint len, int type) {
    int pktlen = len + sizeof(packet_t);
    char* buffer = (char *)malloc(pktlen);
    memset(buffer, 0, pktlen);

    packet_t *pkt = (packet_t *)buffer;
    pkt->len = len;
    pkt->type = type;

    jbyte* bytes = env->GetByteArrayElements(data, 0);
    memcpy(pkt->data, bytes, len);
    env->ReleaseByteArrayElements(data, bytes, 0);
    
    pthread_mutex_lock(&s_mutex);
    s_queue.push(pkt);
    pthread_cond_signal(&s_cond);
    pthread_mutex_unlock(&s_mutex);
}

DEFINE_API(void, fireVideo)(JNIEnv *env, jobject thiz, jbyteArray data, jint len) {
    if (s_status != E_START)
        return;

    LOGI("fireVideo, len=%d", len);
    fireData(env, data, len, E_VIDEO);
}
DEFINE_API(void, fireAudio)(JNIEnv *env, jobject thiz, jbyteArray data, jint len) {
    if (s_status != E_START)
        return;

    LOGI("fireAudio, len=%d", len);
    fireData(env, data, len, E_AUDIO);
}

DEFINE_API(void, release)(JNIEnv *env, jobject thiz) {
    Java_com_zenvv_live_jni_PusherNative_stopPusher(env, thiz);
}

static void ffmpeg_once() {
    avcodec_register_all();
    avfilter_register_all();
    av_register_all();
}
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    ffmpeg_once();
    return JNI_VERSION_1_4;
}

