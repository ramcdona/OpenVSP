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
/// Ignores internal control implementation objects whose names begin
/// with qt_.
class UiSignalBlocker
{
    Q_DECLARE_PRIVATE( UiSignalBlocker )
    Q_DISABLE_COPY( UiSignalBlocker )
    QScopedPointer< UiSignalBlockerPrivate > const d_ptr;
public:
    explicit UiSignalBlocker( QWidget * );
    ~UiSignalBlocker();
};

#endif // UISIGNALBLOCKER_H
