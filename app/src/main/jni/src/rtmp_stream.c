#include "common.h"


//> static variables
static jni_t 	s_jni;
static pusher_t s_pusher;

static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_cond = PTHREAD_COND_INITIALIZER;


//=============================

int checkPublishing(pusher_t *pusher) {
    if (!pusher->publishing || !pusher->readyRtmp) {
        return -1;
    }

    if (!pusher->proto.rtmp || !RTMP_IsConnected(pusher->proto.rtmp)) {
        return -1;
    }

    return 0;
}

static void rtmp_log_debug(int level, const char *format, va_list vl) {
#ifdef DEBUG
    FILE *fp = fopen("/mnt/sdcard/push_rtmp_log.txt", "a+");
    if (fp) {
        vfprintf(fp, format, vl);
        fflush(fp);
        fclose(fp);
    }
#endif
}

void throwNativeInfo(jmethodID methodId, int code) {
    JNIEnv* env = NULL;
    (*s_jni.jvm)->GetEnv(s_jni.jvm, (void**) &env, JNI_VERSION_1_4);
    if (!env) {
        LOGE("cannot get env from jvm");
        return;
    }

    if (env && methodId && s_jni.pusher_obj) {
        (*env)->CallVoidMethodA(env, s_jni.pusher_obj, methodId, (jvalue *)&code);
    }
}

void notifyNativeInfo(int level, int code) {
    switch(level) {
        case LOG_ERROR:
            return throwNativeInfo(s_jni.errorId, code);
        case LOG_STATE:
            return throwNativeInfo(s_jni.stateId, code);
        default:
            break;
    }
    return;
}

//=============================

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv* env = NULL;
    jint result = -1;

    memset(&s_jni, 0, sizeof(s_jni));
    memset(&s_pusher, 0, sizeof(s_pusher));

    s_jni.jvm = vm;
    if (vm) {
        LOGD("jvm init success");
    }

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("jvm get env error");
        return result;
    }
    return JNI_VERSION_1_4;
}

int initJavaPusherNative(JNIEnv *env) {
    if (!s_jni.pusher_obj) {
        LOGE("[%s] pusher_obj is NULL", __FUNCTION__);
        return -1;
    }

    jclass clazz = (*env)->GetObjectClass(env, s_jni.pusher_obj);
    if (!clazz) {
        LOGE("[%s] fail to get class from pusher_obj", __FUNCTION__);
        return -1;
    }

    s_jni.errorId = (*env)->GetMethodID(env, clazz, "onPostNativeError", "(I)V");
    s_jni.stateId = (*env)->GetMethodID(env, clazz, "onPostNativeState", "(I)V");
    return 0;
}

void* publiser(void *args) {

    s_pusher.publishing = 1;
    do {
        LOGE("start to RTMP_Alloc");
        s_pusher.proto.rtmp = RTMP_Alloc();
        if (!s_pusher.proto.rtmp) {
            notifyNativeInfo(LOG_ERROR, E_RTMP_ALLOC);
            goto END;
        }

        LOGI("start to RTMP_Init");
        RTMP_Init(s_pusher.proto.rtmp);
        s_pusher.proto.rtmp->Link.timeout = 7;

        LOGI("RTMP_SetupURL RTMP is: 0x%p, path: %s", s_pusher.proto.rtmp, s_pusher.proto.rtmp_path);
        if (!RTMP_SetupURL(s_pusher.proto.rtmp, s_pusher.proto.rtmp_path)) {
            notifyNativeInfo(LOG_ERROR, E_RTMP_URL);
            goto END;
        }

        // should be called before RTMP_Connect
        RTMP_EnableWrite(s_pusher.proto.rtmp);

        // connect server
        LOGI("start to RTMP_Connect");
        if (!RTMP_Connect(s_pusher.proto.rtmp, NULL)) {
            notifyNativeInfo(LOG_ERROR, E_RTMP_CONNECT);
            goto END;
        }

        // connect stream
        LOGI("start to RTMP_ConnectStream");
        if (!RTMP_ConnectStream(s_pusher.proto.rtmp, 0)) {
            notifyNativeInfo(LOG_ERROR, E_RTMP_STREAM);
            goto END;
        }

        LOGI("RTMP Loop start");
        notifyNativeInfo(LOG_ERROR, E_START);
        s_pusher.readyRtmp = 1;

        add_aac_sequence_header(s_pusher.audio.handle);
        while (s_pusher.publishing) {
            pthread_mutex_lock(&s_mutex);
            pthread_cond_wait(&s_cond, &s_mutex);
            if (!s_pusher.publishing) {
                pthread_mutex_unlock(&s_mutex);
                goto END;
            }

            RTMPPacket *packet = queue_get_first();
            if (packet) {
                queue_delete_first();
            }
            pthread_mutex_unlock(&s_mutex);

            if (packet) {
                queue_delete_first();
                packet->m_nInfoField2 = s_pusher.proto.rtmp->m_stream_id;
                int i = RTMP_SendPacket(s_pusher.proto.rtmp, packet, 1);
                if (!i) {
                    RTMPPacket_Free(packet);
                    notifyNativeInfo(LOG_ERROR, E_RTMP_SEND);
                    goto END;
                }
                RTMPPacket_Free(packet);
            }
        }

END:
        _RTMP_Close(s_pusher.proto.rtmp);
        _RTMP_Free(s_pusher.proto.rtmp);
    } while (0);


    LOGI("Publishing Thread Exit!");

    s_pusher.readyRtmp = 0;
    s_pusher.publishing = 0;
    free(s_pusher.proto.rtmp_path);
    s_pusher.proto.rtmp_path = NULL;

    int len = queue_size();
    for (int i = 0; i < len; ++i) {
        RTMPPacket * packet = queue_get_first();
        if (packet) {
            RTMPPacket_Free(packet);
        }
        queue_delete_first();
    }
    destroy_queue();

    notifyNativeInfo(LOG_STATE, E_STOP);

    (*s_jni.jvm)->DetachCurrentThread(s_jni.jvm);

    pthread_exit(0);
}

