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
#include <QDebug>
using namespace vsp;

#include <assert.h>

class SetEditorWidget : public QDialog {
    Q_OBJECT
    Q_DECLARE_PRIVATE( SetEditorScreen )
    SetEditorScreenPrivate * const d_ptr;

    Q_SLOT void on_geomInSetBrowser_itemChanged(QListWidgetItem * item);
    Q_SLOT void on_setBrowser_currentRowChanged(int index);
    Q_SLOT void on_setNameInput_textChanged(const QString & text);
    Q_SLOT void on_highlightSetButton_clicked();
public:
    SetEditorWidget( SetEditorScreenPrivate * p, QWidget * parent = 0 );

};

class SetEditorScreenPrivate {
    Q_DECLARE_PUBLIC(SetEditorScreen)
    SetEditorScreen * const q_ptr;
public:
    SetEditorWidget Widget;
    Ui::SetEditorScreen Ui;
    bool neverShown;

    SetEditorScreenPrivate(SetEditorScreen * q) : q_ptr(q), Widget(this), neverShown(true) {}
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

#if 0
    //==== Load Set Names and Values ====//
    d->SetEditorUI->setBrowser->clear();
    for ( int i = SET_SHOWN ; i < ( int )set_name_vec.size() ; i++ )
    {
        d->SetEditorUI->setBrowser->add( set_name_vec[i].c_str() );
    }
    d->SetEditorUI->setBrowser->select( d->SelectedSetIndex );
    d->SetEditorUI->setNameInput->value( set_name_vec[d->SelectedSetIndex].c_str() );

    if ( d->SelectedSetIndex <= SET_NOT_SHOWN )
    {
        d->SetEditorUI->setNameInput->deactivate();
    }
    else
    {
        d->SetEditorUI->setNameInput->activate();
    }
#endif
    d->Ui.setBrowser->clear();
    for ( int i = SET_SHOWN ; i < ( int )set_name_vec.size() ; i++ )
    {
        d->Ui.setBrowser->addItem( set_name_vec[i].c_str() );
    }

    //==== Load Geometry ====//
#if 0
    d->SetEditorUI->geomInSetBrowser->clear();
    vector< string > geom_id_vec = veh->GetGeomVec();
    vector< Geom* > geom_vec = veh->FindGeomVec( geom_id_vec );
    for ( int i = 0 ; i < ( int )geom_vec.size() ; i++ )
    {
        string gname = geom_vec[i]->GetName();
        bool flag = geom_vec[i]->GetSetFlag( d->SelectedSetIndex );
        d->SetEditorUI->geomInSetBrowser->add( gname.c_str(), !!flag );

    }
#endif

    if (d->neverShown) {
        d->Ui.setBrowser->setCurrentRow( SET_ALL );
        d->neverShown = false;
    }

    return true;
}

//==== Show Screen ====//
void SetEditorScreen::Show()
{
    Update();
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

//==== Callbacks ====//
#if 0
void SetEditorScreen::CallBack( Fl_Widget *w )
{
    Q_D(SetEditorScreen);
    Vehicle* veh = m_ScreenMgr->GetVehiclePtr();
    if ( w == d->SetEditorUI->geomInSetBrowser )
    {
        int geom_index = d->SetEditorUI->geomInSetBrowser->value() - 1;
        vector< string > geom_id_vec = veh->GetGeomVec();
        if ( geom_index >= 0 && geom_index < ( int )geom_id_vec.size() )
        {
            int flag = d->SetEditorUI->geomInSetBrowser->checked( geom_index + 1 );
            Geom* gptr = veh->FindGeom( geom_id_vec[ geom_index ] );
            if ( gptr )
            {
                gptr->SetSetFlag( d->SelectedSetIndex, !!flag );
            }
        }
    }
    else if ( w == d->SetEditorUI->setBrowser )
    {
        d->SelectedSetIndex = d->SetEditorUI->setBrowser->value();
    }
    else if ( w == d->SetEditorUI->setNameInput )
    {
        string name = string( d->SetEditorUI->setNameInput->value() );
        veh->SetSetName( d->SelectedSetIndex, name );
    }
    else if ( w == d->SetEditorUI->highlightSetButton )
    {
//jrg fix??
//      veh->HightlightSet( d->SelectedSetIndex );
    }

    m_ScreenMgr->SetUpdateFlag( true );
//  m_ScreenMgr->UpdateAllScreens();
}
#endif

SetEditorWidget::SetEditorWidget(SetEditorScreenPrivate * p, QWidget * parent) :
    QDialog(parent), d_ptr(p)
{
    Q_D(SetEditorScreen);
    d->Ui.setupUi(this);
}

void SetEditorWidget::on_geomInSetBrowser_itemChanged(QListWidgetItem * item)
{
    Q_D(SetEditorScreen);
    int geom_index = d->Ui.geomInSetBrowser->currentRow() /* -1 ?! */;
    vector< string > geom_id_vec = d->veh()->GetGeomVec();
    if ( geom_index >= 0 && geom_index < ( int )geom_id_vec.size() )
    {
        bool flag = item->checkState() == Qt::Checked;
        Geom* gptr = d->veh()->FindGeom( geom_id_vec[ geom_index ] );
        if ( gptr )
        {
            gptr->SetSetFlag( d->Ui.setBrowser->currentRow(), flag );
        }
    }
}

void SetEditorWidget::on_setBrowser_currentRowChanged(int index)
{
    Q_D(SetEditorScreen);
    d->Ui.setNameInput->setText( d->veh()->GetSetNameVec()[index+1].c_str() );
    d->Ui.setNameInput->setEnabled( index+1 > SET_NOT_SHOWN );

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
        d->Ui.geomInSetBrowser->addItem(item);
    }
}

void SetEditorWidget::on_setNameInput_textChanged(const QString & text)
{
    Q_D(SetEditorScreen);
    string name = text.toStdString();
    d->veh()->SetSetName( d->Ui.setBrowser->currentRow(), name );
}

void SetEditorWidget::on_highlightSetButton_clicked()
{
    Q_D(SetEditorScreen);
    // jrg fix??
    /// \todo
    //d->veh()->HighlightSet( d->Ui.setBrowser->currentRow() );
}

#include "SetEditorScreen.moc"
