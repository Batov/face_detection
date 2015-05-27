#include <xdc/std.h>

#ifdef __TI_COMPILER_VERSION__ /* XDAIS Rule 13 - this #pragma should only apply to TI codegen */
#pragma CODE_SECTION(TRIK_VIDTRANSCODE_CV_control, ".text:algControl")
#pragma CODE_SECTION(TRIK_VIDTRANSCODE_CV_process, ".text:algProcess")
#pragma CODE_SECTION(TRIK_VIDTRANSCODE_CV_initObj, ".text:algInit")
#pragma CODE_SECTION(TRIK_VIDTRANSCODE_CV_free,    ".text:algFree")
#pragma CODE_SECTION(TRIK_VIDTRANSCODE_CV_alloc,   ".text:algAlloc")
#endif

#include "trik_vidtranscode_cv.h"

Int TRIK_VIDTRANSCODE_CV_alloc(const IALG_Params* _algParams, IALG_Fxns** _algFxns, IALG_MemRec _algMemTab[]);
Int TRIK_VIDTRANSCODE_CV_free(IALG_Handle _algHandle, IALG_MemRec _algMemTab[]);
Int TRIK_VIDTRANSCODE_CV_initObj(IALG_Handle _algHandle, const IALG_MemRec _algMemTab[], IALG_Handle _algParent, const IALG_Params* _algParams);
XDAS_Int32 TRIK_VIDTRANSCODE_CV_process(IVIDTRANSCODE_Handle _algHandle, XDM1_BufDesc* _xdmInBufs, XDM_BufDesc* _xdmOutBufs, IVIDTRANSCODE_InArgs* _vidInArgs, IVIDTRANSCODE_OutArgs* _vidOutArgs);
XDAS_Int32 TRIK_VIDTRANSCODE_CV_control(IVIDTRANSCODE_Handle _algHandle, IVIDTRANSCODE_Cmd _vidCmd, IVIDTRANSCODE_DynamicParams* _vidDynParams, IVIDTRANSCODE_Status* _vidStatus);


#define TRIK_VIDTRANSCODE_CV_IALGFXNS  \
    &TRIK_VIDTRANSCODE_CV_IALG,			/* module ID */				\
    NULL,					/* activate */				\
    TRIK_VIDTRANSCODE_CV_alloc,			/* alloc */				\
    NULL,					/* control (NULL => no control ops) */	\
    NULL,					/* deactivate */			\
    TRIK_VIDTRANSCODE_CV_free,			/* free */				\
    TRIK_VIDTRANSCODE_CV_initObj,		/* init */				\
    NULL,					/* moved */				\
    NULL					/* numAlloc (NULL => IALG_MAXMEMRECS) */

/*
 *  ======== TRIK_VIDTRANSCODE_CV_FXNS ========
 *  This structure defines TRIK's implementation of the IVIDTRANSCODE interface
 *  for the TRIK_VIDTRANSCODE_CV module.
 */
IVIDTRANSCODE_Fxns TRIK_VIDTRANSCODE_CV_FXNS = {
    {TRIK_VIDTRANSCODE_CV_IALGFXNS},
    TRIK_VIDTRANSCODE_CV_process,
    TRIK_VIDTRANSCODE_CV_control,
};

/*
 *  ======== TRIK_VIDTRANSCODE_CV_IALG ========
 *  This structure defines TRIK's implementation of the IALG interface
 *  for the TRIK_VIDTRANSCODE_CV module.
 */
#ifdef __TI_COMPILER_VERSION__ /* satisfy XDAIS symbol requirement without any overhead */

# if defined(__TI_ELFABI__) || defined(__TI_EABI_SUPPORT__)
/* Symbol doesn't have any leading underscores */
asm("TRIK_VIDTRANSCODE_CV_IALG .set TRIK_VIDTRANSCODE_CV_FXNS");
# else // defined(__TI_ELFABI__) || defined(__TI_EABI_SUPPORT__)
/* Symbol has a single leading underscore */
asm("_TRIK_VIDTRANSCODE_CV_IALG .set _TRIK_VIDTRANSCODE_CV_FXNS");
# endif // !defined(__TI_ELFABI__) && !defined(__TI_EABI_SUPPORT__)

