//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "ParmLinkScreen.h"
#include "ParmMgr.h"
#include "LinkMgr.h"
#include "ScreenMgr.h"
#include "GuiDeviceQt.h"
#include "StlHelper.h"
#include "ui_ParmLinkScreen.h"
#include "VspScreenQt_p.h"
#include "UiSignalBlocker.h"
#include <vector>
#include <string>

using std::vector;
using std::string;

static Link * CurrLink() { return LinkMgr.GetCurrLink(); }

class ParmLinkScreenPrivate : public QDialog, public VspScreenQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( ParmLinkScreen )
    Q_PRIVATE_SLOT( self(), void SetUpdateFlag() )
    Ui::ParmLinkScreen Ui;

    SliderInputQt OffsetSlider;
    SliderInputQt ScaleSlider;
    SliderInputQt LowerLimitSlider;
    SliderInputQt UpperLimitSlider;

    vector< string > FindParmNames( bool A_flag, vector< string > & parm_id_vec );

#if 0
    GroupLayoutQt* User1Group;
    GroupLayoutQt* User2Group;
    enum { NUUser_SLIDERS = 10, };
    SliderAdjRangeInput UserSlider[NUUser_SLIDERS];
#endif

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    ParmLinkScreenPrivate( ParmLinkScreen * );

    Q_SLOT void on_compAChoice_currentIndexChanged()
    {
        q_func()->CompGroupLinkChange();
    }
    Q_SLOT void on_groupAChoice_currentIndexChanged()
    {
        q_func()->CompGroupLinkChange();
    }
    Q_SLOT void on_parmAChoice_currentIndexChanged()
    {
        q_func()->CompGroupLinkChange();
    }
    Q_SLOT void on_compBChoice_currentIndexChanged()
    {
        q_func()->CompGroupLinkChange();
    }
    Q_SLOT void on_groupBChoice_currentIndexChanged()
    {
        q_func()->CompGroupLinkChange();
    }
    Q_SLOT void on_parmBChoice_currentIndexChanged()
    {
        q_func()->CompGroupLinkChange();
    }
    Q_SLOT void on_offsetButton_toggled( bool val )
    {
        CurrLink()->SetOffsetFlag( val );
        OffsetSlider.SetActivated( CurrLink()->GetOffsetFlag() );
        LinkMgr.ParmChanged( CurrLink()->GetParmA(), true );
    }
    Q_SLOT void on_scaleButton_toggled( bool val )
    {
        CurrLink()->SetScaleFlag( val );
        ScaleSlider.SetActivated( CurrLink()->GetScaleFlag() );
        LinkMgr.ParmChanged( CurrLink()->GetParmA(), true );
    }
    Q_SLOT void on_lowerLimitButton_toggled( bool val )
    {
        CurrLink()->SetLowerLimitFlag( val );
        LowerLimitSlider.SetActivated( val );
        LinkMgr.ParmChanged( CurrLink()->GetParmA(), true );
    }
    Q_SLOT void on_upperLimitButton_toggled( bool val )
    {
        CurrLink()->SetUpperLimitFlag( val );
        UpperLimitSlider.SetActivated( val );
        LinkMgr.ParmChanged( CurrLink()->GetParmA(), true );
    }
    Q_SLOT void on_addLinkButton_clicked()
    {
        bool success = LinkMgr.AddCurrLink();
        if ( !success ) GetScreenMgr()->Alert( "Error: Identical Parms or Already Linked" );
    }
    Q_SLOT void on_deleteLinkButton_clicked()
    {
        LinkMgr.DelCurrLink();
    }
    Q_SLOT void on_deleteAllLinksButton_clicked()
    {
        LinkMgr.DelAllLinks();
    }
    Q_SLOT void on_linkAllCompButton_clicked()
    {
        bool success = LinkMgr.LinkAllComp();
        if ( !success ) GetScreenMgr()->Alert( "Error: Identical Comps" );
    }
    Q_SLOT void on_linkAllGroupButton_clicked()
    {
        bool success = LinkMgr.LinkAllGroup();
        if ( !success ) GetScreenMgr()->Alert( "Error: Identical Group" );
    }
    Q_SLOT void on_tableWidget_currentCellChanged( int row, int )
    {
        LinkMgr.SetCurrLinkIndex( row );
    }
