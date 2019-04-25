#ifndef __ENVALUER_H_
#define __ENVALUER_H_

#include <inttypes.h>

typedef enum EnvalMode {
    EVL_PSNR,
    EVL_SSIM,
    EVL_BOTH,
    EVL_MAX
} EnvalMode;

typedef int (*EvlCallback)(double , void *);
class Envaluer {
public:
    Envaluer(EnvalMode mode);
    ~Envaluer();
    int open(int width, int height, int format);
    float compute(uint8_t *org_buf, uint8_t *new_buf);
    void close();
    void setCallback(EvlCallback psnr, EvlCallback ssim, void *usr) {
        mPsnrCallback = psnr;
        mSsimCallback = ssim;
        mUsr = usr;
    }
private:
    void ssim_4x4x2_core(const uint8_t *pix1, int stride1,
                         const uint8_t *pix2, int stride2,
                         int sums[2][4]);
    float ssim_end1(int s1, int s2, int ss, int s12);
    float ssim_end4(int sum0[5][4], int sum1[5][4], int width);
    float compute_ssim(uint8_t *pix1, int stride1,
                       uint8_t *pix2, int stride2,
                       int width, int height);
    void end();
    int mMode;
    int mWidth;
    int mHeight;
    int mYuvSize;
    int mBufSize;
    uint8_t *mYuvOrg;
    uint8_t *mYuvNew;
    double *mYPsnr;
    double *mYSsim;
    double mPMean;
    double mSMean;
    clock_t mStartClock;
    int mCount;
    EvlCallback mPsnrCallback;
    EvlCallback mSsimCallback;
    void *mUsr;
};

#endif