#else // __TI_COMPILER_VERSION__
/*
 *  We duplicate the structure here to allow this code to be compiled and
 *  run using non-TI toolchains at the expense of unnecessary data space
 *  consumed by the definition below.
 */
IALG_Fxns TRIK_VIDTRANSCODE_CV_IALG = {
    TRIK_VIDTRANSCODE_CV_IALGFXNS
};
#endif // !__TI_COMPILER_VERSION__




#include <string.h>
#include <limits.h>
#include "internal/vidtranscode_cv.h"

static const char s_version[] = "1.00.00.00";



/*
 *  ======== TRIK_VIDTRANSCODE_CV_alloc ========
 *  Return a table of memory descriptors that describe the memory needed
 *  to construct our object.
 */
/* ARGSUSED - this line tells the TI compiler not to warn about unused args. */
Int TRIK_VIDTRANSCODE_CV_alloc(
    const IALG_Params*		_algParams,
    IALG_Fxns**			_algFxns,
    IALG_MemRec			_algMemTab[])
{
    /* Request memory for my object, ignoring algParams */
    _algMemTab[0].size		= sizeof(TrikCvHandle);
    _algMemTab[0].alignment	= 0;
    _algMemTab[0].space		= IALG_EXTERNAL;
    _algMemTab[0].attrs		= IALG_PERSIST;
    _algMemTab[1].size		= TrikCvFastRamSize;
    _algMemTab[1].alignment	= 0;
    _algMemTab[1].space		= IALG_DARAM0;
    _algMemTab[1].attrs		= IALG_PERSIST;

    /* Return the number of records in the memTab */
    return 2;
}




/*
 *  ======== TRIK_VIDTRANSCODE_CV_free ========
 *  Return a table of memory pointers that should be freed.  Note
 *  that this should include *all* memory requested in the
 *  alloc operation above.
 */
/* ARGSUSED - this line tells the TI compiler not to warn about unused args. */
Int TRIK_VIDTRANSCODE_CV_free(
    IALG_Handle		_algHandle,
    IALG_MemRec		_algMemTab[])
{
    TrikCvHandle* handle = (TrikCvHandle*)_algHandle;

    trikCvHandleDestroy(handle);

    /* Returned data must match one returned in alloc */
    _algMemTab[0].base		= handle;
    _algMemTab[0].size		= sizeof(TrikCvHandle);
    _algMemTab[0].alignment	= 0;
    _algMemTab[0].space		= IALG_EXTERNAL;
    _algMemTab[0].attrs		= IALG_PERSIST;
    _algMemTab[1].base		= handle->m_fastRam;
    _algMemTab[1].size		= handle->m_fastRamSize;
    _algMemTab[1].alignment	= 0;
    _algMemTab[1].space		= IALG_DARAM0;
    _algMemTab[1].attrs		= IALG_PERSIST;

    /* Return the number of records in the memTab */
    return 2;
}




/*
 *  ======== TRIK_VIDTRANSCODE_CV_initObj ========
 *  Initialize the memory allocated on our behalf (including our object).
 */
/* ARGSUSED - this line tells the TI compiler not to warn about unused args. */
Int TRIK_VIDTRANSCODE_CV_initObj(
    IALG_Handle		_algHandle,
    const IALG_MemRec	_algMemTab[],
    IALG_Handle		_algParent,
    const IALG_Params*	_algParams)
{
    TrikCvHandle* handle = (TrikCvHandle*)_algHandle;
    XDAS_Int32 res;

    handle->m_fastRam     = _algMemTab[1].base;
    handle->m_fastRamSize = _algMemTab[1].size;

    if ((res = trikCvHandleInit(handle)) != IALG_EOK)
        return res;
    if ((res = trikCvHandleSetupParams(handle, (TRIK_VIDTRANSCODE_CV_Params*)_algParams)) != IALG_EOK)
        return res;
    if ((res = trikCvHandleSetupDynamicParams(handle, NULL)) != IALG_EOK)
        return res;

    return IALG_EOK;
}




