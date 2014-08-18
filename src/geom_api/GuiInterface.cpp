//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// VehicleMgr.cpp: implementation of the Vehicle Class and Vehicle Mgr Singleton.
//
//////////////////////////////////////////////////////////////////////

#include "GuiInterface.h"
#include <cassert>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef VSP_USE_FLTK
#define QPoint QQPoint
#include "ScreenMgr.h"
#undef QPoint
#endif
#include <QCoreApplication>

#include <stddef.h>

class GuiHelper : public QObject {
    Q_OBJECT
    Q_SLOT void runFLTK() {
#ifdef VSP_USE_FLTK
        Fl::lock();
        Fl::run();
#endif
    }
public:
    GuiHelper() {}
};

GuiInterface * GuiInterface::m_instance = NULL;

//==== Constructor ====//
GuiInterface::GuiInterface()
{
    assert(! m_instance);
    m_ScreenMgr = NULL;
    m_Vehicle = NULL;
    m_instance = this;
}

//==== Destructor ====//
GuiInterface::~GuiInterface()
{
#ifdef VSP_USE_FLTK
    if( m_ScreenMgr )
    {
        delete m_ScreenMgr;
    }
#endif
    m_instance = NULL;
}

void GuiInterface::InitGui( Vehicle* vPtr )
{
    m_Vehicle = vPtr;

#ifdef VSP_USE_FLTK
    if( !m_ScreenMgr )
    {
        m_ScreenMgr = new ScreenMgr( vPtr );
    }
#endif

}

void GuiInterface::StartGui()
{
#ifdef VSP_USE_FLTK
    GuiHelper helper;
    QMetaObject::invokeMethod( &helper, "runFLTK", Qt::QueuedConnection );
    qApp->exec();
#endif
}

void GuiInterface::StartGuiAPI( )
{
#ifdef VSP_USE_FLTK
    if ( m_ScreenMgr )
    {
        m_ScreenMgr->SetRunGui( true );
        m_ScreenMgr->ShowReturnToAPI();
        m_ScreenMgr->ShowScreen( ScreenMgr::VSP_MAIN_SCREEN );
        while( m_ScreenMgr->CheckRunGui() && Fl::wait() );
    }
#endif

}

void GuiInterface::UpdateGui( )
{
#ifdef VSP_USE_FLTK
    if ( m_ScreenMgr )
    {
        m_ScreenMgr->SetUpdateFlag( true );
//      m_ScreenMgr->UpdateAllScreens();
    }
#endif

}

void GuiInterface::PopupMsg( const char* message, bool lock_out )
{
#ifdef VSP_USE_FLTK
    if ( m_ScreenMgr )
    {
        m_ScreenMgr->Alert( message );
    }
#endif

}

#include "GuiInterface.moc"
