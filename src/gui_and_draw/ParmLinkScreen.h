//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#ifndef PARMLINKCREEN_H__INCLUDED_
#define PARMLINKCREEN_H__INCLUDED_

#include "VspScreenQt.h"

class ParmLinkScreenPrivate;
class ParmLinkScreen : public VspScreenQt
{
    VSP_DECLARE_PRIVATE( ParmLinkScreen )
public:
    ParmLinkScreen( ScreenMgr* mgr );
    ~ParmLinkScreen();

    void Show();

#if 0
    virtual void SetTitle( const char* name );
    virtual void Parm_changed( Parm* parm ) {}
    virtual void ClearButtonParms();

    virtual void Show(Geom* geomPtr);
#endif

    virtual void CompGroupLinkChange();

#if 0
    virtual void RegisterParmButton( ParmButton* b )  { m_ParmButtonVec.push_back( b ); }
    virtual void RemoveAllRefs( GeomBase* g );
#endif
};

#endif // PARMLINKCREEN_H__INCLUDED_
