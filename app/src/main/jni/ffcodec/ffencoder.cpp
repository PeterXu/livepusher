#include "ffencoder.h"
#include "ffutil.h"
#include "logtrace.h"

// define the max audio packet size as 128 KB
#define MAX_AUDIO_PACKET_SIZE (128 * 1024)


FFEncoder::FFEncoder(const FFVideoParam &vp, const FFAudioParam &ap) : 
    videoParam(vp), audioParam(ap)
{
    init();
}

FFEncoder::FFEncoder(const FFVideoParam &vp) : videoParam(vp)
{
    init();
}

FFEncoder::FFEncoder(const FFAudioParam &ap) : audioParam(ap)
{
    init();
}

void FFEncoder::init()
{
    // initialize the private fields
    this->outputContext = NULL;
    this->videoStream = NULL;
    this->audioStream = NULL;

    this->videoFrame = NULL;
    this->videoBuffer = NULL;
    this->videoBufferSize = 0;

    this->audioBuffer = NULL;
    this->audioBufferSize = 0;

    this->opened = false;
    this->encodeVideo = this->videoParam.isValid();
    this->encodeAudio = this->audioParam.isValid();

    // initialize libavcodec, and register all codecs and formats
    avcodec_register_all();
    avfilter_register_all();
    av_register_all();
}

FFEncoder::~FFEncoder()
{
    this->close();
}


//////////////////////////////////////////////////////////////////////////
//
//  Methods For Video
//
//////////////////////////////////////////////////////////////////////////

const uint8_t *FFEncoder::getVideoEncodedBuffer() const
{
    return this->videoBuffer;
}

double FFEncoder::getVideoTimeStamp() const
{
    if (!this->opened || !this->encodeVideo) {
        return 0;
    }
    //return (double)this->videoStream->pts.val * this->videoStream->time_base.num / this->videoStream->time_base.den;
    return 0;
}

const FFVideoParam &FFEncoder::getVideoParam() const
{
    return this->videoParam;
}

int FFEncoder::getVideoFrameSize() const
{
    if (!this->opened || !this->encodeVideo) {
        return 0;
    }
    return FFUtil::getImageBufferSize(this->videoParam);
}

int FFEncoder::encodeVideoFrame(const uint8_t *data, PixelFormat format, int width, int height)
{
    if (!this->opened || !this->encodeVideo) {
        return -1;
    }

    FFVideoParam inParam(width, height, format, 0, 0, "");
    AVFrame *frame = FFUtil::allocAVFrameWithBuffer(inParam);
    if (!frame) {
        return -1;
    }

    int ret = FFUtil::fillAVFrameData(frame, data, 0, inParam);
    if(ret <= 0) {
        av_frame_free(&frame);
        return -1;
    }

    frame->pts = AV_NOPTS_VALUE;
    ret = this->encodeVideoData(frame, inParam);
    av_frame_free(&frame);
    return ret;
}

int FFEncoder::encodeVideoData(const AVFrame *frame, const FFVideoParam &param)
{
    AVCodecContext *avctx = this->videoStream->codec;

    // convert the pixel format if needed
    if (param.pixelFormat != avctx->pix_fmt ||
        param.width != avctx->width || param.height != avctx->height) {
        LOGE("FFEncoder::encodeVideoData, VideoParam match error");
        return -1;
    }

    AVPacket *avpkt = av_packet_alloc();
    av_init_packet(avpkt);

    int got_pkt = 0;
    int encodedSize = avcodec_encode_video2(avctx, avpkt, frame, &got_pkt);
    if (encodedSize < 0) {
        return -1;
    }

    av_packet_unref(avpkt);

    return encodedSize;
}

const uint8_t *FFEncoder::getAudioEncodedBuffer() const
{
    return this->audioBuffer;
}

double FFEncoder::getAudioTimeStamp() const
{
    if (!this->opened || !this->encodeAudio)
    {
        return 0;
    }
    //return (double)this->audioStream->pts.val * this->audioStream->time_base.num / this->audioStream->time_base.den;
    return 0;
}

const FFAudioParam &FFEncoder::getAudioParam() const
{
    return this->audioParam;
}

int FFEncoder::getAudioFrameSize() const
{
    if (!this->opened || !this->encodeAudio)
    {
        return 0;
    }

    int frameSize = 0;
    if (this->audioStream->codec && this->audioStream->codec->frame_size > 1)
    {
        frameSize  = this->audioStream->codec->frame_size;
        frameSize *= this->audioStream->codec->channels;    // multiply the channels
        frameSize *= sizeof(short); // convert to bytes
    }
    else
    {
        // hack for PCM audio codec
        //frameSize = this->audioBufferSize / this->audioParam.channels;
        //switch (this->audioStream->codec->codec_id)
        //{
        //    case CODEC_ID_PCM_S16LE:
        //    case CODEC_ID_PCM_S16BE:
        //    case CODEC_ID_PCM_U16LE:
        //    case CODEC_ID_PCM_U16BE:
        //        frameSize >>= 1;
        //        break;
        //    default:
        //        break;
        //}
        frameSize = this->audioBufferSize;  // including all channels, return bytes directly
    }
    return frameSize;
}

