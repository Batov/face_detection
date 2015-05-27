#ifndef TRIK_VIDTRANSCODE_CV_INTERNAL_CV_HSV_RANGE_DETECTOR_SEQPASS_HPP_
#define TRIK_VIDTRANSCODE_CV_INTERNAL_CV_HSV_RANGE_DETECTOR_SEQPASS_HPP_

#ifndef __cplusplus
#error C++-only header
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <cassert>
#include <cmath>
#include <c6x.h>

#include "internal/stdcpp.hpp"


/* **** **** **** **** **** */ namespace trik /* **** **** **** **** **** */ {

/* **** **** **** **** **** */ namespace cv /* **** **** **** **** **** */ {

#warning Eliminate global var too

typedef struct ColorRange {
  uint8_t h0;
  uint8_t h1;
  uint8_t s0;
  uint8_t s1;
  uint8_t v0;
  uint8_t v1;
} ColorRange;

typedef struct Cluster {
  uint8_t h;
  uint8_t s;
  uint8_t v;
} Image;

typedef struct Roi {
  uint16_t left_p;
  uint16_t left_n;
  uint16_t top_p;
  uint16_t top_n;
  uint16_t right_p;
  uint16_t right_n;
  uint16_t bot_p;
  uint16_t bot_n;
} Roi;

typedef struct ImageData {
  uint16_t width;
  uint16_t height;
} ImageData;

typedef union U_Hsv8x3 {
  struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
    uint8_t none;
  } parts;
  uint32_t whole;
} U_Hsv8x3;


//static int32_t s_hsvClusters[cstrs_max][cstrs_max][cstrs_max];

const uint16_t s_dim = 256;
static uint32_t s_pHueHistogram[s_dim];
static uint32_t s_pSatHistogram[s_dim];
static uint32_t s_pValHistogram[s_dim];

static uint32_t s_nHueHistogram[s_dim];
static uint32_t s_nSatHistogram[s_dim];
static uint32_t s_nValHistogram[s_dim];

class HsvRangeDetector
{
  private:
    uint32_t m_maxHue;
    uint32_t m_maxSat;
    uint32_t m_maxVal;
    ImageData m_image;
    Roi       m_roi;

    void initImg(uint32_t _imgWidth, uint32_t _imgHeight, int _detectZoneScale) 
    {
      m_image.width = _imgWidth;
      m_image.height = _imgHeight;

      uint16_t hHeight = _imgHeight/2;
      uint16_t hWidth  = _imgWidth/2;
      uint16_t step    = _imgHeight/_detectZoneScale;

      //ROI bounds
      m_roi.left_p  = hWidth - step;
      m_roi.right_p = hWidth + step;
      m_roi.top_p   = hHeight - step;
      m_roi.bot_p   = hHeight + step;

      //Ih bounds
      m_roi.left_n  = m_roi.left_p - step;
      m_roi.right_n = m_roi.right_p + step;
      m_roi.top_n   = m_roi.top_p - step;
      m_roi.bot_n   = m_roi.bot_p + step;
    } 

  public:
    HsvRangeDetector(int _imgWidth, int _imgHeight, int _detectZoneScale) 
    {
        initImg(_imgWidth, _imgHeight, _detectZoneScale);
    }

    void detect(uint16_t& _h, uint16_t& _hTol, 
                uint16_t& _s, uint16_t& _sTol, 
                uint16_t& _v, uint16_t& _vTol, uint64_t* _rgb888hsv) 
    {
      const uint64_t* restrict img = _rgb888hsv;

    //init arrays
      memset(s_pHueHistogram, 0, s_dim*sizeof(uint32_t));
      memset(s_pSatHistogram, 0, s_dim*sizeof(uint32_t));
      memset(s_pValHistogram, 0, s_dim*sizeof(uint32_t));

      memset(s_nHueHistogram, 0, s_dim*sizeof(uint32_t));
      memset(s_nSatHistogram, 0, s_dim*sizeof(uint32_t));
      memset(s_nValHistogram, 0, s_dim*sizeof(uint32_t));

    //Clusterize image
      m_maxHue = 0;
      m_maxSat = 0;
      m_maxVal = 0;
      int32_t maxHueVal = 0;
      int32_t maxSatVal = 0;
      int32_t maxValVal = 0;

      U_Hsv8x3 pixel;
      uint8_t hue,sat,val;

      uint32_t* restrict p_pHueHistogram = s_pHueHistogram;
      uint32_t* restrict p_nHueHistogram = s_nHueHistogram;

      uint32_t* restrict p_pSatHistogram = s_pSatHistogram;
      uint32_t* restrict p_nSatHistogram = s_nSatHistogram;

      uint32_t* restrict p_pValHistogram = s_pValHistogram;
      uint32_t* restrict p_nValHistogram = s_nValHistogram;
      for (int row = 0; row < m_image.height; row++) {
        for (int col = 0; col < m_image.width; col++) {
          uint32_t p = _loll(*(img++));
          hue = static_cast<uint8_t>(p);
          sat = static_cast<uint8_t>(p>>8);
          val = static_cast<uint8_t>(p>>16);
          
          //positive part of image
          if(m_roi.left_p  < col 
          && m_roi.right_p > col 
          && m_roi.top_p   < row 
          && m_roi.bot_p   > row) {
            p_pHueHistogram[hue]++;
            p_pSatHistogram[sat]++;
            p_pValHistogram[val]++;

            //remember Cluster with highest positive occurrence
            if (p_pHueHistogram[hue] > maxHueVal) {
              m_maxHue = hue;
              maxHueVal = p_pHueHistogram[hue];
            }
            if (p_pSatHistogram[sat] > maxSatVal) {
              m_maxSat = sat;
              maxSatVal = p_pSatHistogram[sat];
            }
            if (p_pValHistogram[val] > maxValVal) {
              m_maxVal = val;
              maxValVal = p_pValHistogram[val];
            }

          } //negative part of image
/*
          else if(m_roi.left_n > col 
          || m_roi.right_n     < col 
          || m_roi.top_n       > row 
          || m_roi.bot_n       < row) {
            p_nHueHistogram[hue]++;
            p_nSatHistogram[sat]++;
            p_nValHistogram[val]++;
          }
*/
        }
      }

      _h = static_cast<uint16_t>(static_cast<double>(m_maxHue)*1.4f);
      _hTol = 20;
      _s = static_cast<uint16_t>(static_cast<double>(m_maxSat)*0.39f);
      _sTol = 30;
      _v = static_cast<uint16_t>(static_cast<double>(m_maxVal)*0.39f);
      _vTol = 30;

    }

};


} /* **** **** **** **** **** * namespace cv * **** **** **** **** **** */

} /* **** **** **** **** **** * namespace trik * **** **** **** **** **** */


#endif // !TRIK_VIDTRANSCODE_CV_INTERNAL_CV_HSV_RANGE_DETECTOR_SEQPASS_HPP_