/*
 *  ======== TRIK_VIDTRANSCODE_CV_process ========
 */
XDAS_Int32 TRIK_VIDTRANSCODE_CV_process(
    IVIDTRANSCODE_Handle	_algHandle,
    XDM1_BufDesc*		_xdmInBufs,
    XDM_BufDesc*		_xdmOutBufs,
    IVIDTRANSCODE_InArgs*	_vidInArgs,
    IVIDTRANSCODE_OutArgs*	_vidOutArgs)
{
    TrikCvHandle* handle = (TrikCvHandle*)_algHandle;
    TRIK_VIDTRANSCODE_CV_InArgs*  vidInArgs  = (TRIK_VIDTRANSCODE_CV_InArgs*)_vidInArgs;
    TRIK_VIDTRANSCODE_CV_OutArgs* vidOutArgs = (TRIK_VIDTRANSCODE_CV_OutArgs*)_vidOutArgs;

    XDM1_SingleBufDesc* xdmInBuf;
    XDM1_SingleBufDesc* xdmOutBuf;
    TrikCvImageBuffer inImage;
    TrikCvImageBuffer outImage;

    XDAS_Int32 res;

    if (   (vidInArgs->base.size  != sizeof(TRIK_VIDTRANSCODE_CV_InArgs))
        || (vidOutArgs->base.size != sizeof(TRIK_VIDTRANSCODE_CV_OutArgs)))
    {
        XDM_SETUNSUPPORTEDPARAM(vidOutArgs->base.extendedError);
        return IVIDTRANSCODE_EUNSUPPORTED;
    }

    if (   _xdmInBufs->numBufs != 1
        || _xdmOutBufs->numBufs < handle->m_params.base.numOutputStreams)
    {
        XDM_SETUNSUPPORTEDPARAM(vidOutArgs->base.extendedError);
        return IVIDTRANSCODE_EFAIL;
    }


    xdmInBuf = &_xdmInBufs->descs[0];
    if (   xdmInBuf->buf == NULL
        || vidInArgs->base.numBytes < 0
        || vidInArgs->base.numBytes > xdmInBuf->bufSize)
    {
        XDM_SETUNSUPPORTEDPARAM(vidOutArgs->base.extendedError);
        return IVIDTRANSCODE_EFAIL;
    }

    XDM_SETACCESSMODE_READ(xdmInBuf->accessMask);
    vidOutArgs->base.bitsConsumed			= vidInArgs->base.numBytes * CHAR_BIT;
    vidOutArgs->base.decodedPictureType			= IVIDEO_NA_PICTURE;
    vidOutArgs->base.decodedPictureStructure		= IVIDEO_CONTENTTYPE_NA;
    vidOutArgs->base.decodedHeight			= handle->m_dynamicParams.inputHeight;
    vidOutArgs->base.decodedWidth			= handle->m_dynamicParams.inputWidth;

    inImage.m_ptr		= xdmInBuf->buf;
    inImage.m_size		= vidInArgs->base.numBytes;

    if (handle->m_params.base.numOutputStreams == 1)
    {
        xdmOutBuf			= &vidOutArgs->base.encodedBuf[0];
        xdmOutBuf->buf			= _xdmOutBufs->bufs[0];
        xdmOutBuf->bufSize		= _xdmOutBufs->bufSizes[0];
        xdmOutBuf->accessMask		= 0;
        outImage.m_ptr			= xdmOutBuf->buf;
        outImage.m_size			= xdmOutBuf->bufSize;
        memset(outImage.m_ptr, 0, outImage.m_size);
    }
    else
    {
        xdmOutBuf			= NULL;
        outImage.m_ptr			= NULL;
        outImage.m_size			= 0;
    }

    if ((res = trikCvProceedImage(handle, &inImage, &outImage, &vidInArgs->alg, &vidOutArgs->alg)) != IVIDTRANSCODE_EOK)
    {
        XDM_SETCORRUPTEDDATA(vidOutArgs->base.extendedError);
        return res;
    }

    if (xdmOutBuf)
    {
        xdmOutBuf->bufSize = outImage.m_size;
        XDM_SETACCESSMODE_WRITE(xdmOutBuf->accessMask);

        vidOutArgs->base.bitsGenerated[0]		= xdmOutBuf->bufSize * CHAR_BIT;
        vidOutArgs->base.encodedPictureType[0]		= vidOutArgs->base.decodedPictureType;
        vidOutArgs->base.encodedPictureStructure[0]	= vidOutArgs->base.decodedPictureStructure;
        vidOutArgs->base.outputID[0]			= vidInArgs->base.inputID;
        vidOutArgs->base.inputFrameSkipTranscodeFlag[0]	= XDAS_FALSE;
    }

    vidOutArgs->base.outBufsInUseFlag			= XDAS_FALSE;

    return IVIDTRANSCODE_EOK;
}




