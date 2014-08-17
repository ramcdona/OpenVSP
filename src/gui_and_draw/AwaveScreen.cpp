//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//////////////////////////////////////////////////////////////////////

#include "AwaveScreen.h"
#include "GuiDevice.h"
#include "Vec3d.h"
#include "ScreenMgr.h"
#include "EventMgr.h"
#include "Vehicle.h"
#include "Util.h"
#include "APIDefines.h"
#include "VspScreenQt_p.h"
#include "ui_AwaveScreen.h"
#include <cmath>

/// \todo Need to implement DoubleSlider and change the double sliders over to it.
class AwaveScreenPrivate : public QDialog, public VspScreenQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( AwaveScreen )
    Q_PRIVATE_SLOT( self(), void SetUpdateFlag() )
    Ui::AwaveScreen Ui;
    bool inUpdate;
    int SelectedSetIndex;
    double StartVal;
    double EndVal;
    double BoundsRange[2];
    int SliceRange[2];
    int numSlices;
    int lastAxis;
    int NumRotSecs;
    int NumRotSecsRange[2];
    double Angle;
    double MNumber;
    double AngleRange[2];
    double MNumberRange[2];
    bool ComputeAngle;
    vec3d Norm;

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    void LoadSetChoice();
    void CallBack( Fl_Widget *w );
    enum Delta { StartChanged, EndChanged };
    void check( Delta );
    AwaveScreenPrivate( AwaveScreen * q);

    Q_SLOT void on_numSlicesSlider_valueChanged( int val )
    {
        numSlices = val;
    }
    Q_SLOT void on_numSlicesInput_valueChanged( int val )
    {
        numSlices = val;
    }
    Q_SLOT void on_numRotSectsSlider_valueChanged( int val )
    {
        NumRotSecs = val;
    }
    Q_SLOT void on_numRotSectsInput_valueChanged( int val )
    {
        NumRotSecs = val;
    }
    Q_SLOT void on_startSlider_valueChanged( int val ) // double!
    {
        StartVal = val;
        check( StartChanged );
    }
    Q_SLOT void on_startInput_valueChanged( double val )
    {
        StartVal = val;
        check( StartChanged );
    }
    Q_SLOT void on_endSlider_valueChanged( int val ) // double!
    {
        EndVal = val;
        check( EndChanged );
    }
    Q_SLOT void on_endInput_valueChanged( double val )
    {
        EndVal = val;
        check( EndChanged );
    }
    Q_SLOT void on_angleButton_toggled( bool val )
    {
        Ui.numberButton->setChecked( !val );
        ComputeAngle = !val;
    }
    Q_SLOT void on_angleSlider_valueChanged( int val ) // double!
    {
        Angle = val;
    }
    Q_SLOT void on_angleInput_valueChanged( double val )
    {
        Angle = val;
    }
    Q_SLOT void on_numberButton_toggled( bool val )
    {
        Ui.angleButton->setChecked( !val );
        ComputeAngle = val;
    }
    Q_SLOT void on_numberSlider_valueChanged( int val ) // double!
    {
        MNumber = val;
    }
    Q_SLOT void on_numberInput_valueChanged( double val )
    {
        MNumber = val;
    }
    Q_SLOT void on_fileButton_clicked()
    {
        string newfile;
        newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Choose slice areas output file", "*.txt" );
        veh()->setExportFileName( vsp::SLICE_TXT_TYPE, newfile );
    }
    Q_SLOT void on_setChoice_currentIndexChanged( int index )
    {
        SelectedSetIndex = index;
    }
    Q_SLOT void on_startButton_clicked();
};
VSP_DEFINE_PRIVATE( AwaveScreen )

AwaveScreenPrivate::AwaveScreenPrivate( AwaveScreen * q) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );
#if 0
    // dead code
    vec3d maxBBox = veh()->GetBndBox().GetMax();
    vec3d minBBox = veh()->GetBndBox().GetMin();
#endif

    Ui.axisChoice->setCurrentIndex( 0 );
    Ui.autoBoundsButton->setChecked( true );
    Ui.angleButton->setChecked( true );

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
    Angle = 45;
    MNumber = sqrt( 2.0 );
    ComputeAngle = false;
    NumRotSecs = 5;
    AngleRange[0] = 1e-6;
    AngleRange[1] = 90;
    MNumberRange[0] = 1;
    MNumberRange[1] = 10;
    NumRotSecsRange[0] = 3;
    NumRotSecsRange[1] = 30;

    ConnectUpdateFlag();
}

AwaveScreen::AwaveScreen( ScreenMgr *mgr ) :
    VspScreenQt( *new AwaveScreenPrivate( this ), mgr )
{
}

