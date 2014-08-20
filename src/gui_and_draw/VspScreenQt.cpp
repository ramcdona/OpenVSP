//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "VspScreenQt.h"
#include "VspScreenQt_p.h"
#include "ScreenMgr.h"
#include <QFile>
#include <QWidget>
#include <QMetaProperty>
#include <QAbstractButton>

VspScreenQt::VspScreenQt( VspScreenQtPrivate & dd, ScreenMgr * mgr ) :
    VspScreen( mgr ),
    d_ptr( &dd )
{
}

void VspScreenQt::Show()
{
    if (Update()) d_func()->widget()->show();
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
    d_func()->widget()->setWindowModality( Qt::NonModal ) ;
}

bool VspScreenQt::Update()
{
    Q_D( VspScreenQt );
    if ( d->inUpdate ) return false;
    QSet<QWidget*> blocked;
    if ( d->blockSignalsInNextUpdate ) {
        // disable signals from the controls
        foreach ( QWidget * w, d->widget()->findChildren<QWidget*>() ) {
            if ( w->signalsBlocked() )
                blocked.insert( w );
            else
                w->blockSignals( true );
        }
    }
    d->inUpdate = true;
    bool rc = d->Update();
    d->inUpdate = false;
    if ( d->blockSignalsInNextUpdate ) {
        // reenable signals from the controls
        foreach ( QWidget * w, d->widget()->findChildren<QWidget*>() ) {
            if ( blocked.contains( w ) ) qDebug() << "blocked in" << w;
            w->blockSignals( blocked.contains( w ) );
        }
        d->blockSignalsInNextUpdate = false;
    }
    return rc;
}

void VspScreenQt::GuiDeviceCallBack( GuiDevice* device )
{
}

VspScreenQt::~VspScreenQt()
{
}

VspScreenQtPrivate::VspScreenQtPrivate( VspScreenQt * q ) :
    blockSignalsInNextUpdate( false ), inUpdate( false ), q_ptr( q )
{
}

Vehicle* VspScreenQtPrivate::veh() {
    return GetScreenMgr()->GetVehiclePtr();
}

ScreenMgr* VspScreenQtPrivate::GetScreenMgr() {
    return q_func()->GetScreenMgr();
}

VspScreen* VspScreenQtPrivate::GetScreen( int id ) {
    return GetScreenMgr()->GetScreen( id );
}

/// Connect the SetUpdateFlag to every widget's user property change notification.
void VspScreenQtPrivate::ConnectUpdateFlag()
{
    const QMetaObject * const mo = widget()->metaObject();
    QMetaMethod flagMethod = mo->method( mo->indexOfSlot("SetUpdateFlag()") );
    foreach ( QWidget * w, widget()->findChildren<QWidget*>() ) {
        QAbstractButton * button = qobject_cast<QAbstractButton*>( w );
        if ( button && !button->isCheckable() ) {
            widget()->connect( button, SIGNAL( clicked() ), SLOT( SetUpdateFlag() ) );
            continue;
        }
        const QMetaObject * const mo = w->metaObject();
        QMetaProperty mp = mo->userProperty();
        if ( mp.isValid() && mp.hasNotifySignal() )
            QObject::connect( w, mp.notifySignal(), widget(), flagMethod );
    }
}

void VspScreenQtPrivate::SetUpdateFlag() {
    GetScreenMgr()->SetUpdateFlag( true );
}

void VspScreenQtPrivate::BlockSignalsInNextUpdate()
{
    blockSignalsInNextUpdate = true;
}

VspScreenQtPrivate::~VspScreenQtPrivate()
{
}
