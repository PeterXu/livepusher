#include <jni.h>
#include <android/log.h>

#include "rtmp.h"
#include "x264.h"
#include "faac.h"
#include <stdio.h>
#include <queue.h>
#include <pthread.h>
// #include <sys/time.h>


//#define DEBUG
#ifdef DEBUG
#define LOGD(...) __android_log_print(3,"NDK",__VA_ARGS__)
#define LOGI(...) __android_log_print(4,"NDK",__VA_ARGS__)
#define LOGE(...) __android_log_print(5,"NDK",__VA_ARGS__)
#else
#define LOGD(...)
#define LOGI(...)
#define LOGE(...)
#endif

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
	int publishing;
	int readyRtmp;
	ULONG start_time;
	ULONG timeoffset;

	audio_enc_t audio;
	video_enc_t video;
	proto_net_t proto;
}pusher_t;

static jni_t 	s_jni;
static pusher_t s_pusher;

static pthread_t s_tid;
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_cond = PTHREAD_COND_INITIALIZER;




void add_aac_sequence_header(audio_enc_t *audio);

//===============================================

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
        return;
    }

	if (env && methodId && s_jni.pusher_obj) {
		(*env)->CallVoidMethodA(env, s_jni.pusher_obj, methodId, (jvalue *)&code);
	}
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
	JNIEnv* env = NULL;
	jint result = -1;
	s_jni.jvm = vm;
	if (vm) {
		LOGD("jvm init success");
	}

	if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		return result;
	}
	return JNI_VERSION_1_4;
}

void* publiser(void *args) {
	JNIEnv *env;
	(*s_jni.jvm)->AttachCurrentThread(s_jni.jvm, &env, 0);
	if (env == NULL) {
		LOGE("Native Attach Thread");
		return NULL;
	}

	if (!s_jni.pusher_obj) {
		LOGE("Native found Java Pubulisher Object");
		return NULL;
	}

	jclass clazz = (*env)->GetObjectClass(env, s_jni.pusher_obj);
	LOGI("Native found Java Pubulisher Class :%d", clazz?1:0);
	s_jni.errorId = (*env)->GetMethodID(env, clazz, "onPostNativeError", "(I)V");
	s_jni.stateId = (*env)->GetMethodID(env, clazz, "onPostNativeState", "(I)V");

	s_pusher.publishing = 1;
	do {
		s_pusher.proto.rtmp = RTMP_Alloc();
		if (!s_pusher.proto.rtmp) {
			LOGD("rtmp alloc failed");
			throwNativeInfo(s_jni.errorId, -104);
			goto END;
		}

		RTMP_Init(s_pusher.proto.rtmp);
		LOGI("RTMP_Init success");

		s_pusher.proto.rtmp->Link.timeout = 5;

		LOGI("RTMP_SetupURL RTMP is :%d, path:%s", s_pusher.proto.rtmp?1:0, s_pusher.proto.rtmp_path);
		if (!RTMP_SetupURL(s_pusher.proto.rtmp, s_pusher.proto.rtmp_path)) {
			LOGD("RTMP_SetupURL() failed!");
			throwNativeInfo(s_jni.errorId, -104);
			goto END;
		}

		// should be called before RTMP_Connect
		RTMP_EnableWrite(s_pusher.proto.rtmp);

		// connect server
		if (!RTMP_Connect(s_pusher.proto.rtmp, NULL)) {
			LOGD("RTMP_Connect() failed!");
			throwNativeInfo(s_jni.errorId, -104);
			goto END;
		}

		// connect stream
		if (!RTMP_ConnectStream(s_pusher.proto.rtmp, 0)) {
			LOGD("RTMP_ConnectStream() failed!");
			throwNativeInfo(s_jni.errorId, -104);
			goto END;
		}

		int tmp = 5;
		LOGI("RTMP Loop start");
		throwNativeInfo(s_jni.stateId, 100);
		s_pusher.readyRtmp = 1;

		add_aac_sequence_header(&s_pusher.audio);
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
					LOGD("RTMP_SendPacket failed");
					throwNativeInfo(s_jni.errorId, -104);
					goto END;
				}
				RTMPPacket_Free(packet);
			}
		}

END:
		_RTMP_Close(s_pusher.proto.rtmp);
		_RTMP_Free(s_pusher.proto.rtmp);
	} while (0);


	s_pusher.readyRtmp = 0;
	s_pusher.publishing = 0;
	LOGD("Exit Thread");

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

	//pthread_mutex_destroy(&mutex);
	//pthread_cond_destroy(&cond);
	throwNativeInfo(s_jni.stateId, 101);
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

