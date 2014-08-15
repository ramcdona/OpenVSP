//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
/// \class SetEditorScreen
/// View and Edit Geom Sets
//
//////////////////////////////////////////////////////////////////////

#if !defined(VSPSETEDITORSCREEN__INCLUDED_)
#define VSPSETEDITORSCREEN__INCLUDED_

#include "VspScreenQt.h"

class SetEditorScreenPrivate;
class SetEditorScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( SetEditorScreen )
public:
    SetEditorScreen( ScreenMgr* mgr );
    ~SetEditorScreen();
    void Show() Q_DECL_OVERRIDE;
};

#endif
