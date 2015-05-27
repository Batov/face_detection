/*
 *  ======== package.xs ========
 *    Implementation of the xdc.IPackage interface.
 */

/*
 *  ======== getLibs ========
 *  Determine the name of the library to use, based on the program
 *  configuration (prog).
 */
function getLibs(prog)
{
    var name = "";
    var suffix = "";

    /*
     * The libraries in this package were built with the XDC Tools.
     *
     * If this package was built with XDC 3.10 or later, we can use the
     * findSuffix() method to locate a compatible library in this package.
     * Otherwise, we fall back to providing a lib with the suffix matching
     * the target of the executable using this library.
     */
    if ("findSuffix" in prog.build.target) {
        suffix = prog.build.target.findSuffix(this);
        if (suffix == null) {
            /* no matching lib found in this package, return "" */
            return ("");
        }
    } else {
        suffix = prog.build.target.suffix;
    }

    /* And finally, the location of the libraries are in lib/<profile>/* */
    name = "lib/" + this.profile + "/trik_vidtranscode_cv.a" + suffix;

    return (name);
}
