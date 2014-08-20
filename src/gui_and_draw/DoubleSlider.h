//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#ifndef DOUBLESLIDER_H
#define DOUBLESLIDER_H

#include <QWidget>

class DoubleSliderPrivate;
class DoubleSlider : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE( DoubleSlider )
    Q_PROPERTY( double maximum READ maximum WRITE setMaximum )
    Q_PROPERTY( double minimum READ minimum WRITE setMinimum )
    Q_PROPERTY( Qt::Orientation orientation READ orientation WRITE setOrientation )
    Q_PROPERTY( double value READ value WRITE setValue NOTIFY valueChanged USER true )
    Q_PRIVATE_SLOT( d_func(), void on_valueChanged( int ) )
    QScopedPointer<DoubleSliderPrivate> const d_ptr;
public:
    explicit DoubleSlider(QWidget *parent = 0);
    ~DoubleSlider();

    Qt::Orientation orientation() const;
    Q_SLOT void setOrientation( Qt::Orientation );
    double minimum() const;
    double maximum() const;
    void setMinimum( double );
    void setMaximum( double );
    Q_SLOT void setRange( double, double );
    double value() const;
    Q_SLOT void setValue( double );
    Q_SIGNAL void valueChanged( double );

protected:
    void resizeEvent( QResizeEvent * ) Q_DECL_OVERRIDE;
};

#endif // DOUBLESLIDER_H
