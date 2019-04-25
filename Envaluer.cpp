#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "Envaluer.h"

#define x264_alloca(x) (void*)(((intptr_t)alloca((x)+15)+15)&~15)
#define XCHG(type,a,b) { type t = a; a = b; b = t; }
#define X264_MIN(a,b) ( (a)<(b) ? (a) : (b) )

Envaluer::Envaluer(EnvalMode mode)
    : mMode(mode),
      mWidth(-1),
      mHeight(-1),
      mYuvSize(0),
      mBufSize(0),
      mYuvOrg(NULL),
      mYuvNew(NULL),
      mYPsnr(NULL),
      mYSsim(NULL),
      mPMean(0),
      mSMean(0),
      mCount(0) {
    
}

Envaluer::~Envaluer() {
    
}

int Envaluer::open(int width, int height, int format) {
    mWidth = width;
    mHeight = height;
    mYuvSize = width * height;
    mYuvOrg = (uint8_t *)malloc(mYuvSize * sizeof(uint8_t));
    mYuvNew = (uint8_t *)malloc(mYuvSize * sizeof(uint8_t));
    mStartClock = clock();
    switch (format) {
        case 420: {
            mYuvSize = (width * height * 3) >> 1;
        }
        break;
        default:break;
    }
    return 0;
}

void Envaluer::close() {
    end();
    free(mYuvOrg);
    free(mYuvNew);
    free(mYPsnr);
    free(mYSsim);
    mYuvOrg = NULL;
    mYuvNew = NULL;
    mYPsnr = NULL;
    mYSsim = NULL;
    mYuvSize = 0;
    mBufSize = 0;
    mPMean = 0;
    mSMean = 0;
    mCount = 0;
}

void Envaluer::end() {
    double diff = 0;
    int psnr_stdv = 0;
    int ssim_stdv = 0;
    int i = 0;

    if (mCount) {
        if (mMode == EVL_PSNR || mMode == EVL_BOTH) {
            mPMean /= mCount;
        }
        if (mMode == EVL_SSIM || mMode == EVL_BOTH) {
            mSMean /= mCount;
        }
        for (psnr_stdv = 0, ssim_stdv = 0, i = 0; i < mCount; i++) {
            if (mMode == EVL_PSNR || mMode == EVL_BOTH) {
                diff = mYPsnr[i] - mPMean;
                psnr_stdv += diff * diff;
            }
            if (mMode == EVL_SSIM || mMode == EVL_BOTH) {
                diff = mYSsim[i] - mSMean;
                ssim_stdv += diff * diff;
            }
        }
        if (mMode == EVL_PSNR || mMode == EVL_BOTH) {
            psnr_stdv = sqrt(psnr_stdv / (mCount - 1));
        }
        if (mMode == EVL_SSIM || mMode == EVL_BOTH) {
            ssim_stdv = sqrt(ssim_stdv / (mCount - 1));
        }
    }

    fprintf(stderr, "%d frames (CPU: %lu s) psnr mean: %.2f ssim mean:%.2f psnr stdv: %.2f ssim stdv: %.2f\n",
            mCount, (unsigned long) ((clock() - mStartClock) / CLOCKS_PER_SEC), mPMean, mSMean, psnr_stdv, ssim_stdv);
}

void Envaluer::ssim_4x4x2_core(const uint8_t *pix1, int stride1,
                               const uint8_t *pix2, int stride2,
                               int sums[2][4])
{
    int x, y, z;
    for(z=0; z<2; z++)
    {
        uint32_t s1=0, s2=0, ss=0, s12=0;
        for(y=0; y<4; y++)
            for(x=0; x<4; x++)
            {
                int a = pix1[x+y*stride1];
                int b = pix2[x+y*stride2];
                s1  += a;
                s2  += b;
                ss  += a*a;
                ss  += b*b;
                s12 += a*b;
            }
        sums[z][0] = s1;
        sums[z][1] = s2;
        sums[z][2] = ss;
        sums[z][3] = s12;
        pix1 += 4;
        pix2 += 4;
    }
}

float Envaluer::ssim_end1( int s1, int s2, int ss, int s12 )
{
    static const int ssim_c1 = (int)(.01*.01*255*255*64 + .5);
    static const int ssim_c2 = (int)(.03*.03*255*255*64*63 + .5);
    int vars = ss*64 - s1*s1 - s2*s2;
    int covar = s12*64 - s1*s2;
    return (float)(2*s1*s2 + ssim_c1) * (float)(2*covar + ssim_c2)\
           / ((float)(s1*s1 + s2*s2 + ssim_c1) * (float)(vars + ssim_c2));
}

