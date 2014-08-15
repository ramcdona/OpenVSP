//
// License terms are missing.
//
/// \class ManageViewScreen
/// The view pan/zoom panel.
//
//////////////////////////////////////////////////////////////////////

#ifndef _VSP_GUI_VIEW_MANAGER_SCREEN_H
#define _VSP_GUI_VIEW_MANAGER_SCREEN_H

#include "VspScreenQt.h"

class ManageViewScreenPrivate;
class ManageViewScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( ManageViewScreen )
public:
    ManageViewScreen( ScreenMgr * mgr );
    ~ManageViewScreen();
};
#endif
