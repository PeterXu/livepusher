#include "common.h"

#undef av_restrict
#define av_restrict
#undef av_export
#define av_export
#include "libavutil/log.h"
#include "libavcodec/aacenc.h"
#include "x264/x264.h"
#include "libavformat/rtmppkt.h"


#define RTMP_HEADER 11

/** RTMP protocol handler state */
typedef enum {
    STATE_START,      ///< client has not done anything yet
    STATE_HANDSHAKED, ///< client has performed handshake
    STATE_FCPUBLISH,  ///< client FCPublishing stream (for output)
    STATE_PLAYING,    ///< client has started receiving multimedia data from server
    STATE_SEEKING,    ///< client has started the seek operation. Back on STATE_PLAYING when the time comes
    STATE_PUBLISHING, ///< client has started sending multimedia data to server (for output)
    STATE_RECEIVING,  ///< received a publish command (for input)
    STATE_SENDING,    ///< received a play command (for output)
    STATE_STOPPED,    ///< the broadcast has been stopped
} ClientState;

typedef struct TrackedMethod {
    char *name;
    int id;
} TrackedMethod;

typedef struct RTMPContext {
    const AVClass *class;
    URLContext*   stream;                     ///< TCP stream used in interactions with RTMP server
    RTMPPacket    *prev_pkt[2];               ///< packet history used when reading and sending packets ([0] for reading, [1] for writing)
    int           nb_prev_pkt[2];             ///< number of elements in prev_pkt
    int           in_chunk_size;              ///< size of the chunks incoming RTMP packets are divided into
    int           out_chunk_size;             ///< size of the chunks outgoing RTMP packets are divided into
    int           is_input;                   ///< input/output flag
    char          *playpath;                  ///< stream identifier to play (with possible "mp4:" prefix)
    int           live;                       ///< 0: recorded, -1: live, -2: both
    char          *app;                       ///< name of application
    char          *conn;                      ///< append arbitrary AMF data to the Connect message
    ClientState   state;                      ///< current state
    int           stream_id;                  ///< ID assigned by the server for the stream
    uint8_t*      flv_data;                   ///< buffer with data for demuxer
    int           flv_size;                   ///< current buffer size
    int           flv_off;                    ///< number of bytes read from current buffer
    int           flv_nb_packets;             ///< number of flv packets published
    RTMPPacket    out_pkt;                    ///< rtmp packet, created from flv a/v or metadata (for output)
    uint32_t      client_report_size;         ///< number of bytes after which client should report to server
    uint32_t      bytes_read;                 ///< number of bytes read from server
    uint32_t      last_bytes_read;            ///< number of bytes read last reported to server
    uint32_t      last_timestamp;             ///< last timestamp received in a packet
    int           skip_bytes;                 ///< number of bytes to skip from the input FLV stream in the next write call
    int           has_audio;                  ///< presence of audio data

    int           has_video;                  ///< presence of video data
    int           received_metadata;          ///< Indicates if we have received metadata about the streams
    uint8_t       flv_header[RTMP_HEADER];    ///< partial incoming flv packet header
    int           flv_header_bytes;           ///< number of initialized bytes in flv_header
    int           nb_invokes;                 ///< keeps track of invoke messages
    char*         tcurl;                      ///< url of the target stream
    char*         flashver;                   ///< version of the flash plugin
    char*         swfhash;                    ///< SHA256 hash of the decompressed SWF file (32 bytes)
    int           swfhash_len;                ///< length of the SHA256 hash
    int           swfsize;                    ///< size of the decompressed SWF file
    char*         swfurl;                     ///< url of the swf player
    char*         swfverify;                  ///< URL to player swf file, compute hash/size automatically
    char          swfverification[42];        ///< hash of the SWF verification
    char*         pageurl;                    ///< url of the web page
    char*         subscribe;                  ///< name of live stream to subscribe
    int           server_bw;                  ///< server bandwidth
    int           client_buffer_time;         ///< client buffer time in ms
    int           flush_interval;             ///< number of packets flushed in the same request (RTMPT only)
    int           encrypted;                  ///< use an encrypted connection (RTMPE only)
    TrackedMethod*tracked_methods;            ///< tracked methods buffer
    int           nb_tracked_methods;         ///< number of tracked methods
    int           tracked_methods_size;       ///< size of the tracked methods buffer
    int           listen;                     ///< listen mode flag
    int           listen_timeout;             ///< listen timeout to wait for new connections
    int           nb_streamid;                ///< The next stream id to return on createStream calls
    double        duration;                   ///< Duration of the stream in seconds as returned by the server (only valid if non-zero)
    char          username[50];
    char          password[50];
    char          auth_params[500];
    int           do_reconnect;
    int           auth_tried;
} RTMPContext;


typedef struct X264Context {
    AVClass        *class;
    x264_param_t    params;
    x264_t         *enc;
    x264_picture_t  pic;
    uint8_t        *sei;
    int             sei_size;
    char *preset;
    char *tune;
    char *profile;
    char *level;
    int fastfirstpass;
    char *wpredp;
    char *x264opts;
    float crf;
    float crf_max;
    int cqp;
    int aq_mode;
    float aq_strength;
    char *psy_rd;
    int psy;
    int rc_lookahead;
    int weightp;
    int weightb;
    int ssim;
    int intra_refresh;
    int bluray_compat;
    int b_bias;
    int b_pyramid;
    int mixed_refs;
    int dct8x8;
    int fast_pskip;
    int aud;
    int mbtree;
    char *deblock;
    float cplxblur;
    char *partitions;
    int direct_pred;
    int slice_max_size;
    char *stats;
    int nal_hrd;
    int avcintra_class;
    int motion_est;
    int forced_idr;
    int coder;
    int a53_cc;
    int b_frame_strategy;
    int chroma_offset;
    int scenechange_threshold;
    int noise_reduction;

    char *x264_params;
} X264Context;