void add_rtmp_packet(RTMPPacket *packet) {
    pthread_mutex_lock(&s_mutex);
    if (s_pusher.publishing && s_pusher.readyRtmp) {
        queue_append_last(packet);
    }
    pthread_cond_signal(&s_cond);
    pthread_mutex_unlock(&s_mutex);
}

void add_aac_sequence_header(faacEncHandle handle) {
    if (!handle) {
        LOGE("[%s] invalid audio enc", __FUNCTION__);
        return;
    }

    ULONG len = 0;
    unsigned char *buf = NULL;
    int ret = faacEncGetDecoderSpecificInfo(handle, &buf, &len);
    if (ret != 0) {
        LOGE("[%s] fail to get enc info", __FUNCTION__);
        return;
    }

    RTMPPacket * packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, len + 2);
    RTMPPacket_Reset(packet);

    unsigned char * body = (unsigned char *) packet->m_body;
    /* header: 0xAF00 + AAC RAW data */
    body[0] = 0xAF;
    body[1] = 0x00;
    memcpy(&body[2], buf, len);

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = len + 2;
    packet->m_nChannel = 0x04;
    packet->m_hasAbsTimestamp = 0;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    add_rtmp_packet(packet);
    free(buf);
}

void add_264_sequence_header(unsigned char* pps, unsigned char* sps,
        int pps_len, int sps_len) {
    int body_size = 13 + sps_len + 3 + pps_len;
    RTMPPacket * packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, body_size);
    RTMPPacket_Reset(packet);

    unsigned char * body = packet->m_body;
    int i = 0;
    body[i++] = 0x17;
    body[i++] = 0x00;
    //composition time 0x000000
    body[i++] = 0x00;
    body[i++] = 0x00;
    body[i++] = 0x00;

    /*AVCDecoderConfigurationRecord*/
    body[i++] = 0x01;
    body[i++] = sps[1];
    body[i++] = sps[2];
    body[i++] = sps[3];
    body[i++] = 0xFF;

    /*sps*/
    body[i++] = 0xE1;
    body[i++] = (sps_len >> 8) & 0xff;
    body[i++] = sps_len & 0xff;
    memcpy(&body[i], sps, sps_len);
    i += sps_len;

    /*pps*/
    body[i++] = 0x01;
    body[i++] = (pps_len >> 8) & 0xff;
    body[i++] = (pps_len) & 0xff;
    memcpy(&body[i], pps, pps_len);
    i += pps_len;

    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 0x04;
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    add_rtmp_packet(packet);
}

void add_264_body(unsigned char * buf, int len) {
    /* discard header: 0x00000001*/
    if (buf[2] == 0x00) { //
        buf += 4;
        len -= 4;
    } else if (buf[2] == 0x01) { //0x000001
        buf += 3;
        len -= 3;
    }

    int body_size = len + 9;
    RTMPPacket * packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, len + 9);

    unsigned char * body = packet->m_body;
    body[0] = 0x27;

    /*key frame*/
    int type = buf[0] & 0x1f;
    if (type == NAL_SLICE_IDR) {
        body[0] = 0x17;
    }

    body[1] = 0x01; /*nal unit*/
    body[2] = 0x00;
    body[3] = 0x00;
    body[4] = 0x00;

    body[5] = (len >> 24) & 0xff;
    body[6] = (len >> 16) & 0xff;
    body[7] = (len >> 8) & 0xff;
    body[8] = (len) & 0xff;

    /*copy data*/
    memcpy(&body[9], buf, len);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = body_size;
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nChannel = 0x04;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    packet->m_nTimeStamp = RTMP_GetTime() - s_pusher.start_time;
    add_rtmp_packet(packet);
}

