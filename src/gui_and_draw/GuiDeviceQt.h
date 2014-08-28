//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#ifndef GUIDEVICEQT_H
#define GUIDEVICEQT_H

#include "VspScreenQt.h"
#include "GuiDevice.h"
#include <QObject>

class QWidget;
class QAbstractButton;
class DoubleSlider;
class QDoubleSpinBox;


class GuiDeviceQtPrivate;
class GuiDeviceQt : public GuiDevice
{
    Q_DECLARE_PRIVATE( GuiDeviceQt )
protected:
    QScopedPointer<GuiDeviceQtPrivate> const d_ptr;
public:
    void Init( VspScreenQt* screen );
    void Activate() Q_DECL_OVERRIDE;
    void Deactivate() Q_DECL_OVERRIDE;
    void SetWidth( int ) Q_DECL_OVERRIDE;
    int GetWidth() const Q_DECL_OVERRIDE;
    void SetX( int ) Q_DECL_OVERRIDE;
    int GetX() const Q_DECL_OVERRIDE;
#if 0
    virtual void OffsetX( int x );
#endif
    ~GuiDeviceQt();

protected:
    GuiDeviceQt( GuiDeviceQtPrivate & );

    //==== First Widget Is Assumed Resizable For Set Width =====//
    virtual void AddWidget( QWidget* w, bool resizable_flag = false );
    virtual void ClearAllWidgets();
};

class ToggleButtonQtPrivate;
/// Toggle type button with light. Linked to a (BoolParm).
class ToggleButtonQt : public GuiDeviceQt
{
    VSP_DECLARE_PRIVATE( ToggleButtonQt )
public:
    ToggleButtonQt();
    void Init( VspScreenQt* screen, QAbstractButton* button );
    QAbstractButton * GetButton();
    ~ToggleButtonQt();
protected:
    void SetValAndLimits( Parm* p ) Q_DECL_OVERRIDE;
};

class SliderQtPrivate;
/// A linear slider. Linked to a (Parm/IntParm).
class SliderQt : public GuiDeviceQt
{
    VSP_DECLARE_PRIVATE( SliderQt )
public:
    SliderQt();
    void Init( VspScreenQt* screen, DoubleSlider* slider_widget, double range );
    void SetRange( double range );
    ~SliderQt();
protected:
    SliderQt( SliderQtPrivate & );
    void SetValAndLimits( Parm* p ) Q_DECL_OVERRIDE;
};

class LogSliderQtPrivate;
/// A log10 slider. Linked to a (Parm/IntParm).
class LogSliderQt : public SliderQt
{
    VSP_DECLARE_PRIVATE( LogSliderQt )
public:
    LogSliderQt();
    ~LogSliderQt();
protected:
    void SetValAndLimits( Parm* p ) Q_DECL_OVERRIDE;
};

class InputQtPrivate;
class InputQt : public GuiDeviceQt
{
    VSP_DECLARE_PRIVATE( InputQt )
public:
    InputQt();

#if 0
    void Init( VspScreenQt* screen, QDoubleSpinBox* input, const char* format, QAbstractButton* parm_button = NULL );
    void SetFormat( const char* format );
#endif
    void Init( VspScreenQt* screen, QDoubleSpinBox* input, int decimals, QAbstractButton* parm_button = NULL );
    void SetDecimals( int );
    void SetButtonNameUpdate( bool flag );
    ~InputQt();
protected:
    void SetValAndLimits( Parm* p ) Q_DECL_OVERRIDE;
};

class ParmButtonQtPrivate;
/// A button with a label. Linked to a (Parm).
class ParmButtonQt : public GuiDeviceQt
{
    VSP_DECLARE_PRIVATE( ParmButtonQt )
public:
    ParmButtonQt();
    void Init( VspScreenQt* screen, QAbstractButton* button );
    void Update( const string& parm_id ) Q_DECL_OVERRIDE;
    void SetButtonNameUpdate( bool flag );
    ~ParmButtonQt();
protected:
    void SetValAndLimits( Parm* p ) Q_DECL_OVERRIDE;
};

/// Combo of a Slider (or LogSlider) and Input and optional Parm Button. Linked to a (Parm).
class SliderInputQtPrivate;
class SliderInputQt : public GuiDeviceQt
{
    VSP_DECLARE_PRIVATE( SliderInputQt )
public:
    SliderInputQt();
    void Init( VspScreenQt* screen, DoubleSlider*, QDoubleSpinBox*,
               double range, int decimals, QAbstractButton* parm_button = 0,
               bool log_slider = false );
    void Update( const string& parm_id ) Q_DECL_OVERRIDE;
    void SetRange( double range );
    void SetDecimals( int decimals );
    void SetButtonNameUpdate( bool flag );
    ~SliderInputQt();
protected:
    virtual void SetValAndLimits( Parm* ) Q_DECL_OVERRIDE;
};

#endif // GUIDEVICEQT_H
