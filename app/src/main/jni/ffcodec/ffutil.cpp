#include "ffutil.h"
#include "logtrace.h"

int FFUtil::convertPixFmt(AVFrame *srcFrame, AVFrame *dstFrame, 
        const FFVideoParam &srcParam, const FFVideoParam &dstParam)
{
    SwsContext *img_convert_ctx = NULL;
    img_convert_ctx = sws_getContext(
            srcParam.width, srcParam.height, srcParam.pixelFormat,
            dstParam.width, dstParam.height, dstParam.pixelFormat,
            SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (img_convert_ctx == NULL) {
        LOGE("[%s] fail to sws_getContext", __FUNCTION__);
        return -1;
    }

    int dstHeight = sws_scale(img_convert_ctx, srcFrame->data, srcFrame->linesize, 0, srcParam.height,
            dstFrame->data, dstFrame->linesize);
    sws_freeContext(img_convert_ctx);
    return (dstHeight == dstParam.height) ? 0 : -1;
}

static AVFrame *av_frame_with_buffer(PixelFormat fmt, int width, int height) {
    AVFrame *frame = av_frame_alloc();
    frame->format = fmt;
    frame->height = width;
    frame->width = height;
    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        av_frame_free(&frame);
        frame = NULL;
    }
    return frame;
}

int FFUtil::convertPixFmt(const uint8_t *src, int srclen, int srcw, int srch, PixelFormat srcfmt, 
        uint8_t *dst, int dstlen, int dstw, int dsth, PixelFormat dstfmt)
{
    int ret = 0;
    if (!src || !dst) {
        LOGE("[%s] src or dst is NULL", __FUNCTION__);
        return -1;
    }

    // src input frame
    AVFrame *srcFrame = av_frame_with_buffer(srcfmt, srcw, srch);
    if (!srcFrame) {
        LOGE("[%s] fail to get frame buffer", __FUNCTION__);
        return -1;
    }

    FFVideoParam srcParam(srcw, srch, srcfmt, 0, 0, "");
    int ret = av_image_fill_arrays(srcFrame->data, srcFrame->linesize, 
            src, srcParam.pixelFormat, srcParam.width, srcParam.height, 0);
    if (ret < 0) {
        LOGE("[%s] fail to fill src frame", __FUNCTION__);
        av_frame_free(&srcFrame);
        return -1;
    }

    // dst output frame
    AVFrame *dstFrame = av_frame_alloc();

    FFVideoParam dstParam(dstw, dsth, dstfmt, 0, 0, "");
    ret = av_image_fill_arrays(dstFrame->data, dstFrame->linesize, 
            dst, dstParam.pixelFormat, dstParam.width, dstParam.height, 0);
    if (ret < 0) {
        LOGE("[%s] fail to fill dst frame", __FUNCTION__);
        av_frame_free(&dstFrame);
        return -1;
    }

    if (convertPixFmt(srcFrame, dstFrame, srcParam, dstParam) < 0){
        LOGE("[%s] fail to convertPixFmt", __FUNCTION__);
        av_frame_free(&dstFrame);
        return -1;
    }

    ret = av_image_copy_to_buffer(dst, dstlen, dstFrame->data, dstFrame->linesize, 
            dstParam.pixelFormat, dstParam.width, dstParam.height, 1);
    return ret;
}

