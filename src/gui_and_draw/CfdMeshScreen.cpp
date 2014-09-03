//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// geomScreen.cpp: implementation of the geomScreen class.
//
//////////////////////////////////////////////////////////////////////

#include "CfdMeshScreen.h"
#include "GridDensity.h"
#include "CfdMeshMgr.h"
#include "StreamUtil.h"
#include "ScreenBase.h"
#include "GuiDeviceQt.h"
#include "Vehicle.h"
#include "ScreenMgr.h"
#include <FL/Fl_File_Chooser.H>
#include "VspScreenQt_p.h"
#include "ui_CfdMeshScreen.h"


class CfdMeshScreenPrivate : public QDialog, public VspScreenQtPrivate {
    Q_OBJECT
    Q_DECLARE_PUBLIC( CfdMeshScreen )
    Q_PRIVATE_SLOT( self(), void SetUpdateFlag() )
    Ui::CfdMeshScreen Ui;

    ToggleButtonQt DrawMeshButton;
    ToggleButtonQt DrawSourceButton;
    ToggleButtonQt DrawFarButton;
    ToggleButtonQt DrawFarPreButton;
    ToggleButtonQt DrawBadButton;
    ToggleButtonQt DrawSymmButton;
    ToggleButtonQt DrawWakeButton;
    ToggleButtonQt DrawTagsButton;

    ToggleButtonQt DatToggleButton;
    ToggleButtonQt KeyToggleButton;
    ToggleButtonQt ObjToggleButton;
    ToggleButtonQt PolyToggleButton;
    ToggleButtonQt StlToggleButton;
    ToggleButtonQt TriToggleButton;
    ToggleButtonQt GmshToggleButton;
    ToggleButtonQt SrfToggleButton;
    ToggleButtonQt TkeyToggleButton;

    ToggleButtonQt IntersectSubSurfsButton;

    SliderInputQt LengthSlider;
    SliderInputQt RadiusSlider;

    SliderInputQt Length2Slider;
    SliderInputQt Radius2Slider;

    SliderInputQt U1Slider;
    SliderInputQt W1Slider;

    SliderInputQt U2Slider;
    SliderInputQt W2Slider;

    SliderInputQt BodyEdgeSizeSlider;
    SliderInputQt MinEdgeSizeSlider;
    SliderInputQt MaxGapSizeSlider;
    SliderInputQt NumCircSegmentSlider;
    SliderInputQt GrowRatioSlider;

    SliderInputQt FarXScaleSlider;
    SliderInputQt FarYScaleSlider;
    SliderInputQt FarZScaleSlider;

#if 0
    FractParmSlider FarXScaleSlider;
    FractParmSlider FarYScaleSlider;
    FractParmSlider FarZScaleSlider;
#endif

    SliderInputQt FarXLocationSlider;
    SliderInputQt FarYLocationSlider;
    SliderInputQt FarZLocationSlider;

    SliderInputQt FarEdgeLengthSlider;
    SliderInputQt FarGapSizeSlider;
    SliderInputQt FarCircSegmentSlider;

    SliderInputQt WakeScaleSlider;
    SliderInputQt WakeAngleSlider;

    map< string, int > CompIDMap;
    vector< string > GeomVec;

    Vehicle* Vehicle;

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    std::string truncateFileName( const std::string &fn, int len );
    explicit CfdMeshScreenPrivate( CfdMeshScreen * );

