#ifndef __FFUTIL_H_
#define __FFUTIL_H_

#include "ffheader.h"
#include "ffvideoparam.h"

class FF_EXPORT FFUtil {
public:
    ///
    /// @brief  Convert the pixel format of the input image
    ///
    /// @param  [in]  srcFrame    The source image picture to be converted
    /// @param  [out] dstFrame    The output image picture which has been converted
    /// @param  [in]  srcParam    The parameters of the source image picture
    /// @param  [in]  dstParam    The parameters of the dest image picture
    ///
    /// @return 0 when conversion is successful, otherwise a negative int
    ///
    static int convertPixFmt(AVFrame *srcFrame, AVFrame *dstFrame, 
            const FFVideoParam &srcParam, const FFVideoParam &dstParam);

    static int convertPixFmt(const uint8_t *src, int srclen, int srcw, int srch, PixelFormat srcfmt, 
            uint8_t *dst, int dstlen, int dstw, int dsth, PixelFormat dstfmt);

    static AVFrame *allocAVFrameWithBuffer(PixelFormat fmt, int width, int height);
    static AVFrame *allocAVFrameWithBuffer(const FFVideoParam &param);

    static int fillAVFrameData(AVFrame *frame, const uint8_t *src, int srcLen, const FFVideoParam &param);
    static int copyAVFrameData(const AVFrame *frame, uint8_t *dst, int dstLen, const FFVideoParam &param);
    static int getImageBufferSize(const FFVideoParam &param);
};


#endif
