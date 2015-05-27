/*
 *  ======== main.c ========
 */
#include <xdc/std.h>
#include <ti/sdo/ce/CERuntime.h>
#include <xdc/runtime/Log.h>
#include <xdc/runtime/Diags.h>
#include <ti/sysbios/BIOS.h>
#include <ti/ipc/Ipc.h>


/*
 *  ======== main ========
 */
Void main (Int argc, Char *argv [])
{
    Int     status;

    do {
        status = Ipc_start();
    } while (status < 0);

    /* init Codec Engine */
    CERuntime_init();

    BIOS_start();
}
