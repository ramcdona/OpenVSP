//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

#include "Util.h"
#include <QFile>
#include <cmath>
#include <ctime>

/// Generate a unique random string of \a length.
string GenerateRandomID( int length )
{
    static bool seed = false;
    if ( !seed )
    {
        seed = true;
        srand( ( unsigned int )time( NULL ) );
    }

    static char str[256];
    for ( int i = 0 ; i < length ; i++ )
    {
        str[i] = ( char )( ( rand() % 26 ) + 65 );
    }
    return string( str, length );
}

/// Read and return the contents of an entire file.
QString ReadFile( const QString &fileName )
{
    QFile f( fileName );
    if ( !f.open( QIODevice::ReadOnly ) ) return QString::Null();
    return QString::fromLocal8Bit(f.readAll());
}