    Q_SLOT void on_rigorLimitButton_toggled( bool val )
    {
        CfdMeshMgr.GetGridDensityPtr()->SetRigorLimit( val );
    }
    Q_SLOT void on_farMeshButton_toggled( bool val )
    {
        CfdMeshMgr.GetCfdSettingsPtr()->SetFarMeshFlag( val );
    }
    Q_SLOT void on_halfMeshButton_toggled( bool val )
    {
        CfdMeshMgr.GetCfdSettingsPtr()->SetHalfMeshFlag( val );
    }
    Q_SLOT void on_finalMeshButton_clicked()
    {
        redirecter redir( std::cout, CfdMeshMgr.m_OutStream );
        CfdMeshMgr.GenerateMesh(); /// \todo This blocks, ugh.

        // Hide all geoms.
        vector<string> geomIds = veh()->GetGeomVec();
        for( int i = 0; i < (int)geomIds.size(); i++ )
        {
            GeomBase* gPtr = veh()->FindGeom( geomIds[i] );
            if ( gPtr )
            {
                gPtr->m_GuiDraw.SetNoShowFlag( true );
            }
        }
    }
    Q_SLOT void on_compChoice_currentIndexChanged( int id )
    {
        CfdMeshMgr.SetCurrGeomID( GeomVec[ id ] );
        CfdMeshMgr.SetCurrMainSurfIndx( 0 );
    }
    Q_SLOT void on_surfChoice_currentIndexChanged( int id )
    {
        CfdMeshMgr.SetCurrMainSurfIndx( id );
    }
    Q_SLOT void on_wakeCompChoice_currentIndexChanged( int id )
    {
        CfdMeshMgr.SetCurrGeomID( GeomVec[ id ] );
    }
    Q_SLOT void on_farCompChoice_currentIndexChanged( int id )
    {
        CfdMeshMgr.SetFarGeomID( GeomVec[ id ] );
    }
    Q_SLOT void on_sourceBrowser_currentRowChanged( int id )
    {
        CfdMeshMgr.GUI_Val( "SourceID", id );
    }
    Q_SLOT void on_setChoice_currentIndexChanged( int id )
    {
        CfdMeshMgr.GetCfdSettingsPtr()->m_SelectedSetIndex = id;
    }
    Q_SLOT void on_addSourceButton_clicked()
    {
        int type = Ui.sourceTypeChoice->currentIndex();
        if ( type >= 0 && type < NUM_SOURCE_TYPES )
        {
            CfdMeshMgr.AddSource( type );
        }
    }
    Q_SLOT void on_deleteSourceButton_clicked()
    {
        CfdMeshMgr.DeleteCurrSource();
    }
    Q_SLOT void on_adjLenDownButton_clicked()
    {
        CfdMeshMgr.AdjustAllSourceLen( 1.0 / 1.1 );
    }
    Q_SLOT void on_adjLenUpButton_clicked()
    {
        CfdMeshMgr.AdjustAllSourceLen( 1.1 );
    }
    Q_SLOT void on_adjLenDownDownButton_clicked()
    {
        CfdMeshMgr.AdjustAllSourceLen( 1.0 / 1.5 );
    }
    Q_SLOT void on_adjLenUpUpButton_clicked()
    {
        CfdMeshMgr.AdjustAllSourceLen( 1.5 );
    }
    Q_SLOT void on_adjRadDownButton_clicked()
    {
        CfdMeshMgr.AdjustAllSourceRad( 1.0 / 1.1 );
    }
    Q_SLOT void on_adjRadUpButton_clicked()
    {
        CfdMeshMgr.AdjustAllSourceRad( 1.1 );
    }
    Q_SLOT void on_adjRadDownDownButton_clicked()
    {
        CfdMeshMgr.AdjustAllSourceRad( 1.0 / 1.5 );
    }
    Q_SLOT void on_adjRadUpUpButton_clicked()
    {
        CfdMeshMgr.AdjustAllSourceRad( 1.5 );
    }
    Q_SLOT void on_datButton_clicked()
    {
        string newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select NASCART .dat file.", "*.dat" );
        if ( !newfile.empty() )
        {
            CfdMeshMgr.GetCfdSettingsPtr()->SetExportFileName( newfile, CfdMeshSettings::DAT_FILE_NAME );
        }
    }
    Q_SLOT void on_keyButton_clicked()
    {
        string newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select NASCART .key file.", "*.key" );
        if ( !newfile.empty() )
        {
            CfdMeshMgr.GetCfdSettingsPtr()->SetExportFileName( newfile, CfdMeshSettings::KEY_FILE_NAME );
        }
    }
    Q_SLOT void on_objButton_clicked()
    {
        string newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select .obj file.", "*.obj" );
        if ( !newfile.empty() )
        {
            CfdMeshMgr.GetCfdSettingsPtr()->SetExportFileName( newfile, CfdMeshSettings::OBJ_FILE_NAME );
        }
    }
    Q_SLOT void on_polyButton_clicked()
    {
        string newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select .poly file.", "*.poly" );
        if ( !newfile.empty() )
        {
            CfdMeshMgr.GetCfdSettingsPtr()->SetExportFileName( newfile, CfdMeshSettings::POLY_FILE_NAME );
        }
    }
    Q_SLOT void on_stlButton_clicked()
    {
        string newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select .stl file.", "*.stl" );
        if ( !newfile.empty() )
        {
            CfdMeshMgr.GetCfdSettingsPtr()->SetExportFileName( newfile, CfdMeshSettings::STL_FILE_NAME );
        }
    }
    Q_SLOT void on_triButton_clicked()
    {
        string newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select .tri file.", "*.tri" );
        if ( !newfile.empty() )
        {
            CfdMeshMgr.GetCfdSettingsPtr()->SetExportFileName( newfile, CfdMeshSettings::TRI_FILE_NAME );
        }
    }
    Q_SLOT void on_gmshButton_clicked()
    {
        string newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select .msh file.", "*.msh" );
        if ( !newfile.empty() )
        {
            CfdMeshMgr.GetCfdSettingsPtr()->SetExportFileName( newfile, CfdMeshSettings::GMSH_FILE_NAME );
        }
    }
    Q_SLOT void on_srfButton_clicked()
    {
        string newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select .srf file.", "*.srf" );
        if ( !newfile.empty() )
        {
            CfdMeshMgr.GetCfdSettingsPtr()->SetExportFileName( newfile, CfdMeshSettings::SRF_FILE_NAME );
        }
    }
    Q_SLOT void on_tkeyButton_clicked()
    {
        string newfile = GetScreenMgr()->GetSelectFileScreen()->FileSave( "Select .tkey file.", "*.tkey" );
        if ( !newfile.empty() )
        {
            CfdMeshMgr.GetCfdSettingsPtr()->SetExportFileName( newfile, CfdMeshSettings::TKEY_FILE_NAME );
        }
    }
    Q_SLOT void on_addWakeButton_clicked()
    {
        bool flag = Ui.addWakeButton->isChecked();
        vector<string> geomVec = veh()->GetGeomVec();
        string currGeomID = CfdMeshMgr.GetCurrGeomID();
        Geom* g = veh()->FindGeom( currGeomID );
        if ( g )
        {
            g->SetWakeActiveFlag( flag );
        }
    }
    Q_SLOT void on_farComponentGenButton_toggled( bool val )
    {
        CfdMeshMgr.GetCfdSettingsPtr()->SetFarCompFlag( val );
    }
    Q_SLOT void on_farManLocButton_toggled( bool val )
    {
        CfdMeshMgr.GetCfdSettingsPtr()->SetFarManLocFlag( val );
    }
    Q_SLOT void on_farAbsSizeButton_toggled( bool val )
    {
        CfdMeshMgr.GetCfdSettingsPtr()->SetFarAbsSizeFlag( val );
    }
};
VSP_DEFINE_PRIVATE( CfdMeshScreen )

