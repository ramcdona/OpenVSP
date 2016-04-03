//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "BlankScreen.h"
#include "BlankGeom.h"
#include "ScreenMgr.h"
#include "EventMgr.h"
#include "Vehicle.h"
#include "ParmMgr.h"

#include <assert.h>


//==== Constructor ====//
BlankScreen::BlankScreen( ScreenMgr* mgr ) : GeomScreen( mgr, 300, 525, "Blank" )
{
    Fl_Group* mass_tab = AddTab( "Mass" );
    Fl_Group* mass_group = AddSubGroup( mass_tab, 5 );

    m_MassLayout.SetGroupAndScreen( mass_group, this );

    m_MassLayout.AddDividerBox( "Axis Display" );
    m_MassLayout.AddSlider( m_AxisLengthSlider, "Length", 10, "%7.5f" );
    m_MassLayout.AddYGap();

    m_MassLayout.AddDividerBox( "Point Mass" );
    m_MassLayout.AddYGap();

    m_MassLayout.AddButton( m_PointMassButton, "Point Mass" );
    m_MassLayout.AddSlider( m_MassSlider, "Mass", 5, "%7.3f" );
    m_MassLayout.AddSlider( m_IxxSlider, "Ixx", 5, "%7.3f" );
    m_MassLayout.AddSlider( m_IyySlider, "Ixy", 5, "%7.3f" );
    m_MassLayout.AddSlider( m_IzzSlider, "Izz", 5, "%7.3f" );
    m_MassLayout.AddSlider( m_IxySlider, "Ixy", 5, "%7.3f" );
    m_MassLayout.AddSlider( m_IxzSlider, "Ixz", 5, "%7.3f" );
    m_MassLayout.AddSlider( m_IyzSlider, "Iyz", 5, "%7.3f" );


    RemoveTab( GetTab( m_SubSurfTab_ind ) );

}


//==== Show Blank Screen ====//
void BlankScreen::Show()
{
    if ( Update() )
    {
        m_FLTK_Window->show();
    }
}

bool BlankScreen::Update()
{
    assert( m_ScreenMgr );

    Geom* geom_ptr = m_ScreenMgr->GetCurrGeom();
    if ( !geom_ptr || geom_ptr->GetType().m_Type != BLANK_GEOM_TYPE )
    {
        Hide();
        return false;
    }

    GeomScreen::Update();

    //==== Update Blank Specific Parms ====//
    BlankGeom* blank_geom_ptr = dynamic_cast< BlankGeom* >( geom_ptr );
    assert( blank_geom_ptr );
    m_PointMassButton.Update( blank_geom_ptr->m_PointMassFlag.GetID() );
    m_MassSlider.Update( blank_geom_ptr->m_PointMass.GetID() );
    m_IxxSlider.Update( blank_geom_ptr->m_PointIxx.GetID() );
    m_IyySlider.Update( blank_geom_ptr->m_PointIyy.GetID() );
    m_IzzSlider.Update( blank_geom_ptr->m_PointIzz.GetID() );
    m_IxySlider.Update( blank_geom_ptr->m_PointIxy.GetID() );
    m_IxzSlider.Update( blank_geom_ptr->m_PointIxz.GetID() );
    m_IyzSlider.Update( blank_geom_ptr->m_PointIyz.GetID() );

    m_AxisLengthSlider.Update( blank_geom_ptr->m_AxisLength.GetID() );

    return true;
}

//==== Non Menu Callbacks ====//
void BlankScreen::CallBack( Fl_Widget *w )
{
    GeomScreen::CallBack( w );
}