float Envaluer::ssim_end4( int sum0[5][4], int sum1[5][4], int width )
{
    int i;
    float ssim = 0.0;
    for( i = 0; i < width; i++ )
        ssim += ssim_end1( sum0[i][0] + sum0[i+1][0] + sum1[i][0] + sum1[i+1][0],
                           sum0[i][1] + sum0[i+1][1] + sum1[i][1] + sum1[i+1][1],
                           sum0[i][2] + sum0[i+1][2] + sum1[i][2] + sum1[i+1][2],
                           sum0[i][3] + sum0[i+1][3] + sum1[i][3] + sum1[i+1][3] );
    return ssim;
}

float Envaluer::compute_ssim(uint8_t *pix1, int stride1,
                   uint8_t *pix2, int stride2,
                   int width, int height)
{
    int x, y, z;
    float ssim = 0.0;
    // printf("[%s:%d]==\n", __FUNCTION__, __LINE__);
    int (*sum0)[4] = (int (*)[4])x264_alloca(4 * (width/4+3) * sizeof(int));
    int (*sum1)[4] = (int (*)[4])x264_alloca(4 * (width/4+3) * sizeof(int));
    // void *sumTmp0 = (void *)sum0;
    // void *sumTmp1 = (void *)sum1;
    width >>= 2;
    height >>= 2;
    z = 0;
    // printf("[%s:%d]width:%d height:%d\n", __FUNCTION__, __LINE__, width, height);
    for( y = 1; y < height; y++ )
    {
        for( ; z <= y; z++ )
        {
            int (*t)[4] = sum0;
            sum0 = sum1;
            sum1 = t;
            // XCHG( , sum0, sum1 );
            for( x = 0; x < width; x+=2 )
                ssim_4x4x2_core( &pix1[4*(x+z*stride1)], stride1, &pix2[4*(x+z*stride2)], stride2, &sum0[x] );
        }
        for( x = 0; x < width-1; x += 4 ) {
            ssim += ssim_end4( sum0+x, sum1+x, X264_MIN(4,width-x-1) );
            // printf("[%s:%d]==ssim:%.3f\n", __FUNCTION__, __LINE__, ssim);
        }
    }
    // printf("[%s:%d]==\n", __FUNCTION__, __LINE__);
    return ssim / ((width-1) * (height-1));
}

float Envaluer::compute(uint8_t *org_buf, uint8_t *new_buf) {
    double yrmse = 0;
    double diff = 0;
    int inc = 1;
    int i = 0;
    // printf("[%s:%d]==\n", __FUNCTION__, __LINE__);
    if (++mCount > mBufSize) {
      mBufSize += 0xffff;
      if (mMode == EVL_PSNR || mMode == EVL_BOTH) {
        mYPsnr = (double *)realloc(mYPsnr, mBufSize * sizeof(*mYPsnr));
        if (!mYPsnr) {
            return -1;
        }
      }
      if (mMode == EVL_SSIM || mMode == EVL_BOTH) {
      mYSsim = (double *)realloc(mYSsim, mBufSize * sizeof(*mYSsim));
        if (!mYSsim) {
            return -1;
        }
      }
    }
    if (mMode == EVL_SSIM || mMode == EVL_BOTH) {
        mSMean += mYSsim[mCount - 1] = compute_ssim(org_buf, mWidth, new_buf, mWidth, mWidth, mHeight);
        // printf("ssim %.3f\n", mYSsim[mCount - 1]);
        mSsimCallback(mYSsim[mCount - 1], mUsr);
    }
    if (mMode == EVL_PSNR || mMode == EVL_BOTH) {
        for (yrmse = 0, i = inc - 1; i < (inc == 1 ? mYuvSize : mWidth*mHeight); i += inc) {
            diff = org_buf[i] - new_buf[i];
            yrmse += diff * diff;
        }
        mPMean += mYPsnr[mCount - 1] = yrmse ? 20 * (log10(255 / sqrt(yrmse / mYuvSize))) : 0;
        // printf("psnr %.3f\n", mYPsnr[mCount - 1]);
        mPsnrCallback(mYPsnr[mCount - 1], mUsr);
    }
    return 0;
}

