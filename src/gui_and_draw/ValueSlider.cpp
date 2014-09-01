//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "ValueSlider.h"
#include "DoubleSlider.h"
#include <QBoxLayout>
#include <QDoubleSpinBox>
#include <QResizeEvent>
#include <QGraphicsColorizeEffect>

class ValueSliderPrivate {
    Q_DECLARE_PUBLIC( ValueSlider )
    ValueSlider * const q_ptr;
    QBoxLayout layout;
    QDoubleSpinBox spinbox;
    DoubleSlider slider;
    QColor colorization;

    ValueSliderPrivate( ValueSlider * q );
    void on_qt_slider_valueChanged( double );
    void on_qt_spinbox_valueChanged( double );
    QBoxLayout::Direction LayoutDir() {
        return slider.orientation() == Qt::Horizontal ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom;
    }
    class SignalsBlocker {
        Q_DISABLE_COPY( SignalsBlocker )
        ValueSliderPrivate * const q;
        bool blocked;
    public:
        SignalsBlocker( ValueSliderPrivate * q_ ) : q( q_ ) {
            blocked = q->spinbox.blockSignals( true );
            q->slider.blockSignals( true );
        }
        ~SignalsBlocker() {
            q->spinbox.blockSignals( blocked );
            q->slider.blockSignals( blocked );
        }
    };
};

ValueSliderPrivate::ValueSliderPrivate( ValueSlider * q ) :
    q_ptr( q ),
    layout( QBoxLayout::LeftToRight, q )
{
    layout.addWidget( &spinbox );
    layout.addWidget( &slider );
    layout.setDirection( LayoutDir() );
    layout.setMargin( 0 );
    slider.setObjectName("qt_slider");
    spinbox.setObjectName("qt_spinbox");
}

void ValueSliderPrivate::on_qt_slider_valueChanged( double val )
{
    Q_Q( ValueSlider );
    SignalsBlocker block( this );
    spinbox.setValue( val );
    emit q->valueChanged( val );
}

void ValueSliderPrivate::on_qt_spinbox_valueChanged( double val )
{
    Q_Q( ValueSlider );
    SignalsBlocker block( this );
    slider.setValue( val );
    emit q->valueChanged( val );
}

ValueSlider::ValueSlider( QWidget *parent ) :
    QWidget( parent ),
    d_ptr( new ValueSliderPrivate( this ))
{
    setRange( 0.0, 1.0 );
    QMetaObject::connectSlotsByName( this );
}

Qt::Orientation ValueSlider::orientation() const
{
    return d_func()->slider.orientation();
}

void ValueSlider::setOrientation( Qt::Orientation o )
{
    Q_D( ValueSlider );
    d->slider.setOrientation( o );
    d->layout.setDirection( d->LayoutDir() );
}

double ValueSlider::minimum() const
{
    return d_func()->slider.minimum();
}

double ValueSlider::maximum() const
{
    return d_func()->slider.maximum();
}

void ValueSlider::setMinimum( double min )
{
    setRange( min, maximum() );
}

void ValueSlider::setMaximum( double max )
{
    setRange( minimum(), max );
}

void ValueSlider::setRange( double min, double max )
{
    Q_D( ValueSlider );
    ValueSliderPrivate::SignalsBlocker block( d );
    d->slider.setRange( min, max );
    d->spinbox.setRange( min, max );
}

int ValueSlider::decimals() const
{
    return d_func()->spinbox.decimals();
}

void ValueSlider::setDecimals( int decimals )
{
    d_func()->spinbox.setDecimals( decimals );
}

QAbstractSpinBox::ButtonSymbols ValueSlider::buttonSymbols() const
{
    return d_func()->spinbox.buttonSymbols();
}

void ValueSlider::setButtonSymbols( QAbstractSpinBox::ButtonSymbols bsyms )
{
    d_func()->spinbox.setButtonSymbols( bsyms );
}

QColor ValueSlider::colorization() const
{
    return d_func()->colorization;
}

void ValueSlider::setColorization( const QColor & color )
{
    Q_D( ValueSlider );
    d->colorization = color;
    if ( color.isValid() ) {
        auto fx = new QGraphicsColorizeEffect();
        fx->setColor( color );
        d->slider.setGraphicsEffect( fx );
    } else {
        d->slider.setGraphicsEffect( 0 );
    }
}

double ValueSlider::value() const
{
    Q_D( const ValueSlider );
    return d->slider.value();
}

void ValueSlider::setValue( double val )
{
    Q_D( ValueSlider );
    ValueSliderPrivate::SignalsBlocker block( d );
    d->slider.setValue( val );
    d->spinbox.setValue( val );
}

ValueSlider::~ValueSlider() {}

#include "moc_ValueSlider.cpp"
