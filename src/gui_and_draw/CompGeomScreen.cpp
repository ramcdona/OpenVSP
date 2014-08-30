//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "CompGeomScreen.h"
#include "ScreenMgr.h"
#include "EventMgr.h"
#include "Vehicle.h"
#include "StlHelper.h"
#include "APIDefines.h"
#include "Util.h"
#include "VspScreenQt_p.h"
#include "ui_CompGeomScreen.h"

class CompGeomScreenPrivate : public QDialog, public VspScreenQtPrivate {
    Q_OBJECT
    Q_DECLARE_PUBLIC( CompGeomScreen )
    Q_PRIVATE_SLOT( self(), void SetUpdateFlag() )
    Ui::CompGeomScreen Ui;

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    CompGeomScreenPrivate( CompGeomScreen * );

    Q_SLOT void on_csvFileButton_toggled( bool val )
    {
        veh()->setExportCompGeomCsvFile( val );
    }
    Q_SLOT void on_tsvFileButton_toggled( bool val )
    {
        veh()->setExportDragBuildTsvFile( val );
    }
    Q_SLOT void on_csvFileChooseButton_clicked()
    {
        veh()->setExportFileName( vsp::COMP_GEOM_CSV_TYPE,
                                  GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select comp_geom output file.", "*.csv" ) );
    }
    Q_SLOT void on_tsvFileChooseButton_clicked()
    {
        veh()->setExportFileName( vsp::DRAG_BUILD_TSV_TYPE,
                                  GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select comp_geom output file.", "*.tsv" ) );
    }
    Q_SLOT void on_txtFileChooseButton_clicked()
    {
        veh()->setExportFileName( vsp::COMP_GEOM_TXT_TYPE,
                                  GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select comp_geom output file.", "*.txt" ) );
    }
    Q_SLOT void on_executeButton_clicked()
    {
        string geom = veh()->CompGeomAndFlatten(
                    Ui.setChoice->currentIndex(), 0, 0, Ui.halfMeshButton->isChecked(),
                    Ui.subSurfButton->isChecked()
                    );
        if ( geom.compare( "NONE" ) != 0 )
        {
            Ui.outputTextDisplay->setText( ReadFile( veh()->getExportFileName( vsp::COMP_GEOM_TXT_TYPE ).c_str() ) );
        }
    }
};

CompGeomScreenPrivate::CompGeomScreenPrivate( CompGeomScreen * q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );
    Ui.halfMeshButton->setChecked( true );
    Ui.subSurfButton->setChecked( true );
    BlockSignalsInNextUpdate();
    ConnectUpdateFlag();
}

CompGeomScreen::CompGeomScreen( ScreenMgr* mgr ) :
    VspScreenQt( *new CompGeomScreenPrivate( this ), mgr )
{
}

bool CompGeomScreenPrivate::Update()
{
    Vehicle* const veh = this->veh();
    LoadSetChoice( Ui.setChoice, KeepIndex );

    Ui.csvFileButton->setChecked( veh->getExportCompGeomCsvFile() );
    Ui.tsvFileButton->setChecked( veh->getExportDragBuildTsvFile() );

    Ui.csvFileOutput->setText( veh->getExportFileName( vsp::COMP_GEOM_CSV_TYPE ).c_str() );
    Ui.tsvFileOutput->setText( veh->getExportFileName( vsp::DRAG_BUILD_TSV_TYPE ).c_str() );
    Ui.txtFileOutput->setText( veh->getExportFileName( vsp::COMP_GEOM_TXT_TYPE ).c_str() );

    return true;
}

CompGeomScreen::~CompGeomScreen()
{
}

#include "CompGeomScreen.moc"