CfdMeshScreenPrivate::CfdMeshScreenPrivate( CfdMeshScreen* q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );

    IntersectSubSurfsButton.Init( q, Ui.intersectSubButton );
    DrawMeshButton.Init( q, Ui.viewMeshButton );
    DrawSourceButton.Init( q, Ui.viewSourceButton );
    DrawFarButton.Init( q, Ui.viewFarMeshButton );
    DrawFarPreButton.Init( q, Ui.viewFarPreButton );
    DrawBadButton.Init( q, Ui.viewBadButton );
    DrawSymmButton.Init( q, Ui.viewSymmButton );
    DrawWakeButton.Init( q, Ui.viewWakeButton );
    DrawTagsButton.Init( q, Ui.viewTags );

    BodyEdgeSizeSlider.Init( q, Ui.bodyEdgeSizeSlider, Ui.bodyEdgeSizeInput, 1.0, 5 );
    MinEdgeSizeSlider.Init( q, Ui.minEdgeSizeSlider, Ui.minEdgeSizeInput, 1.0, 5);
    MaxGapSizeSlider.Init( q, Ui.maxGapSizeSlider, Ui.maxGapSizeInput, 1.0, 5 );
    NumCircSegmentSlider.Init( q, Ui.numCircSegmentSlider, Ui.numCircSegmentInput, 100.0, 5 );
    GrowRatioSlider.Init( q, Ui.growRatioSlider, Ui.growRatioInput, 2.0, 5 );

    FarEdgeLengthSlider.Init( q, Ui.farEdgeSizeSlider, Ui.farEdgeSizeInput, 1.0, 5 );
    FarGapSizeSlider.Init( q, Ui.farGapSizeSlider, Ui.farGapSizeInput, 1.0, 5 );
    FarCircSegmentSlider.Init( q, Ui.farCircSegmentSlider, Ui.farCircSegmentInput, 100.0, 5 );

    FarXScaleSlider.Init( q, Ui.farXScaleSlider, Ui.farXScaleInput, 10.0, 5 );
    FarYScaleSlider.Init( q, Ui.farYScaleSlider, Ui.farYScaleInput, 10.0, 5 );
    FarZScaleSlider.Init( q, Ui.farZScaleSlider, Ui.farZScaleInput, 10.0, 5 );

    FarXLocationSlider.Init( q, Ui.farXLocSlider, Ui.farXLocInput, 5.0, 5 );
    FarYLocationSlider.Init( q, Ui.farYLocSlider, Ui.farYLocInput, 5.0, 5 );
    FarZLocationSlider.Init( q, Ui.farZLocSlider, Ui.farZLocInput, 5.0, 5 );

    WakeScaleSlider.Init( q, Ui.wakeScaleSlider, Ui.wakeScaleInput, 10.0, 5 );
    WakeAngleSlider.Init( q, Ui.wakeAngleSlider, Ui.wakeAngleInput, 10.0, 5 );

    LengthSlider.Init( q, Ui.lengthSlider, Ui.lengthInput, 1.0, 5 );
    RadiusSlider.Init( q, Ui.radiusSlider, Ui.radiusInput, 1.0, 5 );

    Length2Slider.Init( q, Ui.length2Slider, Ui.length2Input, 1.0, 5 );
    Radius2Slider.Init( q, Ui.radius2Slider, Ui.radius2Input, 1.0, 5 );

    U1Slider.Init( q, Ui.u1Slider, Ui.u1Input, 1.0, 5 );
    W1Slider.Init( q, Ui.w1Slider, Ui.w1Input, 1.0, 5 );

    U2Slider.Init( q, Ui.u2Slider, Ui.u2Input, 1.0, 5 );
    W2Slider.Init( q, Ui.w2Slider, Ui.w2Input, 1.0, 5 );

    DatToggleButton.Init( q, Ui.datToggle );
    KeyToggleButton.Init( q, Ui.keyToggle );
    ObjToggleButton.Init( q, Ui.objToggle );
    PolyToggleButton.Init( q, Ui.polyToggle );
    StlToggleButton.Init( q, Ui.stlToggle );
    TriToggleButton.Init( q, Ui.triToggle );
    GmshToggleButton.Init( q, Ui.gmshToggle );
    SrfToggleButton.Init( q, Ui.srfToggle );
    TkeyToggleButton.Init( q, Ui.tkeyToggle );
    BlockSignalsInUpdates();
    ConnectUpdateFlag();
    disconnect( Ui.sourceTypeChoice, 0, 0, 0 );
}

