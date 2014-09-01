//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "ManageGeomScreen.h"
#include "ScreenMgr.h"
#include "EventMgr.h"
#include "Vehicle.h"
#include "StlHelper.h"
#include "StringUtil.h"
#include "APIDefines.h"

#include "GuiDevice.h"
#include "PodScreen.h"
#include "FuselageScreen.h"
#include "WingScreen.h"
#include "BlankScreen.h"
#include "MeshScreen.h"
#include "StackScreen.h"
#include "CustomScreen.h"
#include "DrawObj.h"

#include "ui_ManageGeomScreen.h"
#include "VspScreenQt_p.h"
#include <cassert>

using namespace vsp;
using std::vector;
using std::string;

class ManageGeomScreenPrivate : public QDialog, public VspScreenQtPrivate {
    Q_OBJECT
    Q_DECLARE_PUBLIC( ManageGeomScreen )
    Q_PRIVATE_SLOT( self(), void SetUpdateFlag() )

    enum { POD_GEOM_SCREEN, FUSELAGE_GEOM_SCREEN, MS_WING_GEOM_SCREEN, BLANK_GEOM_SCREEN,
           MESH_GEOM_SCREEN, STACK_GEOM_SCREEN, CUSTOM_GEOM_SCREEN, NUM_GEOM_SCREENS
         };

    Ui::ManageGeomScreen Ui;
    int LastTopLine;
    int SetIndex;
    int TypeIndex;
    bool CollapseFlag;
    string LastSelectedGeomID;

    vector< VspScreen* > GeomScreenVec;
    vector< string > DisplayedGeomVec;
    vector< DrawObj > PickList;

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    ManageGeomScreenPrivate( ManageGeomScreen * );

    void ShowHideGeomScreens();
    void CreateScreens();
    void UpdateGeomScreens();
    void AddGeom();
    void LoadBrowser();
    void LoadActiveGeomOutput();
    void LoadTypeChoice();
    void UpdateDrawType();
    void SelectGeomBrowser( string geom_id );
    bool IsParentSelected( string geom_id, vector< string > & selVec );
    void NoShowActiveGeoms( bool flag );
    void SelectAll();
    void SetGeomDisplayType( int type );
    void EditName( string name );
    void SetSubDrawFlag( bool f );
    void SetFeatureDrawFlag( bool f );
    vector< string > GetActiveGeoms();
    vector< string > GetSelectedBrowserItems();
    void UpdateDrawObjs();

    Q_SLOT void on_geomTypeChoice_currentIndexChanged( int val )
    {
        TypeIndex = val;
    }
    Q_SLOT void on_addGeomButton_clicked()
    {
        AddGeom();
    }
    Q_SLOT void on_geomBrowser_itemSelectionChanged();
    Q_SLOT void on_geomBrowser_itemClicked( QListWidgetItem* );
    Q_SLOT void on_noshowGeomButton_clicked()
    {
        NoShowActiveGeoms( true );
    }
    Q_SLOT void on_showGeomButton_clicked()
    {
        NoShowActiveGeoms( false );
    }
    Q_SLOT void on_selectAllGeomButton_clicked()
    {
        SelectAll();
    }
    Q_SLOT void on_activeGeomInput_textChanged( const QString & str ) // Geom or Aircraft Name
    {
        EditName( str.toLocal8Bit().constData() );
    }
    Q_SLOT void on_cutGeomButton_clicked()
    {
        veh()->CutActiveGeomVec();
    }
    Q_SLOT void on_copyGeomButton_clicked()
    {
        veh()->CopyActiveGeomVec();
    }
    Q_SLOT void on_pasteGeomButton_clicked()
    {
        veh()->PasteClipboard();
    }
    Q_SLOT void on_wireGeomButton_clicked()
    {
        SetGeomDisplayType( GeomGuiDraw::GEOM_DRAW_WIRE );
    }
    Q_SLOT void on_hiddenGeomButton_clicked()
    {
        SetGeomDisplayType( GeomGuiDraw::GEOM_DRAW_HIDDEN );
    }
    Q_SLOT void on_shadeGeomButton_clicked()
    {
        SetGeomDisplayType( GeomGuiDraw::GEOM_DRAW_SHADE );
    }
    Q_SLOT void on_textureGeomButton_clicked()
    {
        SetGeomDisplayType( GeomGuiDraw::GEOM_DRAW_TEXTURE );
    }
    Q_SLOT void on_moveUpButton_clicked()
    {
        veh()->ReorderActiveGeom( Vehicle::REORDER_MOVE_UP );
    }
    Q_SLOT void on_moveDownButton_clicked()
    {
        veh()->ReorderActiveGeom( Vehicle::REORDER_MOVE_DOWN );
    }
    Q_SLOT void on_moveTopButton_clicked()
    {
        veh()->ReorderActiveGeom( Vehicle::REORDER_MOVE_TOP );
    }
    Q_SLOT void on_moveBotButton_clicked()
    {
        veh()->ReorderActiveGeom( Vehicle::REORDER_MOVE_BOTTOM );
    }
    Q_SLOT void on_setChoice_currentIndexChanged( int val )
    {
        SetIndex = val;
    }
    Q_SLOT void on_showSetButton_clicked()
    {
        veh()->SetShowSet( SetIndex + SET_FIRST_USER );
    }
    Q_SLOT void on_showSubToggle_toggled( bool val )
    {
        SetSubDrawFlag( val );
    }
    Q_SLOT void on_showFeatureToggle_toggled( bool val )
    {
        SetFeatureDrawFlag( val );
    }
};
VSP_DEFINE_PRIVATE( ManageGeomScreen )

