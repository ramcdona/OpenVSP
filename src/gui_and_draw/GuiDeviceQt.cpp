//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "GuiDeviceQt.h"
#include "DoubleSlider.h"
#include "ParmMgr.h"
#include "ScreenMgr.h"
#include <QObject>
#include <QDoubleSpinBox>
#include <QAbstractButton>

//
// GuiDeviceQt
//

class GuiDeviceQtPrivate {
    Q_DECLARE_PUBLIC( GuiDeviceQt )
public:
    GuiDeviceQt * const q_ptr;

    VspScreenQt* Screen;
    int ResizableWidgetIndex;
    vector< QWidget* > Widgets;

    GuiDeviceQtPrivate( GuiDeviceQt * q ) :
        q_ptr( q ), Screen( 0 ), ResizableWidgetIndex( 0 ) {}
};

GuiDeviceQt::GuiDeviceQt( GuiDeviceQtPrivate & d ) :
    d_ptr(&d)
{}

void GuiDeviceQt::Init( VspScreenQt * screen )
{
    d_func()->Screen = screen;
}

void GuiDeviceQt::AddWidget( QWidget* w, bool resizable_flag )
{
    Q_D(GuiDeviceQt);
    if ( w )
    {
        d->Widgets.push_back( w );
    }
    if ( resizable_flag )
    {
        d->ResizableWidgetIndex = (int)d->Widgets.size() - 1;
    }
}

void GuiDeviceQt::ClearAllWidgets()
{
    d_func()->Widgets.clear();
}

void GuiDeviceQt::Activate()
{
    for ( auto widget : d_func()->Widgets )
    {
        widget->setEnabled( true );
    }
}

void GuiDeviceQt::Deactivate()
{
    for ( auto widget : d_func()->Widgets )
    {
        widget->setEnabled( false );
    }
}

GuiDeviceQt::~GuiDeviceQt() {}

//
// ToggleButtonQt
//

class ToggleButtonQtPrivate : public QObject, public GuiDeviceQtPrivate {
    Q_OBJECT
    Q_DECLARE_PUBLIC( ToggleButtonQt )
    QAbstractButton* Button;

    ToggleButtonQtPrivate( ToggleButtonQt* q ) :
        GuiDeviceQtPrivate( q ), Button( 0 ) {}

    Q_SLOT void on_toggled( bool );
};

ToggleButtonQt::ToggleButtonQt() :
    GuiDeviceQt( * new ToggleButtonQtPrivate( this ) )
{
    m_Type = GDEV_TOGGLE_BUTTON;
}

void ToggleButtonQt::Init( VspScreenQt * screen, QAbstractButton * button )
{
    Q_ASSERT( button );
    Q_D( ToggleButtonQt );
    GuiDeviceQt::Init( screen );
    AddWidget( button );
    d->Button = button;
    d->Button->setCheckable( true );
    d->connect( d->Button, SIGNAL( toggled(bool) ), SLOT( on_toggled(bool) ) );
}

/// Set Button Value
void ToggleButtonQt::SetValAndLimits( Parm* p )
{
    Q_D( ToggleButtonQt );
    Q_ASSERT( d->Button );
    if ( !p ) return;

    BoolParm* bool_p = dynamic_cast<BoolParm*>( p );
    Q_ASSERT( bool_p );

    d->Button->setChecked( bool_p->Get() );
}

void ToggleButtonQtPrivate::on_toggled( bool val )
{
    Q_Q( ToggleButtonQt );
    Parm* parm_ptr = q->SetParmID( q->m_ParmID );
    if ( !parm_ptr ) return;
    parm_ptr->SetFromDevice( val );
    Screen->GuiDeviceCallBack( q );
}

//
// SliderQt
//

class SliderQtPrivate : public QObject, public GuiDeviceQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( SliderQt )
protected:
    DoubleSlider* Slider;
    double Range;
    double MinBound;
    double MaxBound;

    SliderQtPrivate( SliderQt * q );
    Q_SLOT virtual void on_valueChanged( double );
};

SliderQtPrivate::SliderQtPrivate( SliderQt * q ) :
    GuiDeviceQtPrivate( q ),
    Slider(0),
    Range(10.0),
    MinBound(0.0),
    MaxBound(0.0)
{}

SliderQt::SliderQt() :
    GuiDeviceQt( * new SliderQtPrivate( this ) )
{
    m_Type = GDEV_SLIDER;
}

void SliderQt::Init( VspScreenQt* screen, DoubleSlider* slider_widget, double range )
{
    Q_D( SliderQt );
    GuiDeviceQt::Init( screen );
    AddWidget( slider_widget );
    SetRange( range );
    d->Slider = slider_widget;
    assert( d->Slider );
    d->connect( d->Slider, SIGNAL( valueChanged(double) ), SLOT( on_valueChanged(double) ) );
}