CfdMeshScreen::CfdMeshScreen( ScreenMgr* mgr ) :
    VspScreenQt( *new CfdMeshScreenPrivate(this), mgr )
{
}

void CfdMeshScreen::Show()
{
    d_func()->SetUpdateFlag();
    VspScreenQt::Show();
}

void CfdMeshScreen::Hide()
{
    VspScreenQt::Hide();
    d_func()->SetUpdateFlag();
}

bool CfdMeshScreenPrivate::Update()
{
    LoadSetChoice( Ui.setChoice, CfdMeshMgr.GetCfdSettingsPtr()->m_SelectedSetIndex() );

    CfdMeshMgr.UpdateSourcesAndWakes();
    CfdMeshMgr.UpdateDomain();

    //==== Base Len ====//

    BodyEdgeSizeSlider.Update( CfdMeshMgr.GetGridDensityPtr()->m_BaseLen.GetID() );
    MinEdgeSizeSlider.Update( CfdMeshMgr.GetGridDensityPtr()->m_MinLen.GetID() );
    MaxGapSizeSlider.Update( CfdMeshMgr.GetGridDensityPtr()->m_MaxGap.GetID() );
    NumCircSegmentSlider.Update( CfdMeshMgr.GetGridDensityPtr()->m_NCircSeg.GetID() );
    GrowRatioSlider.Update( CfdMeshMgr.GetGridDensityPtr()->m_GrowRatio.GetID() );
    IntersectSubSurfsButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_IntersectSubSurfs.GetID() );

    FarXScaleSlider.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_FarXScale.GetID() );
    FarYScaleSlider.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_FarYScale.GetID() );
    FarZScaleSlider.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_FarZScale.GetID() );


