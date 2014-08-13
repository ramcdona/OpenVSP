//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

//******************************************************************************
//
//   Select File Screen Class
//
//   J.R. Gloudemans - 1/27/11
//******************************************************************************

#ifndef SELECTFILESCREEN_H
#define SELECTFILESCREEN_H

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

    std::string FileChooser( const char* title, const char* filter );
    std::string FileChooser( const char* title, const char* filter, const char* dir );
};

#endif