/*
 *  ======== TRIK_VIDTRANSCODE_CV_control ========
 */
XDAS_Int32 TRIK_VIDTRANSCODE_CV_control(
    IVIDTRANSCODE_Handle		_algHandle,
    IVIDTRANSCODE_Cmd			_vidCmd,
    IVIDTRANSCODE_DynamicParams*	_vidDynParams,
    IVIDTRANSCODE_Status*		_vidStatus)
{
    TrikCvHandle* handle = (TrikCvHandle*)_algHandle;
    XDAS_Int32 retVal = IVIDTRANSCODE_EFAIL;

    /* initialize for the general case where we don't access the data buffer */
    XDM_CLEARACCESSMODE_READ(_vidStatus->data.accessMask);
    XDM_CLEARACCESSMODE_WRITE(_vidStatus->data.accessMask);

    switch (_vidCmd)
    {
        case XDM_GETSTATUS:
        case XDM_GETBUFINFO:
            _vidStatus->extendedError = 0;

            _vidStatus->bufInfo.minNumInBufs = 1;
            _vidStatus->bufInfo.minNumOutBufs = 1;
            _vidStatus->bufInfo.minInBufSize[0] = 0;
            _vidStatus->bufInfo.minOutBufSize[0] = 0;

            XDM_SETACCESSMODE_WRITE(_vidStatus->data.accessMask);
            retVal = IVIDTRANSCODE_EOK;
            break;

        case XDM_SETPARAMS:
            if (_vidDynParams->size == sizeof(TRIK_VIDTRANSCODE_CV_DynamicParams))
                retVal = trikCvHandleSetupDynamicParams(handle, (TRIK_VIDTRANSCODE_CV_DynamicParams*)_vidDynParams);
            else
                retVal = IVIDTRANSCODE_EUNSUPPORTED;
            break;

        case XDM_RESET:
        case XDM_SETDEFAULT:
            retVal = trikCvHandleSetupDynamicParams(handle, NULL);
            break;

        case XDM_FLUSH:
            retVal = IVIDTRANSCODE_EOK;
            break;

        case XDM_GETVERSION:
            if (_vidStatus->data.buf != NULL && _vidStatus->data.bufSize >= strlen(s_version)+1)
            {
                memcpy(_vidStatus->data.buf, s_version, strlen(s_version)+1);
                XDM_SETACCESSMODE_WRITE(_vidStatus->data.accessMask);
                retVal = IVIDTRANSCODE_EOK;
            }
            else
                retVal = IVIDTRANSCODE_EFAIL;
            break;

        default:
            /* unsupported cmd */
            retVal = IVIDTRANSCODE_EFAIL;
            break;
    }

    return retVal;
}


