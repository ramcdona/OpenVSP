//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
/// \class SelectFileScreen
/// Provides a file selector dialog for existing and new files.
//
//////////////////////////////////////////////////////////////////////

#ifndef SELECTFILESCREEN__INCLUDED_
#define SELECTFILESCREEN__INCLUDED_

#include <QScopedPointer>
#include <string>

class SelectFileScreenPrivate;
class SelectFileScreen
{
    Q_DISABLE_COPY(SelectFileScreen)
    Q_DECLARE_PRIVATE(SelectFileScreen)
    QScopedPointer<SelectFileScreenPrivate> const d_ptr;
public:
    SelectFileScreen();
    virtual ~SelectFileScreen();

    std::string FileOpen( const char* title, const char* filter, const char* dir = 0);
    std::string FileSave( const char* title, const char* filter, const char* dir = 0);
};

#endif // SELECTFILESCREEN__INCLUDED_
