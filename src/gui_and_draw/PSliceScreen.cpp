//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//////////////////////////////////////////////////////////////////////

#include "PSliceScreen.h"
#include "ScreenMgr.h"
#include "EventMgr.h"
#include "Vehicle.h"
#include "APIDefines.h"
#include "GuiDevice.h"
#include "Vec3d.h"
#include "Util.h"
#include "VspScreenQt_p.h"
#include "ui_PSliceScreen.h"
#include <cassert>

class PSliceScreenPrivate : public QDialog, public VspScreenQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( PSliceScreen )
    Q_PRIVATE_SLOT( self(), void SetUpdateFlag() )
    Ui::PSliceScreen Ui;
    int SelectedSetIndex;
    double StartVal;
    double EndVal;
    double BoundsRange[2];
    int SliceRange[2];
    int numSlices;
    int lastAxis;
    vec3d Norm;

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    void LoadSetChoice();
    enum Delta { StartChanged, EndChanged };
    void check( Delta );
    PSliceScreenPrivate( PSliceScreen * );

    Q_SLOT void on_numSlicesSlider_valueChanged( int val )
    {
        numSlices = val;
    }
    Q_SLOT void on_numSlicesInput_valueChanged( int val )
    {
        numSlices = val;
    }
    Q_SLOT void on_startSlider_valueChanged( int val ) // double
    {
        on_startInput_valueChanged( val );
    }
    Q_SLOT void on_startInput_valueChanged( double val )
    {
        StartVal = val;
        check( StartChanged );
    }
    Q_SLOT void on_endSlider_valueChanged( int val ) // double
    {
        on_endInput_valueChanged( val );
    }
    Q_SLOT void on_endInput_valueChanged( double val )
    {
        EndVal = val;
        check( EndChanged );
    }
    Q_SLOT void on_txtFileChooseButton_clicked()
    {
        string newfile;
        newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Choose slice areas output file", "*.txt" );
        veh()->setExportFileName( vsp::SLICE_TXT_TYPE, newfile );
    }
    Q_SLOT void on_setChoice_currentIndexChanged( int index )
    {
        SelectedSetIndex = index;
    }
    Q_SLOT void on_startButton_clicked()
    {
        string id = veh()->PSliceAndFlatten( SelectedSetIndex, numSlices, Norm,
                                             Ui.autoBoundsButton->isChecked(), StartVal, EndVal );
        if ( id.compare( "NONE" ) != 0 )
        {
            Ui.outputTextDisplay->setText( ReadFile( veh()->getExportFileName( vsp::SLICE_TXT_TYPE ).c_str() ) );
        }
    }
};

PSliceScreenPrivate::PSliceScreenPrivate( PSliceScreen * q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );

    /// \todo Use the bounding box to determine slicing limits
#if 0
    vec3d maxBBox = veh()->GetBndBox().GetMax();
    vec3d minBBox = veh()->GetBndBox().GetMin();
#endif

    Ui.axisChoice->setCurrentIndex( 0 );
    Ui.autoBoundsButton->setChecked( true );

    SelectedSetIndex = 0;
    lastAxis = 0;
    BoundsRange[0] = 0;
    BoundsRange[1] = 10;
    StartVal = 0;
    EndVal = 10;
    Norm = vec3d( 1, 0, 0 );
    numSlices = 10;
    SliceRange[0] = 3;
    SliceRange[1] = 100;

    BlockSignalsInNextUpdate();
    ConnectUpdateFlag();
}

PSliceScreen::PSliceScreen( ScreenMgr *mgr ) :
    VspScreenQt( *new PSliceScreenPrivate( this ), mgr )
{
}

bool PSliceScreenPrivate::Update()
{
    int const decimals = 3; /* %6.3f */
    Vehicle* const veh = this->veh();

    LoadSetChoice();

    Ui.txtFileOutput->setText( veh->getExportFileName( vsp::SLICE_TXT_TYPE ).c_str() );

    vec3d maxBBox = veh->GetBndBox().GetMax();
    vec3d minBBox = veh->GetBndBox().GetMin();
    double max;
    double min;
    int axisIndex = Ui.axisChoice->currentIndex();

    min = minBBox[axisIndex];
    max = maxBBox[axisIndex];
    Norm.set_xyz( 0, 0, 0 );
    Norm[axisIndex] = 1;

    // Set Range again if axis has changed
    if ( lastAxis != Ui.axisChoice->currentIndex() )
    {
        BoundsRange[0] = min;
        BoundsRange[1] = max;
        lastAxis = Ui.axisChoice->currentIndex();
    }
    if ( StartVal < BoundsRange[0] )
    {
        BoundsRange[0] = StartVal;
    }
    if ( EndVal > BoundsRange[1] )
    {
        BoundsRange[1] = EndVal;
    }

    Ui.startSlider->setRange( BoundsRange[0], BoundsRange[1] );
    Ui.startSlider->setValue( StartVal );
    Ui.startInput->setDecimals( decimals );
    Ui.startInput->setRange( BoundsRange[0], BoundsRange[1] );
    Ui.startInput->setValue( StartVal );

    Ui.endSlider->setRange( BoundsRange[0], BoundsRange[1] );
    Ui.endSlider->setValue( EndVal );
    Ui.endInput->setDecimals( decimals );
    Ui.endInput->setRange( BoundsRange[0], BoundsRange[1] );
    Ui.endInput->setValue( EndVal );

    // Num Slices
    numSlices = qBound( SliceRange[0], numSlices, SliceRange[1] );
    Ui.numSlicesSlider->setRange( SliceRange[0], SliceRange[1] );
    Ui.numSlicesSlider->setValue( numSlices );
    Ui.numSlicesInput->setRange( SliceRange[0], SliceRange[1] );
    Ui.numSlicesInput->setValue( numSlices );

    // Deactivate Bound Control if AutoBounds is on
    Ui.startSlider->setDisabled( Ui.autoBoundsButton->isChecked() );
    Ui.startInput->setDisabled( Ui.autoBoundsButton->isChecked() );
    Ui.endSlider->setDisabled( Ui.autoBoundsButton->isChecked() );
    Ui.endInput->setDisabled( Ui.autoBoundsButton->isChecked() );

    return true;
}

void PSliceScreenPrivate::LoadSetChoice()
{
    Ui.setChoice->clear();
    foreach( string setName, veh()->GetSetNameVec() )
    {
        Ui.setChoice->addItem( setName.c_str() );
    }
    Ui.setChoice->setCurrentIndex( SelectedSetIndex );
}

/// Check to make sure start is less than end
void PSliceScreenPrivate::check( PSliceScreenPrivate::Delta delta )
{
    if ( StartVal > EndVal )
    {
        if ( delta == StartChanged )
        {
            EndVal = StartVal;
        }
        else if ( delta == EndChanged )
        {
            StartVal = EndVal;
        }
    }
}

PSliceScreen::~PSliceScreen()
{
}

#include "PSliceScreen.moc"
