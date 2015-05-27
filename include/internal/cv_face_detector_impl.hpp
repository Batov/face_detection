#ifndef TRIK_VIDTRANSCODE_CV_INTERNAL_CV_FACE_DETECTOR_SEQPASS_HPP_
#define TRIK_VIDTRANSCODE_CV_INTERNAL_CV_FACE_DETECTOR_SEQPASS_HPP_

#ifndef __cplusplus
#error C++-only header
#endif

#include <cassert>
#include <cmath>
#include <c6x.h>

#include "internal/stdcpp.hpp"
#include "trik_vidtranscode_cv.h"
#include "internal/cv_hsv_range_detector.hpp"


/* **** **** **** **** **** */ namespace trik /* **** **** **** **** **** */ {

/* **** **** **** **** **** */ namespace cv /* **** **** **** **** **** */ {

#define HEIGHT 240
#define WIDTH 320

#define THRESHOLD 35 

#define HUEFROM 120
#define HUETO 160
#define SATFROM 20
#define SATTO 90

#define TRUE 1
#define FALSE 0

#define DENSITY 0.7

#define IDEALFACE 1.2
#define STEP 0.2


#warning Eliminate global var
static uint64_t s_rgb888hsv[HEIGHT*WIDTH];
static uint32_t s_wi2wo[WIDTH];
static uint32_t s_hi2ho[HEIGHT];
bool detectedPixels[WIDTH*HEIGHT];
bool detectedPixels2[WIDTH*HEIGHT];

template <>
class FaceDetector<TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV422P, TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB565X> : public CVAlgorithm
{
  private:

    uint64_t m_detectRange;
    uint32_t m_detectExpected;
    double m_srcToDstShift;

    int32_t  m_targetX;
    int32_t  m_targetY;
    uint32_t m_targetPoints;

    TrikCvImageDesc m_inImageDesc;
    TrikCvImageDesc m_outImageDesc;

    static uint16_t* restrict s_mult43_div;  // allocated from fast ram
    static uint16_t* restrict s_mult255_div; // allocated from fast ram

    static void __attribute__((always_inline)) writeOutputPixel(uint16_t* restrict _rgb565ptr,
                                                                const uint32_t _rgb888)
    {
      *_rgb565ptr = ((_rgb888>>19)&0x001f) | ((_rgb888>>5)&0x07e0) | ((_rgb888<<8)&0xf800);
    }

    void __attribute__((always_inline)) drawOutputPixelBound(const int32_t targetX,
                                                             const int32_t targetY,
                                                             const int32_t targetXBot,
                                                             const int32_t targetXTop,
                                                             const int32_t targetYBot,
                                                             const int32_t targetYTop,
                                                             const TrikCvImageBuffer& _outImage,
                                                             const uint32_t _rgb888) const
    {
      const int32_t srcCol = range<int32_t>(targetXBot, targetX, targetXTop);
      const int32_t srcRow = range<int32_t>(targetYBot, targetY, targetYTop);
    
      const int32_t dstRow = s_hi2ho[srcRow];
      const int32_t dstCol = s_wi2wo[srcCol];

      const uint32_t dstOfs = dstRow*m_outImageDesc.m_lineLength + dstCol*sizeof(uint16_t);
      writeOutputPixel(reinterpret_cast<uint16_t*>(_outImage.m_ptr+dstOfs), _rgb888);
    }

void __attribute__((always_inline)) drawHorRgbLine(const int32_t targetX, 
                                                const int32_t targetY,
                                                const int32_t radius,  
                                                const TrikCvImageBuffer& _outImage,
                                                const uint32_t _rgb888)
    {
      const int32_t widthBot  = 0;
      const int32_t widthTop  = m_inImageDesc.m_width-1;
      const int32_t heightBot = 0;
      const int32_t heightTop = m_inImageDesc.m_height-1;

      for (int adj = -radius; adj <= radius; ++adj)
      {
        drawOutputPixelBound(targetX+adj  , targetY, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(targetX+adj  , targetY+1, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
         drawOutputPixelBound(targetX+adj  , targetY+2, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
      }
    }

void __attribute__((always_inline)) putMyPixel(const int32_t targetX, 
                                                const int32_t targetY,    
                                                const TrikCvImageBuffer& _outImage,
                                                const uint32_t _rgb888)
{
      const int32_t widthBot  = 0;
      const int32_t widthTop  = m_inImageDesc.m_width-1;
      const int32_t heightBot = 0;
      const int32_t heightTop = m_inImageDesc.m_height-1;
      drawOutputPixelBound(targetX,targetY, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
}

void __attribute__((always_inline)) drawVerRgbLine(const int32_t targetX, 
                                                const int32_t targetY,
                                                const int32_t radius,  
                                                const TrikCvImageBuffer& _outImage,
                                                const uint32_t _rgb888)
    {
      const int32_t widthBot  = 0;
      const int32_t widthTop  = m_inImageDesc.m_width-1;
      const int32_t heightBot = 0;
      const int32_t heightTop = m_inImageDesc.m_height-1;

      for (int adj = -radius; adj <= radius; ++adj)
      {
        drawOutputPixelBound(targetX  , targetY + adj, widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(targetX+1  , targetY + adj , widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
        drawOutputPixelBound(targetX+2  , targetY + adj , widthBot, widthTop, heightBot, heightTop, _outImage, _rgb888);
      }
    }
    

    static bool __attribute__((always_inline)) detectHsvPixel(const uint32_t _hsv,
                                                              const uint64_t _hsv_range,
                                                              const uint32_t _hsv_expect)
    {
      const uint32_t u32_hsv_det = _cmpltu4(_hsv, _hill(_hsv_range))
                                 | _cmpgtu4(_hsv, _loll(_hsv_range));

      return (u32_hsv_det == _hsv_expect);
    }

    static uint64_t DEBUG_INLINE convert2xYuyvToRgb888(const uint32_t _yuyv)
    {
      const int64_t  s64_yuyv1   = _mpyu4ll(_yuyv,
                                             (static_cast<uint32_t>(static_cast<uint8_t>(409/4))<<24)
                                            |(static_cast<uint32_t>(static_cast<uint8_t>(298/4))<<16)
                                            |(static_cast<uint32_t>(static_cast<uint8_t>(516/4))<< 8)
                                            |(static_cast<uint32_t>(static_cast<uint8_t>(298/4))    ));
      const uint32_t u32_yuyv2   = _dotpus4(_yuyv,
                                             (static_cast<uint32_t>(static_cast<uint8_t>(-208/4))<<24)
                                            |(static_cast<uint32_t>(static_cast<uint8_t>(-100/4))<< 8));
      const uint32_t u32_rgb_h   = _add2(_packh2( 0,         _hill(s64_yuyv1)),
                                         (static_cast<uint32_t>(static_cast<uint16_t>(128/4 + (-128*409-16*298)/4))));
      const uint32_t u32_rgb_l   = _add2(_packlh2(u32_yuyv2, _loll(s64_yuyv1)),
                                          (static_cast<uint32_t>(static_cast<uint16_t>(128/4 + (+128*100+128*208-16*298)/4)<<16))
                                         |(static_cast<uint32_t>(static_cast<uint16_t>(128/4 + (-128*516-16*298)/4))));
      const uint32_t u32_y1y1    = _pack2(_loll(s64_yuyv1), _loll(s64_yuyv1));
      const uint32_t u32_y2y2    = _pack2(_hill(s64_yuyv1), _hill(s64_yuyv1));
      const uint32_t u32_rgb_p1h = _clr(_shr2(_add2(u32_rgb_h, u32_y1y1), 6), 16, 31);
      const uint32_t u32_rgb_p1l =      _shr2(_add2(u32_rgb_l, u32_y1y1), 6);
      const uint32_t u32_rgb_p2h = _clr(_shr2(_add2(u32_rgb_h, u32_y2y2), 6), 16, 31);
      const uint32_t u32_rgb_p2l =      _shr2(_add2(u32_rgb_l, u32_y2y2), 6);
      const uint32_t u32_rgb_p1 = _spacku4(u32_rgb_p1h, u32_rgb_p1l);
      const uint32_t u32_rgb_p2 = _spacku4(u32_rgb_p2h, u32_rgb_p2l);
      return _itoll(u32_rgb_p2, u32_rgb_p1);
    }


   uint32_t __attribute__((always_inline)) vicinity(const uint32_t mRow,const uint32_t mCol, const bool sign)
    {
      if ((mRow>6) && (mRow<234) && (mCol>6)  && (mCol<314))
      {
        uint32_t r = 0;
        #pragma MUST_ITERATE(10,10)
        for(uint32_t i = 0; i<10; i++)
        {
        #pragma MUST_ITERATE(10,10)
          for(uint32_t j = 0; j<10; j++)
          if (sign)
            r += (detectedPixels[(mRow-5+i)*WIDTH + (mCol-5+j)]);
            else
            r += !(detectedPixels[(mRow-5+i)*WIDTH + (mCol-5+j)]);
         }
        return r;
      }
      else return 0;
      return 0;
    }

    static uint32_t DEBUG_INLINE convertRgb888ToHsv(const uint32_t _rgb888)
    {
      const uint32_t u32_rgb_or16    = _unpkhu4(_rgb888);
      const uint32_t u32_rgb_gb16    = _unpklu4(_rgb888);

      const uint32_t u32_rgb_max2    = _maxu4(_rgb888, _rgb888>>8);
      const uint32_t u32_rgb_max     = _clr(_maxu4(u32_rgb_max2, u32_rgb_max2>>8), 8, 31); // top 3 bytes were non-zeroes!
      const uint32_t u32_rgb_max_max = _pack2(u32_rgb_max, u32_rgb_max);

      const uint32_t u32_hsv_ooo_val_x256   = u32_rgb_max<<8; // get max in 8..15 bits

      const uint32_t u32_rgb_min2    = _minu4(_rgb888, _rgb888>>8);
      const uint32_t u32_rgb_min     = _minu4(u32_rgb_min2, u32_rgb_min2>>8); // top 3 bytes are zeroes
      const uint32_t u32_rgb_delta   = u32_rgb_max-u32_rgb_min;

      /* optimized by table based multiplication with power-2 divisor, simulate 255*(max-min)/max */
      const uint32_t u32_hsv_sat_x256       = s_mult255_div[u32_rgb_max]
                                            * u32_rgb_delta;

      /* optimized by table based multiplication with power-2 divisor, simulate 43*(med-min)/(max-min) */
      const uint32_t u32_hsv_hue_mult43_div = _pack2(s_mult43_div[u32_rgb_delta],
                                                     s_mult43_div[u32_rgb_delta]);
      int32_t s32_hsv_hue_x256;
      const uint32_t u32_rgb_cmp = _cmpeq2(u32_rgb_max_max, u32_rgb_gb16);
      if (u32_rgb_cmp == 0)
          s32_hsv_hue_x256 = static_cast<int32_t>((0x10000*0)/3)
                           + static_cast<int32_t>(_dotpn2(u32_hsv_hue_mult43_div,
                                                          _packhl2(u32_rgb_gb16, u32_rgb_gb16)));
      else if (u32_rgb_cmp == 1)
          s32_hsv_hue_x256 = static_cast<int32_t>((0x10000*2)/3)
                           + static_cast<int32_t>(_dotpn2(u32_hsv_hue_mult43_div,
                                                          _packlh2(u32_rgb_or16, u32_rgb_gb16)));
      else // 2, 3
          s32_hsv_hue_x256 = static_cast<int32_t>((0x10000*1)/3)
                           + static_cast<int32_t>(_dotpn2(u32_hsv_hue_mult43_div,
                                                          _pack2(  u32_rgb_gb16, u32_rgb_or16)));

      const uint32_t u32_hsv_hue_x256      = static_cast<uint32_t>(s32_hsv_hue_x256);
      const uint32_t u32_hsv_sat_hue_x256  = _pack2(u32_hsv_sat_x256, u32_hsv_hue_x256);

      const uint32_t u32_hsv               = _packh4(u32_hsv_ooo_val_x256, u32_hsv_sat_hue_x256);
      return u32_hsv;
    }

    void DEBUG_INLINE convertImageYuyvToHsv(const TrikCvImageBuffer& _inImage)
    {
      const uint32_t srcImageRowEffectiveSize       = m_inImageDesc.m_width;
      const uint32_t srcImageRowEffectiveToFullSize = m_inImageDesc.m_lineLength - srcImageRowEffectiveSize;
      const int8_t* restrict srcImageRowY     = _inImage.m_ptr;
      const int8_t* restrict srcImageRowC     = _inImage.m_ptr + m_inImageDesc.m_lineLength*m_inImageDesc.m_height;
      const int8_t* restrict srcImageToY      = srcImageRowY + m_inImageDesc.m_lineLength*m_inImageDesc.m_height;
      uint64_t* restrict rgb888hsvptr         = s_rgb888hsv;

      assert(m_inImageDesc.m_height % 4 == 0); // verified in setup
#pragma MUST_ITERATE(4, ,4)
      while (srcImageRowY != srcImageToY)
      {
        assert(reinterpret_cast<intptr_t>(srcImageRowY) % 8 == 0); // let's pray...
        assert(reinterpret_cast<intptr_t>(srcImageRowC) % 8 == 0); // let's pray...
        const uint32_t* restrict srcImageColY4 = reinterpret_cast<const uint32_t*>(srcImageRowY);
        const uint32_t* restrict srcImageColC4 = reinterpret_cast<const uint32_t*>(srcImageRowC);
        srcImageRowY += srcImageRowEffectiveSize;
        srcImageRowC += srcImageRowEffectiveSize;

        assert(m_inImageDesc.m_width % 32 == 0); // verified in setup
#pragma MUST_ITERATE(32/4, ,32/4)
        while (reinterpret_cast<const int8_t*>(srcImageColY4) != srcImageRowY)
        {
          assert(reinterpret_cast<const int8_t*>(srcImageColC4) != srcImageRowC);

          const uint32_t yy4x = *srcImageColY4++;
          const uint32_t uv4x = _swap4(*srcImageColC4++);

          const uint32_t yuyv12 = (_unpklu4(yy4x)) | (_unpklu4(uv4x)<<8);
          const uint32_t yuyv34 = (_unpkhu4(yy4x)) | (_unpkhu4(uv4x)<<8);

          const uint64_t rgb12 = convert2xYuyvToRgb888(yuyv12);
          *rgb888hsvptr++ = _itoll(_loll(rgb12), convertRgb888ToHsv(_loll(rgb12)));
          *rgb888hsvptr++ = _itoll(_hill(rgb12), convertRgb888ToHsv(_hill(rgb12)));

          const uint64_t rgb34 = convert2xYuyvToRgb888(yuyv34);
          *rgb888hsvptr++ = _itoll(_loll(rgb34), convertRgb888ToHsv(_loll(rgb34)));
          *rgb888hsvptr++ = _itoll(_hill(rgb34), convertRgb888ToHsv(_hill(rgb34)));
        }

        srcImageRowY += srcImageRowEffectiveToFullSize;
        srcImageRowC += srcImageRowEffectiveToFullSize;
      }
    }

void clasterizePixel(const uint32_t _hsv)
{
}

    void clasterizeImage()
    {
      const uint64_t* restrict rgb888hsvptr = s_rgb888hsv;
      const uint32_t width          = m_inImageDesc.m_width;
      const uint32_t height         = m_inImageDesc.m_height;

      const uint64_t u64_hsv_range  = m_detectRange;
      const uint32_t u32_hsv_expect = m_detectExpected;

      assert(m_inImageDesc.m_height % 4 == 0); // verified in setup
#pragma MUST_ITERATE(4, ,4)
      for (uint32_t srcRow=0; srcRow < height; ++srcRow)
      {

        assert(m_inImageDesc.m_width % 32 == 0); // verified in setup
#pragma MUST_ITERATE(32, ,32)
        for (uint32_t srcCol=0; srcCol < width; ++srcCol)
        {
          const uint64_t rgb888hsv = *rgb888hsvptr++;
          clasterizePixel(rgb888hsv);
          const bool det = detectHsvPixel(_loll(rgb888hsv), u64_hsv_range, u32_hsv_expect);

        }
      }
    }

    void DEBUG_INLINE proceedImageHsv(TrikCvImageBuffer& _outImage)
    {
      const uint64_t* restrict rgb888hsvptr = s_rgb888hsv;
      const uint32_t width        = WIDTH;  //= m_inImageDesc.m_width;
      const uint32_t height       = HEIGHT; // = m_inImageDesc.m_height;
      const uint32_t dstLineLength  = m_outImageDesc.m_lineLength;
      const double srcToDstShift  = m_srcToDstShift;
      const uint64_t u64_hsv_range  = m_detectRange;
      const uint32_t u32_hsv_expect = m_detectExpected;
      uint32_t targetPointsPerRow;
      uint32_t targetPointsCol;
            
      const uint32_t* restrict p_hi2ho = s_hi2ho;
      assert(m_inImageDesc.m_height % 4 == 0); // verified in setup
      
#pragma MUST_ITERATE(HEIGHT, HEIGHT)
      for (uint32_t srcRow=0; srcRow < height; ++srcRow)
      {
        const uint32_t dstRow = *(p_hi2ho++);
        uint16_t* restrict dstImageRow = reinterpret_cast<uint16_t*>(_outImage.m_ptr + dstRow*dstLineLength);
        const uint32_t* restrict p_wi2wo = s_wi2wo;
        assert(m_inImageDesc.m_width % 32 == 0); // verified in setup
#pragma MUST_ITERATE(WIDTH, WIDTH)
        for (uint32_t srcCol=0; srcCol < width; ++srcCol)
        {
          const uint32_t dstCol    = *(p_wi2wo++);
          const uint64_t rgb888hsv = *rgb888hsvptr++;

          const bool det = detectHsvPixel(_loll(rgb888hsv), u64_hsv_range, u32_hsv_expect);
          detectedPixels[srcRow*WIDTH + srcCol] = det;
           writeOutputPixel(dstImageRow+dstCol, _hill(rgb888hsv));          
        }     
      }
}

  public:
    virtual bool setup(const TrikCvImageDesc& _inImageDesc,
                       const TrikCvImageDesc& _outImageDesc,
                       int8_t* _fastRam, size_t _fastRamSize)
    {
      m_inImageDesc  = _inImageDesc;
      m_outImageDesc = _outImageDesc;

      if (   m_inImageDesc.m_width < 0
          || m_inImageDesc.m_height < 0
          || m_inImageDesc.m_width  % 32 != 0
          || m_inImageDesc.m_height % 4  != 0)
        return false;

      #define min(x,y) x < y ? x : y;
      const double srcToDstShift = min(static_cast<double>(m_outImageDesc.m_width)/m_inImageDesc.m_width, 
                                 static_cast<double>(m_outImageDesc.m_height)/m_inImageDesc.m_height);

      const uint32_t widthIn  = _inImageDesc.m_width;
      const uint32_t widthOut = _outImageDesc.m_width;
      uint32_t* restrict p_wi2wo = s_wi2wo;
      for(int i = 0; i < widthIn; i++) {
          *(p_wi2wo++) = i*srcToDstShift;
      }

      const uint32_t heightIn  = _inImageDesc.m_height;
      const uint32_t heightOut = _outImageDesc.m_height;
      uint32_t* restrict p_hi2ho = s_hi2ho;
      for(uint32_t i = 0; i < heightIn; i++) {
          *(p_hi2ho++) = i*srcToDstShift;
      }

      /* Static member initialization on first instance creation */
      if (s_mult43_div == NULL || s_mult255_div == NULL)
      {
        if (_fastRamSize < (1u<<8)*sizeof(*s_mult43_div) + (1u<<8)*sizeof(*s_mult255_div))
          return false;

        s_mult43_div  = reinterpret_cast<typeof(s_mult43_div)>(_fastRam);
        _fastRam += (1u<<8)*sizeof(*s_mult43_div);
        s_mult255_div = reinterpret_cast<typeof(s_mult255_div)>(_fastRam);
        _fastRam += (1u<<8)*sizeof(*s_mult255_div);

        s_mult43_div[0] = 0;
        s_mult255_div[0] = 0;
        for (uint32_t idx = 1; idx < (1u<<8); ++idx)
        {
          s_mult43_div[ idx] = (43u  * (1u<<8)) / idx;
          s_mult255_div[idx] = (255u * (1u<<8)) / idx;
        }
      }
      
      return true;
    }



    virtual bool run(const TrikCvImageBuffer& _inImage, TrikCvImageBuffer& _outImage,
                     const TrikCvAlgInArgs& _inArgs, TrikCvAlgOutArgs& _outArgs)
    {
      if (m_inImageDesc.m_height * m_inImageDesc.m_lineLength > _inImage.m_size)
        return false;
      if (m_outImageDesc.m_height * m_outImageDesc.m_lineLength > _outImage.m_size)
        return false;
      _outImage.m_size = m_outImageDesc.m_height * m_outImageDesc.m_lineLength;

      m_targetX = 0;
      m_targetY = 0;
      m_targetPoints = 0;
      
      uint32_t detectHueFrom = range<int32_t>(0, (static_cast<int32_t>(HUEFROM) * 255) / 359, 255); // scaling 0..359 to 0..255
      uint32_t detectHueTo   = range<int32_t>(0, (static_cast<int32_t>(HUETO) * 255) / 359, 255); // scaling 0..359 to 0..255
      uint32_t detectSatFrom = range<int32_t>(0, (static_cast<int32_t>(SATFROM) * 255) / 100, 255); // scaling 0..100 to 0..255
      uint32_t detectSatTo   = range<int32_t>(0, (static_cast<int32_t>(SATTO) * 255) / 100, 255); // scaling 0..100 to 0..255
      uint32_t detectValFrom = range<int32_t>(0, (static_cast<int32_t>(0) * 255) / 100, 255); // scaling 0..100 to 0..255
      uint32_t detectValTo   = range<int32_t>(0, (static_cast<int32_t>(100) * 255) / 100, 255); // scaling 0..100 to 0..255


      if (detectHueFrom <= detectHueTo)
      {
        m_detectRange = _itoll((detectValFrom<<16) | (detectSatFrom<<8) | detectHueFrom,
                               (detectValTo  <<16) | (detectSatTo  <<8) | detectHueTo  );
        m_detectExpected = 0x0;
      }
      else
      {
        assert(detectHueFrom > 0 && detectHueTo < 255);
        m_detectRange = _itoll((detectValFrom<<16) | (detectSatFrom<<8) | (detectHueTo  +1),
                               (detectValTo  <<16) | (detectSatTo  <<8) | (detectHueFrom-1));
        m_detectExpected = 0x1;
      }

#ifdef DEBUG_REPEAT
      for (unsigned repeat = 0; repeat < DEBUG_REPEAT; ++repeat) {
#endif

      if (m_inImageDesc.m_height > 0 && m_inImageDesc.m_width > 0)
      {
        convertImageYuyvToHsv(_inImage);
        proceedImageHsv(_outImage);
      }

#ifdef DEBUG_REPEAT
      } // repeat
#endif
        assert(m_inImageDesc.m_height > 0 && m_inImageDesc.m_width > 0); // more or less safe since no target points would be detected otherwise
        

      uint32_t TR, TC, DR, DC;
      
      TR = HEIGHT;
      TC = WIDTH;
      DR = 0;
      DC = 0;

      #pragma MUST_ITERATE(4, ,4)
      for (uint32_t srcRow=0; srcRow < HEIGHT; ++srcRow)
      {
      int32_t targetPointsPerRow = 0;
      int32_t targetPointsCol = 0;
#pragma MUST_ITERATE(32, ,32)
        for (uint32_t srcCol=0; srcCol < WIDTH; ++srcCol)
        {
      if ((detectedPixels[srcRow*WIDTH + srcCol]) && (vicinity(srcRow,srcCol,1)>THRESHOLD))
      {
            if (TR>srcRow) TR = srcRow;
            if (TC>srcCol) TC = srcCol;
            if (DR<srcRow) DR = srcRow;
            if (DC<srcCol) DC = srcCol;
            
            targetPointsPerRow += 1;
            targetPointsCol += srcCol;
            
           // putMyPixel(srcCol,srcRow,_outImage,   0x00ff00);
            
            /*putMyPixel(srcCol-1,srcRow,_outImage,   0x00ff00);
            putMyPixel(srcCol,srcRow-1,_outImage,   0x00ff00);
            putMyPixel(srcCol,srcRow+1,_outImage,   0x00ff00);
            putMyPixel(srcCol+1,srcRow,_outImage,   0x00ff00);
            
            putMyPixel(srcCol-1,srcRow-1,_outImage,   0x00ff00);
            putMyPixel(srcCol+1,srcRow-1,_outImage,   0x00ff00);
            putMyPixel(srcCol-1,srcRow+1,_outImage,   0x00ff00);
            putMyPixel(srcCol+1,srcRow+1,_outImage,   0x00ff00);
            
            putMyPixel(srcCol-2,srcRow,_outImage,   0x00ff00);
            putMyPixel(srcCol,srcRow-2,_outImage,   0x00ff00);
            putMyPixel(srcCol,srcRow+2,_outImage,   0x00ff00);
            putMyPixel(srcCol+2,srcRow,_outImage,   0x00ff00);*/
               
      }
      }
        m_targetX      += targetPointsCol;
        m_targetY      += srcRow*targetPointsPerRow;
        m_targetPoints += targetPointsPerRow;
     }
     
     uint32_t points = (DR-TR)*(DC-TC);
     const int32_t targetX = m_targetX/m_targetPoints;
     const int32_t targetY = m_targetY/m_targetPoints;
     const uint32_t targetRadius = std::ceil(std::sqrt(static_cast<float>(m_targetPoints))) + 15;
     
     float theDensity = m_targetPoints/points;

    if ((m_targetPoints > 200) && (theDensity>DENSITY))
      { 
      
       int W = DC - TC;
       int H = DR - TR;
       float Box = H/W;
       
      /* drawHorRgbLine(TC + (DC-TC)/2 , TR, (DC-TC)/2, _outImage, 0x00ff00);
       drawHorRgbLine(TC + (DC-TC)/2 , DR, (DC-TC)/2, _outImage, 0x00ff00);
       drawVerRgbLine(TC ,TR + (DR-TR)/2, (DR-TR)/2, _outImage, 0x00ff00);
       drawVerRgbLine(DC ,TR + (DR-TR)/2, (DR-TR)/2, _outImage, 0x00ff00);*/
      
        if ((Box>IDEALFACE-STEP) && (Box< IDEALFACE+STEP))
       {
       drawHorRgbLine(TC + (DC-TC)/2 , TR, (DC-TC)/2, _outImage, 0xff0000);
       drawHorRgbLine(TC + (DC-TC)/2 , DR, (DC-TC)/2, _outImage, 0xff0000);
       drawVerRgbLine(TC ,TR + (DR-TR)/2, (DR-TR)/2, _outImage, 0xff0000);
       drawVerRgbLine(DC ,TR + (DR-TR)/2, (DR-TR)/2, _outImage, 0xff0000);
       }
      
          //------------------------------------------------------------------------
     /* drawHorRgbLine(targetX , targetY - targetRadius,targetRadius, _outImage, 0x00ff00);
        drawHorRgbLine(targetX , targetY + targetRadius,targetRadius, _outImage, 0x00ff00);
        drawVerRgbLine(targetX - targetRadius , targetY ,targetRadius, _outImage, 0x00ff00);
        drawVerRgbLine(targetX + targetRadius, targetY ,targetRadius, _outImage, 0x00ff00);
       */
      //------------------------------------------------------------------------
          
        _outArgs.targetX = ((targetX - static_cast<int32_t>(m_inImageDesc.m_width) /2) * 100*2) / static_cast<int32_t>(m_inImageDesc.m_width);
        _outArgs.targetY = ((targetY - static_cast<int32_t>(m_inImageDesc.m_height)/2) * 100*2) / static_cast<int32_t>(m_inImageDesc.m_height);
      }
      else
      {
        _outArgs.targetX = 0;
        _outArgs.targetY = 0;
      }

      return true;
    }
};

uint16_t* restrict FaceDetector<TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV422P,
                                TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB565X>::s_mult43_div = NULL;
uint16_t* restrict FaceDetector<TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV422P,
                                TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB565X>::s_mult255_div = NULL;


} /* **** **** **** **** **** * namespace cv * **** **** **** **** **** */

} /* **** **** **** **** **** * namespace trik * **** **** **** **** **** */


#endif // !TRIK_VIDTRANSCODE_CV_INTERNAL_CV_FACE_DETECTOR_SEQPASS_HPP_

