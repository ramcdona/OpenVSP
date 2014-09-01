//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
/// \class ManageGeomScreen
/// Create/Delete Geom Screen
/// \author J.R Gloudemans
//
//////////////////////////////////////////////////////////////////////

#ifndef VSPMANAGEGEOMSCREEN__INCLUDED_
#define VSPMANAGEGEOMSCREEN__INCLUDED_

#include "VspScreenQt.h"
#include <string>
#include <vector>

class DrawObj;
class ManageGeomScreenPrivate;
class ManageGeomScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( ManageGeomScreen )
public:
    ManageGeomScreen( ScreenMgr* mgr );
    virtual ~ManageGeomScreen();

    void Show() Q_DECL_OVERRIDE;

    /// Get Feedback Group's name.
    virtual std::string getFeedbackGroupName();

    /// Set current geom to geom with specific ID.
    void Set( std::string geomID );

    /// Push Pick button once.
    void TriggerPickSwitch();

    void LoadDrawObjs( std::vector< DrawObj* > & draw_obj_vec );
};

#endif // VSPMANAGEGEOMSCREEN__INCLUDED_