void SliderQt::SetValAndLimits( Parm* parm_ptr )
{
    Q_D( SliderQt );
    assert( d->Slider );
    double new_val = parm_ptr->Get();

    if ( m_NewParmFlag || new_val < d->MinBound || new_val > d->MaxBound )
    {
        d->MinBound = max( new_val - d->Range, parm_ptr->GetLowerLimit() );
        d->MaxBound = min( new_val + d->Range, parm_ptr->GetUpperLimit() );
        d->Slider->setRange( d->MinBound, d->MaxBound );
    }

    if ( CheckValUpdate( new_val ) )
    {
        d->Slider->setValue( new_val );
    }

    m_LastVal = new_val;
}

void SliderQtPrivate::on_valueChanged( double new_val )
{
    Q_Q( SliderQt );
    Parm* parm_ptr = q->SetParmID( q->m_ParmID );
    if ( !parm_ptr ) return;

    parm_ptr->SetFromDevice( new_val );

    Screen->GuiDeviceCallBack( q );
}

//
// LogSliderQt
//

class LogSliderQtPrivate : public SliderQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( LogSliderQt )

    LogSliderQtPrivate( LogSliderQt * q ) : SliderQtPrivate( q ) {}
    void on_valueChanged( double ) Q_DECL_OVERRIDE;
};

LogSliderQt::LogSliderQt() : SliderQt( *new LogSliderQtPrivate( this ))
{
    m_Type = GDEV_LOG_SLIDER;
}

void LogSliderQt::SetValAndLimits( Parm* parm_ptr )
{
    Q_D( LogSliderQt );
    double m_Tol = 0.000001;
    assert( d->Slider );
    double new_val = parm_ptr->Get();

    if ( m_NewParmFlag || new_val < ( d->MinBound - m_Tol ) || new_val > ( d->MaxBound + m_Tol ) )
    {
        d->MinBound = max( new_val - d->Range, parm_ptr->GetLowerLimit() );
        d->MaxBound = min( new_val + d->Range, parm_ptr->GetUpperLimit() );
        d->Slider->setRange( log10( d->MinBound ), log10( d->MaxBound ) );
    }

    if ( CheckValUpdate( new_val ) )
    {
        d->Slider->setValue( log10( new_val ) );
    }

    m_LastVal = new_val;
}


void LogSliderQtPrivate::on_valueChanged( double val )
{
    Q_Q( LogSliderQt );
    Parm* parm_ptr = q->SetParmID( q->m_ParmID );
    if ( !parm_ptr ) return;

    double new_val = pow( 10, val );
    parm_ptr->SetFromDevice( new_val );

    Screen->GuiDeviceCallBack( q );
}

//
// InputQt
//

class InputQtPrivate : public QObject, public GuiDeviceQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( InputQt )
    QDoubleSpinBox* Input;
    bool ParmButtonFlag;
    ParmButtonQt ParmButton;

    InputQtPrivate( InputQt * q );
    Q_SLOT void on_valueChanged( double );
};

InputQtPrivate::InputQtPrivate( InputQt * q ) :
    GuiDeviceQtPrivate( q ),
    Input( 0 ),
    ParmButtonFlag( false )
{}

InputQt::InputQt() :
    GuiDeviceQt( *new InputQtPrivate( this ) )
{
    m_Type = GDEV_INPUT;
}

void InputQt::Init( VspScreenQt* screen, QDoubleSpinBox* input, int decimals, QAbstractButton* parm_button )
{
    Q_D( InputQt );
    assert( input );
    GuiDeviceQt::Init( screen );
    AddWidget( parm_button );
    AddWidget( input, true );

    d->Input = input;
    d->Input->setKeyboardTracking( false ); // FL_WHEN_ENTER_KEY | FL_WHEN_RELEASE
    SetDecimals( decimals );
    d->connect( d->Input, SIGNAL( valueChanged(double) ), SLOT( on_valueChanged(double) ) );

    d->ParmButtonFlag = false;
    if ( parm_button )
    {
        d->ParmButtonFlag = true;
        d->ParmButton.Init( screen, parm_button );
    }
}

void InputQt::SetDecimals( int decimals )
{
    Q_D( InputQt );
    if ( d->Input ) d->Input->setDecimals( decimals );
}

void InputQt::SetButtonNameUpdate( bool flag )
{
    d_func()->ParmButton.SetButtonNameUpdate( flag );
}

void InputQt::SetValAndLimits( Parm* parm_ptr )
{
    Q_D( InputQt );
    assert( d->Input );
    double new_val = parm_ptr->Get();

    if ( CheckValUpdate( new_val ) )
    {
        d->Input->setValue( new_val );
    }
    m_LastVal = new_val;

    if ( d->ParmButtonFlag )
    {
        d->ParmButton.Update( parm_ptr->GetID() );
    }
}

