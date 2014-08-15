//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#define QPoint QQPoint
#include "SetEditorScreen.h"
#include "ScreenMgr.h"
#include "EventMgr.h"
#include "Vehicle.h"
#include "StlHelper.h"
#include "APIDefines.h"
#include "GuiDevice.h"
#undef QPoint
#include "VspScreenQt_p.h"
#include "ui_SetEditorScreen.h"
using namespace vsp;

class SetEditorScreenPrivate : public QDialog, public VspScreenQtPrivate {
    Q_OBJECT
    Q_DECLARE_PUBLIC( SetEditorScreen )
    Ui::SetEditorScreen Ui;

    Q_SLOT void on_geomInSetBrowser_itemChanged( QListWidgetItem * item );
    Q_SLOT void on_setBrowser_currentItemChanged( QListWidgetItem * item );
    Q_SLOT void on_setNameInput_textEdited( const QString & text );
    Q_SLOT void on_highlightSetButton_clicked();
    bool Update() Q_DECL_OVERRIDE;

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    SetEditorScreenPrivate( SetEditorScreen * );
};
VSP_DEFINE_PRIVATE( SetEditorScreen )

SetEditorScreen::SetEditorScreen( ScreenMgr* mgr ) :
    VspScreenQt( *new SetEditorScreenPrivate( this ), mgr )
{
}

SetEditorScreen::~SetEditorScreen()
{
}

bool SetEditorScreenPrivate::Update()
{
    vector< string > set_name_vec = veh()->GetSetNameVec();

    //==== Load Set Names and Values ====//
    int row = Ui.setBrowser->currentRow();
    Ui.setBrowser->clear();
    for ( int i = SET_SHOWN ; i < ( int )set_name_vec.size() ; i++ )
    {
        QListWidgetItem * item = new QListWidgetItem;
        item->setText( set_name_vec[i].c_str() );
        item->setData( Qt::UserRole, i );
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren );
        Ui.setBrowser->addItem( item );
    }
    Ui.setBrowser->setCurrentRow( row );

    return true;
}

void SetEditorScreen::Show()
{
    VspScreenQt::Show();
    d_func()->Ui.setBrowser->setCurrentRow( 0 );
}

SetEditorScreenPrivate::SetEditorScreenPrivate( SetEditorScreen * q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );
    /// \todo Reenable upon fixing on_highlightSetButton_clicked()
    Ui.highlightSetButton->setEnabled( false );
}

void SetEditorScreenPrivate::on_geomInSetBrowser_itemChanged( QListWidgetItem * item )
{
    Q_ASSERT( item );
    if (! Ui.setBrowser->currentItem()) return;

    //==== Load Geometry ====//
    int set_index = Ui.setBrowser->currentItem()->data(Qt::UserRole).toInt();
    int geom_index = Ui.geomInSetBrowser->row( item ) /* -1 ?! */;
    vector< string > geom_id_vec = veh()->GetGeomVec();
    if ( geom_index >= 0 && geom_index < ( int )geom_id_vec.size() )
    {
        bool flag = item->checkState() == Qt::Checked;
        Geom* gptr = veh()->FindGeom( geom_id_vec[ geom_index ] );
        if ( gptr )
        {
            gptr->SetSetFlag( set_index, flag );
        }
    }
    SetUpdateFlag();
}

void SetEditorScreenPrivate::on_setBrowser_currentItemChanged( QListWidgetItem * item )
{
    if (! item) return;
    int index = item->data( Qt::UserRole ).toInt();
    Ui.setNameInput->setText( veh()->GetSetNameVec()[index].c_str() );
    Ui.setNameInput->setEnabled( index > SET_NOT_SHOWN );

    Ui.geomInSetBrowser->clear();
    vector< string > geom_id_vec = veh()->GetGeomVec();
    vector< Geom* > geom_vec = veh()->FindGeomVec( geom_id_vec );
    for ( int i = 0 ; i < ( int )geom_vec.size() ; i++ )
    {
        string gname = geom_vec[i]->GetName();
        bool flag = geom_vec[i]->GetSetFlag( index );
        QListWidgetItem * item = new QListWidgetItem( gname.c_str() );
        item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren | Qt::ItemIsEditable | Qt::ItemIsEnabled );
        item->setCheckState( flag ? Qt::Checked : Qt::Unchecked );
        Ui.geomInSetBrowser->addItem( item );
    }
}

void SetEditorScreenPrivate::on_setNameInput_textEdited(const QString & text)
{
    string name = text.toStdString();
    QListWidgetItem * item = Ui.setBrowser->currentItem();
    int index = item->data( Qt::UserRole ).toInt();
    item->setText( text );
    veh()->SetSetName( index, name );
    SetUpdateFlag();
}

void SetEditorScreenPrivate::on_highlightSetButton_clicked()
{
    /// \todo jrg fix??
#if 0
    veh()->HighlightSet( Ui.setBrowser->currentRow() );
#endif
}

#include "SetEditorScreen.moc"