int FFEncoder::encodeAudioFrame(const uint8_t *frameData, int dataSize)
{
    if (!this->opened)
    {
        return -1;
    }

    if (!this->encodeAudio)
    {
        return -1;
    }

    if (this->audioStream->codec->frame_size <= 1 && dataSize < 1)
    {
        return -1;
    }

    return this->encodeAudioData((short*)frameData, dataSize/sizeof(short));
}

// private method
int FFEncoder::encodeAudioData(short *frameData, int dataSize)
{
    // the output size of the buffer which stores the encoded data
    int audioSize = this->audioBufferSize;

    if (this->audioStream->codec->frame_size <=1 && dataSize > 0)
    {
        // For PCM related codecs, the output size of the encoded data is
        // calculated from the size of the input audio frame.
        audioSize = dataSize;

        // The following codes are used for calculating "short" size from original "sample" size.
        // The codes are not needed any more because now the input size is already in "short" unit.

        // calculated the PCM size from input data size
        //switch(this->audioStream->codec->codec_id)
        //{
        //    case CODEC_ID_PCM_S32LE:
        //    case CODEC_ID_PCM_S32BE:
        //    case CODEC_ID_PCM_U32LE:
        //    case CODEC_ID_PCM_U32BE:
        //        audioSize <<= 1;
        //        break;
        //    case CODEC_ID_PCM_S24LE:
        //    case CODEC_ID_PCM_S24BE:
        //    case CODEC_ID_PCM_U24LE:
        //    case CODEC_ID_PCM_U24BE:
        //    case CODEC_ID_PCM_S24DAUD:
        //        audioSize = audioSize / 2 * 3;
        //        break;
        //    case CODEC_ID_PCM_S16LE:
        //    case CODEC_ID_PCM_S16BE:
        //    case CODEC_ID_PCM_U16LE:
        //    case CODEC_ID_PCM_U16BE:
        //        break;
        //    default:
        //        audioSize >>= 1;
        //        break;
        //}
    }

    // encode the frame
    AVCodecContext *audioCodecContext = this->audioStream->codec;

    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    

#if 0
    int got_packet = 0;
    int encodedSize = avcodec_encode_audio(audioCodecContext, &pkt, frameData, &got_packet);

    if (encodedSize < 0)
    {
        return -1;
    }
    else
    {
        return encodedSize;
    }
#endif
    return 0;
}


int FFEncoder::openAudio() {
    if (this->encodeAudio) {
        return -1;
    }

    if (!this->outputContext) {
        LOGE("FFEncoder.%s, no context!", __FUNCTION__);
        return -1;
    }

    if(this->audioParam.codecName.empty()) {
        LOGE("FFEncoder.%s, no codec name", __FUNCTION__);
        return -1;
    }

    // find the audio encoder
    // use the codec name preferentially if it is specified in the input param
    AVCodec *audioCodec = NULL;
    audioCodec = avcodec_find_encoder_by_name(this->audioParam.codecName.c_str());
    if (!audioCodec) {
        LOGE("FFEncoder.%s, invalid codec!", __FUNCTION__);
        return -1;
    }

    // add the audio stream with stream id 1
    this->audioStream = avformat_new_stream(this->outputContext, audioCodec);
    if (!this->audioStream) {
        LOGE("FFEncoder.%s, failed to new stream!", __FUNCTION__);
        return -1;
    }

    // set the parameters for audio codec context
    AVCodecContext *audioCodecContext = this->audioStream->codec;
    audioCodecContext->codec_id       = audioCodec->id;
    audioCodecContext->codec_type     = CODEC_TYPE_AUDIO;
    audioCodecContext->bit_rate       = this->audioParam.bitRate;
    audioCodecContext->sample_rate    = this->audioParam.sampleRate;
    audioCodecContext->channels       = this->audioParam.channels;

    // open the audio codec
    if (avcodec_open2(audioCodecContext, audioCodec, NULL) < 0) {
        LOGE("FFEncoder.%s, failed to open codec!", __FUNCTION__);
        return -1;
    }

    // TODO: how to determine the buffer size?
    // allocate the output buffer
    this->audioBufferSize = 4 * MAX_AUDIO_PACKET_SIZE;
    this->audioBuffer     = (uint8_t*)(av_malloc(this->audioBufferSize));

    return 0;
}

