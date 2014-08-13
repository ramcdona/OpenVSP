//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// SetEditorScreen: View and Edit Geom Sets
// J.R Gloudemans
//
//////////////////////////////////////////////////////////////////////

#if !defined(VSPSETEDITORSCREEN__INCLUDED_)
#define VSPSETEDITORSCREEN__INCLUDED_

#include "ScreenBase.h"
#include <QScopedPointer>

class Fl_Widget;
class SetEditorUI;
class SetEditorScreenPrivate;
class SetEditorScreen : public VspScreen
{
    Q_DECLARE_PRIVATE(SetEditorScreen)
    QScopedPointer<SetEditorScreenPrivate> const d_ptr;
public:
    SetEditorScreen( ScreenMgr* mgr );
    ~SetEditorScreen();
    void Show() Q_DECL_OVERRIDE;
    void Hide() Q_DECL_OVERRIDE;
    bool Update() Q_DECL_OVERRIDE;
    bool IsShown() Q_DECL_OVERRIDE;
    void SetNonModal() Q_DECL_OVERRIDE;
};

#endif
