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
#include "setEditorFlScreen.h"
#include <FL/Fl.H>
#undef QPoint
#include "ui_SetEditorScreen.h"
using namespace vsp;

#include <assert.h>

class SetEditorWidget : public QDialog {
    Q_OBJECT
    Q_DECLARE_PRIVATE( SetEditorScreen )
    SetEditorScreenPrivate * const d_ptr;

    Q_SLOT void on_geomInSetBrowser_itemChanged( QListWidgetItem * item );
    Q_SLOT void on_setBrowser_currentItemChanged( QListWidgetItem * item );
    Q_SLOT void on_setNameInput_textEdited( const QString & text );
    Q_SLOT void on_highlightSetButton_clicked();
public:
    SetEditorWidget( SetEditorScreenPrivate * p, QWidget * parent = 0 );
};

class SetEditorScreenPrivate {
    SetEditorScreen * const q_ptr;
public:
    Q_DECLARE_PUBLIC(SetEditorScreen)
    SetEditorWidget Widget;
    Ui::SetEditorScreen Ui;

    SetEditorScreenPrivate( SetEditorScreen * q ) : q_ptr( q ), Widget( this ) {}
    Vehicle* veh() { return q_func()->GetScreenMgr()->GetVehiclePtr(); }
};

//==== Constructor ====//
SetEditorScreen::SetEditorScreen( ScreenMgr* mgr ) : VspScreen( mgr ),
    d_ptr(new SetEditorScreenPrivate(this))
{
}

SetEditorScreen::~SetEditorScreen()
{
}

//==== Update Screen ====//
bool SetEditorScreen::Update()
{
    Q_D(SetEditorScreen);
    Vehicle* veh = m_ScreenMgr->GetVehiclePtr();

    vector< string > set_name_vec = veh->GetSetNameVec();

    //==== Load Set Names and Values ====//
    int row = d->Ui.setBrowser->currentRow();
    d->Ui.setBrowser->clear();
    for ( int i = SET_SHOWN ; i < ( int )set_name_vec.size() ; i++ )
    {
        QListWidgetItem * item = new QListWidgetItem;
        item->setText( set_name_vec[i].c_str() );
        item->setData( Qt::UserRole, i );
        item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren );
        d->Ui.setBrowser->addItem( item );
    }
    d->Ui.setBrowser->setCurrentRow( row );

    return true;
}

//==== Show Screen ====//
void SetEditorScreen::Show()
{
    Q_D(SetEditorScreen);
    Update();
    d->Ui.setBrowser->setCurrentRow( 0 );
    d_func()->Widget.show();
}

//==== Hide Screen ====//
void SetEditorScreen::Hide()
{
    d_func()->Widget.hide();
}

bool SetEditorScreen::IsShown()
{
    return d_func()->Widget.isVisible();
}

void SetEditorScreen::SetNonModal()
{
}

SetEditorWidget::SetEditorWidget( SetEditorScreenPrivate * p, QWidget * parent ) :
    QDialog( parent ), d_ptr( p )
{
    Q_D( SetEditorScreen );
    d->Ui.setupUi( this );
    /// \todo Reenable upon fixing on_highlightSetButton_clicked()
    d->Ui.highlightSetButton->setEnabled( false );
}

//==== Callbacks ====//
void SetEditorWidget::on_geomInSetBrowser_itemChanged( QListWidgetItem * item )
{
    Q_D( SetEditorScreen );
    Q_ASSERT( item );
    if (! d->Ui.setBrowser->currentItem()) return;

    //==== Load Geometry ====//
    int set_index = d->Ui.setBrowser->currentItem()->data(Qt::UserRole).toInt();
    int geom_index = d->Ui.geomInSetBrowser->row( item ) /* -1 ?! */;
    vector< string > geom_id_vec = d->veh()->GetGeomVec();
    if ( geom_index >= 0 && geom_index < ( int )geom_id_vec.size() )
    {
        bool flag = item->checkState() == Qt::Checked;
        Geom* gptr = d->veh()->FindGeom( geom_id_vec[ geom_index ] );
        if ( gptr )
        {
            gptr->SetSetFlag( set_index, flag );
        }
    }
    d->q_func()->GetScreenMgr()->SetUpdateFlag( true );
}

void SetEditorWidget::on_setBrowser_currentItemChanged( QListWidgetItem * item )
{
    Q_D( SetEditorScreen );
    if (! item) return;
    int index = item->data( Qt::UserRole ).toInt();
    d->Ui.setNameInput->setText( d->veh()->GetSetNameVec()[index].c_str() );
    d->Ui.setNameInput->setEnabled( index > SET_NOT_SHOWN );

    d->Ui.geomInSetBrowser->clear();
    vector< string > geom_id_vec = d->veh()->GetGeomVec();
    vector< Geom* > geom_vec = d->veh()->FindGeomVec( geom_id_vec );
    for ( int i = 0 ; i < ( int )geom_vec.size() ; i++ )
    {
        string gname = geom_vec[i]->GetName();
        bool flag = geom_vec[i]->GetSetFlag( index );
        QListWidgetItem * item = new QListWidgetItem( gname.c_str() );
        item->setFlags( Qt::ItemIsUserCheckable | Qt::ItemNeverHasChildren | Qt::ItemIsEditable | Qt::ItemIsEnabled );
        item->setCheckState( flag ? Qt::Checked : Qt::Unchecked );
        d->Ui.geomInSetBrowser->addItem( item );
    }
}

void SetEditorWidget::on_setNameInput_textEdited(const QString & text)
{
    Q_D( SetEditorScreen );
    string name = text.toStdString();
    QListWidgetItem * item = d->Ui.setBrowser->currentItem();
    int index = item->data( Qt::UserRole ).toInt();
    item->setText( text );
    d->veh()->SetSetName( index, name );
    d->q_func()->GetScreenMgr()->SetUpdateFlag( true );
}

void SetEditorWidget::on_highlightSetButton_clicked()
{
    Q_D(SetEditorScreen);
    /// \todo jrg fix??
    //d->veh()->HighlightSet( d->Ui.setBrowser->currentRow() );
}

#include "SetEditorScreen.moc"
