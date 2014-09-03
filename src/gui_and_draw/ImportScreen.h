//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//////////////////////////////////////////////////////////////////////

#ifndef VSPIMPORTSCREEN__INCLUDED_
#define VSPIMPORTSCREEN__INCLUDED_

#include "VspScreenQt.h"
#include <string>

class ImportScreenPrivate;
class ImportScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( ImportScreen )
public:
//  enum {  STEREOLITH, NASCART, CART3D_TRI, XSEC_SURF, XSEC_MESH };
    ImportScreen( ScreenMgr* mgr );
    ~ImportScreen();

    std::string ImportFile( int type );
};

#endif // VSPIMPORTSCREEN__INCLUDED_
