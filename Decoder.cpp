#include "Decoder.h"
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
}
Decoder::Decoder()
    : mFormatCtx(NULL),
      mCodecCtx(NULL),
      mStream(NULL),
      mStreamIndex(-1),
      mPixFmt(AV_PIX_FMT_NONE),
      mWidth(0),
      mHeight(0),
      mEof(false),
      mFrameCount(0) {
    // av_register_all();
}

Decoder::~Decoder() {
    avcodec_free_context(&mCodecCtx);
    avformat_close_input(&mFormatCtx);
    // av_free(mImageData[0]);
}

void Decoder::global_init() {
    av_register_all();
}

int Decoder::open(const char *input_file) {
    int ret = avformat_open_input(&mFormatCtx, input_file, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open source file %s\n", input_file);
        return ret;
    }

    /* retrieve stream information */
    ret = avformat_find_stream_info(mFormatCtx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return ret;
    }
    ret = open_codec_context(&mStreamIndex, &mCodecCtx, mFormatCtx, AVMEDIA_TYPE_VIDEO);
    if (ret >= 0) {
        mStream = mFormatCtx->streams[mStreamIndex];

        /* allocate image where the decoded image will be put */
        mWidth = mCodecCtx->width;
        mHeight = mCodecCtx->height;
        mPixFmt = mCodecCtx->pix_fmt;
        printf("[%s:%d]==mWidth:%d mHeight:%d mPixFmt:%d\n", __FUNCTION__, __LINE__, mWidth, mHeight, mPixFmt);
        // ret = av_image_alloc(mImageData, mImageLinesize,
        //                      mWidth, mHeight, mPixFmt, 1);
        // if (ret < 0) {
        //     fprintf(stderr, "Could not allocate raw video buffer\n");
        //     return ret;
        // }
        // video_dst_bufsize = ret;
    }
    return ret;
}

int Decoder::open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream\n",
                av_get_media_type_string(type));
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }

        /* Init the decoders, with or without reference counting */
        // av_dict_set(&opts, "refcounted_frames", mRefcount ? "1" : "0", 0);
        if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

int Decoder::decode_packet(AVPacket *pkt, AVFrame *frame, int *got_frame)
{
    int ret = 0;
    int decoded = pkt->size;
    int width = mWidth;
    int height = mHeight;

    *got_frame = 0;

    if (pkt->stream_index == mStreamIndex) {
        /* decode video frame */
        ret = avcodec_decode_video2(mCodecCtx, frame, got_frame, pkt);
        if (ret < 0) {
            fprintf(stderr, "Error decoding video frame (%s)\n", av_err2str(ret));
            return ret;
        }

        if (*got_frame) {
            // printf("[%s:%d]==width:%d height:%d\n", __FUNCTION__, __LINE__, frame->width, frame->height);
            if (frame->width != width || frame->height != height ||
                frame->format != mPixFmt) {
                /* To handle this change, one could call av_image_alloc again and
                 * decode the following frames into another rawvideo file. */
                fprintf(stderr, "Error: Width, height and pixel format have to be "
                        "constant in a rawvideo file, but the width, height or "
                        "pixel format of the input video changed:\n"
                        "old: width = %d, height = %d, format = %s\n"
                        "new: width = %d, height = %d, format = %s\n",
                        width, height, av_get_pix_fmt_name(mPixFmt),
                        frame->width, frame->height,
                        av_get_pix_fmt_name((enum AVPixelFormat)frame->format));
                return -1;
            }

            // printf("mFrameCount:%d coded_n:%d\n",
            //        mFrameCount++, frame->coded_picture_number);

            /* copy decoded frame to destination buffer:
             * this is required since rawvideo expects non aligned data */
            // av_image_copy(mImageData, mImageLinesize,
            //               (const uint8_t **)(frame->data), frame->linesize,
            //               mPixFmt, width, height);

            /* write to rawvideo file */
            // fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
        }
    }

    /* If we use frame reference counting, we own the data and need
     * to de-reference it when we don't use it anymore */
    // if (*got_frame)
    //     av_frame_unref(frame);

    return decoded;
}

int Decoder::decode(AVFrame *frame) {
    int ret = 0;
    AVPacket pkt;
    int got_frame = 0;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    while (!got_frame) {
        if (!mEof) {
            // printf("[%s:%d]==\n", __FUNCTION__, __LINE__);
            ret = av_read_frame(mFormatCtx, &pkt);
            // printf("[%s:%d]==ret:%d\n", __FUNCTION__, __LINE__, ret);
            if (ret < 0) {
                mEof = true;
                return ret;
            }
        }
        AVPacket orig_pkt = pkt;
        do {
            // printf("[%s:%d]==\n", __FUNCTION__, __LINE__);
            ret = decode_packet(&pkt, frame, &got_frame);
            // printf("[%s:%d]==ret:%d\n", __FUNCTION__, __LINE__, ret);
            if (ret < 0)
                break;
            pkt.data += ret;
            pkt.size -= ret;
        } while (pkt.size > 0);
        av_packet_unref(&orig_pkt);
    }
    return ret;
}

