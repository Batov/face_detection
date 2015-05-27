#ifndef TRIK_VIDTRANSCODE_CV_H_
#define TRIK_VIDTRANSCODE_CV_H_

#include <xdc/std.h>
#include <ti/xdais/ialg.h>
#include <ti/xdais/dm/ividtranscode.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


/*
 *  ======== TRIK_VIDTRANSCODE_CV ========
 *  Our implementation of the IVIDTRANSCODE interface
 */
extern IVIDTRANSCODE_Fxns TRIK_VIDTRANSCODE_CV_FXNS;
extern IALG_Fxns TRIK_VIDTRANSCODE_CV_IALG;


typedef enum TRIK_VIDTRANSCODE_CV_VideoFormat
{
  TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_UNKNOWN = 0,
  TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB888 = XDM_CUSTOMENUMBASE,
  TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB565,
  TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_RGB565X,
  TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV444,
  TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV422,
  TRIK_VIDTRANSCODE_CV_VIDEO_FORMAT_YUV422P
} TRIK_VIDTRANSCODE_CV_VideoFormat;


typedef struct TRIK_VIDTRANSCODE_CV_Params {
    IVIDTRANSCODE_Params	base;
} TRIK_VIDTRANSCODE_CV_Params;


typedef struct TRIK_VIDTRANSCODE_CV_DynamicParams {
    IVIDTRANSCODE_DynamicParams	base;

    XDAS_Int32			inputHeight;
    XDAS_Int32			inputWidth;
    XDAS_Int32			inputLineLength;

    XDAS_Int32			outputLineLength[IVIDTRANSCODE_MAXOUTSTREAMS];
} TRIK_VIDTRANSCODE_CV_DynamicParams;


typedef struct TRIK_VIDTRANSCODE_CV_InArgsAlg {
    XDAS_UInt16		detectHueFrom; // [0..359]
    XDAS_UInt16		detectHueTo;   // [0..359]
    XDAS_UInt8		detectSatFrom; // [0..100]
    XDAS_UInt8		detectSatTo;   // [0..100]
    XDAS_UInt8		detectValFrom; // [0..100]
    XDAS_UInt8		detectValTo;   // [0..100]
    XDAS_Bool     autoDetectHsv;// [true|false]
} TRIK_VIDTRANSCODE_CV_InArgsAlg;

typedef struct TRIK_VIDTRANSCODE_CV_InArgs {
    IVIDTRANSCODE_InArgs		base;
    TRIK_VIDTRANSCODE_CV_InArgsAlg	alg;
} TRIK_VIDTRANSCODE_CV_InArgs;


typedef struct TRIK_VIDTRANSCODE_CV_OutArgsAlg {
    XDAS_Int8		targetX;    // [-100..100]
    XDAS_Int8		targetY;    // [-100..100]
    XDAS_UInt8		targetSize; // [0..100]
    XDAS_UInt16		detectHue; // [0..256]
    XDAS_UInt16		detectHueTolerance;   // [0..256]
    XDAS_UInt16		detectSat; // [0..256]
    XDAS_UInt16		detectSatTolerance;   // [0..256]
    XDAS_UInt16		detectVal; // [0..256]
    XDAS_UInt16		detectValTolerance;   // [0..256]
} TRIK_VIDTRANSCODE_CV_OutArgsAlg;

typedef struct TRIK_VIDTRANSCODE_CV_OutArgs {
    IVIDTRANSCODE_OutArgs		base;
    TRIK_VIDTRANSCODE_CV_OutArgsAlg	alg;
} TRIK_VIDTRANSCODE_CV_OutArgs;


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // !TRIK_VIDTRANSCODE_CV_H_
