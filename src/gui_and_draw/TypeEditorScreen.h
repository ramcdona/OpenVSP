//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
/// \class TypeEditorScreen
/// View and Edit Types Of Geom
/// \author J.R Gloudemans
//
//////////////////////////////////////////////////////////////////////

#ifndef VSPTYPEEDITORSCREEN__INCLUDED_
#define VSPTYPEEDITORSCREEN__INCLUDED_

#include "VspScreenQt.h"

class TypeEditorScreenPrivate;
class TypeEditorScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( TypeEditorScreen )
public:
    TypeEditorScreen( ScreenMgr* mgr );
    ~TypeEditorScreen();
};

#endif // VSPTYPEEDITORSCREEN__INCLUDED_
