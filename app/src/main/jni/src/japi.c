#include <jni.h>

#include "common.h"

#ifndef DEFINE_API
#define DEFINE_API(v, f) JNIEXPORT v JNICALL Java_com_zenvv_live_jni_PusherNative_##f
#endif


enum {
    E_AUDIO = 0,
    E_VIDEO = 1,
};

enum {
    E_INIT = 0,
    E_START,
    E_STOP,
};


static char s_iopts[2][32][MAX_PATH] = {{{"\0"}}};
static char s_eopts[2][32][MAX_PATH] = {{{"\0"}}};

static volatile int s_status = E_INIT;
static pthread_cond_t s_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

extern URLProtocol ff_unix_protocol;

typedef struct UnixContext {
    const AVClass *class;
    struct sockaddr_un addr;
    int timeout;
    int listen;
    int type;
    int fd;
} UnixContext;

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
        //const char *colorspace = " C420mpeg2 XYSCSS=420MPEG2";
        //const char *colorspace = " C420paldv XYSCSS=420PALDV";

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
    argv[argc++] = strdup("-listen");
    argv[argc++] = strdup("1");
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

void *ff_loop(void *param) {
    if (!param)
        return NULL;

    int  argc = 0;
    char *argv[128];
    memset(argv, 0, 128*sizeof(char *));
    get_options(&argc, argv, (char *)param);

    LOGI("ff_loop: run ...");
    s_status = E_START;
    extern int ffmpeg_main(int argc, char **argv);
    ffmpeg_main(argc, (char **)argv);

    for (int k=0; k < argc; k++) {
        if (argv[k]) free(argv[k]);
    }

    LOGI("ff_loop: exit");
    pthread_exit(0);
}

void *net_loop(void *param) {
    // init unix protocol with server mode
    UnixContext ctx;
    memset(&ctx, 0, sizeof(UnixContext));
    ctx.listen = 1;
    ctx.type = SOCK_STREAM;

    URLContext ff_context;
    memset(&ff_context, 0, sizeof(URLContext));
    ff_context.priv_data = &ctx;
    ff_context.flags = 0;

    s_status = E_START;

    LOGI("net_loop begin");

    do {
        int ret = ff_unix_protocol.url_open(&ff_context, FIFO_VIDEO, 0);
        if (ret < 0) {
            LOGE("net_loop, url_open fail ret=%d", ret);
            break;
        }

        LOGI("net_loop, recv one connection");
        do {
            pthread_mutex_lock(&s_mutex);
            pthread_cond_wait(&s_cond, &s_mutex);
            if (s_status != E_START) {
                pthread_mutex_unlock(&s_mutex);
                break;
            }

            packet_t *pkt = queue_get_first();
            if (pkt) {
                queue_delete_first();
            }
            pthread_mutex_unlock(&s_mutex);

            if (pkt) {
                ret = ff_unix_protocol.url_write(&ff_context, pkt->data, pkt->len);
                LOGI("net_loop, url_write ret=%d", ret);
                free(pkt);
            }
        } while(1);

        ff_unix_protocol.url_close(&ff_context);
    }while(s_status == E_START);

    destroy_queue();

    LOGI("net_loop exit");
    pthread_exit(0);
}

DEFINE_API(void, startPusher)(JNIEnv *env, jobject thiz, jstring url) {
    if (s_status == E_START) 
        return;

    create_queue();
    pthread_t tid1;
    pthread_create(&tid1, NULL, net_loop, NULL);

#if 0
    static char stream_url[1024] = {0};
    const char *purl = (*env)->GetStringUTFChars(env, url, 0);
    strcpy(stream_url, purl);
    (*env)->ReleaseStringUTFChars(env, url, purl);

    strcpy(stream_url, "/sdcard/ztest_out2.flv");
    LOGI("startPusher, url: %s", stream_url);
    pthread_t tid;
    pthread_create(&tid, NULL, ff_loop, stream_url);
#endif
}

DEFINE_API(void, stopPusher)(JNIEnv *env, jobject thiz) {
    if (s_status != E_START)
        return;
    s_status = E_STOP;

    LOGI("stopPusher, begin");
    pthread_mutex_lock(&s_mutex);
    pthread_cond_signal(&s_cond);
    pthread_mutex_unlock(&s_mutex);

    ffmpeg_cleanup(0);
    LOGI("stopPusher, end");
}

DEFINE_API(void, fireVideo)(JNIEnv *env, jobject thiz, jbyteArray data, jint len) {
    if (s_status != E_START)
        return;

    LOGI("fireVideo, len=%d", len);
    char* buffer = malloc(len+16);
    memset(buffer, 0, len+16);
    packet_t *pkt = (packet_t *)buffer;

    int ret = sprintf(pkt->data, "%s\n", Y4M_FRAME_MAGIC);
    jbyte* bytes = (*env)->GetByteArrayElements(env, data, 0);
    memcpy(pkt->data+ret, bytes, len);
    (*env)->ReleaseByteArrayElements(env, data, bytes, 0);
    pkt->len = ret + len;
    
    //fireData(env, data, len, fd);
    pthread_mutex_lock(&s_mutex);
    if (s_status == E_START) {
        queue_append_last(pkt);
    }
    pthread_cond_signal(&s_cond);
    pthread_mutex_unlock(&s_mutex);
}
DEFINE_API(void, fireAudio)(JNIEnv *env, jobject thiz, jbyteArray data, jint len) {
    if (s_status != E_START)
        return;

    //fireData(env, data, len, fd);
}

DEFINE_API(void, release)(JNIEnv *env, jobject thiz) {
    Java_com_zenvv_live_jni_PusherNative_stopPusher(env, thiz);
}

static void ffmpeg_once() {
    avcodec_register_all();
#if CONFIG_AVDEVICE
    avdevice_register_all();
#endif
    avfilter_register_all();
    av_register_all();
}
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    //ffmpeg_once();
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