//  char xstr[255];
//  char ystr[255];
//  char zstr[255];
//  sprintf( xstr, "%0.4f", CfdMeshMgr.GetFarLength() );
//  sprintf( ystr, "%0.4f", CfdMeshMgr.GetFarWidth() );
//  sprintf( zstr, "%0.4f", CfdMeshMgr.GetFarHeight() );
//  m_CfdMeshUi.farXScaleAbsInput->value(xstr);
//  m_CfdMeshUi.farYScaleAbsInput->value(ystr);
//  m_CfdMeshUi.farZScaleAbsInput->value(zstr);


    FarXLocationSlider.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_FarXLocation.GetID() );
    FarYLocationSlider.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_FarYLocation.GetID() );
    FarZLocationSlider.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_FarZLocation.GetID() );

    FarEdgeLengthSlider.Update( CfdMeshMgr.GetGridDensityPtr()->m_FarMaxLen.GetID() );
    FarGapSizeSlider.Update( CfdMeshMgr.GetGridDensityPtr()->m_FarMaxGap.GetID() );
    FarCircSegmentSlider.Update( CfdMeshMgr.GetGridDensityPtr()->m_FarNCircSeg.GetID() );

    WakeScaleSlider.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_WakeScale.GetID() );
    WakeAngleSlider.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_WakeAngle.GetID() );

    //==== Load Geom Choice ====//
    GeomVec = veh()->GetGeomVec();

    Ui.compChoice->clear();
    Ui.surfChoice->clear();
    Ui.wakeCompChoice->clear();
    Ui.farCompChoice->clear();
    CompIDMap.clear();

    for ( int i = 0 ; i < ( int )GeomVec.size() ; i++ )
    {
        Geom* g = veh()->FindGeom( GeomVec[i] );
        if ( g )
        {
            auto str = QString("%1_%2").arg( i ).arg( g->GetName().c_str() );
            Ui.compChoice->addItem( str );
            if( g->HasWingTypeSurfs() )
            {
                Ui.wakeCompChoice->addItem( str );
            }
            Ui.farCompChoice->addItem( str );
            CompIDMap[ GeomVec[i] ] = i;
        }
    }

    string currGeomID = CfdMeshMgr.GetCurrGeomID();

    if( currGeomID.length() == 0 && GeomVec.size() > 0 )
    {
        // Handle case default case.
        currGeomID = GeomVec[0];
        CfdMeshMgr.SetCurrGeomID( currGeomID );
    }

    Geom* currGeom = veh()->FindGeom( currGeomID );

    Ui.compChoice->setCurrentIndex( CompIDMap[ currGeomID ] );
    Ui.wakeCompChoice->setCurrentIndex( CompIDMap[ currGeomID ] );

    string farGeomID = CfdMeshMgr.GetFarGeomID();
    Ui.farCompChoice->setCurrentIndex( CompIDMap[ farGeomID ] );

    BaseSource* source = CfdMeshMgr.GetCurrSource();

    if ( source )
    {
        LengthSlider.Activate();
        RadiusSlider.Activate();
        Ui.SourceNameInput->setEnabled( true );

        LengthSlider.Update( source->m_Len.GetID() );
        RadiusSlider.Update( source->m_Rad.GetID() );

        Ui.SourceNameInput->setText( source->GetName().c_str() );

        if ( source->GetType() == POINT_SOURCE )
        {
            U1Slider.Activate();
            W1Slider.Activate();

            PointSource* ps = ( PointSource* )source;

            U1Slider.Update( ps->m_ULoc.GetID() );
            W1Slider.Update( ps->m_WLoc.GetID() );

            Ui.sectionHeader_EditSourceTitle->setText( "Edit Point Source" );

            Length2Slider.Deactivate();
            Radius2Slider.Deactivate();
            U2Slider.Deactivate();
            W2Slider.Deactivate();
        }
        else if ( source->GetType() == LINE_SOURCE )
        {
            Length2Slider.Activate();
            Radius2Slider.Activate();
            U1Slider.Activate();
            W1Slider.Activate();
            U2Slider.Activate();
            W2Slider.Activate();

            LineSource* ps = ( LineSource* )source;

            U1Slider.Update( ps->m_ULoc1.GetID() );
            W1Slider.Update( ps->m_WLoc1.GetID() );

            U2Slider.Update( ps->m_ULoc2.GetID() );
            W2Slider.Update( ps->m_WLoc2.GetID() );

            Length2Slider.Update( ps->m_Len2.GetID() );
            Radius2Slider.Update( ps->m_Rad2.GetID() );

            Ui.sectionHeader_EditSourceTitle->setText( "Edit Line Source" );
        }
        else if ( source->GetType() == BOX_SOURCE )
        {
            U1Slider.Activate();
            W1Slider.Activate();
            U2Slider.Activate();
            W2Slider.Activate();

            BoxSource* ps = ( BoxSource* )source;

            U1Slider.Update( ps->m_ULoc1.GetID() );
            W1Slider.Update( ps->m_WLoc1.GetID() );

            U2Slider.Update( ps->m_ULoc2.GetID() );
            W2Slider.Update( ps->m_WLoc2.GetID() );

            Ui.sectionHeader_EditSourceTitle->setText( "Edit Box Source" );

            Length2Slider.Deactivate();
            Radius2Slider.Deactivate();
        }
    }
    else
    {
        LengthSlider.Deactivate();
        RadiusSlider.Deactivate();
        Length2Slider.Deactivate();
        Radius2Slider.Deactivate();
        U1Slider.Deactivate();
        W1Slider.Deactivate();
        U2Slider.Deactivate();
        W2Slider.Deactivate();
        Ui.SourceNameInput->setEnabled( false );
        Ui.sectionHeader_EditSourceTitle->setText( "" );
    }

    //==== Load Up Source Browser ====//
    int currSourceID = -1;

    Ui.sourceBrowser->clear();

    if( currGeom )
    {
        vector< BaseSource* > sVec = currGeom->GetCfdMeshMainSourceVec();
        for ( int i = 0 ; i < ( int )sVec.size() ; i++ )
        {
            if ( source == sVec[i] )
            {
                currSourceID = i;
            }
            Ui.sourceBrowser->addItem( sVec[i]->GetName().c_str() );
        }
        if ( currSourceID >= 0 && currSourceID < ( int )sVec.size() )
        {
            Ui.sourceBrowser->item( currSourceID )->setSelected( true );
        }

        int nmain = currGeom->GetNumMainSurfs();
        for ( int i = 0; i < nmain; i++ )
        {
            Ui.surfChoice->addItem( QString( "Surf_%1" ).arg( i ) );
        }
        int currMainSurfID = CfdMeshMgr.GetCurrMainSurfIndx();
        if( currMainSurfID >= 0 && currMainSurfID < nmain )
        {
            Ui.surfChoice->setCurrentIndex( currMainSurfID );
        }
    }

    DrawMeshButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_DrawMeshFlag.GetID() );
    DrawSourceButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_DrawSourceFlag.GetID() );
    DrawFarButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_DrawFarFlag.GetID() );
    DrawFarPreButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_DrawFarPreFlag.GetID() );
    DrawBadButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_DrawBadFlag.GetID() );
    DrawSymmButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_DrawSymmFlag.GetID() );
    DrawWakeButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_DrawWakeFlag.GetID() );
    DrawTagsButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->m_ColorTagsFlag.GetID() );

    Ui.halfMeshButton->setChecked( CfdMeshMgr.GetCfdSettingsPtr()->GetHalfMeshFlag() );
    Ui.rigorLimitButton->setChecked( CfdMeshMgr.GetGridDensityPtr()->GetRigorLimit() );

    string datname = CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileName( CfdMeshSettings::DAT_FILE_NAME );
    Ui.datName->setText( truncateFileName( datname, 40 ).c_str() );
    string keyname = CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileName( CfdMeshSettings::KEY_FILE_NAME );
    Ui.keyName->setText( truncateFileName( keyname, 40 ).c_str() );
    string objname = CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileName( CfdMeshSettings::OBJ_FILE_NAME );
    Ui.objName->setText( truncateFileName( objname, 40 ).c_str() );
    string polyname = CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileName( CfdMeshSettings::POLY_FILE_NAME );
    Ui.polyName->setText( truncateFileName( polyname, 40 ).c_str() );
    string stlname = CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileName( CfdMeshSettings::STL_FILE_NAME );
    Ui.stlName->setText( truncateFileName( stlname, 40 ).c_str() );
    string triname = CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileName( CfdMeshSettings::TRI_FILE_NAME );
    Ui.triName->setText( truncateFileName( triname, 40 ).c_str() );
    string gmshname = CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileName( CfdMeshSettings::GMSH_FILE_NAME );
    Ui.gmshName->setText( truncateFileName( gmshname, 40 ).c_str() );
    string srfname = CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileName( CfdMeshSettings::SRF_FILE_NAME );
    Ui.srfName->setText( truncateFileName( srfname, 40 ).c_str() );
    string tkeyname = CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileName( CfdMeshSettings::TKEY_FILE_NAME );
    Ui.tkeyName->setText( truncateFileName( tkeyname, 40).c_str() );

    //==== Export Flags ====//

    DatToggleButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileFlag( CfdMeshSettings::DAT_FILE_NAME )->GetID() );
    KeyToggleButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileFlag( CfdMeshSettings::KEY_FILE_NAME )->GetID() );
    ObjToggleButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileFlag( CfdMeshSettings::OBJ_FILE_NAME )->GetID() );
    PolyToggleButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileFlag( CfdMeshSettings::POLY_FILE_NAME )->GetID() );
    StlToggleButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileFlag( CfdMeshSettings::STL_FILE_NAME )->GetID() );
    TriToggleButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileFlag( CfdMeshSettings::TRI_FILE_NAME )->GetID() );
    GmshToggleButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileFlag( CfdMeshSettings::GMSH_FILE_NAME )->GetID() );
    SrfToggleButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileFlag( CfdMeshSettings::SRF_FILE_NAME )->GetID() );
    TkeyToggleButton.Update( CfdMeshMgr.GetCfdSettingsPtr()->GetExportFileFlag( CfdMeshSettings::TKEY_FILE_NAME)->GetID() );

    //==== Wake Flag ====//
    if( currGeom )
    {
        Ui.addWakeButton->setChecked( currGeom->GetWakeActiveFlag() );
    }

    //=== Domain tab GUI active areas ===//
    if ( CfdMeshMgr.GetCfdSettingsPtr()->GetFarMeshFlag() )
    {
        Ui.farParametersGroup->setEnabled( true );

        if( CfdMeshMgr.GetCfdSettingsPtr()->GetFarCompFlag() )
        {
            Ui.farBoxGroup->setEnabled( false );
            Ui.farCompGroup->setEnabled( true );
        }
        else
        {
            Ui.farBoxGroup->setEnabled( true );
            Ui.farCompGroup->setEnabled( false );
            Ui.farXYZLocationGroup->setEnabled( CfdMeshMgr.GetCfdSettingsPtr()->GetFarManLocFlag() );
        }
    }
    else
    {
        Ui.farParametersGroup->setEnabled( false );
    }

    //=== Domain tab GUI radio & highlight buttons ===//
    Ui.farMeshButton->setChecked( CfdMeshMgr.GetCfdSettingsPtr()->GetFarMeshFlag() );

    if( CfdMeshMgr.GetCfdSettingsPtr()->GetFarCompFlag() )
    {
        Ui.farComponentGenButton->setChecked( true ); // exclusive radio button
    }
    else
    {
        Ui.farBoxGenButton->setChecked( true ); // exclusive radio button
    }

    auto farAbsSize = CfdMeshMgr.GetCfdSettingsPtr()->GetFarAbsSizeFlag();
    Ui.farAbsSizeButton->setChecked( farAbsSize );
    Ui.farRelSizeButton->setChecked( !farAbsSize );

    auto farManLocFlag = CfdMeshMgr.GetCfdSettingsPtr()->GetFarManLocFlag();
    Ui.farManLocButton->setChecked( farManLocFlag );
    Ui.farCenLocButton->setChecked( !farManLocFlag );

    return true;
}