ManageGeomScreenPrivate::ManageGeomScreenPrivate( ManageGeomScreen * q ) :
    VspScreenQtPrivate( q ),
    LastTopLine( 0 ),
    SetIndex( 0 ),
    TypeIndex( 0 ),
    CollapseFlag( false ),
    LastSelectedGeomID( "NONE" )
{
    Ui.setupUi( this );
    BlockSignalsInUpdates();
    ConnectUpdateFlag();
    // We're interested in selection changes, not current item changes.
    disconnect( Ui.geomBrowser, 0, this, SLOT( SetUpdateFlag() ) );
    connect( Ui.geomBrowser, SIGNAL( itemSelectionChanged() ), SLOT( SetUpdateFlag() ) );
    connect( Ui.geomBrowser, SIGNAL( itemClicked(QListWidgetItem*) ), SLOT( SetUpdateFlag() ) );
}

ManageGeomScreen::ManageGeomScreen( ScreenMgr* mgr ) :
    VspScreenQt( *new ManageGeomScreenPrivate( this ), mgr )
{
    d_func()->CreateScreens();
}

ManageGeomScreen::~ManageGeomScreen()
{
    /// \todo We should use a smart container or a container of smart pointers.
    Q_D( ManageGeomScreen );
    qDeleteAll( d->GeomScreenVec.begin(), d->GeomScreenVec.end() );
}

bool ManageGeomScreenPrivate::Update()
{
    if ( q_ptr->IsShown() )
    {
        LoadBrowser();
        LoadActiveGeomOutput();
        LoadSetChoice( Ui.setChoice, SetIndex, StartWithUserSets );
        LoadTypeChoice();
        UpdateDrawType();
    }

    UpdateGeomScreens();

    return true;
}

/// Update All Geom Screens
void ManageGeomScreenPrivate::UpdateGeomScreens()
{
    foreach ( VspScreen* screen, GeomScreenVec )
    {
        if ( screen->IsShown() )
        {
            screen->Update();
        }
    }
}

void ManageGeomScreen::Show()
{
    Q_D( ManageGeomScreen );
    d->SetUpdateFlag();
    d->widget()->show();
}

