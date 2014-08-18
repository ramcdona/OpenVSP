//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#if !defined(VSPCOMPGEOMSCREEN__INCLUDED_)
#define VSPCOMPGEOMSCREEN__INCLUDED_

#include "VspScreenQt.h"

class CompGeomScreenPrivate;
class CompGeomScreen : public VspScreenQt
{
    Q_DECLARE_PRIVATE( CompGeomScreen )
public:
    CompGeomScreen( ScreenMgr* mgr );
    ~CompGeomScreen();
};


#endif // VSPCOMPGEOMSCREEN__INCLUDED_