void CfdMeshScreen::AddOutputText( const string &text )
{
    Q_D( CfdMeshScreen );
    d->Ui.outputText->append( text.c_str() );
    d->Ui.outputText->ensureCursorVisible();
}

void CfdMeshScreen::LoadDrawObjs( vector< DrawObj* > &draw_obj_vec )
{
    if ( d_func()->isVisible() )
    {
        CfdMeshMgr.LoadDrawObjs( draw_obj_vec );
    }
}

string CfdMeshScreenPrivate::truncateFileName( const string &fn, int len )
{
    string trunc( fn );
    if ( (int)trunc.length() > len )
    {
        trunc.erase( 0, trunc.length() - len );
        trunc.replace( 0, 3, "..." );
    }
    return trunc;
}

#if 0
void CfdMeshScreen::CallBack( Fl_Widget* w )
{
    bool update_flag = true;

//  else if ( m_FarXScaleSlider->GuiChanged( w ) )
//  {
//      double val = m_FarXScaleSlider->GetVal();
//      bool change = false;
//
//      if ( CfdMeshMgr.GetFarAbsSizeFlag() )
//      {
//          CfdMeshMgr.SetFarAbsSizeFlag( false );
//          change = true;
//      }
//
//      CfdMeshMgr.SetFarXScale( val );
//      CfdMeshMgr.UpdateDomain();
//      char xstr[255];
//      sprintf( xstr, "%0.4f", CfdMeshMgr.GetFarLength() );
//      cfdMeshUi.farXScaleAbsInput->value(xstr);
//
//      if ( change )
//          CfdMeshMgr.SetFarAbsSizeFlag( true );
//
//      update_flag = false;
//  }
//  else if ( m_FarYScaleSlider->GuiChanged( w ) )
//  {
//      double val = m_FarYScaleSlider->GetVal();
//      bool change = false;
//
//      if ( CfdMeshMgr.GetFarAbsSizeFlag() )
//      {
//          CfdMeshMgr.SetFarAbsSizeFlag( false );
//          change = true;
//      }
//
//      CfdMeshMgr.SetFarYScale( val );
//      CfdMeshMgr.UpdateDomain();
//      char ystr[255];
//      sprintf( ystr, "%0.4f", CfdMeshMgr.GetFarWidth() );
//      cfdMeshUi.farYScaleAbsInput->value(ystr);
//
//      if ( change )
//          CfdMeshMgr.SetFarAbsSizeFlag( true );
//
//      update_flag = false;
//  }
//  else if ( m_FarZScaleSlider->GuiChanged( w ) )
//  {
//      double val = m_FarZScaleSlider->GetVal();
//      bool change = false;
//
//      if ( CfdMeshMgr.GetFarAbsSizeFlag() )
//      {
//          CfdMeshMgr.SetFarAbsSizeFlag( false );
//          change = true;
//      }
//
//      CfdMeshMgr.SetFarZScale( val );
//      CfdMeshMgr.UpdateDomain();
//      char zstr[255];
//      sprintf( zstr, "%0.4f", CfdMeshMgr.GetFarHeight() );
//      cfdMeshUi.farZScaleAbsInput->value(zstr);
//
//      if ( change )
//          CfdMeshMgr.SetFarAbsSizeFlag( true );
//
//      update_flag = false;
//  }

//  else if ( w == cfdMeshUi.SourceNameInput )
//  {
//      CfdMeshMgr.GUI_Val( "SourceName", cfdMeshUi.SourceNameInput->value() );
//  }

//  else if ( w == m_CfdMeshUi.farXScaleAbsInput )
//  {
//      bool change = false;
//
//      if ( !CfdMeshMgr.GetFarAbsSizeFlag() )
//      {
//          CfdMeshMgr.SetFarAbsSizeFlag( true );
//          change = true;
//      }
//
//      double val = atof( m_CfdMeshUi.farXScaleAbsInput->value() );
//      CfdMeshMgr.SetFarLength( val );
//      CfdMeshMgr.UpdateDomain();
//      double scale = CfdMeshMgr.GetFarXScale();
//      m_FarXScaleSlider.SetVal( scale );
//      m_FarXScaleSlider.UpdateGui();
//
//      update_flag = false;
//
//      if ( change )
//          CfdMeshMgr.SetFarAbsSizeFlag( false );
//  }
//
//  else if ( w == m_CfdMeshUi.farYScaleAbsInput )
//  {
//      bool change = false;
//
//      if ( !CfdMeshMgr.GetFarAbsSizeFlag() )
//      {
//          CfdMeshMgr.SetFarAbsSizeFlag( true );
//          change = true;
//      }
//
//      double val = atof( m_CfdMeshUi.farYScaleAbsInput->value() );
//      CfdMeshMgr.SetFarWidth( val );
//      CfdMeshMgr.UpdateDomain();
//      double scale = CfdMeshMgr.GetFarYScale();
//      m_FarYScaleSlider.SetVal( scale );
//      m_FarYScaleSlider.UpdateGui();
//
//      update_flag = false;
//
//      if ( change )
//          CfdMeshMgr.SetFarAbsSizeFlag( false );
//  }
//
//  else if ( w == m_CfdMeshUi.farZScaleAbsInput )
//  {
//      bool change = false;
//
//      if ( !CfdMeshMgr.GetFarAbsSizeFlag() )
//      {
//          CfdMeshMgr.SetFarAbsSizeFlag( true );
//          change = true;
//      }
//
//      double val = atof( m_CfdMeshUi.farZScaleAbsInput->value() );
//      CfdMeshMgr.SetFarHeight( val );
//      CfdMeshMgr.UpdateDomain();
//      double scale = CfdMeshMgr.GetFarZScale();
//      m_FarZScaleSlider.SetVal( scale );
//      m_FarZScaleSlider.UpdateGui();
//
//      update_flag = false;
//
//      if ( change )
//          CfdMeshMgr.SetFarAbsSizeFlag( false );
//  }

    if ( update_flag )
    {
        Update();
    }

    m_ScreenMgr->SetUpdateFlag( true );

}
#endif

void CfdMeshScreen::parm_changed( Parm* parm )
{
}

CfdMeshScreen::~CfdMeshScreen()
{
}

#include "CfdMeshScreen.moc"