#if 0
    else if ( m_OffsetSlider->GuiChanged( w ) )
    {
      currLink->SetOffset( m_OffsetSlider->GetVal() );
      parmLinkMgrPtr->ParmChanged( currLink->GetParmA(), true );
    }
    else if ( m_ScaleSlider->GuiChanged( w ) )
    {
      currLink->SetScale( m_ScaleSlider->GetVal() );
      parmLinkMgrPtr->ParmChanged( currLink->GetParmA(), true );
    }
    else if ( m_LowerLimitSlider->GuiChanged( w ) )
    {
      currLink->SetLowerLimit( m_LowerLimitSlider->GetVal() );
      parmLinkMgrPtr->ParmChanged( currLink->GetParmA(), true );
    }
    else if ( m_UpperLimitSlider->GuiChanged( w ) )
    {
      currLink->SetUpperLimit( m_UpperLimitSlider->GetVal() );
      parmLinkMgrPtr->ParmChanged( currLink->GetParmA(), true );
    }
#endif
};
VSP_DEFINE_PRIVATE( ParmLinkScreen )

ParmLinkScreenPrivate::ParmLinkScreenPrivate( ParmLinkScreen * q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );


    OffsetSlider.Init( q, Ui.offsetSlider, Ui.offsetInput, 100.0, 3 );
    ScaleSlider.Init( q, Ui.scaleSlider, Ui.scaleInput, 1.0, 5 );
    LowerLimitSlider.Init( q, Ui.lowerLimitSlider, Ui.lowerLimitInput, 10.0, 1 );
    UpperLimitSlider.Init( q, Ui.upperLimitSlider, Ui.upperLimitInput, 10.0, 1 );

    ////==== User Parms ====//
#if 0
    User1Group = new GroupLayoutQt( q, Ui.userParmGroup1 );
    User2Group = new GroupLayoutQt( q, Ui.userParmGroup2 );

    User1Group->AddSlider( UserSlider[0], "User_0", 1, "%7.3f" );
    User1Group->AddSlider( UserSlider[1], "User_1", 1, "%7.3f" );
    User1Group->AddSlider( UserSlider[2], "User_2", 1, "%7.3f" );
    User1Group->AddSlider( UserSlider[3], "User_3", 1, "%7.3f" );
    User1Group->AddSlider( UserSlider[4], "User_4", 1, "%7.3f" );

    User2Group->AddSlider( UserSlider[5], "User_5", 1, "%7.3f" );
    User2Group->AddSlider( UserSlider[6], "User_6", 1, "%7.3f" );
    User2Group->AddSlider( UserSlider[7], "User_7", 1, "%7.3f" );
    User2Group->AddSlider( UserSlider[8], "User_8", 1, "%7.3f" );
    User2Group->AddSlider( UserSlider[9], "User_9", 1, "%7.3f" );
#endif

    BlockSignalsInUpdates();
    ConnectUpdateFlag();
}

ParmLinkScreen::ParmLinkScreen( ScreenMgr* mgr ) :
    VspScreenQt( *new ParmLinkScreenPrivate( this ), mgr )
{
}

void ParmLinkScreen::Show()
{
    Q_D( ParmLinkScreen );
    //Show( aircraftPtr->getUserGeom() );
    d->SetUpdateFlag();
    d->show();
}

#if 0
void ParmLinkScreen::Show( Geom* geomPtr )
{
    Q_D( ParmLinkScreen )
    //==== Check For Duplicate Comp Names ====//

    UserGeom* currGeom = (UserGeom*)geomPtr;

    d->User1Slider->set_parm_ptr( &currGeom->userParm1 );
    d->User1Input->set_parm_ptr(  &currGeom->userParm1 );
    d->User2Slider->set_parm_ptr( &currGeom->userParm2 );
    d->User2Input->set_parm_ptr(  &currGeom->userParm2 );
    d->User3Slider->set_parm_ptr( &currGeom->userParm3 );
    d->User3Input->set_parm_ptr(  &currGeom->userParm3 );
    d->User4Slider->set_parm_ptr( &currGeom->userParm4 );
    d->User4Input->set_parm_ptr(  &currGeom->userParm4 );
    d->User5Slider->set_parm_ptr( &currGeom->userParm5 );
    d->User5Input->set_parm_ptr(  &currGeom->userParm5 );
    d->User6Slider->set_parm_ptr( &currGeom->userParm6 );
    d->User6Input->set_parm_ptr(  &currGeom->userParm6 );
    d->User7Slider->set_parm_ptr( &currGeom->userParm7 );
    d->User7Input->set_parm_ptr(  &currGeom->userParm7 );
    d->User8Slider->set_parm_ptr( &currGeom->userParm8 );
    d->User8Input->set_parm_ptr(  &currGeom->userParm8 );

    d->User1Button->set_parm_ptr( &currGeom->userParm1 );
    d->User2Button->set_parm_ptr( &currGeom->userParm2 );
    d->User3Button->set_parm_ptr( &currGeom->userParm3 );
    d->User4Button->set_parm_ptr( &currGeom->userParm4 );
    d->User5Button->set_parm_ptr( &currGeom->userParm5 );
    d->User6Button->set_parm_ptr( &currGeom->userParm6 );
    d->User7Button->set_parm_ptr( &currGeom->userParm7 );
    d->User8Button->set_parm_ptr( &currGeom->userParm8 );

    parmMgrPtr->LoadAllParms();
    update();
    d->Ui.show();
}
#endif