int FFEncoder::openVideo() {
    if (this->encodeVideo) {
        return -1;
    }

    if (!this->outputContext) {
        LOGE("FFEncoder.%s, no context!", __FUNCTION__);
        return -1;
    }

    if(this->videoParam.codecName.empty()) {
        LOGE("FFEncoder.%s, no codec name", __FUNCTION__);
        return -1;
    }

    // find the video encoder
    // use the codec name preferentially if it is specified in the input param
    AVCodec *videoCodec = NULL;
    videoCodec = avcodec_find_encoder_by_name(this->videoParam.codecName.c_str());
    if (!videoCodec) {
        LOGE("FFEncoder.%s, find no codec!", __FUNCTION__);
        return -1;
    }

    // add the video stream with stream id 0
    this->videoStream = avformat_new_stream(this->outputContext, videoCodec);
    if (!this->videoStream) {
        LOGE("FFEncoder.%s, failed to new stream!", __FUNCTION__);
        return -1;
    }

    // set the parameters for video codec context
    AVCodecContext *videoCodecContext = this->videoStream->codec;
    videoCodecContext->codec_id       = videoCodec->id;
    videoCodecContext->codec_type     = CODEC_TYPE_VIDEO;
    videoCodecContext->bit_rate       = this->videoParam.bitRate;
    videoCodecContext->width          = this->videoParam.width;
    videoCodecContext->height         = this->videoParam.height;
    videoCodecContext->time_base.den  = this->videoParam.frameRate;
    videoCodecContext->time_base.num  = 1;

    // tune for video encoding
    videoCodecContext->gop_size = 24;
    videoCodecContext->qmin = 3;
    videoCodecContext->qmax = 33;
    videoCodecContext->max_qdiff = 4;
    videoCodecContext->qcompress = 0.6f;

    videoCodecContext->me_range = 64;		
    //videoCodecContext->me_method = ME_FULL;
    videoCodecContext->me_range = 64;
    //videoCodecContext->partitions = X264_PART_I4X4 | X264_PART_I8X8 | X264_PART_P8X8 | X264_PART_P4X4 | X264_PART_B8X8;
    //videoCodecContext->coder_type = FF_CODER_TYPE_AC;
    videoCodecContext->max_b_frames = 1;

    // set the PixelFormat of the target encoded video
    if (videoCodec->pix_fmts) {
        // try to find the PixelFormat required by the input param,
        // use the default PixelFormat directly if required format not found
        const enum PixelFormat *p= videoCodec->pix_fmts;
        for ( ; *p != PIX_FMT_NONE; p ++) {
            if (*p == this->videoParam.pixelFormat)
                break;
        }
        if (*p == PIX_FMT_NONE)
            videoCodecContext->pix_fmt = videoCodec->pix_fmts[0];
        else
            videoCodecContext->pix_fmt = *p;
    }

    // open the video codec
    if (avcodec_open2(videoCodecContext, videoCodec, NULL) < 0) {
        LOGE("FFEncoder.%s, find but failed to open codec!", __FUNCTION__);
        return -1;
    }

    // allocate the output buffer
    // the maximum possible buffer size could be the raw bmp format with R/G/B/A
    this->videoBufferSize = 4 * this->videoParam.width * this->videoParam.height;
    this->videoBuffer     = (uint8_t*)(av_malloc(this->videoBufferSize));

    // allocate the temporal video frame buffer for pixel format conversion if needed
    // FIXME: always allocate it when format or size is different
    if (this->videoParam.pixelFormat != videoCodecContext->pix_fmt) {
        this->videoFrame = (AVPicture *)av_malloc(sizeof(AVPicture));
#if 0
        if (   this->videoFrame == NULL
                || avpicture_alloc(this->videoFrame, videoCodecContext->pix_fmt, videoCodecContext->width, videoCodecContext->height) < 0 )
        {
            LOGE("FFEncoder.open, failed to alloc video frame!");
            return -1;
        }
#endif
    }

    return 0;
}

int FFEncoder::open()
{
    LOGI("FFEncoder.open, begin!");
    if (this->opened) {
        LOGW("FFEncoder.%s, opened!", __FUNCTION__);
        return -1;
    }

    // allocate the output media context
    this->outputContext = avformat_alloc_context();
    if (!this->outputContext) {
        LOGE("FFEncoder.%s, failed to alloc context!", __FUNCTION__);
        return -1;
    }

    openVideo();
    openAudio();
    this->opened = true;
    LOGI("FFEncoder.%s, end!", __FUNCTION__);

    return 0;
}

void FFEncoder::closeAudio() {
    if (this->encodeAudio) {
        // close the audio stream and codec
        avcodec_close(this->audioStream->codec);
        av_freep(&this->audioStream->codec);
        av_freep(&this->audioStream);
        av_freep(&this->audioBuffer);
        this->audioBufferSize = 0;
    }
}

void FFEncoder::closeVideo() {
    if (this->encodeVideo) {
        // close the video stream and codec
        avcodec_close(this->videoStream->codec);
        av_freep(&this->videoStream->codec);
        av_freep(&this->videoStream);
        av_freep(&this->videoBuffer);
        this->videoBufferSize = 0;
        if (this->videoFrame != NULL) {
            //avpicture_free(this->videoFrame);
            av_freep(&this->videoFrame);
        }
    }
}

void FFEncoder::close()
{
    if (!this->opened) {
        return;
    }

    closeAudio();
    closeVideo();
    av_freep(&this->outputContext);

    this->opened = false;
    this->encodeVideo = false;
    this->encodeAudio = false;
}

