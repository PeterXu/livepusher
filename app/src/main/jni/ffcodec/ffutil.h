#ifndef __FFUTIL_H_
#define __FFUTIL_H_

#include "ffheader.h"
#include "ffvideoparam.h"

class FF_EXPORT FFUtil {
public:
    ///
    /// @brief  Convert the pixel format of the input image
    ///
    /// @param  [in]  srcParam    The parameters of the source image picture
    /// @param  [in]  dstContext  The codec context of the output (destination) image picture
    /// @param  [in]  srcPic      The source image picture to be converted
    /// @param  [out] dstPic      Return the output image picture which has been converted
    ///
    /// @return 0 when conversion is successful, otherwise a negative int
    ///
    static int convertPixFmt(AVPicture *srcPic, AVPicture *dstPic, const FFVideoParam *srcParam, const FFVideoParam *dstParam);

    static int convertPixFmt(const uint8_t *src, int srclen, int srcw, int srch, PixelFormat srcfmt, 
            uint8_t *dst, int dstlen, int dstw, int dsth, PixelFormat dstfmt);

};


#endif
