#include "Decoder.h"
#include "Envaluer.h"
extern "C" {
#include <libavutil/imgutils.h>
}

struct Context {
    FILE *psnr_file;
    FILE *ssim_file;
};

static int psnr_write(double v, void *usr) {
    Context *c = (Context *)usr;
    FILE *fp = c->psnr_file;
    char str[10] = {0};
    sprintf(str, "%.3f\n", v);
    int len = fwrite(str, sizeof(char), strlen(str), fp);
    // printf("[%s:%d]==len:%d\n", __FUNCTION__, __LINE__, len);
    fflush(fp);
    return len;
}

static int ssim_write(double v, void *usr) {
    Context *c = (Context *)usr;
    FILE *fp = c->ssim_file;
    char str[10] = {0};
    sprintf(str, "%.3f\n", v);
    int len = fwrite(str, sizeof(char), strlen(str), fp);
    // printf("[%s:%d]==len:%d\n", __FUNCTION__, __LINE__, len);
    fflush(fp);
    return len;
}

int main (int argc, char **argv) {
    Context context;
    char *org_file_name = NULL;
    char *new_file_name = NULL;
    EnvalMode mode = EVL_BOTH; //0表示计算psnr和ssim, 1表示只计算psnr，2表示只计算ssim

    const char *psnr_file_name = "psnr.txt";
    const char *ssim_file_name = "ssim.txt";
    FILE *psnr_file = NULL;
    FILE *ssim_file = NULL;
    int ret = 0;
    AVFrame *frame[2] = {0};
    uint8_t *org_image_data[4];
    int      org_image_linesize[4];
    uint8_t *new_image_data[4];
    int      new_image_linesize[4];
    int decode_max_nm = -1;
    int decode_count = 0;

    Decoder decoder[2];

    if (argc < 3) {
        return -1;
    }
    else if (argc > 3) {
        if (!strcmp(argv[3], "psnr")) {
            mode = EVL_PSNR;
        }
        else if (!strcmp(argv[3], "ssim")) {
            mode = EVL_SSIM;
        }
        else {

        }
    }
    else if (argc > 4) {
        decode_max_nm = atoi(argv[4]);
    }
    decode_count = decode_max_nm;
    Envaluer envaluer(mode);
    org_file_name = argv[1];
    new_file_name = argv[2];

    psnr_file = fopen(psnr_file_name, "w");
    if (psnr_file == NULL) {
        return -1;
    }
    context.psnr_file = psnr_file;
    ssim_file = fopen(ssim_file_name, "w");
    if (ssim_file == NULL) {
        return -1;
    }
    context.ssim_file = ssim_file;

    envaluer.setCallback(psnr_write, ssim_write, &context);

    Decoder::global_init();
    
    frame[0] = av_frame_alloc();
    frame[1] = av_frame_alloc();

    ret = decoder[0].open(org_file_name);
    if (ret < 0) {
        return -1;
    }
    ret = av_image_alloc(org_image_data, org_image_linesize,
                            decoder[0].width(), decoder[0].height(), decoder[0].pix_fmt(), 1);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw video buffer\n");
        return ret;
    }
    ret = decoder[1].open(new_file_name);
    if (ret < 0) {
        return -1;
    }
    ret = av_image_alloc(new_image_data, new_image_linesize,
                            decoder[1].width(), decoder[1].height(), decoder[1].pix_fmt(), 1);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw video buffer\n");
        return ret;
    }

    envaluer.open(decoder[0].width(), decoder[0].height(), decoder[0].pix_fmt());
    // int cnt = 10;
    while(1) {
        if (decode_max_nm > 0) {
            if (decode_count-- <= 0) {
                break;
            }
        }
        ret = decoder[0].decode(frame[0]);
        // printf("[%s:%d]==ret:%d\n", __FUNCTION__, __LINE__, ret);
        if (ret < 0) {
            break;
        }
        av_image_copy(org_image_data, org_image_linesize,
                    (const uint8_t **)(frame[0]->data), frame[0]->linesize,
                    decoder[0].pix_fmt(), decoder[0].width(), decoder[0].height());
        ret = decoder[1].decode(frame[1]);
        // printf("[%s:%d]==ret:%d\n", __FUNCTION__, __LINE__, ret);
        if (ret < 0) {
            printf("[%s:%d]==ret:%d\n", __FUNCTION__, __LINE__, ret);
            break;
        }
        // printf("[%s:%d]==\n", __FUNCTION__, __LINE__);
        av_image_copy(new_image_data, new_image_linesize,
                    (const uint8_t **)(frame[1]->data), frame[1]->linesize,
                    decoder[1].pix_fmt(), decoder[1].width(), decoder[1].height());
        // printf("[%s:%d]==\n", __FUNCTION__, __LINE__);
        envaluer.compute(org_image_data[0], new_image_data[0]);
    }
    envaluer.close();
    av_frame_free(&frame[0]);
    av_frame_free(&frame[1]);
    av_free(org_image_data[0]);
    av_free(new_image_data[0]);
    fclose(psnr_file);
    fclose(ssim_file);
    return ret;
}