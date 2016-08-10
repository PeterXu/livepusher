#include "ffutil.h"
#include "logtrace.h"


int FFUtil::convertPixFmt(const uint8_t *src, int srclen, int srcw, int srch, PixelFormat srcfmt, 
        uint8_t *dst, int dstlen, int dstw, int dsth, PixelFormat dstfmt)
{
    if (!src || !dst) 
    {
        LOGE("[%s] src or dst is NULL", __FUNCTION__);
        return -1;
    }

    // src input frame
    AVPicture srcPic;
    FFVideoParam srcParam(srcw, srch, srcfmt, 0, 0, "");
#if 0
    if(avpicture_fill(&srcPic, (uint8_t *)src, srcParam.pixelFormat, srcParam.width, srcParam.height) == -1) 
    {
        LOGE("[%s] fail to avpicture_fill for src picture", __FUNCTION__);
        return -1;
    }
#endif

    // dst output frame
    AVPicture dstPic;
    FFVideoParam dstParam(dstw, dsth, dstfmt, 0, 0, "");
#if 0
    if(avpicture_alloc(&dstPic, dstParam.pixelFormat, dstParam.width, dstParam.height) == -1) 
    {
        LOGE("[%s] fail to avpicture_alloc for dst picture", __FUNCTION__);
        return -1;
    }
#endif

    if (convertPixFmt(&srcPic, &dstPic, &srcParam, &dstParam) < 0)
    {
        LOGE("[%s] fail to convertPixFmt", __FUNCTION__);
        return -1;
    }

    //return avpicture_layout(&dstPic, dstParam.pixelFormat, dstParam.width, dstParam.height, dst, dstlen);
    return 0;
}

int FFUtil::convertPixFmt(AVPicture *srcPic, AVPicture *dstPic, const FFVideoParam *srcParam, const FFVideoParam *dstParam)
{
    SwsContext *img_convert_ctx = NULL;
    img_convert_ctx = sws_getContext(
            srcParam->width, srcParam->height, srcParam->pixelFormat,
            dstParam->width, dstParam->height, dstParam->pixelFormat,
            SWS_FAST_BILINEAR, NULL, NULL, NULL);
    if (img_convert_ctx == NULL)
    {
        LOGE("[%s] fail to sws_getContext", __FUNCTION__);
        return -1;
    }

    //int dstheight = sws_scale(img_convert_ctx, srcPic->data, srcPic->linesize, 0, srcParam->height, dstPic->data, dstPic->linesize);
    int dstheight = 0;
    sws_freeContext(img_convert_ctx);
    return (dstheight == dstParam->height) ? 0 : -1;
}

