//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#ifndef UISIGNALBLOCKER_H
#define UISIGNALBLOCKER_H

#include <QScopedPointer>

class QWidget;
class UiSignalBlockerPrivate;

/// Blocks all signals from a given Ui base widget and its children.
class UiSignalBlocker
{
    Q_DECLARE_PRIVATE( UiSignalBlocker )
    Q_DISABLE_COPY( UiSignalBlocker )
    QScopedPointer< UiSignalBlockerPrivate > const d_ptr;
public:
    /// Creates a blocker and invokes block() on the \a widget, if provided.
    explicit UiSignalBlocker( QWidget * widget = 0 );
    /// Blocks signals on the \a widget and all of its children.
    /// Ignores internal control implementation objects whose names begin
    /// with qt_. If \a widget is zero, behaves like unblock().
    /// If any signals were blocked, they will be unblocked before the \a widget
    /// is processed.
    void block( QWidget * widget );
    /// Unblocks all blocked signals.
    void unblock();
    /// Unblocks all blocked signals.
    ~UiSignalBlocker();
};

#endif // UISIGNALBLOCKER_H