void ManageGeomScreenPrivate::LoadBrowser()
{
    //==== Save List of Selected Geoms ====//
    vector< string > activeVec = veh()->GetActiveGeomVec();

    QModelIndex topIndex = Ui.geomBrowser->indexAt(QPoint( 2, 2 ));
    if ( topIndex.isValid() )
        LastTopLine = topIndex.row();
    else
        LastTopLine = 0;

    //==== Display Vehicle Name ====//
    Ui.geomBrowser->clear();
    Ui.geomBrowser->addItem( veh()->GetName().c_str() );

    //==== Get Geoms To Display ====//
    DisplayedGeomVec = veh()->GetGeomVec( true );

    //==== Step Thru Comps ====//
    for ( int i = 0 ; i < ( int )DisplayedGeomVec.size() ; i++ )
    {
        Geom* gPtr = veh()->FindGeom( DisplayedGeomVec[i] );
        if ( gPtr )
        {
            QFont font;
            string str;
            //==== Check if Parent is Selected ====//
            if ( IsParentSelected( DisplayedGeomVec[i], activeVec ) )
            {
                font.setBold( true );
            }

            int numindents = gPtr->CountParents( 0 );
            for ( int j = 0 ; j < numindents ; j++ )
            {
                str.append( "--" );
            }

            if ( gPtr->m_TransAttachFlag() == GeomXForm::ATTACH_TRANS_NONE &&
                    gPtr->m_RotAttachFlag() == GeomXForm::ATTACH_ROT_NONE )
            {
                str.append( "> " );
            }
            else
            {
                str.append( "^ " );
            }

            if ( !gPtr->m_GuiDraw.GetDisplayChildrenFlag() )
            {
                str.append( "(+) " );
            }

            str.append( gPtr->GetName() );

            if ( gPtr->m_GuiDraw.GetNoShowFlag() )
            {
                str.append( "(no show)" );
            }

            auto item = new QListWidgetItem( str.c_str() );
            item->setFont( font );
            item->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
            Ui.geomBrowser->addItem( item );
        }
    }

    //==== Restore List of Selected Geoms ====//
    for ( int i = 0 ; i < ( int )activeVec.size() ; i++ )
    {
        SelectGeomBrowser( activeVec[i] );
    }
}

void ManageGeomScreenPrivate::SelectGeomBrowser( string geom_id )
{
    //==== Select If Match ====//
    //==== Position Browser ====//
    for ( int i = 0 ; i < ( int )DisplayedGeomVec.size() ; i++ )
    {
        if ( DisplayedGeomVec[i] == geom_id )
        {
            Ui.geomBrowser->item( i + 1 )->setSelected( true );
            Ui.geomBrowser->scrollToItem( Ui.geomBrowser->item( i + 1 ) );
            break; // the items are unique
        }
    }

    if ( !CollapseFlag && LastTopLine < ( ( int )DisplayedGeomVec.size() - 1 ) )
    {
        Ui.geomBrowser->scrollToItem( Ui.geomBrowser->item( LastTopLine ) );
    }

}

/// Is Parent (or Higher) Selected?
bool ManageGeomScreenPrivate::IsParentSelected( string geom_id, vector< string > & selVec )
{
    Geom* checkGeom = veh()->FindGeom( geom_id );
    while ( checkGeom )
    {
        if ( vector_contains_val( selVec, checkGeom->GetID() ) )
        {
            return true;
        }

        string parent_id = checkGeom->GetParentID();
        checkGeom = veh()->FindGeom( parent_id );
    }
    return false;
}

vector< string > ManageGeomScreenPrivate::GetSelectedBrowserItems()
{
    vector< string> selVec;

    if ( DisplayedGeomVec.empty() )
    {
        return selVec;
    }

    //==== Account For Aircraft Name ====//
    for ( int i = 1 ; i < Ui.geomBrowser->count() ; i++ )
    {
        if ( Ui.geomBrowser->item( i )->isSelected() && ( int )DisplayedGeomVec.size() > ( i - 1 ) )
        {
            selVec.push_back( DisplayedGeomVec[i - 1] );
        }
    }
    return selVec;
}

void ManageGeomScreenPrivate::LoadActiveGeomOutput()
{
    vector< string > activeVec = veh()->GetActiveGeomVec();
    if ( activeVec.empty() )
    {
        Ui.activeGeomInput->setText( veh()->GetName().c_str() );
    }
    else if ( activeVec.size() == 1 )
    {
        Geom* gptr = veh()->FindGeom( activeVec[0] );
        if ( gptr )
        {
            Ui.activeGeomInput->setText( gptr->GetName().c_str() );
        }
    }
    else
    {
        Ui.activeGeomInput->setText( "<multiple>" );
    }
}

void ManageGeomScreenPrivate::LoadTypeChoice()
{
    Ui.geomTypeChoice->clear();

    int num_type  = veh()->GetNumGeomTypes();
    int num_fixed = veh()->GetNumFixedGeomTypes();
    int cnt = 1;

    for ( int i = 0 ; i < num_type ; i++ )
    {
        GeomType type = veh()->GetGeomType( i );

        QString item( type.m_Name.c_str() );
        if ( i == ( num_fixed - 1 ) )
        {
            item = "_" + item;
        }

        if ( !type.m_FixedFlag )
        {
            item = QString( "%1.  " ).arg( cnt ) + item;
            cnt++;
        }
        Ui.geomTypeChoice->addItem( item );
    }

    Ui.geomTypeChoice->setCurrentIndex( TypeIndex );
}