#if 0
void ParmLinkScreen::RemoveAllRefs( GeomBase* gPtr )
{
    Q_D( ParmLinkScreen );
    vector< ParmButton* > tempVec;

    for ( int i = 0 ; i < (int)d->ParmButtonVec.size() ; i++ )
    {
      Parm* p =  d->ParmButtonVec[i]->get_parm_ptr();
      if ( p && p->get_geom_base() != gPtr )
          tempVec.push_back( d->ParmButtonVec[i] );
    }
    d->ParmButtonVec = tempVec;
}
#endif

static QTableWidgetItem * textItem( const char * text )
{
    auto item = new QTableWidgetItem( text );
    item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    return item;
}

bool ParmLinkScreenPrivate::Update()
{
    const char rArrow[] = "\xe2\x9e\x9e";
    bool flag;
    LinkMgr.CheckLinks();
    LinkMgr.BuildLinkableParmData();
    Link* currLink = LinkMgr.GetCurrLink();

    //==== Geom Names A ====//
    Ui.compAChoice->clear();
    vector< string > containVecA;
    int indA = LinkMgr.GetCurrContainerVec( currLink->GetParmA(), containVecA );
    for ( int i = 0 ; i < ( int )containVecA.size() ; i++ )
    {
        auto str = QString( "%1-%2" ).arg( i ).arg( containVecA[i].c_str() );
        Ui.compAChoice->addItem( str );
    }
    Ui.compAChoice->setCurrentIndex( indA );

    //==== Group Names A ====//
    Ui.groupAChoice->clear();
    vector< string > groupNameVecA;
    indA = LinkMgr.GetCurrGroupNameVec( currLink->GetParmA(), groupNameVecA );
    const_foreach( auto & group, groupNameVecA ) {
        Ui.groupAChoice->addItem( group.c_str() );
    }
    Ui.groupAChoice->setCurrentIndex( indA );

    //==== Parm Names A =====//
    Ui.parmAChoice->clear();
    vector< string > parmIDVecA;
    indA = LinkMgr.GetCurrParmIDVec( currLink->GetParmA(), parmIDVecA );
    vector< string > parmNameVecA = FindParmNames( true, parmIDVecA );
    const_foreach( auto & parmName, parmNameVecA )
    {
        Ui.parmAChoice->addItem( parmName.c_str() );
    }
    Ui.parmAChoice->setCurrentIndex( indA );

    //==== Geom Names B ====//
    Ui.compBChoice->clear();
    vector< string > containVecB;
    int indB = LinkMgr.GetCurrContainerVec( currLink->GetParmB(), containVecB );
    for ( int i = 0 ; i < ( int )containVecB.size() ; i++ )
    {
        auto str = QString("%1-%2").arg( i ).arg( containVecB[i].c_str() );
        Ui.compBChoice->addItem( str );
    }
    Ui.compBChoice->setCurrentIndex( indB );

    //==== Group Names B ====//
    Ui.groupBChoice->clear();
    vector< string > groupNameVecB;
    indB = LinkMgr.GetCurrGroupNameVec( currLink->GetParmB(), groupNameVecB );
    const_foreach( auto & groupName, groupNameVecB )
    {
        Ui.groupBChoice->addItem( groupName.c_str() );
    }
    Ui.groupBChoice->setCurrentIndex( indB );

    //==== Parm Names B =====//
    Ui.parmBChoice->clear();
    vector< string > parmIDVecB;
    indB = LinkMgr.GetCurrParmIDVec( currLink->GetParmB(), parmIDVecB );
    vector< string > parmNameVecB = FindParmNames( false, parmIDVecB );
    const_foreach( auto & parmName, parmNameVecB )
    {
        Ui.parmBChoice->addItem( parmName.c_str() );
    }
    Ui.parmBChoice->setCurrentIndex( indB );

    //===== Update Offset ====//
    OffsetSlider.Update( currLink->m_Offset.GetID() );
    flag = currLink->GetOffsetFlag();
    Ui.offsetButton->setChecked( flag );
    OffsetSlider.SetActivated( flag );

    //===== Update Scale ====//
    ScaleSlider.Update( currLink->m_Scale.GetID() );
    flag = currLink->GetScaleFlag();
    Ui.scaleButton->setChecked( flag );
    ScaleSlider.SetActivated( flag );

    //===== Update Lower Limit ====//
    LowerLimitSlider.Update ( currLink->m_LowerLimit.GetID() );
    flag = currLink->GetLowerLimitFlag();
    Ui.lowerLimitButton->setChecked( flag );
    LowerLimitSlider.SetActivated( flag );

    //===== Update Upper Limit ====//
    UpperLimitSlider.Update( currLink->m_UpperLimit.GetID() );
    flag = currLink->GetUpperLimitFlag();
    Ui.upperLimitButton->setChecked( flag );
    UpperLimitSlider.SetActivated( flag );

    //==== Update Link Browser ====//
    Ui.tableWidget->clear();
    Ui.tableWidget->setColumnCount( 7 );

    static int widths[] = { 75, 75, 90, 20, 75, 75, 80, 0 }; // widths for each column
    int col = 0;
    for ( int * width = widths; *width; width++ )
    {
        Ui.tableWidget->setColumnWidth( col++, *width );
    }

    Ui.tableWidget->setHorizontalHeaderLabels(
                QStringList() << "COMP_A" << "GROUP" << "PARM"
                << rArrow << "COMP_B" << "GROUP" << "PARM"
                );

    int num_links = LinkMgr.GetNumLinks();
    Ui.tableWidget->setRowCount( num_links );
    for ( int i = 0 ; i < num_links ; i++ )
    {
        Link* pl = LinkMgr.GetLink( i );

        string c_name_A, g_name_A, p_name_A;
        string c_name_B, g_name_B, p_name_B;
        ParmMgr.GetNames( pl->GetParmA(), c_name_A, g_name_A, p_name_A );
        ParmMgr.GetNames( pl->GetParmB(), c_name_B, g_name_B, p_name_B );

        Ui.tableWidget->setItem( i, 0, textItem( c_name_A.c_str() ) );
        Ui.tableWidget->setItem( i, 1, textItem( g_name_A.c_str() ) );
        Ui.tableWidget->setItem( i, 2, textItem( p_name_A.c_str() ) );
        Ui.tableWidget->setItem( i, 3, textItem( rArrow ) );
        Ui.tableWidget->setItem( i, 4, textItem( c_name_B.c_str() ) );
        Ui.tableWidget->setItem( i, 5, textItem( g_name_B.c_str() ) );
        Ui.tableWidget->setItem( i, 6, textItem( p_name_B.c_str() ) );
    }
    Ui.tableWidget->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );

    int index = LinkMgr.GetCurrLinkIndex();
    if ( index >= 0 && index < num_links )
    {
        Ui.tableWidget->setCurrentCell( index, 0 );
    }

