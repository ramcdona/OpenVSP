//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#ifndef VSPSCREEN_H
#define VSPSCREEN_H

#include <QtGlobal>
#define QPoint QQPoint
#include <FL/Fl_Device.H>
#undef QPoint

class ScreenMgr;
class GuiDevice;
class Fl_Widget;
class Fl_Double_Window;

/// The base class of screens managed by ScreenMgr
class VspScreen
{
public:
    VspScreen(ScreenMgr*);
    virtual ~VspScreen();

    virtual void Show() = 0;
    virtual bool IsShown() = 0;
    virtual void Hide() = 0;
    virtual bool Update() = 0;
    virtual void GuiDeviceCallBack( GuiDevice* device ) = 0;
    virtual void SetNonModal() = 0;

    ScreenMgr* GetScreenMgr() const {
        return m_ScreenMgr;
    }
protected:
    ScreenMgr* m_ScreenMgr;
};

/// A concrete screen implemented using FLTK
class  VspScreenFLTK : public VspScreen
{
public:
    VspScreenFLTK( ScreenMgr* mgr  );
    virtual ~VspScreenFLTK();

    virtual void CallBack( Fl_Widget *w )               {}
    void GuiDeviceCallBack( GuiDevice* device ) Q_DECL_OVERRIDE {}
    static void staticCB( Fl_Widget *w, void* data )
    {
        static_cast< VspScreenFLTK* >( data )->CallBack( w );
    }

    virtual void CloseCallBack( Fl_Widget *w )          {}
    static void staticCloseCB( Fl_Widget *w, void* data )
    {
        static_cast< VspScreenFLTK* >( data )->CloseCallBack( w );
    }

    virtual void SetFlWindow( Fl_Double_Window* win )
    {
        m_FLTK_Window = win;
    }
    virtual Fl_Double_Window* GetFlWindow()
    {
        return m_FLTK_Window;
    }
    virtual void Show();
    virtual bool IsShown();
    virtual void Hide();
    virtual bool Update()
    {
        return false;
    }
    virtual void SetNonModal() Q_DECL_OVERRIDE;

    /*!
    * Return Feedback Group Name.  Feedback Group Name identifies which GUI
    * is waitting on feedback.  By default, the names is "".  The name can
    * be any string.  You can set GUIs to the same name and getting the
    * same feedbacks.
    */
    virtual std::string getFeedbackGroupName();

protected:
    Fl_Double_Window* m_FLTK_Window;
};

#endif // VSPSCREEN_H
