/*!
 *  ======== VIDTRANSCODE_CV ========
 *  VIDTRANSCODE_CV codec specification
 *
 *  This file specifies information necessary to integrate with the Codec
 *  Engine.
 *
 */
metaonly module VIDTRANSCODE_CV inherits ti.sdo.ce.vidtranscode.IVIDTRANSCODE
{
    /*!
     *  ======== ialgFxns ========
     *  name of this algorithm's xDAIS alg fxn table
     */
    override readonly config String ialgFxns = "TRIK_VIDTRANSCODE_CV_FXNS";

}