/// Add Geom - Type From Type Choice
void ManageGeomScreenPrivate::AddGeom()
{
    GeomType type = veh()->GetGeomType( TypeIndex );

    if ( type.m_Type < NUM_GEOM_TYPE )
    {
        string added_id = veh()->AddGeom( type );
        veh()->SetActiveGeom( added_id );
        ShowHideGeomScreens();
    }
}

/// Item in Geom Browser Was Selected
void ManageGeomScreenPrivate::on_geomBrowser_itemSelectionChanged()
{
    vector< string > selVec = GetSelectedBrowserItems();
    veh()->SetActiveGeomVec( selVec );
    LoadActiveGeomOutput();

//  m_ScreenMgr->UpdateAllScreens();
    ShowHideGeomScreens();

/// \todo jrg FIX!!!
//  aircraftPtr->triggerDraw();
}

void ManageGeomScreenPrivate::on_geomBrowser_itemClicked( QListWidgetItem* )
{
    vector< string > selVec = GetSelectedBrowserItems();

    //==== Find Last Selected Geom ====//
    int last = Ui.geomBrowser->currentRow();
    if ( ( last >= 1 ) && qApp->keyboardModifiers() & Qt::AltModifier )   // Select Children
    {
        Geom* lastSelGeom = veh()->FindGeom( DisplayedGeomVec[last - 1] );
        if ( lastSelGeom )
        {
            vector<string> cVec;
            lastSelGeom->LoadIDAndChildren( cVec );
            for ( int i = 1 ; i < ( int )cVec.size(); i++ )
            {
                SelectGeomBrowser( cVec[i] );
                selVec.push_back( cVec[i] );
            }
            veh()->SetActiveGeomVec( selVec );
            LoadActiveGeomOutput();
        }
    }

    //==== Check if Geom Already Selected ====//
    /// \todo This is a hack. The geom browser should become a tree view
    /// (say QTreeWidget) - item collapsing is then a natural behavior.
    CollapseFlag = false;
    if ( LastSelectedGeomID != "NONE" && selVec.size() == 1 )
    {
        string lastSel = selVec[0];
        if ( lastSel == LastSelectedGeomID  )
        {
            CollapseFlag = true;
            Geom* lastSelGeom = veh()->FindGeom( LastSelectedGeomID );
            if ( lastSelGeom )
            {
                lastSelGeom->m_GuiDraw.ToggleDisplayChildrenFlag();
                if ( lastSelGeom->GetChildIDVec().size() == 0 )     // No Children Dont Collapse
                {
                    lastSelGeom->m_GuiDraw.SetDisplayChildrenFlag( true );
                }
            }
        }
    }
    LastSelectedGeomID = "NONE";
    if ( selVec.size() == 1 )
    {
        LastSelectedGeomID = selVec[0];
    }
}


//==== Show/NoShow Active Geoms and Children ====//
void ManageGeomScreenPrivate::NoShowActiveGeoms( bool flag )
{

    //==== Load Active Geom IDs And Children ====//
    vector<string> geom_id_vec;
    vector<string> active_geom_vec = veh()->GetActiveGeomVec();
    for ( int i = 0 ; i < ( int )active_geom_vec.size() ; i++ )
    {
        Geom* gPtr = veh()->FindGeom( active_geom_vec[i] );
        if ( gPtr )
        {
            gPtr->LoadIDAndChildren( geom_id_vec );
            gPtr->m_GuiDraw.SetNoShowFlag( flag );
        }
    }

    //==== Set No Show Flag ====//
    vector< Geom* > geom_vec = veh()->FindGeomVec( geom_id_vec );
    for ( int i = 0 ; i < ( int )geom_vec.size() ; i++ )
    {
        geom_vec[i]->m_GuiDraw.SetNoShowFlag( flag );
    }

/// \todo jrg FIX!!!
//  aircraftPtr->triggerDraw();
    LoadBrowser();
}