void InputQtPrivate::on_valueChanged( double new_val )
{
    Q_Q( InputQt );
    Parm* parm_ptr = q->SetParmID( q->m_ParmID );
    if ( !parm_ptr ) return;

    parm_ptr->SetFromDevice( new_val );
    q->m_LastVal = new_val;

    Screen->GuiDeviceCallBack( q );
}

//
// ParmButtonQt
//

class ParmButtonQtPrivate : public QObject, public GuiDeviceQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( ParmButtonQt )
    QAbstractButton* Button;
    bool ButtonNameUpdate;

    ParmButtonQtPrivate( ParmButtonQt * q );
    Q_SLOT void on_clicked();
};

ParmButtonQtPrivate::ParmButtonQtPrivate( ParmButtonQt * q ) :
    GuiDeviceQtPrivate( q ),
    Button( 0 ),
    ButtonNameUpdate( false )
{}

ParmButtonQt::ParmButtonQt() :
    GuiDeviceQt( *new ParmButtonQtPrivate( this ) )
{
    m_Type = GDEV_PARM_BUTTON;
}

void ParmButtonQt::Init( VspScreenQt* screen, QAbstractButton* button )
{
    Q_D( ParmButtonQt );
    GuiDeviceQt::Init( screen );
    AddWidget( button );
    d->Button = button;
    assert( d->Button );
    d->connect( d->Button, SIGNAL( clicked() ), SLOT( on_clicked() ) );
}

void ParmButtonQt::Update( const string& parm_id )
{
    Q_D( ParmButtonQt );
    GuiDeviceQt::Update( parm_id );

    if( d->ButtonNameUpdate )
    {
        Parm* parm_ptr = SetParmID( parm_id );
        if ( parm_ptr )
        {
            d->Button->setText( parm_ptr->GetName().c_str() );
        }
    }
}

void ParmButtonQt::SetButtonNameUpdate( bool flag )
{
    d_func()->ButtonNameUpdate = flag;
}

void ParmButtonQt::SetValAndLimits( Parm* )
{}

void ParmButtonQtPrivate::on_clicked()
{
    Q_Q( ParmButtonQt );
    ParmMgr.SetActiveParm( q->m_ParmID );

    Screen->GetScreenMgr()->ShowScreen( ScreenMgr::VSP_PARM_SCREEN );
#if 0
//  Screen->GetScreenMgr()->ShowParmScreen(parm_ptr, Fl::event_x_root(), Fl::event_y_root());
#endif
    Screen->GuiDeviceCallBack( q );
}

//
// SliderInputQt
//

class SliderInputQtPrivate : public GuiDeviceQtPrivate
{
    Q_DECLARE_PUBLIC( SliderInputQt )
    SliderQt Slider;
    InputQt Input;

    bool LogSliderFlag;
    LogSliderQt LogSlider;

    bool ParmButtonFlag;
    ParmButtonQt ParmButton;

    SliderInputQtPrivate( SliderInputQt* );
};

SliderInputQtPrivate::SliderInputQtPrivate( SliderInputQt * q ) :
    GuiDeviceQtPrivate( q ),
    LogSliderFlag( false ),
    ParmButtonFlag( false )
{
}

void SliderInputQt::Init( VspScreenQt* screen, DoubleSlider* slider, QDoubleSpinBox* input,
                          double range, int decimals, QAbstractButton* parm_button,
                          bool log_slider )
{
    Q_D( SliderInputQt );
    if ( parm_button )
    {
        d->ParmButtonFlag = true;
        d->ParmButton.Init( screen, parm_button );
    }

    d->LogSliderFlag = log_slider;
    if ( d->LogSliderFlag )
    {
        d->LogSlider.Init( screen, slider, range );
    }
    else
    {
        d->Slider.Init( screen, slider, range );
    }

    d->Input.Init( screen, input, decimals );

    ClearAllWidgets();
    AddWidget( parm_button );
    AddWidget( slider, true );
    AddWidget( input );
}

void SliderInputQt::Update( const string& parm_id )
{
    Q_D( SliderInputQt );
    if ( d->LogSliderFlag )
    {
        d->LogSlider.Update( parm_id );
    }
    else
    {
        d->Slider.Update( parm_id );
    }
    d->Input.Update( parm_id );
    if ( d->ParmButtonFlag )
    {
        d->ParmButton.Update( parm_id );
    }
}

void SliderInputQt::SetRange( double range )
{
    d_func()->Slider.SetRange( range );
}

void SliderInputQt::SetDecimals( int decimals )
{
    d_func()->Input.SetDecimals( decimals );
}

void SliderInputQt::SetButtonNameUpdate( bool flag )
{
    d_func()->ParmButton.SetButtonNameUpdate( flag );
}

void SliderInputQt::SetValAndLimits( Parm* ) {}

SliderInputQt::~SliderInputQt() {}

#include "GuiDeviceQt.moc"
