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
#include <QComboBox>
#include <QScopedValueRollback>
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
    typedef QScopedValueRollback<bool> BoolRollback;
    BoolRollback updateFlagRollback( GetScreenMgr()->GetUpdateFlag() );
    QScopedValueRollback<BoolRollback*> roll(d->updateFlagRollback);
    d->updateFlagRollback = &updateFlagRollback;
    QSet<QWidget*> blocked;
    if ( d->blockSignalsInNextUpdate || d->blockSignalsInUpdates ) {
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
    if ( d->blockSignalsInNextUpdate || d->blockSignalsInUpdates ) {
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

int VspScreenQt::x()
{
    return d_func()->widget()->x();
}

int VspScreenQt::y()
{
    return d_func()->widget()->y();
}

int VspScreenQt::w()
{
    return d_func()->widget()->width();
}

int VspScreenQt::h()
{
    return d_func()->widget()->height();
}

void VspScreenQt::position( int X, int Y )
{
    d_func()->widget()->move( X, Y );
}

VspScreenQt::~VspScreenQt()
{
}

VspScreenQtPrivate::VspScreenQtPrivate( VspScreenQt * q ) :
    blockSignalsInNextUpdate( false ),
    blockSignalsInUpdates( false ),
    inUpdate( false ),
    updateFlagRollback( 0 ),
    q_ptr( q )
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
        // Skip internal Qt subcontrols.
        if ( w->objectName().startsWith( "qt_" ) )
            continue;
        // Non-checkable buttons interest us only when they get clicked.
        QAbstractButton * button = qobject_cast<QAbstractButton*>( w );
        if ( button && !button->isCheckable() ) {
            widget()->connect( button, SIGNAL( clicked() ), SLOT( SetUpdateFlag() ) );
            continue;
        }
        // Update when the user property notifies of a change.
        const QMetaObject * const mo = w->metaObject();
        QMetaProperty mp = mo->userProperty();
        if ( mp.isValid() && mp.hasNotifySignal() )
            QObject::connect( w, mp.notifySignal(), widget(), flagMethod );
    }
    EnableUpdateFlags();
}

/// Sets the global update flag on the screen manager. Does nothing by default
/// until EnableUpdateFlags() is first called.
void VspScreenQtPrivate::SetUpdateFlag() {
    if ( enableUpdateFlags ) GetScreenMgr()->SetUpdateFlag( true );
}

/// Commits changes to UpdateFlag. Has no effect outside of Update.
void VspScreenQtPrivate::CommitUpdateFlag()
{
    if ( updateFlagRollback ) updateFlagRollback->commit();
}

void VspScreenQtPrivate::BlockSignalsInNextUpdate()
{
    blockSignalsInNextUpdate = true;
}

void VspScreenQtPrivate::BlockSignalsInUpdates()
{
    blockSignalsInUpdates = true;
}

void VspScreenQtPrivate::EnableUpdateFlags()
{
    enableUpdateFlags = true;
}

void VspScreenQtPrivate::LoadSetChoice( QComboBox * widget, int index)
{
    if ( index == KeepIndex ) {
        index = qMax(0, widget->currentIndex());
    }
    widget->clear();
    foreach( string setName, veh()->GetSetNameVec() )
    {
        widget->addItem( setName.c_str() );
    }
    widget->setCurrentIndex( index );
}

VspScreenQtPrivate::~VspScreenQtPrivate()
{
}
