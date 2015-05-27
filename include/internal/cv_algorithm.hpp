#ifndef TRIK_VIDTRANSCODE_CV_INTERNAL_CV_ALGORITHM_HPP_
#define TRIK_VIDTRANSCODE_CV_INTERNAL_CV_ALGORITHM_HPP_

#ifndef __cplusplus
#error C++-only header
#endif

#include "internal/stdcpp.hpp"


/* **** **** **** **** **** */ namespace trik /* **** **** **** **** **** */ {

/* **** **** **** **** **** */ namespace cv /* **** **** **** **** **** */ {


class CVAlgorithm : private noncopyable
{
  public:
    virtual bool setup(const TrikCvImageDesc& _inImageDesc,
                       const TrikCvImageDesc& _outImageDesc,
                       int8_t* _fastRam, size_t _fastRamSize) = 0;
    virtual bool run(const TrikCvImageBuffer& _inImage, TrikCvImageBuffer& _outImage,
                     const TrikCvAlgInArgs& _inArgs, TrikCvAlgOutArgs& _outArgs) = 0;

    virtual ~CVAlgorithm() {}

  protected:
    CVAlgorithm() {}
};


} /* **** **** **** **** **** * namespace cv * **** **** **** **** **** */

} /* **** **** **** **** **** * namespace trik * **** **** **** **** **** */


#endif // !TRIK_VIDTRANSCODE_CV_INTERNAL_CV_ALGORITHM_HPP_