void add_aac_sequence_header(audio_enc_t *audio) {
	if (!audio || !audio->handle) {
		return;
	}

	ULONG len;
	unsigned char *buf;
	faacEncGetDecoderSpecificInfo(audio->handle, &buf, &len);

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
	int type = buf[0] & 0x1f;
	/*key frame*/
	body[0] = 0x27;
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
	//packet->m_nTimeStamp = -1;
	packet->m_nTimeStamp = RTMP_GetTime() - s_pusher.start_time;
	add_rtmp_packet(packet);
}

void add_aac_body(unsigned char * buf, int len) {
	//	encoder set outputformat = 1, ADTS's first 7 bytes; if outputformat=0, donot discard these 7 bytes
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
	//packet->m_nTimeStamp = -1;
	packet->m_nTimeStamp = RTMP_GetTime() - s_pusher.start_time;
	add_rtmp_packet(packet);
}

static void x264_log_default2(void *p_unused, int i_level, const char *psz_fmt, va_list arg) {
	FILE *fp = fopen("mnt/sdcard/x264_log.txt", "a+");
	if (fp) {
		vfprintf(fp, psz_fmt, arg);
		fflush(fp);
		fclose(fp);
	}
}

void setVideoOptions(video_enc_t *video, jint width, jint height, jint bitrate, jint fps) {
	LOGD("Video options: %dx%d, %dkb/s, %d", width, height, bitrate/1000, fps);
	x264_param_t param;
	if (video->handle) {
		LOGD("Video encoder has been opened");
		if (video->fps != fps || video->bitrate != bitrate || video->height != height || video->width != width) {
			x264_encoder_close(video->handle);
			video->handle = 0;
			x264_free(video->pic_in);
			x264_free(video->pic_out);
		} else {
			return;
		}
	}

	video->width = width;
	video->height = height;
	video->bitrate = bitrate;
	video->fps = fps;
	video->y_len = width * height;
	video->u_v_len = video->y_len / 4;
	x264_param_default_preset(&param, "ultrafast", "zerolatency");

	//param.pf_log = x264_log_default2;
	param.i_level_idc = 52; 	// base_line: 5.2
	param.i_csp = X264_CSP_I420;
	param.i_width = width;
	param.i_height = height;
	param.i_bframe = 0;
	param.rc.i_rc_method = X264_RC_ABR;
	param.rc.i_bitrate = bitrate / 1000;
	param.rc.i_vbv_max_bitrate = bitrate / 1000 * 1.2;
	param.rc.i_vbv_buffer_size = bitrate / 1000;
	param.b_repeat_headers = 1;
	param.i_fps_num = fps;
	param.i_fps_den = 1;
	param.b_vfr_input = 0;
	param.i_keyint_max = fps * 2;
	param.i_threads = 1;
	param.i_timebase_den = param.i_fps_num;
	param.i_timebase_num = param.i_fps_den;

	//baseline
	x264_param_apply_profile(&param, "baseline");
	LOGD("x264 param: %s", x264_param2string(&param,1));
	video->handle = x264_encoder_open(&param);
	if (!video->handle) {
		LOGI("Video encoder open fail");
		throwNativeInfo(s_jni.errorId, -103);
		return;
	}

	video->pic_in = malloc(sizeof(x264_picture_t));
	video->pic_out = malloc(sizeof(x264_picture_t));
	x264_picture_alloc(video->pic_in, X264_CSP_I420, video->width, video->height);
}

JNIEXPORT void JNICALL Java_com_zenvv_live_jni_PusherNative_setVideoOptions(
		JNIEnv *env, jobject thiz, jint width, jint height, jint bitrate,
		jint fps) {
	setVideoOptions(&s_pusher.video, width, height, bitrate, fps);
}


void setAudioOptions(audio_enc_t *audio, jint sampleRate, jint channel) {
	if (audio->handle) {
		LOGD("audio encoder has been opened");
		return;
	}

	unsigned long nChannels = channel;
	int nSampleRate = sampleRate;
	LOGD("audio options:%d %d", nSampleRate, nChannels);
	audio->handle = faacEncOpen(nSampleRate, nChannels, &audio->nInputSamples, &audio->nMaxOutputBytes);
	if (!audio->handle) {
		LOGI("audio encoder open fail");
		throwNativeInfo(s_jni.errorId, -102);
		return;
	}

	faacEncConfigurationPtr pConfiguration = faacEncGetCurrentConfiguration(audio->handle);
	pConfiguration->mpegVersion = MPEG4;
	pConfiguration->allowMidside = 1;
	pConfiguration->aacObjectType = LOW;
	pConfiguration->outputFormat = 0;
	pConfiguration->useTns = 1;
	pConfiguration->useLfe = 0;
	pConfiguration->inputFormat = FAAC_INPUT_16BIT;
	pConfiguration->quantqual = 100;
	pConfiguration->bandWidth = 0;
	pConfiguration->shortctl = SHORTCTL_NORMAL;

	if (!faacEncSetConfiguration(audio->handle, pConfiguration)) {
		LOGI( "audio encoder configration fail");
	}
}

