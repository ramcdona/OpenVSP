//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "DegenGeomScreen.h"
#include "ScreenMgr.h"
#include "EventMgr.h"
#include "Vehicle.h"
#include "StlHelper.h"
#include "APIDefines.h"
#include "ui_DegenGeomScreen.h"
#include "VspScreenQt_p.h"

class DegenGeomScreenPrivate : public QDialog, public VspScreenQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( DegenGeomScreen )
    Q_PRIVATE_SLOT( self(), void SetUpdateFlag() )
    Ui::DegenGeomScreen Ui;

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    DegenGeomScreenPrivate( DegenGeomScreen * q );

    Q_SLOT void on_csvFileButton_toggled( bool val )
    {
        veh()->setExportDegenGeomCsvFile( val );
    }
    Q_SLOT void on_csvFileChooseButton_clicked()
    {
        veh()->setExportFileName( vsp::DEGEN_GEOM_CSV_TYPE,
                                  GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select degen geom CSV output file.", "*.csv" ) );
    }
    Q_SLOT void on_mFileButton_toggled( bool val )
    {
        veh()->setExportDegenGeomMFile( val );
    }
    Q_SLOT void on_mFileChooseButton_clicked()
    {
        veh()->setExportFileName( vsp::DEGEN_GEOM_M_TYPE,
                                  GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select degen geom Matlab output file.", "*.m" ) );
    }
    Q_SLOT void on_computeButton_clicked();
};

DegenGeomScreenPrivate::DegenGeomScreenPrivate( DegenGeomScreen * q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );
    BlockSignalsInUpdates();
    ConnectUpdateFlag();
    disconnect( Ui.setChoice, 0, 0, 0 );
}

DegenGeomScreen::DegenGeomScreen( ScreenMgr* mgr ) :
    VspScreenQt( *new DegenGeomScreenPrivate( this ), mgr )
{
}

bool DegenGeomScreenPrivate::Update()
{
    LoadSetChoice( Ui.setChoice, KeepIndex );
    Ui.csvFileButton->setChecked( veh()->getExportDegenGeomCsvFile() );
    Ui.mFileButton->setChecked( veh()->getExportDegenGeomMFile() );
    Ui.csvFileOutput->setText( veh()->getExportFileName( vsp::DEGEN_GEOM_CSV_TYPE ).c_str() );
    Ui.mFileOutput->setText( veh()->getExportFileName( vsp::DEGEN_GEOM_M_TYPE ).c_str() );
    return true;
}

DegenGeomScreen::~DegenGeomScreen() {}

/// \todo This contains pseudo-synchronous hacks.
void DegenGeomScreenPrivate::on_computeButton_clicked()
{
    Ui.outputTextDisplay->clear();
    Ui.outputTextDisplay->append( "Computing degenerate geometry..." );
    QCoreApplication::processEvents();
    veh()->CreateDegenGeom( Ui.setChoice->currentIndex() );
    Ui.outputTextDisplay->append("Done!\n");
    QCoreApplication::processEvents();
    if ( veh()->getExportDegenGeomCsvFile() || veh()->getExportDegenGeomMFile() )
    {
        Ui.outputTextDisplay->append( "--------------------------------" );
        Ui.outputTextDisplay->append( "Writing output..." );
        QCoreApplication::processEvents();
        Ui.outputTextDisplay->append( veh()->WriteDegenGeomFile().c_str() );
    }
}

#include "DegenGeomScreen.moc"
