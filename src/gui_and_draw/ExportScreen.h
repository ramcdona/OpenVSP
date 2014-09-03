//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#ifndef VSPEXPORTSCREEN__INCLUDED_
#define VSPEXPORTSCREEN__INCLUDED_

#include "VspScreenQt.h"
#include <string>

class ExportScreenPrivate;
class ExportScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( ExportScreen )
public:
    ExportScreen( ScreenMgr* mgr );
    ~ExportScreen();

    std::string ExportFile( int write_set, int type );
};

#endif // VSPEXPORTSCREEN__INCLUDED_
