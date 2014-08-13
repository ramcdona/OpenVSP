//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

#include "ExitStatus.h"

// Bitwise adds ecode to the current exit status code and returns to current exit status code
int vsp_add_and_get_estatus( int ecode )
{
    static int exit_status_code = ESTATUS_NO_ERRORS;
    exit_status_code |= ecode;

    return exit_status_code;
}