AwaveScreen::~AwaveScreen()
{
}

bool AwaveScreenPrivate::Update()
{
    int const decimals = 3; // %6.3f
    Vehicle* veh = this->veh();
    LoadSetChoice();

    Ui.fileOutput->setText( veh->getExportFileName( vsp::SLICE_TXT_TYPE ).c_str() );

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
    numSlices = qMax( SliceRange[0], numSlices );
    if ( numSlices > SliceRange[1] )
    {
        /// \todo Shouldn't this be numSlices = SliceRange[1] ?
        SliceRange[1] = numSlices;
    }

    /// \todo KO setRange will modify the value if the value is out-of-bounds etc.
    Ui.numSlicesSlider->setRange( SliceRange[0], SliceRange[1] );
    Ui.numSlicesSlider->setValue( numSlices );
    Ui.numSlicesInput->setRange( SliceRange[0], SliceRange[1] );
    Ui.numSlicesInput->setValue( numSlices );

    Ui.startSlider->setDisabled( Ui.autoBoundsButton->isChecked() );
    Ui.startInput->setDisabled( Ui.autoBoundsButton->isChecked() );
    Ui.endSlider->setDisabled( Ui.autoBoundsButton->isChecked() );
    Ui.endInput->setDisabled( Ui.autoBoundsButton->isChecked() );

    // Number Rot Slices
    if ( NumRotSecs < NumRotSecsRange[0] )
    {
        NumRotSecs = NumRotSecsRange[0];
    }
    if ( NumRotSecs > NumRotSecsRange[1] )
    {
        /// \todo Shouldn't this be NumRotSecs = NumRotSecsRange[1] ?
        NumRotSecsRange[1] = NumRotSecs;
    }

    Ui.numRotSectsSlider->setRange( NumRotSecsRange[0], NumRotSecsRange[1] );
    Ui.numRotSectsSlider->setValue( NumRotSecs );
    Ui.numRotSectsInput->setRange( NumRotSecsRange[0], NumRotSecsRange[1] );
    Ui.numRotSectsInput->setValue( NumRotSecs );

    // Angle or Mach Number
    if ( ComputeAngle )
    {
        /// \todo Shouldn't there be a check for the upper end of the range?
        MNumber = qMax( MNumberRange[0], MNumber );
        Angle = asin( 1 / MNumber ) * RAD_2_DEG;
    }
    else
    {
        Angle = qBound( AngleRange[0], Angle, AngleRange[1] );
        MNumber = 1 / sin( Angle * DEG_2_RAD );
    }
    Ui.angleSlider->setDisabled( ComputeAngle );
    Ui.angleInput->setDisabled( ComputeAngle );
    Ui.numberSlider->setEnabled( ComputeAngle );
    Ui.numberInput->setEnabled( ComputeAngle );

    Ui.angleSlider->setRange( AngleRange[0], AngleRange[1] );
    Ui.angleSlider->setValue( Angle );
    Ui.angleInput->setDecimals( decimals );
    Ui.angleInput->setValue( Angle );

    MNumber = qMin( MNumber, MNumberRange[1] );

    Ui.numberSlider->setRange( MNumberRange[0], MNumberRange[1] );
    Ui.numberSlider->setValue( MNumber );
    Ui.numberInput->setDecimals( decimals );
    Ui.numberInput->setRange( MNumberRange[0], MNumberRange[1] );
    Ui.numberInput->setValue( MNumber );

    return true;
}

void AwaveScreenPrivate::LoadSetChoice()
{
    Ui.setChoice->clear();

    vector< string > set_name_vec = veh()->GetSetNameVec();

    for ( int i = 0 ; i < ( int )set_name_vec.size() ; i++ )
    {
        Ui.setChoice->addItem( set_name_vec[i].c_str() );
    }
    Ui.setChoice->setCurrentIndex( SelectedSetIndex );
}

void AwaveScreenPrivate::on_startButton_clicked()
{
    double AngleControlVal = ComputeAngle ? MNumber : Angle;

    string id = veh()->AwaveSliceAndFlatten(
                SelectedSetIndex, numSlices, NumRotSecs, AngleControlVal, ComputeAngle, Norm,
                Ui.autoBoundsButton->isChecked(), StartVal, EndVal );
    if ( id.compare( "NONE" ) != 0 )
    {
        QString const fn = veh()->getExportFileName( vsp::SLICE_TXT_TYPE ).c_str();
        Ui.outputTextDisplay->setText( ReadFile( fn ) );
        SetUpdateFlag();
    }
}

/// Check to make sure start is less than end
void AwaveScreenPrivate::check( AwaveScreenPrivate::Delta delta )
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

#include "AwaveScreen.moc"
