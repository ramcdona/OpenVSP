//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#ifndef VSPDEGENGEOMSCREEN__INCLUDED_
#define VSPDEGENGEOMSCREEN__INCLUDED_

#include "VspScreenQt.h"

class DegenGeomScreenPrivate;
class DegenGeomScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( VspScreenQt )
public:
    DegenGeomScreen( ScreenMgr* mgr );
    ~DegenGeomScreen();
};

#endif // VSPDEGENGEOMSCREEN__INCLUDED_
