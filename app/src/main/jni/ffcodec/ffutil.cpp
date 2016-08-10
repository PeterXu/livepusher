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

AVFrame *FFUtil::allocAVFrameWithBuffer(PixelFormat fmt, int width, int height) {
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

AVFrame *FFUtil::allocAVFrameWithBuffer(const FFVideoParam &param) {
    return allocAVFrameWithBuffer(param.pixelFormat, param.width, param.height);
}

int FFUtil::fillAVFrameData(AVFrame *frame, const uint8_t *src, int srcLen, const FFVideoParam &param) {
    return av_image_fill_arrays(frame->data, frame->linesize, src, 
            param.pixelFormat, param.width, param.height, 0);
}

int FFUtil::copyAVFrameData(const AVFrame *frame, uint8_t *dst, int dstLen, const FFVideoParam &param) {
    return av_image_copy_to_buffer(dst, dstLen, frame->data, frame->linesize, 
                param.pixelFormat, param.width, param.height, 0);
}

int FFUtil::convertPixFmt(
        const uint8_t *src, int srclen, int srcw, int srch, PixelFormat srcfmt, 
        uint8_t *dst, int dstlen, int dstw, int dsth, PixelFormat dstfmt)
{
    if (!src || !dst) {
        LOGE("[%s] src or dst is NULL", __FUNCTION__);
        return -1;
    }

    int ret = -1;
    AVFrame *srcFrame = NULL;
    AVFrame *dstFrame = NULL;

    FFVideoParam srcParam(srcw, srch, srcfmt, 0, 0, "");
    FFVideoParam dstParam(dstw, dsth, dstfmt, 0, 0, "");

    do {
        srcFrame = allocAVFrameWithBuffer(srcParam);
        if (!srcFrame) {
            LOGE("[%s] fail to get src frame buffer", __FUNCTION__);
            break;
        }

        ret = fillAVFrameData(srcFrame, src, srclen, srcParam);
        if (ret < 0) {
            LOGE("[%s] fail to fill src frame", __FUNCTION__);
            break;
        }

        dstFrame = allocAVFrameWithBuffer(dstParam);
        if (!dstFrame) {
            LOGE("[%s] fail to get dst frame buffer", __FUNCTION__);
            break;
        }

        if (convertPixFmt(srcFrame, dstFrame, srcParam, dstParam) < 0){
            LOGE("[%s] fail to convertPixFmt", __FUNCTION__);
            break;
        }

        ret = copyAVFrameData((const AVFrame *)dstFrame, dst, dstlen, dstParam);
    }while(0);

    av_frame_free(&srcFrame);
    av_frame_free(&dstFrame);

    return ret;
}

