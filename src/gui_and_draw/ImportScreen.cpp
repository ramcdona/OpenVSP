//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//////////////////////////////////////////////////////////////////////

#include "ImportScreen.h"
#include "ScreenMgr.h"
#include "Vehicle.h"
#include "APIDefines.h"
#include "VspScreenQt_p.h"
#include "ui_ImportScreen.h"

using namespace vsp;

class ImportScreenPrivate : public QDialog, public VspScreenQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( ImportScreen )
    Ui::ImportScreen Ui;

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE { return true; }
    ImportScreenPrivate( ImportScreen * );

    Q_SLOT void on_sterolithButton_clicked()
    {
        q_func()->ImportFile( IMPORT_STL );
    }
    Q_SLOT void on_nascartButton_clicked()
    {
        q_func()->ImportFile( IMPORT_NASCART );
    }
    Q_SLOT void on_Cart3DTriButton_clicked()
    {
        q_func()->ImportFile( IMPORT_CART3D_TRI );
    }
    Q_SLOT void on_xsecButton_clicked()
    {
        q_func()->ImportFile( IMPORT_XSEC_MESH );
    }
};
VSP_DEFINE_PRIVATE( ImportScreen )

ImportScreenPrivate::ImportScreenPrivate( ImportScreen * q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );
    EnableUpdateFlags();
}

ImportScreen::ImportScreen( ScreenMgr* mgr ) :
    VspScreenQt( *new ImportScreenPrivate( this ), mgr )
{
}

std::string ImportScreen::ImportFile( int type )
{
    Q_D( ImportScreen );
    std::string in_file;
    switch ( type ) {
    case IMPORT_STL:
        in_file = m_ScreenMgr->GetSelectFileScreen()->FileOpen( "Import STL file?", "*.stl" );
        break;
    case IMPORT_NASCART:
        in_file = m_ScreenMgr->GetSelectFileScreen()->FileOpen( "Import NASCART file?", "*.dat" );
        break;
    case IMPORT_CART3D_TRI:
        in_file = m_ScreenMgr->GetSelectFileScreen()->FileOpen( "Import Cart3D Tri File?", "*.tri" );
        break;
    case IMPORT_XSEC_MESH:
        in_file = m_ScreenMgr->GetSelectFileScreen()->FileOpen( "Import XSec File?", "*.hrm" );
        break;
    default:
        d->show();
        return in_file;
    }

    if ( !in_file.empty() && in_file[ in_file.size() - 1] != '/' )
    {
        d->veh()->ImportFile( in_file, type );
        d->SetUpdateFlag();
        d->accept();
    }
    else
    {
        d->reject();
    }
    return in_file;
}

ImportScreen::~ImportScreen() {}

#include "ImportScreen.moc"
