#include <jni.h>
#include <stdio.h>
#include <pthread.h>
#include <cpu-features.h>
#include <android/log.h>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "libavutil/log.h"

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


#ifndef DEFINE_API
#define DEFINE_API(v, f) JNIEXPORT v JNICALL Java_com_zenvv_live_jni_PusherNative_##f
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif


#define FIFO_AUDIO "/sdcard/zunix_audio"
#define FIFO_VIDEO "/sdcard/zunix_video"

enum {
    E_AUDIO = 0,
    E_VIDEO = 1,
};

enum {
    E_None = 0,
    E_INIT,
    E_START,
    E_STOP,
};


static char s_iopts[2][32][MAX_PATH] = {{{"\0"}}};
static char s_eopts[2][32][MAX_PATH] = {{{"\0"}}};
static volatile int s_pipe[2] = {-1, -1};
static volatile int s_status = E_None;


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
    const char *str = (*env)->GetStringUTFChars(env, opts, 0);
    setOptions((char *)str, oopts);
    (*env)->ReleaseStringUTFChars(env, opts, str);
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

static int initPipe(int *fd, const char *fname) {
    if (!fname)
        return -1;

    if (*fd > 0)
        return *fd;

    unlink(fname);

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        LOGE("open unix socket");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, fname);

    int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1) {
        LOGE("connect unix socket: %s", fname);
        close(sock);
        return -1;
    }

    *fd = sock;
    return *fd;
}
static void closePipe(int *fd) {
    if (*fd > 0) {
        close(*fd);
        *fd = -1;
    }
}

DEFINE_API(void, setVideoOptions)(JNIEnv *env, jobject thiz, jstring iopts, jstring eopts) {
    setMediaOptions(env, iopts, s_iopts[E_VIDEO]);
    setMediaOptions(env, eopts, s_eopts[E_VIDEO]);
}
DEFINE_API(void, setAudioOptions)(JNIEnv *env, jobject thiz, jstring iopts, jstring eopts) {
    setMediaOptions(env, iopts, s_iopts[E_AUDIO]);
    setMediaOptions(env, eopts, s_eopts[E_AUDIO]);
}

void write_y4m_header(int fd) {
    static int volatile firstVideo = 1;
    if (firstVideo) {
        firstVideo = 0;

#define Y4M_LINE_MAX 256
#define Y4M_MAGIC "YUV4MPEG2"
#define Y4M_FRAME_MAGIC "FRAME"
#define MAX_FRAME_HEADER 80

        int width=640, height=480;
        int raten=20, rated=1;
        int aspectn=0, aspectd=0;
        char inter = 'p';
        const char *colorspace = " C420jpeg XYSCSS=420JPEG";

        char buf[Y4M_LINE_MAX] = {0};
        int n = snprintf(buf, Y4M_LINE_MAX, "%s W%d H%d F%d:%d I%c A%d:%d%s\n",
                Y4M_MAGIC, width, height, raten, rated, inter,
                aspectn, aspectd, colorspace);
        LOGI("y4m header: %s", buf);
        write(fd, buf, strlen(buf));
    }
}

void get_options(int *pargc, char *argv[], char *url) {
    char tmpstr[2048] = {0};

    int argc = 0;
    argv[argc++] = strdup("ffmpeg\0");
    //argv[argc++] = strdup("-d\0");

    // set video options
    setArgvOptions(argv, &argc, s_iopts[E_VIDEO]);
    argv[argc++] = strdup("-i");
    argv[argc++] = strdup("unix:/"FIFO_VIDEO);
    //argv[argc++] = strdup("/sdcard/ffmpeg/test.y4m");
    setArgvOptions(argv, &argc, s_eopts[E_VIDEO]);

#if 0
    // set audio options
    setArgvOptions(argv, &argc, s_iopts[E_AUDIO]);
    argv[argc++] = strdup("-i");
    argv[argc++] = strdup("unix:/"FIFO_AUDIO);
    setArgvOptions(argv, &argc, s_eopts[E_AUDIO]);
#endif

    // set url
    argv[argc++] = strdup("-f");
    argv[argc++] = strdup("flv");
    argv[argc++] = strdup("-y");
    argv[argc++] = strdup(url);

    // set param num
    *pargc = argc;

    // print all params
    for (int k=0, pos=0; k < argc; k++) {
        pos += sprintf(tmpstr+pos, "%s ", argv[k]);
    }
    LOGI("==> %s", tmpstr);
}