JNIEXPORT void JNICALL Java_com_zenvv_live_jni_PusherNative_setAudioOptions(
		JNIEnv *env, jobject thiz, jint sampleRate, jint channel) {
	setAudioOptions(&s_pusher.audio, sampleRate, channel);
}

void startPusher(pusher_t *pusher, const char* path) {
	LOGD("Native start Pusher");

	pusher->proto.rtmp_path = malloc(strlen(path) + 1);
	memset(pusher->proto.rtmp_path, 0, strlen(path) + 1);
	memcpy(pusher->proto.rtmp_path, path, strlen(path));
	create_queue();

	//RTMP_LogSetCallback(rtmp_log_debug);
	//FILE *fp = fopen("mnt/sdcard/rtmp_log1.txt", "a+");
	//RTMP_LogSetOutput(fp);

	pthread_create(&s_tid, NULL, publiser, NULL);
	pusher->start_time = RTMP_GetTime();
}

JNIEXPORT void JNICALL Java_com_zenvv_live_jni_PusherNative_startPusher(
		JNIEnv *env, jobject thiz, jstring url) {

	if (!s_jni.pusher_obj) {
		s_jni.pusher_obj = (*env)->NewGlobalRef(env, thiz);
	}

	const char* path = (*env)->GetStringUTFChars(env, url, 0);
	startPusher(&s_pusher, path);
	(*env)->ReleaseStringUTFChars(env, url, path);
}



void fireAudio(audio_enc_t *audio, jbyte* b_buffer, jint len) {
	if (!audio->handle) {
		return;
	}

	unsigned char * bitbuf = (unsigned char*) malloc(audio->nMaxOutputBytes * sizeof(unsigned char));
	int byteslen = faacEncEncode(audio->handle, (int32_t *) b_buffer,
			audio->nInputSamples, bitbuf, audio->nMaxOutputBytes);
	if (byteslen > 0) {
		add_aac_body(bitbuf, byteslen);
	}
	free(bitbuf);
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

JNIEXPORT jint JNICALL Java_com_zenvv_live_jni_PusherNative_getInputSamples() { // for audio
	return s_pusher.audio.nInputSamples;
}

void fireVideo(video_enc_t *video, jbyte *nv21_buffer) {
	if (!video || !video->handle) {
		return;
	}

	jbyte* u = video->pic_in->img.plane[1];
	jbyte* v = video->pic_in->img.plane[2];
	// nv21 to yuv420p  y = w*h,u/v=w*h/4
	// nv21 = yvu, yuv420p=yuv, y=y, u=y+1+1, v=y+1
	memcpy(video->pic_in->img.plane[0], nv21_buffer, video->y_len);
	for (int i = 0; i < video->u_v_len; i++) {
		*(u + i) = *(nv21_buffer + video->y_len + i * 2 + 1);
		*(v + i) = *(nv21_buffer + video->y_len + i * 2);
	}

	int nNal = -1;
	x264_nal_t *nal = NULL;
	x264_picture_init(video->pic_out);
	if (x264_encoder_encode(video->handle, &nal, &nNal, video->pic_in, video->pic_out) < 0) {
		LOGI("x264 encoder fail");
		return;
	}

	video->pic_in->i_pts++;
	int sps_len, pps_len;
	unsigned char sps[100];
	unsigned char pps[100];
	memset(sps, 0, 100);
	memset(pps, 0, 100);

	for (int i = 0; i < nNal; i++) {
		if (nal[i].i_type == NAL_SPS) { //sps
			sps_len = nal[i].i_payload - 4;
			memcpy(sps, nal[i].p_payload + 4, sps_len);
		} else if (nal[i].i_type == NAL_PPS) { //pps
			pps_len = nal[i].i_payload - 4;
			memcpy(pps, nal[i].p_payload + 4, pps_len);
			add_264_sequence_header(pps, sps, pps_len, sps_len);
		} else {
			add_264_body(nal[i].p_payload, nal[i].i_payload);
		}
	}
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

	if (s_pusher.audio.handle) {
		faacEncClose(s_pusher.audio.handle);
		s_pusher.audio.handle = 0;
	}

	if (s_pusher.video.handle) {
		x264_encoder_close(s_pusher.video.handle);
		s_pusher.video.handle = 0;
	}

	if (s_jni.pusher_obj) {
		(*env)->DeleteGlobalRef(env, s_jni.pusher_obj);
		s_jni.pusher_obj = 0;
	}

	x264_free(s_pusher.video.pic_in);
	x264_free(s_pusher.video.pic_out);
}