extern AVCodec          ff_libx264_encoder;
extern AVCodec          ff_libopenh264_encoder;
extern AVCodec          ff_aac_encoder;
extern AVOutputFormat   ff_flv_muxer;
extern URLProtocol      ff_rtmp_protocol;


static AVCodecContext mCodecCtx[2];


void initAudioCodec(AVCodecContext *avctx) {
    memset(avctx, 0, sizeof(AVCodecContext));
    avctx->priv_data = malloc(sizeof(AACEncContext));
    memset(avctx->priv_data, 0, sizeof(AACEncContext));

    AACEncContext *s = avctx->priv_data;

    avctx->channels = 1;
    avctx->bit_rate = 32; // kb/s
    avctx->sample_rate = 44100;
    avctx->profile = FF_PROFILE_AAC_MAIN; 
    avctx->strict_std_compliance = -2;

    ff_aac_encoder.init(avctx);
}
void initVideoCodec(AVCodecContext *avctx) {
    memset(&mCodecCtx[E_VIDEO], 0, sizeof(AVCodecContext));
    avctx->priv_data = malloc(sizeof(X264Context));
    memset(avctx->priv_data, 0, sizeof(X264Context));

    X264Context *x4 = avctx->priv_data;

    avctx->flags = 0;
    avctx->level = 51;
    avctx->bit_rate = 800*1024; // b/s
    avctx->rc_buffer_size = 0;
    avctx->rc_max_rate = 0;
    avctx->flags = 0;
    avctx->i_quant_factor = 32;
    avctx->b_quant_factor = 0;
    avctx->gop_size = 10;
    avctx->max_b_frames = 0;

    avctx->qmin = 10;
    avctx->qmax = 36;
    avctx->max_qdiff = 5;
    avctx->qcompress = 0.5;
    avctx->refs = 1;

    avctx->width = 640;
    avctx->height = 480;
    avctx->keyint_min = 10;
    //avctx->coder_type = 1;  //> FF_CODER_TYPE_AC

    avctx->profile = FF_PROFILE_H264_BASELINE;
    avctx->sample_aspect_ratio.num = 4;
    avctx->sample_aspect_ratio.den = 3;
    avctx->time_base.den = 15;
    avctx->time_base.num = 1;
    avctx->thread_count = 1;
    //avctx->thread_type = FF_THREAD_SLICE;
    avctx->pix_fmt = AV_PIX_FMT_YUVJ420P;

    ff_libx264_encoder.init(avctx);
}

void encodeVideoFrame(AVCodecContext *avctx, 
        char *data, enum AVPixelFormat format, int width, int height) {
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    AVFrame *frame = av_frame_alloc();
    int ret = av_image_fill_arrays(frame->data, frame->linesize, data, format, width, height, 0);
    if (ret < 0) {
        goto end;
    }
    
    frame->pts = AV_NOPTS_VALUE;
    //frame->pict_type = AV_PICTURE_TYPE_I;

    int got_packet = 0;
    ret = ff_libx264_encoder.encode2(avctx, &pkt, frame, &got_packet);
    if (ret < 0) {
        goto end;
    }

    if (got_packet) {
    }

end:
    if (frame) av_frame_free(&frame);
}

void encodeAudioFrame(AVCodecContext *avctx, char *data) {
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

#if 0
    AVFrame *frame = av_frame_alloc();
    int ret = av_image_fill_arrays(frame->data, frame->linesize, data, 0, 0, 0, 0);
    if (ret < 0) {
        goto end;
    }

    int got_packet = 0;
    ret = ff_aac_encoder.encode2(avctx, &pkt, frame, &got_packet);
    if (ret < 0) {
        goto end;
    }

    if (got_packet) {
    }
#endif

end:
    //if (frame) av_frame_free(&frame);
    return;
}

void initRtmpProto(URLContext *s, const char* url) {
    s->priv_data = malloc(sizeof(RTMPContext));
    memset(s->priv_data, 0, sizeof(RTMPContext));

    RTMPContext *rt = s->priv_data;

    ff_rtmp_protocol.url_open(s, url, 0);
}
void sendRtmpPacket(URLContext *s, const uint8_t *buf, int size) {
    ff_rtmp_protocol.url_write(s, buf, size);
}


void initCodec() {
    AVCodecContext *avctx1 = &mCodecCtx[E_AUDIO];
    initAudioCodec(avctx1);

    AVCodecContext *avctx2 = &mCodecCtx[E_VIDEO];
    initVideoCodec(avctx2);
}
void releaseCodec() {
    if (mCodecCtx[E_AUDIO].priv_data) {
        free(mCodecCtx[E_AUDIO].priv_data);
        mCodecCtx[E_AUDIO].priv_data = NULL;
    }

    if (mCodecCtx[E_VIDEO].priv_data) {
        free(mCodecCtx[E_VIDEO].priv_data);
        mCodecCtx[E_VIDEO].priv_data = NULL;
    }
}

