//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
/// \class CfdMeshScreen
/// \author J.R Gloudemans
//
//////////////////////////////////////////////////////////////////////

#if !defined(VSPCFDMESHSCREEN__INCLUDED_)
#define VSPCFDMESHSCREEN__INCLUDED_

#include "VspScreenQt.h"
#include <string>
#include <vector>

class Parm;
class DrawObj;
class CfdMeshScreenPrivate;
class CfdMeshScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( CfdMeshScreen )
public:
    CfdMeshScreen( ScreenMgr* mgr );
    void Show() Q_DECL_OVERRIDE;
    void Hide() Q_DECL_OVERRIDE;
    ~CfdMeshScreen();

    void AddOutputText( const std::string &text );

    void parm_changed( Parm* parm );

    void LoadDrawObjs( std::vector< DrawObj* > &draw_obj_vec );
};

#endif
