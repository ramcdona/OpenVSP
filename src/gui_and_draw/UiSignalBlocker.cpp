//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "UiSignalBlocker.h"
#include <QWidget>
#include <QMap>

typedef QMap<QWidget*, bool> BlockMap;

class UiSignalBlockerPrivate {
public:
    BlockMap map;
};

UiSignalBlocker::UiSignalBlocker( QWidget * b ) :
    d_ptr( new UiSignalBlockerPrivate )
{
    Q_D( UiSignalBlocker );
    d->map[b] = b->blockSignals( true );
    foreach ( QWidget * w, b->findChildren< QWidget* >() )
    {
        if ( w->objectName().startsWith( "qt_" ) ) continue;
        d->map[w] = w->blockSignals( true );
    }
}

UiSignalBlocker::~UiSignalBlocker()
{
    Q_D( UiSignalBlocker );
    for ( BlockMap::const_iterator it = d->map.begin(); it != d->map.end(); ++it )
    {
        it.key()->blockSignals( it.value() );
    }
}
