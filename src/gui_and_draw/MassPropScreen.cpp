//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//////////////////////////////////////////////////////////////////////

#include "MassPropScreen.h"
#include "ScreenMgr.h"
#include "EventMgr.h"
#include "Vehicle.h"
#include "APIDefines.h"
#include "VspScreenQt_p.h"
#include "ui_MassPropScreen.h"

class MassPropScreenPrivate : public QDialog, public VspScreenQtPrivate {
    Q_OBJECT
    Q_DECLARE_PUBLIC( MassPropScreen )
    Q_PRIVATE_SLOT( self(), void SetUpdateFlag() )
    Ui::MassPropScreen Ui;
    int NumMassSlices;
    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    void LoadSetChoice();
    MassPropScreenPrivate( MassPropScreen * );

    Q_SLOT void on_computeButton_clicked()
    {
        veh()->MassPropsAndFlatten( Ui.setChoice->currentIndex(), veh()->m_NumMassSlices );
    }
    Q_SLOT void on_numSlicesSlider_valueChanged( int val )
    {
        NumMassSlices = val;
    }
    Q_SLOT void on_numSlicesInput_valueChanged( int val )
    {
        NumMassSlices = val;
    }
    Q_SLOT void on_fileExportButton_clicked()
    {
        string newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Choose mass properties output file", "*.txt" );
        veh()->setExportFileName( vsp::MASS_PROP_TXT_TYPE, newfile );
    }
};

MassPropScreenPrivate::MassPropScreenPrivate( MassPropScreen * q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );
    Ui.numSlicesSlider->setRange( 10, 200 );

    BlockSignalsInNextUpdate();
    ConnectUpdateFlag();
}

MassPropScreen::MassPropScreen( ScreenMgr *mgr ) :
    VspScreenQt( *new MassPropScreenPrivate( this ), mgr )
{
}


bool MassPropScreenPrivate::Update()
{
    LoadSetChoice();
    Vehicle* const vehiclePtr = veh();
    const int decimals = 3; // %6.3f

    Ui.numSlicesSlider->setValue( vehiclePtr->m_NumMassSlices );
    Ui.numSlicesInput->setValue( vehiclePtr->m_NumMassSlices );

    Ui.totalMassOutput->setDecimals( decimals );
    Ui.totalMassOutput->setValue( vehiclePtr->m_TotalMass );

    vec3d cg = vehiclePtr->m_CG;
    Ui.xCgOutput->setDecimals( decimals );
    Ui.xCgOutput->setValue( cg.x() );
    Ui.yCgOutput->setDecimals( decimals );
    Ui.yCgOutput->setValue( cg.y() );
    Ui.zCgOutput->setDecimals( decimals );
    Ui.zCgOutput->setValue( cg.z() );


    vec3d moi = vehiclePtr->m_IxxIyyIzz;
    Ui.ixxOutput->setDecimals( decimals );
    Ui.ixxOutput->setValue( moi.x() );
    Ui.iyyOutput->setDecimals( decimals );
    Ui.iyyOutput->setValue( moi.y() );
    Ui.izzOutput->setDecimals( decimals );
    Ui.izzOutput->setValue( moi.z() );


    /// \todo FIXME: This is likely wrong!
    vec3d pmoi = vehiclePtr->m_IxyIxzIyz;
    Ui.ixyOutput->setDecimals( decimals );
    Ui.ixyOutput->setValue( pmoi.x() );
    Ui.ixzOutput->setDecimals( decimals );
    Ui.ixzOutput->setValue( pmoi.y() );
    Ui.iyzOutput->setDecimals( decimals );
    Ui.iyzOutput->setValue( pmoi.y() );

    Ui.fileExportOutput->setText( vehiclePtr->getExportFileName( vsp::MASS_PROP_TXT_TYPE ).c_str() );

    return true;
}

void MassPropScreenPrivate::LoadSetChoice()
{
    int index = Ui.setChoice->currentIndex();
    foreach ( string setName, veh()->GetSetNameVec() )
    {
        Ui.setChoice->addItem( setName.c_str() );
    }
    Ui.setChoice->setCurrentIndex( index != -1 ? index : 0 );
}

MassPropScreen::~MassPropScreen()
{
}

#include "MassPropScreen.moc"
