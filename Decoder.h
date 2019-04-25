#ifndef __DECODER_H_
#define __DECODER_H_

#include <iostream>
extern "C" {
#include <libavformat/avformat.h>
}
class Decoder {
public:
    Decoder();
    ~Decoder();
    static void global_init();
    int open(const char *input_file);
    int decode(AVFrame *frame);
    inline int width() {
        return mWidth;
    }
    inline int height() {
        return mHeight;
    }
    inline enum AVPixelFormat pix_fmt() {
        return mPixFmt;
    }
private:
    int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type);
    int decode_packet(AVPacket *pkt, AVFrame *frame, int *got_frame);
    // uint8_t *mImageData[4];
    // int      mImageLinesize[4];
    AVFormatContext *mFormatCtx;
    AVCodecContext *mCodecCtx;
    AVStream *mStream;
    int mStreamIndex;
    enum AVPixelFormat mPixFmt;
    int mWidth;
    int mHeight;
    bool mEof;
    int mFrameCount;
};

#endif
