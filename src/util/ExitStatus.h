//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

#if !defined(EXITSTATUS__INCLUDED_)
#define EXITSTATUS__INCLUDED_

enum
{
    ESTATUS_NO_ERRORS = 0,
    ESTATUS_GENERAL_ERROR = 1,
    ESTATUS_INVALID_FILE_ERROR = 2
}; // Exit status error codes that can be bit masked together if multiple errors

int vsp_add_and_get_estatus( int ecode );

#endif