/// Select All Geoms
void ManageGeomScreenPrivate::SelectAll()
{
    vector< string > const all_geom_vec = veh()->GetGeomVec();
    veh()->SetActiveGeomVec( all_geom_vec );

    //==== Restore List of Selected Geoms ====//
    for ( int i = 0 ; i < ( int )all_geom_vec.size() ; i++ )
    {
        SelectGeomBrowser( all_geom_vec[i] );
    }

    LoadActiveGeomOutput();

/// \todo jrg FIX!!!
//  aircraftPtr->triggerDraw();
}

/// Load Active Geom IDs and Children
vector< string > ManageGeomScreenPrivate::GetActiveGeoms()
{
    //==== Load Active Geom IDs And Children ====//
    vector<string> geom_id_vec;
    vector<string> const active_geom_vec = veh()->GetActiveGeomVec();
    for ( int i = 0 ; i < ( int )active_geom_vec.size() ; i++ )
    {
        Geom* gPtr = veh()->FindGeom( active_geom_vec[i] );
        if ( gPtr )
        {
            if ( gPtr->m_GuiDraw.GetDisplayChildrenFlag() )
            {
                geom_id_vec.push_back( active_geom_vec[i] );
            }
            else
            {
                gPtr->LoadIDAndChildren( geom_id_vec );
            }
        }
    }

    return geom_id_vec;
}

void ManageGeomScreenPrivate::SetGeomDisplayType( int type )
{
    vector<string> geom_id_vec = GetActiveGeoms();
    //==== Set Display Type ====//
    vector< Geom* > geom_vec = veh()->FindGeomVec( geom_id_vec );
    for ( int i = 0 ; i < ( int )geom_vec.size() ; i++ )
    {
        geom_vec[i]->m_GuiDraw.SetDrawType( type );
    }

/// \todo jrg FIX!!!
//  aircraftPtr->triggerDraw();
}

void ManageGeomScreenPrivate::EditName( string name )
{
    vector<string> active_geom_vec = veh()->GetActiveGeomVec();

    //==== Dont Change Multiple Names ====//
    if ( active_geom_vec.size() > 1 )
    {
        return;
    }

    if ( active_geom_vec.size() == 0 )
    {
        veh()->SetName( name );
    }
    else
    {
        Geom* g_ptr = veh()->FindGeom( active_geom_vec[0] );
        if ( g_ptr )
        {
            g_ptr->SetName( name );
        }
    }
/// \todo jrg FIX!!!
//  Trigger Edit Screen Update...
}


//void ManageGeomScreenPrivate::s_select_none(int src) {
//  aircraftPtr->setActiveGeom(NULL);
//
//  if (src != ScriptMgr::SCRIPT)
//  {
//      deselectGeomBrowser();          // select none
//      screenMgrPtr->hideGeomScreens();
//      screenMgrPtr->getGroupScreen()->hide();
//      loadActiveGeomOutput();
//      aircraftPtr->flagActiveGeom();
//      aircraftPtr->triggerDraw();
//      geomUI->geomBrowser->value(1);
//  }
//  if (src == ScriptMgr::GUI) scriptMgr->addLine("select none");
//}

void ManageGeomScreenPrivate::CreateScreens()
{
    GeomScreenVec.resize( NUM_GEOM_SCREENS );
    GeomScreenVec[POD_GEOM_SCREEN] = new PodScreen( GetScreenMgr() );
    GeomScreenVec[FUSELAGE_GEOM_SCREEN] = new FuselageScreen( GetScreenMgr() );
    GeomScreenVec[MS_WING_GEOM_SCREEN] = new WingScreen( GetScreenMgr() );
    GeomScreenVec[BLANK_GEOM_SCREEN] = new BlankScreen( GetScreenMgr() );
    GeomScreenVec[MESH_GEOM_SCREEN] = new MeshScreen( GetScreenMgr() );
    GeomScreenVec[STACK_GEOM_SCREEN] = new StackScreen( GetScreenMgr() );
    GeomScreenVec[CUSTOM_GEOM_SCREEN] = new CustomScreen( GetScreenMgr() );

    for ( int i = 0 ; i < ( int )GeomScreenVec.size() ; i++ )
    {
        GeomScreenVec[i]->SetNonModal();
    }
}

/// Show Hide Geom Screen Depending on Active Geoms
void ManageGeomScreenPrivate::ShowHideGeomScreens()
{
    // Hide All Geom Screens
    // Show Screen - Each Screen Will Test Check Valid Active Geom Type
    for ( int i = 0 ; i < ( int )GeomScreenVec.size() ; i++ )
    {
        GeomScreenVec[i]->Hide();
        GeomScreenVec[i]->Show();
    }
}