#if 0
    for ( int i = 0 ; i < NUUser_SLIDERS ; i++ )
    {
        UserSlider[i].Update( LinkMgr.GetUserParmId( i ) );
    }
#endif

    return false;
}

#if 0
void ParmLinkScreen::ClearButtonParms()
{
    Q_D( ParmLinkScreen );
    for ( int i = 0 ; i < (int)d->ParmButtonVec.size() ; i++ )
    {
        d->ParmButtonVec[i]->set_parm_ptr( 0 );
    }

}
#endif

#if 0
void ParmLinkScreen::SetTitle( const char* name )
{
    Q_D( ParmLinkScreen );
    QString title = "PARMLINK : ";
    title.concatenate( name );
    d->Ui.screenHeader->setText( title );
}
#endif

void ParmLinkScreen::CompGroupLinkChange()
{
    Q_D( ParmLinkScreen );
    UiSignalBlocker blocker( d );
    LinkMgr.SetCurrLinkIndex( -1 );

    LinkMgr.SetParm( true, d->Ui.compAChoice->currentIndex(),
                     d->Ui.groupAChoice->currentIndex(), d->Ui.parmAChoice->currentIndex() );
    LinkMgr.SetParm( false, d->Ui.compBChoice->currentIndex(),
                     d->Ui.groupBChoice->currentIndex(), d->Ui.parmBChoice->currentIndex() );

#if 0
    parmLinkMgrPtr->SetParm( true,
      ui->compAChoice->value(), ui->groupAChoice->value(), ui->parmAChoice->value() );
    parmLinkMgrPtr->SetParm( false,
      ui->compBChoice->value(), ui->groupBChoice->value(), ui->parmBChoice->value() );
    Update();
#endif
}


vector< string > ParmLinkScreenPrivate::FindParmNames( bool A_flag, vector< string > & parm_id_vec )
{
    vector< string > name_vec;
    for ( int i = 0 ; i < ( int )parm_id_vec.size() ; i++ )
    {
        string name;

        Parm* p = ParmMgr.FindParm( parm_id_vec[i] );
        if ( p )
        {
            name = p->GetName();
        }

        name_vec.push_back( name );
    }


    return name_vec;
}

ParmLinkScreen::~ParmLinkScreen() {}

#include "ParmLinkScreen.moc"
