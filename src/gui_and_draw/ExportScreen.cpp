//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "ExportScreen.h"
#include "ScreenMgr.h"
#include "Vehicle.h"
#include "APIDefines.h"
#include "ui_ExportScreen.h"
#include "VspScreenQt_p.h"

using namespace vsp;

class ExportScreenPrivate: public QDialog, public VspScreenQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( ExportScreen )
    Ui::ExportScreen Ui;

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    ExportScreenPrivate( ExportScreen * );

    void ExportFile( int type ) {
        q_func()->ExportFile( Ui.setChoice->currentIndex(), type );
    }
    Q_SLOT void on_xsecButton_clicked()
    {
        ExportFile( EXPORT_XSEC );
    }
    Q_SLOT void on_sterolithButton_clicked()
    {
        ExportFile( EXPORT_STL );
    }
    Q_SLOT void on_nascartButton_clicked()
    {
        ExportFile( EXPORT_NASCART );
    }
    Q_SLOT void on_cart3dButton_clicked()
    {
        ExportFile( EXPORT_CART3D );
    }
    Q_SLOT void on_gmshButton_clicked()
    {
        ExportFile( EXPORT_GMSH );
    }
    Q_SLOT void on_povrayButton_clicked()
    {
        ExportFile( EXPORT_POVRAY );
    }
    Q_SLOT void on_x3dButton_clicked()
    {
        ExportFile( EXPORT_X3D );
    }
    Q_SLOT void on_stepButton_clicked()
    {
        ExportFile( EXPORT_STEP );
    }
    Q_SLOT void on_bezButton_clicked()
    {
        ExportFile( EXPORT_BEZ );
    }
};
VSP_DEFINE_PRIVATE( ExportScreen )

ExportScreenPrivate::ExportScreenPrivate( ExportScreen * q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );
    EnableUpdateFlags();
}

ExportScreen::ExportScreen( ScreenMgr* mgr ) :
    VspScreenQt( *new ExportScreenPrivate( this ), mgr )
{}

bool ExportScreenPrivate::Update()
{
    LoadSetChoice( Ui.setChoice, KeepIndex );
    return true;
}

std::string ExportScreen::ExportFile( int write_set, int type )
{
    Q_D( ExportScreen );
    std::string newfile;
    switch ( type ) {
    case EXPORT_XSEC:
        newfile = m_ScreenMgr->GetSelectFileScreen()->FileSave( "Write XSec File?", "*.hrm" );
        break;
    case EXPORT_STL:
        newfile = m_ScreenMgr->GetSelectFileScreen()->FileSave( "Write STL File?", "*.stl" );
        break;
    case EXPORT_RHINO3D:
        newfile = m_ScreenMgr->GetSelectFileScreen()->FileSave( "Write Rhino3D File?", "*.3dm" );
        break;
    case EXPORT_NASCART:
        newfile = m_ScreenMgr->GetSelectFileScreen()->FileSave( "Write NASCART Files?", "*.dat" );
        break;
    case EXPORT_CART3D:
        newfile = m_ScreenMgr->GetSelectFileScreen()->FileSave( "Write Cart3D Files?", "*.tri" );
        break;
    case EXPORT_GMSH:
        newfile = m_ScreenMgr->GetSelectFileScreen()->FileSave( "Write GMsh Files?", "*.msh" );
        break;
    case EXPORT_POVRAY:
        newfile = m_ScreenMgr->GetSelectFileScreen()->FileSave( "Write POVRAY File?", "*.pov" );
        break;
    case EXPORT_X3D:
        newfile = m_ScreenMgr->GetSelectFileScreen()->FileSave( "Write X3D File?", "*.x3d" );
        break;
    case EXPORT_STEP:
        newfile = m_ScreenMgr->GetSelectFileScreen()->FileSave( "Write STEP File?", "*.stp" );
        break;
    case EXPORT_BEZ:
        newfile = m_ScreenMgr->GetSelectFileScreen()->FileSave( "Write Bezier File?", "*.bez" );
        break;
    case -1:
        d->show();
        return newfile;
    }

    if ( !newfile.empty() && newfile[ newfile.size() - 1] != '/' )
    {
        d->veh()->ExportFile( newfile, write_set, type );
        d->SetUpdateFlag();
        d->accept();
    }
    else
    {
        d->reject();
    }
    return newfile;
}

ExportScreen::~ExportScreen() {}

#include "ExportScreen.moc"
