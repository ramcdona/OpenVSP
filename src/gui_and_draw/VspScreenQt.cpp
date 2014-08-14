//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "VspScreenQt.h"
#include "VspScreenQt_p.h"
#include "ScreenMgr.h"
#include <QWidget>

VspScreenQt::VspScreenQt( VspScreenQtPrivate & dd, ScreenMgr * mgr ) :
    VspScreen( mgr ),
    d_ptr( &dd )
{
}

void VspScreenQt::Show()
{
    Update();
    d_func()->widget()->show();
}

void VspScreenQt::Hide()
{
    d_func()->widget()->hide();
}

bool VspScreenQt::IsShown()
{
    return d_func()->widget()->isVisible();
}

void VspScreenQt::SetNonModal()
{
    d_func()->widget()->setWindowModality( Qt::NonModal) ;
}

bool VspScreenQt::Update()
{
    return false;
}

VspScreenQt::~VspScreenQt()
{
}

VspScreenQtPrivate::VspScreenQtPrivate( VspScreenQt * q ) :
    q_ptr( q )
{
}

Vehicle* VspScreenQtPrivate::veh() {
    return GetScreenMgr()->GetVehiclePtr();
}

ScreenMgr* VspScreenQtPrivate::GetScreenMgr() {
    return q_func()->GetScreenMgr();
}

void VspScreenQtPrivate::SetUpdateFlag() {
    GetScreenMgr()->SetUpdateFlag( true );
}

VspScreenQtPrivate::~VspScreenQtPrivate()
{
}
