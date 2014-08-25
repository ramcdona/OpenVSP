//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "VspScreen.h"
#include <FL/Fl_Double_Window.H>
#include <string>
#include <cassert>

VspScreen::VspScreen( ScreenMgr * mgr ) :
    m_ScreenMgr( mgr )
{
}

VspScreen::~VspScreen()
{
}

VspScreenFLTK::VspScreenFLTK( ScreenMgr* mgr ) :
    VspScreen( mgr ),
    m_FLTK_Window( NULL )
{
}

VspScreenFLTK::~VspScreenFLTK()
{
}

/// Show Window
void VspScreenFLTK::Show()
{
    assert( m_FLTK_Window );
    m_FLTK_Window->show();
}

/// Is Window Shown?
bool VspScreenFLTK::IsShown()
{
    assert( m_FLTK_Window );
    return !!( m_FLTK_Window->shown() );
}

/// Hide Window
void VspScreenFLTK::Hide()
{
    assert( m_FLTK_Window );
    m_FLTK_Window->hide();
}

std::string VspScreenFLTK::getFeedbackGroupName()
{
    return "";
}

void VspScreenFLTK::SetNonModal()
{
    m_FLTK_Window->set_non_modal();
}

int VspScreenFLTK::x()
{
    return m_FLTK_Window->x();
}

int VspScreenFLTK::y()
{
    return m_FLTK_Window->y();
}

int VspScreenFLTK::w()
{
    return m_FLTK_Window->w();
}

int VspScreenFLTK::h()
{
    return m_FLTK_Window->h();
}

void VspScreenFLTK::position( int X, int Y )
{
    m_FLTK_Window->position( X, Y );
}
