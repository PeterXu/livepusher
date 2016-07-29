#include <jni.h>
#include <stdio.h>
#include <pthread.h>
#include <cpu-features.h>
#include <android/log.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define DEBUG
#ifdef DEBUG
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "NDK", __VA_ARGS__)
#else
#define LOGD(...)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "NDK", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  "NDK", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "NDK", __VA_ARGS__)


#ifndef DEFINE_API
#define DEFINE_API(v, f) JNIEXPORT v JNICALL Java_com_zenvv_live_jni_PusherNative_##f
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#define FIFO_AUDIO "/data/ffmpeg_pipe_audio"
#define FIFO_VIDEO "/data/ffmpeg_pipe_video"

enum {
    E_AUDIO = 0,
    E_VIDEO = 1,
};

static char s_iopts[2][32][MAX_PATH] = {{{"\0"}}};
static char s_eopts[2][32][MAX_PATH] = {{{"\0"}}};

static int s_rpipe[2] = {-1, -1};
static int s_wpipe[2] = {-1, -1};

enum {
    E_None = 0,
    E_START,
    E_STOP,
};

static volatile int s_status = E_None;


static void setOptions(char *str, char *oopts[]) {
    int argc = 0;
    const char delim[] = "\t ";
    char *argv = strtok(str, delim);
    while(argv != NULL && argc <= 31) {
        if (strlen(argv) < MAX_PATH)
            strcpy(oopts[argc++], argv);
        argv = strtok(NULL, delim);
    }
}
static void setMediaOptions(JNIEnv *env, jstring opts, char *oopts[]) {
    const char *str = (*env)->GetStringUTFChars(env, opts, 0);
    setOptions((char *)str, oopts);
    (*env)->ReleaseStringUTFChars(env, opts, str);
}
static void setArgvOptions(char *argv[], int *argc, char *oopts[]) {
    for (int k=0; k < 32; k++) {
        if (strlen(oopts[k]) <= 0) 
            break;

        int pos = *argc;
        strcpy(argv[pos], oopts[k]);
        *argc = pos + 1;
    }
}

static int initPipe(int *fd, const char *fname, int mode) {
    if (*fd > 0) {
        if(access(fname, F_OK) == -1){  
            if(mkfifo(fname, 0777) < 0) {  
                return -1;
            }
        }
        *fd = open(fname, mode);
    }
    return *fd;
}
static void closePipe(int *fd) {
    if (*fd > 0) {
        close(*fd);
        *fd = -1;
    }
}


DEFINE_API(void, setVideoOptions)(JNIEnv *env, jobject thiz, jstring iopts, jstring eopts) {
    setMediaOptions(env, iopts, (char **)s_iopts[E_VIDEO]);
    setMediaOptions(env, eopts, (char **)s_eopts[E_VIDEO]);
}
DEFINE_API(void, setAudioOptions)(JNIEnv *env, jobject thiz, jstring iopts, jstring eopts) {
    setMediaOptions(env, iopts, (char **)s_iopts[E_AUDIO]);
    setMediaOptions(env, eopts, (char **)s_eopts[E_AUDIO]);
}

extern int ffmpeg_main(int argc, char **argv);
DEFINE_API(void, startPusher)(JNIEnv *env, jobject thiz, jstring url) {
    if (s_status == E_START) 
        return;

    int  argc = 2;
    char argv[128][MAX_PATH] = {{"ffmpeg\0"}, {"-d\0"}};

    // set video options
    setArgvOptions((char **)argv, &argc, (char **)s_iopts[E_VIDEO]);
    int fd1 = initPipe(&s_rpipe[E_VIDEO], FIFO_VIDEO, O_RDONLY);
    sprintf(argv[argc++], "-i pipe:%d", fd1);
    setArgvOptions((char **)argv, &argc, (char **)s_eopts[E_VIDEO]);

    // set audio options
    setArgvOptions((char **)argv, &argc, (char **)s_iopts[E_AUDIO]);
    int fd2 = initPipe(&s_rpipe[E_AUDIO], FIFO_AUDIO, O_RDONLY);
    sprintf(argv[argc++], "-i pipe:%d", fd2);
    setArgvOptions((char **)argv, &argc, (char **)s_eopts[E_AUDIO]);

    // set url
    const char *purl = (*env)->GetStringUTFChars(env, url, 0);
    strcpy(argv[argc++], "-f");
    strcpy(argv[argc++], "flv");
    strcpy(argv[argc++], purl);
    (*env)->ReleaseStringUTFChars(env, url, purl);

    // print all params
    int pos = 0;
    char param[2048] = {0};
    for (int k=0; k < argc; k++) {
        if (strlen(argv[k]) <= 0)
            break;
        pos += sprintf(param+pos, "%s ", argv[k]);
    }
    LOGI("==> %s", param);

    ffmpeg_main(argc, (char **)argv);
    s_status = E_START;
}

DEFINE_API(void, stopPusher)(JNIEnv *env, jobject thiz) {
    if (s_status != E_START)
        return;

    pid_t pid = getpid();
    if (pid > 1) {
        kill(pid, SIGINT);
        sleep(1);
    }
    s_status = E_STOP;
}

static void fireData(JNIEnv *env, jbyteArray data, jint len, int fd) {
    jbyte* buffer = (*env)->GetByteArrayElements(env, data, 0);
    write(fd, buffer, len);
    (*env)->ReleaseByteArrayElements(env, data, buffer, 0);
}

DEFINE_API(void, fireVideo)(JNIEnv *env, jobject thiz, jbyteArray data, jint len) {
    if (s_status != E_START)
        return;

    int fd = initPipe(&s_wpipe[E_VIDEO], FIFO_VIDEO, O_WRONLY);
    if (fd > 0) {
        fireData(env, data, len, fd);
    }
}
DEFINE_API(void, fireAudio)(JNIEnv *env, jobject thiz, jbyteArray data, jint len) {
    if (s_status != E_START)
        return;

    int fd = initPipe(&s_wpipe[E_AUDIO], FIFO_AUDIO, O_WRONLY);
    if (fd > 0) {
        fireData(env, data, len, fd);
    }
}

DEFINE_API(void, release)(JNIEnv *env, jobject thiz) {
    Java_com_zenvv_live_jni_PusherNative_stopPusher(env, thiz);

    closePipe(&s_rpipe[E_AUDIO]);
    closePipe(&s_rpipe[E_VIDEO]);

    closePipe(&s_wpipe[E_AUDIO]);
    closePipe(&s_wpipe[E_VIDEO]);
}

