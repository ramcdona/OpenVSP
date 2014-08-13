//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

#include "Exit.h"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <QApplication>

// Exits the GUI event loops
void vsp_exit()
{
    while (Fl::first_window()) Fl::first_window()->hide();
    qApp->quit();
}
