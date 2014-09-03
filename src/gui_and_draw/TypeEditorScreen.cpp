//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "TypeEditorScreen.h"
#include "ScreenMgr.h"
#include "CustomGeom.h"
#include "ui_TypeEditorScreen.h"
#include "VspScreenQt_p.h"

class TypeEditorScreenPrivate : public QDialog, public VspScreenQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( TypeEditorScreen )
    Q_PRIVATE_SLOT( self(), void SetUpdateFlag() )
    Ui::TypeEditorScreen Ui;

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    TypeEditorScreenPrivate( TypeEditorScreen * );

    Q_SLOT void on_addButton_clicked()
    {
        vector< string > geom_id_vec = veh()->GetValidTypeGeoms();
        int geomIndex = Ui.geomChoice->currentIndex();
        if ( geomIndex >= 0 && geomIndex < ( int )geom_id_vec.size() )
        {
            veh()->AddType( geom_id_vec[geomIndex] );
        }
    }
    Q_SLOT void on_typeNameInput_textChanged( const QString & val )
    {
        int offset = veh()->GetNumFixedGeomTypes();
        int typeIndex = Ui.typeBrowser->currentRow();
        GeomType type = veh()->GetGeomType( typeIndex + offset );
        if ( type.m_FixedFlag == false )
        {
            type.m_Name = val.toStdString();
            veh()->SetGeomType( typeIndex + offset, type );
        }
    }
    Q_SLOT void on_deleteType_clicked()
    {
        int offset = veh()->GetNumFixedGeomTypes();
        veh()->DeleteType( Ui.typeBrowser->currentRow() + offset );
        Ui.typeBrowser->setCurrentRow(-1);
    }
    Q_SLOT void on_customScriptFileButton_clicked()
    {
        int scriptIndex = Ui.customScriptsBrowser->currentRow();
        vector< string > mod_name_vec = CustomGeomMgr.GetCustomScriptModuleNames();
        if ( scriptIndex >= 0 && scriptIndex < (int)mod_name_vec.size() )
        {
            string module_name = mod_name_vec[scriptIndex];

            string dir = CustomGeomMgr.GetScriptDir();
            string savefile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Save Custom Geom Script", "*.as",  dir.c_str() );

            CustomGeomMgr.SaveScriptContentToFile( module_name, savefile );
        }
    }
};
VSP_DEFINE_PRIVATE( TypeEditorScreen )

TypeEditorScreenPrivate::TypeEditorScreenPrivate( TypeEditorScreen * q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );
    BlockSignalsInUpdates();
    ConnectUpdateFlag();
}

TypeEditorScreen::TypeEditorScreen( ScreenMgr* mgr ) :
    VspScreenQt( *new TypeEditorScreenPrivate( this ), mgr )
{}

bool TypeEditorScreenPrivate::Update()
{
    //==== Load Type Names and Values ====//
    //==== Only Display Editable Types =====//
    vector< GeomType > type_vec = veh()->GetEditableGeomTypes();
    int typeIndex = qMin( Ui.typeBrowser->currentRow(), ( int )type_vec.size() - 1 );
    Ui.typeBrowser->clear();
    for ( int i = 0 ; i < ( int )type_vec.size() ; i++ )
    {
        Ui.typeBrowser->addItem( type_vec[i].m_Name.c_str() );
    }

    if ( typeIndex >= 0 )
    {
        Ui.typeBrowser->setCurrentRow( typeIndex );
        Ui.typeNameInput->setText( type_vec[typeIndex].m_Name.c_str() );
    }
    else
    {
        Ui.typeNameInput->clear();
    }

    //==== Load Geometry ====//
    int geomIndex = Ui.geomChoice->currentIndex();
    Ui.geomChoice->clear();
    vector< string > geom_id_vec = veh()->GetValidTypeGeoms();
    vector< Geom* > geom_vec = veh()->FindGeomVec( geom_id_vec );
    for ( int i = 0 ; i < ( int )geom_vec.size() ; i++ )
    {
        QString str = QString( "%1.  %2" ).arg( i + 1 ).arg( geom_vec[i]->GetName().c_str() );
        Ui.geomChoice->addItem( str );
    }

    if ( geomIndex >= 0 )
    {
        Ui.geomChoice->setCurrentIndex( geomIndex );
    }

    //==== Load Custom Script Module Names ====//
    int scriptIndex = Ui.customScriptsBrowser->currentRow();
    Ui.customScriptsBrowser->clear();
    vector< string > mod_name_vec = CustomGeomMgr.GetCustomScriptModuleNames();
    for ( int i = 0 ; i < ( int )mod_name_vec.size() ; i++ )
    {
        Ui.customScriptsBrowser->addItem( mod_name_vec[i].c_str() );
    }

    if ( scriptIndex >= ( int )mod_name_vec.size() )
        scriptIndex = -1;
    if ( scriptIndex >= 0 )
        Ui.customScriptsBrowser->setCurrentRow( scriptIndex );

    return true;
}

TypeEditorScreen::~TypeEditorScreen() {}

#include "TypeEditorScreen.moc"