void *main_loop(void *param) {
    if (!param)
        return NULL;

    int  argc = 0;
    char *argv[128];
    memset(argv, 0, 128*sizeof(char *));
    get_options(&argc, argv, (char *)param);

    LOGI("main_loop: run ...");
    s_status = E_START;
    extern int ffmpeg_main(int argc, char **argv);
    ffmpeg_main(argc, (char **)argv);

    for (int k=0; k < argc; k++) {
        if (argv[k]) free(argv[k]);
    }

    LOGI("main_loop: exit");
    pthread_exit(0);
}

DEFINE_API(void, startPusher)(JNIEnv *env, jobject thiz, jstring url) {
    if (s_status == E_START) 
        return;

    static char stream_url[1024] = {0};
    const char *purl = (*env)->GetStringUTFChars(env, url, 0);
    strcpy(stream_url, purl);
    (*env)->ReleaseStringUTFChars(env, url, purl);

    strcpy(stream_url, "/sdcard/ztest_out2.flv");
    LOGI("startPusher, url: %s", stream_url);
    pthread_t tid;
    pthread_create(&tid, NULL, main_loop, stream_url);
}

DEFINE_API(void, stopPusher)(JNIEnv *env, jobject thiz) {
    if (s_status != E_START)
        return;

    LOGI("stopPusher, begin");
    extern void ffmpeg_cleanup(int ret);
    ffmpeg_cleanup(0);
    s_status = E_STOP;
    LOGI("stopPusher, end");
}

static void fireData(JNIEnv *env, jbyteArray data, jint len, int fd) {
    jbyte* buffer = (*env)->GetByteArrayElements(env, data, 0);
    write(fd, buffer, len);
    (*env)->ReleaseByteArrayElements(env, data, buffer, 0);
}

DEFINE_API(void, fireVideo)(JNIEnv *env, jobject thiz, jbyteArray data, jint len) {
    if (s_status != E_START)
        return;

    int fd = initPipe(&s_pipe[E_VIDEO], FIFO_VIDEO);
    if (fd <= 0) {
        LOGE("fireVideo: cannot initPipe");
        return;
    }

    LOGI("fireVideo, len=%d", len);
    write_y4m_header(fd);

    char frame[MAX_FRAME_HEADER] = {0};
    snprintf(frame, MAX_FRAME_HEADER, "%s\n", Y4M_FRAME_MAGIC);
    write(fd, frame, strlen(frame));

    fireData(env, data, len, fd);
}
DEFINE_API(void, fireAudio)(JNIEnv *env, jobject thiz, jbyteArray data, jint len) {
    if (s_status != E_START)
        return;

    int fd = initPipe(&s_pipe[E_AUDIO], FIFO_AUDIO);
    if (fd <= 0) {
        LOGE("fireAudio: cannot initPipe");
        return;
    }

    fireData(env, data, len, fd);
}

DEFINE_API(void, release)(JNIEnv *env, jobject thiz) {
    Java_com_zenvv_live_jni_PusherNative_stopPusher(env, thiz);

    closePipe(&s_pipe[E_VIDEO]);
    closePipe(&s_pipe[E_AUDIO]);
}

void ffmpeg_once() {
    avcodec_register_all();
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    avfilter_register_all();
    av_register_all();
}
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    ffmpeg_once();
    return JNI_VERSION_1_4;
}

void log_callback_android(void *ptr, int level, const char *fmt, va_list vl)
{
    if (level == AV_LOG_QUIET)
        return;

    int level2 = ANDROID_LOG_DEBUG;
    if (level >= AV_LOG_VERBOSE)
        level2 = ANDROID_LOG_DEBUG;
    else if (level >= AV_LOG_INFO)
        level2 = ANDROID_LOG_INFO;
    else if (level >= AV_LOG_WARNING)
        level2 = ANDROID_LOG_WARN;
    else if (level >= AV_LOG_PANIC)
        level2 = ANDROID_LOG_ERROR;

    __android_log_vprint(level2, "NDK2", fmt, vl);
}