/// Show or Hide Subsurface Lines
void ManageGeomScreenPrivate::SetSubDrawFlag( bool f )
{
    vector<string> geom_id_vec = GetActiveGeoms();
    vector< Geom* > geom_vec = veh()->FindGeomVec( geom_id_vec );

    for ( int i = 0 ; i < ( int )geom_vec.size() ; i++ )
    {
        geom_vec[i]->m_GuiDraw.SetDispSubSurfFlag( f );
    }
}

/// Show or Hide Feature Lines
void ManageGeomScreenPrivate::SetFeatureDrawFlag( bool f )
{
    vector<string> geom_id_vec = GetActiveGeoms();
    vector< Geom* > geom_vec = veh()->FindGeomVec( geom_id_vec );

    for ( int i = 0 ; i < ( int )geom_vec.size() ; i++ )
    {
        geom_vec[i]->m_GuiDraw.SetDispFeatureFlag( f );
    }
}

std::string ManageGeomScreen::getFeedbackGroupName()
{
    return std::string("GeomGUIGroup");
}

void ManageGeomScreen::Set( std::string geomId )
{
    Q_D( ManageGeomScreen );
    d->veh()->SetActiveGeom(geomId);

    d->ShowHideGeomScreens();
    d->SetUpdateFlag();
}

void ManageGeomScreen::TriggerPickSwitch()
{
    d_func()->Ui.pickGeomButton->toggle();
}

void ManageGeomScreen::LoadDrawObjs( vector< DrawObj* > & draw_obj_vec )
{
    Q_D( ManageGeomScreen );
    d->UpdateDrawObjs();

    for( int i = 0; i < ( int )d->PickList.size(); i++ )
    {
        draw_obj_vec.push_back( &d->PickList[i] );
    }
}

void ManageGeomScreenPrivate::UpdateDrawObjs()
{
    PickList.clear();
    if ( !Ui.pickGeomButton->isChecked() ) return;

    vector< Geom* > geom_vec = veh()->FindGeomVec( veh()->GetGeomVec( false ) );
    for( int i = 0; i < ( int )geom_vec.size(); i++ )
    {
        std::vector< DrawObj* > geom_drawobj_vec;
        geom_vec[i]->LoadDrawObjs( geom_drawobj_vec );

        for( int j = 0; j < ( int )geom_drawobj_vec.size(); j++ )
        {
            if( geom_drawobj_vec[j]->m_Visible )
            {
                // Ignore bounding boxes.
                if( geom_drawobj_vec[j]->m_GeomID.compare(0, string(BBOXHEADER).size(), BBOXHEADER) != 0 )
                {
                    DrawObj pickObj;
                    pickObj.m_Type = DrawObj::VSP_PICK_GEOM;
                    pickObj.m_GeomID = PICKGEOMHEADER + geom_drawobj_vec[j]->m_GeomID;
                    pickObj.m_PickSourceID = geom_drawobj_vec[j]->m_GeomID;
                    pickObj.m_FeedbackGroup = q_func()->getFeedbackGroupName();

                    PickList.push_back( pickObj );
                }
            }
        }
    }
}

void ManageGeomScreenPrivate::UpdateDrawType()
{
    vector<string> geom_id_vec = GetActiveGeoms();
    vector< Geom* > geom_vec = veh()->FindGeomVec( geom_id_vec );
    int num_geoms = (int)geom_vec.size();

    // Handle case where there are not any geoms selected.
    if ( num_geoms == 0 ) Ui.showSubToggle->setChecked( false );

    int num_sub_on = 0;
    int num_feature_on = 0;

    for ( int i = 0; i < (int)geom_vec.size(); i++ )
    {
        if ( geom_vec[i]->m_GuiDraw.GetDispSubSurfFlag() )
            num_sub_on++;

        if ( geom_vec[i]->m_GuiDraw.GetDispFeatureFlag() )
            num_feature_on++;
    }

    float flag_average = num_sub_on/(float)num_geoms;
    Ui.showSubToggle->setChecked( flag_average > 0.5 );

    flag_average = num_feature_on/(float)num_geoms;
    Ui.showFeatureToggle->setChecked( flag_average > 0.5 );
}

#include "ManageGeomScreen.moc"
