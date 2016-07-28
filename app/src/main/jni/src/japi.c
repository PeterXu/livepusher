#include <jni.h>
#include <stdio.h>
#include <pthread.h>
#include <cpu-features.h>
#include <android/log.h>

#define DEBUG
#ifdef DEBUG
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "NDK", __VA_ARGS__)
#else
#define LOGD(...)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "NDK", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  "NDK", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "NDK", __VA_ARGS__)


#define DEFINE_API(v, f) JNIEXPORT v JNICALL Java_com_zenvv_live_jni_PusherNative_##f

static int  s_argc = 1;
static char s_argv[64][32] = {{"ffmpeg\0"}};

static void setOptions(char *str) {
    char delim[] = "\t ";
    char *argv = strtok(str, delim);
    while(argv != NULL && s_argc <= 63) {
        if (strlen(argv) <= 31)
            strcpy(s_argv[s_argc++], argv);
        argv = strtok(NULL, delim);
    }
}


DEFINE_API(void, setAudioOptions)(JNIEnv *env, jobject thiz, jstring opts) {
    const char *str = (*env)->GetStringUTFChars(env, opts, 0);
    setOptions((char *)str);
    (*env)->ReleaseStringUTFChars(env, opts, str);
}
DEFINE_API(void, setVideoOptions)(JNIEnv *env, jobject thiz, jstring opts) {
    const char *str = (*env)->GetStringUTFChars(env, opts, 0);
    setOptions((char *)str);
    (*env)->ReleaseStringUTFChars(env, opts, str);
}

extern int ffmpeg_main(int argc, char **argv);
DEFINE_API(void, startPusher)(JNIEnv *env, jobject thiz, jstring url) {
    ffmpeg_main(s_argc, (char **)s_argv);
}
DEFINE_API(void, stopPusher)(JNIEnv *env, jobject thiz) {
    pid_t pid = getpid();
    if (pid > 1) {
        kill(pid, SIGINT);
    }
}

DEFINE_API(void, fireAudio)(JNIEnv *env, jobject thiz, jbyteArray data) {
}
DEFINE_API(void, fireVideo)(JNIEnv *env, jobject thiz, jbyteArray data) {
}

DEFINE_API(void, release)(JNIEnv *env, jobject thiz) {
}

