#include "common.h"


#ifdef HAVE_X264

void preset_x264_param(x264_param_t *param, video_enc_t *video);
void x264_log_default2(void *p_unused, int i_level, const char *psz_fmt, va_list arg);


void fireVideo(video_enc_t *video, uint8_t *nv21_buffer) {
    if (!video || !video->handle) {
        LOGE("video handle is NULL");
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
        LOGE("x264 encoder fail");
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

int setVideoOptions(video_enc_t *video, jint width, jint height, jint bitrate, jint fps) {
    LOGI("[%s] options: %dx%d, %dkb/s, %d", __FUNCTION__, width, height, bitrate/1000, fps);
    if (video->handle) {
        LOGD("[%s] encoder has been opened", __FUNCTION__);
        if (video->height != height || video->width != width ||
                video->fps != fps || video->bitrate != bitrate) {
            x264_encoder_close(video->handle);
            video->handle = 0;
            x264_free(video->pic_in);
            x264_free(video->pic_out);
        } else {
            return 0;
        }
    }

    // save state
    video->width = width;
    video->height = height;
    video->bitrate = bitrate;
    video->fps = fps;
    video->y_len = width * height;
    video->u_v_len = video->y_len / 4;

    // set x264 param and open encoder
    x264_param_t param;
    preset_x264_param(&param, video);
    char * paramstr = x264_param2string(&param,1);
    LOGD("x264 param: %s", paramstr);
    x264_free(paramstr);

    video->handle = x264_encoder_open(&param);
    if (!video->handle) {
        LOGE("[%s] fail to open encoder", __FUNCTION__);
        notifyNativeInfo(LOG_ERROR, E_VIDEO_ENCODER);
        return -1;
    }

    video->pic_in = malloc(sizeof(x264_picture_t));
    video->pic_out = malloc(sizeof(x264_picture_t));
    int ret = x264_picture_alloc(video->pic_in, X264_CSP_I420, width, height);
    return ret;
}

void releaseVideo(video_enc_t *video) {
    if (video && video->handle) {
        x264_encoder_close(video->handle);
        video->handle = NULL;
    }

    if (video && video->pic_in) {
        x264_picture_clean(video->pic_in);
        x264_free(video->pic_in);
    }

    if (video && video->pic_out) {
        x264_free(video->pic_out);
    }
}

void preset_x264_param(x264_param_t *param, video_enc_t *video) {
    int threads = android_getCpuCount();
    if (threads <= 0) threads = 1;
    threads = 1;

    x264_param_default_preset(param, "ultrafast", "zerolatency");

    //param.pf_log = x264_log_default2;
    param->i_level_idc = 52; 	// base_line: 5.2
    param->i_csp = X264_CSP_I420;
    param->i_width = video->width;
    param->i_height = video->height;
    param->i_bframe = 1;
    param->b_repeat_headers = 1;
    param->i_fps_num = video->fps;
    param->i_fps_den = 1;
    param->b_vfr_input = 0;

    param->i_keyint_max = video->fps * 2; //gop
    //param->i_keyint_min = X264_KEYINT_MIN_AUTO;
    param->i_keyint_min = 10;
    param->b_intra_refresh = 0;

    param->i_timebase_den = param->i_fps_num;
    param->i_timebase_num = param->i_fps_den;
    param->i_threads = threads;
    param->b_sliced_threads = 0;

    param->rc.i_rc_method = X264_RC_ABR;
    param->rc.i_bitrate = video->bitrate / 1000;
    param->rc.i_vbv_max_bitrate = video->bitrate * 1.2 / 1000;
    param->rc.i_vbv_buffer_size = video->bitrate / 1000;

    param->rc.i_qp_max = 50;
    param->rc.i_qp_min = 10;
    param->rc.i_qp_step = 4;
    param->rc.f_qcompress = 0.6;

#if 0
    param->analyse.i_trellis = 2;
    param->analyse.i_subpel_refine = 8;
    param->analyse.inter = X264_ANALYSE_I4x4|X264_ANALYSE_PSUB8x8;
    param->analyse.f_psy_rd = 0.5;
    param->vui.i_colmatrix = 8;
#endif
    param->analyse.b_mixed_references = 1;
    param->b_cabac = 1;

    // set profile
    //x264_param_apply_profile(param, "baseline");
    x264_param_apply_profile(param, "main");
}

void x264_log_default2(void *p_unused, int i_level, const char *psz_fmt, va_list arg) {
    FILE *fp = fopen("mnt/sdcard/x264_log.txt", "a+");
    if (fp) {
        vfprintf(fp, psz_fmt, arg);
        fflush(fp);
        fclose(fp);
    }
}

#else

int setVideoOptions(video_enc_t *video, jint width, jint height, jint bitrate, jint fps) {
    LOGI("[%s] options: %dx%d, %dkb/s, %d", __FUNCTION__, width, height, bitrate/1000, fps);
    if (video->handle) {
        LOGD("[%s] encoder has been opened", __FUNCTION__);
        if (video->height != height || video->width != width ||
                video->fps != fps || video->bitrate != bitrate) {
            releaseVideo(video);
        } else {
            return 0;
        }
    }

    // save state
    video->width = width;
    video->height = height;
    video->bitrate = bitrate;
    video->fps = fps;
    video->y_len = width * height;
    video->u_v_len = video->y_len / 4;

    int ret = -1;
    do {
        ISVCEncoder* pEncoder = NULL;
        ret = WelsCreateSVCEncoder(&pEncoder);
        if (ret != 0) {
            LOGE("[%s] fail to open encoder", __FUNCTION__);
            notifyNativeInfo(LOG_ERROR, E_VIDEO_ENCODER);
            break;
        }
        video->handle = (void *)pEncoder;

        int usageType = CAMERA_VIDEO_REAL_TIME;
        SEncParamBase param;
        memset (&param, 0, sizeof (SEncParamBase));
        param.iUsageType = usageType;
        param.fMaxFrameRate = video->fps;
        param.iPicWidth = video->width;
        param.iPicHeight = video->height;
        param.iTargetBitrate = video->bitrate;
        ret = (*pEncoder)->Initialize (pEncoder, &param);
        if (ret != 0) {
            LOGE("[%s] fail to config encoder", __FUNCTION__);
            notifyNativeInfo(LOG_ERROR, E_VIDEO_ENCODER);
            break;
        }

        int logLevel = WELS_LOG_INFO;
        ret = (*pEncoder)->SetOption (pEncoder, ENCODER_OPTION_TRACE_LEVEL, &logLevel);

        int videoFormat = videoFormatI420;
        ret = (*pEncoder)->SetOption (pEncoder, ENCODER_OPTION_DATAFORMAT, &videoFormat);
    }while(0);

    if (ret != 0) {
        releaseVideo(video);
    }else {
        video->pic_len = video->width * video->height * 3 / 2;
        video->pic_in = malloc(video->pic_len+16);
        memset(video->pic_in, 0, video->pic_len);
    }

    return ret;
}

void fireVideo(video_enc_t *video, uint8_t *nv21_buffer) {
    if (!video || !video->handle) {
        LOGE("video handle is NULL");
        return;
    }

    int frameSize = video->width * video->height * 3 / 2;
    if(video->pic_len < (size_t)frameSize) {
        LOGE("frameSize is wrong: %d < %d", video->pic_len, frameSize);
        return;
    }

    SFrameBSInfo info;
    memset (&info, 0, sizeof (SFrameBSInfo));

    SSourcePicture pic;
    memset (&pic, 0, sizeof (SSourcePicture));
    pic.iPicWidth = video->width;
    pic.iPicHeight = video->height;
    pic.iColorFormat = videoFormatI420;
    pic.iStride[0] = pic.iPicWidth;
    pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
    pic.pData[0] = video->pic_in;
    pic.pData[1] = pic.pData[0] + video->width * video->height;
    pic.pData[2] = pic.pData[1] + (video->width * video->height >> 2);


    // nv21 to yuv420p  y = w*h,u/v=w*h/4
    // nv21 = yvu, yuv420p=yuv, y=y, u=y+1+1, v=y+1
    uint8_t* u = pic.pData[1];
    uint8_t* v = pic.pData[2];
    memcpy(pic.pData[0], nv21_buffer, video->y_len);
    for (int i = 0; i < video->u_v_len; i++) {
        *(u + i) = *(nv21_buffer + video->y_len + i * 2 + 1);
        *(v + i) = *(nv21_buffer + video->y_len + i * 2);
    }

    ISVCEncoder* pEncoder = (ISVCEncoder*)video->handle;
    int rv = (*pEncoder)->EncodeFrame (pEncoder, &pic, &info);
    if (rv == cmResultSuccess) {
        if (info.eFrameType != videoFrameTypeSkip) {
            //output bitstream
        }
    }
}

void releaseVideo(video_enc_t *video) {
    if (video && video->handle) {
        ISVCEncoder* pEncoder = (ISVCEncoder*)video->handle;
        (*pEncoder)->Uninitialize(pEncoder);
        WelsDestroySVCEncoder(pEncoder);
        video->handle = NULL;
    }
}

#endif
