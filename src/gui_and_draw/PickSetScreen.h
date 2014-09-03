//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#ifndef VSPPICKSETSCREEN__INCLUDED_
#define VSPPICKSETSCREEN__INCLUDED_

#include "VspScreenQt.h"
#include <string>

class PickSetScreenPrivate;
class PickSetScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( PickSetScreen )
public:
    PickSetScreen( ScreenMgr * );
    ~PickSetScreen();

    int PickSet( const std::string & title );
};

#endif // VSPPICKSETSCREEN__INCLUDED_