void add_aac_body(unsigned char * buf, int len) {
    //	encoder set outputformat = 1, ADTS's first 7 bytes; 
    //	if outputformat=0, donot discard these 7 bytes
    //	outputformat = 0
    //	buf += 7;
    //	len -= 7;

    int body_size = len + 2;
    RTMPPacket * packet = malloc(sizeof(RTMPPacket));
    RTMPPacket_Alloc(packet, body_size);

    unsigned char * body = packet->m_body;
    /* 0xAF01 + AAC RAW data */
    body[0] = 0xAF;
    body[1] = 0x01;
    memcpy(&body[2], buf, len);

    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = body_size;
    packet->m_nChannel = 0x04;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet->m_nTimeStamp = RTMP_GetTime() - s_pusher.start_time;
    add_rtmp_packet(packet);
}


/**
 * For Audio
 */

JNIEXPORT void JNICALL Java_com_zenvv_live_jni_PusherNative_setAudioOptions(
        JNIEnv *env, jobject thiz, jint sampleRate, jint channel, jint bitrate) {
    setAudioOptions(&s_pusher.audio, sampleRate, channel, bitrate);
}

JNIEXPORT void JNICALL Java_com_zenvv_live_jni_PusherNative_fireAudio(
        JNIEnv *env, jobject thiz, jbyteArray buffer, jint len) {
    if (checkPublishing(&s_pusher) != 0) {
        return;
    }

    jbyte* b_buffer = (*env)->GetByteArrayElements(env, buffer, 0);
    fireAudio(&s_pusher.audio, b_buffer, len);
    (*env)->ReleaseByteArrayElements(env, buffer, b_buffer, 0);
}

JNIEXPORT jint JNICALL Java_com_zenvv_live_jni_PusherNative_getInputSamples() {
    return s_pusher.audio.nInputSamples;
}


/**
 * For Video
 */

JNIEXPORT void JNICALL Java_com_zenvv_live_jni_PusherNative_setVideoOptions(
        JNIEnv *env, jobject thiz, jint width, jint height, jint bitrate, jint fps) {
    setVideoOptions(&s_pusher.video, width, height, bitrate, fps);
}

JNIEXPORT void JNICALL Java_com_zenvv_live_jni_PusherNative_fireVideo(
        JNIEnv *env, jobject thiz, jbyteArray buffer) {
    if (checkPublishing(&s_pusher) != 0) {
        return;
    }

    jbyte *nv21_buffer = (*env)->GetByteArrayElements(env, buffer, 0);
    fireVideo(&s_pusher.video, nv21_buffer);
    (*env)->ReleaseByteArrayElements(env, buffer, nv21_buffer, 0);
}



/**
 * control pusher
 */

void startPusher(pusher_t *pusher, const char* path) {
    LOGI("Native start Pusher");

    pusher->proto.rtmp_path = malloc(strlen(path) + 1);
    memset(pusher->proto.rtmp_path, 0, strlen(path) + 1);
    memcpy(pusher->proto.rtmp_path, path, strlen(path));
    create_queue();

    RTMP_LogSetCallback(rtmp_log_debug);

    pthread_t tid;
    pthread_create(&tid, NULL, publiser, NULL);
    pusher->start_time = RTMP_GetTime();
}

JNIEXPORT void JNICALL Java_com_zenvv_live_jni_PusherNative_startPusher(
        JNIEnv *env, jobject thiz, jstring url) {
    if (!s_jni.pusher_obj) {
        s_jni.pusher_obj = (*env)->NewGlobalRef(env, thiz);
    }

    int ret = initJavaPusherNative(env);
    if (ret != 0) {
        return;
    }

    const char* path = (*env)->GetStringUTFChars(env, url, 0);
    startPusher(&s_pusher, path);
    (*env)->ReleaseStringUTFChars(env, url, path);
}

JNIEXPORT void JNICALL Java_com_zenvv_live_jni_PusherNative_stopPusher(
        JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&s_mutex);
    s_pusher.publishing = 0;
    pthread_cond_signal(&s_cond);
    pthread_mutex_unlock(&s_mutex);
}

JNIEXPORT void JNICALL Java_com_zenvv_live_jni_PusherNative_release(
        JNIEnv *env, jobject thiz) {
    Java_com_zenvv_live_jni_PusherNative_stopPusher(env, thiz);

    releaseAudio(&s_pusher.audio);
    releaseVideo(&s_pusher.video);

    if (s_jni.pusher_obj) {
        (*env)->DeleteGlobalRef(env, s_jni.pusher_obj);
        s_jni.pusher_obj = NULL;
    }
}

