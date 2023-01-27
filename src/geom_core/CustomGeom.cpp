//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "APIDefines.h"
#include "CustomGeom.h"
#include "ParmMgr.h"
#include "ScriptMgr.h"
#include "Vehicle.h"
#include "VSP_Geom_API.h"

using namespace vsp;

/*! Constructor */
CustomGeomMgrSingleton::CustomGeomMgrSingleton()
{
}

/*! Scan Custom Directory And Return All Possible Types */
void CustomGeomMgrSingleton::ReadCustomScripts(Vehicle *veh)
{
    //==== Only Read Once ====//
    static bool init_flag = false;
    if (init_flag)
        return;
    init_flag = true;

    // jrg Test Include
    // string inc_content = ScriptMgr.ExtractContent( "CustomScripts/TestIncludes.as" );
    // string repl_content = ScriptMgr.ReplaceIncludes( inc_content, "CustomScripts/" );

    m_CustomTypeVec.clear();

    vector<string> scriptDirs = veh->GetCustomScriptDirs();

    for (int k = 0; k < scriptDirs.size(); k++)
    {
        // ReadScriptsFromDir is clever enough to not allow duplicate content.  Duplicate
        // content returns a repeated module name.  Repeated file names with different content
        // are made unique and returned.  This filters duplicate module names to prevent displaying
        // duplicates.
        vector<string> mod_vec = ScriptMgr.ReadScriptsFromDir(scriptDirs[k], ".vsppart");

        for (int i = 0; i < (int)mod_vec.size(); i++)
        {
            if (m_ModuleGeomIDMap.find(mod_vec[i]) == m_ModuleGeomIDMap.end())
            {
                m_CustomTypeVec.push_back(GeomType(CUSTOM_GEOM_TYPE, mod_vec[i], false, mod_vec[i], mod_vec[i]));
                m_ModuleGeomIDMap[mod_vec[i]] = string();
            }
        }
    }
}

/*! Init Custom Geom */
void CustomGeomMgrSingleton::InitGeom(const string &id, const string &module_name, const string &display_name)
{
    Vehicle *veh = VehicleMgr.GetVehicle();
    Geom *gptr = veh->FindGeom(id);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        m_CurrGeom = id;
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        custom_geom->SetScriptModuleName(module_name);
        custom_geom->SetDisplayName(display_name);
        custom_geom->InitGeom();

        m_ModuleGeomIDMap[module_name] = id;
    }
}

/*!
    Function to add a new Parm of input type, name, and group for a custom Geom component
    \code{.cpp}
    string length = AddParm( PARM_DOUBLE_TYPE, "Length", "Design" );

    SetParmValLimits( length, 10.0, 0.001, 1.0e12 );

    SetParmDescript( length, "Total Length of Custom Geom" );
    \endcode
    \sa PARM_TYPE
    \param [in] type Parm type enum (i.e. PARM_DOUBLE_TYPE)
    \param [in] name Parm name
    \param [in] group Parm group
    \return Parm ID
  */

string CustomGeomMgrSingleton::AddParm(int type, const string &name, const string &group)
{
    Vehicle *veh = VehicleMgr.GetVehicle();
    Geom *gptr = veh->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);

        return custom_geom->AddParm(type, name, group);
    }

    return string();
}

/*!
    Get the Parm ID for the input Parm index of the current custom Geom
    \param [in] index Parm index
    \return Parm ID
*/

string CustomGeomMgrSingleton::GetCustomParm(int index)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        return custom_geom->FindParmID(index);
    }
    return string();
}

/*!
    Add a GUI element for the current custom Geom. Some inputs may not be used depending on the input type. The Parm name and group name are used to
    identify the Parm ID associated with the GUI element.
    \code{.cpp}
    //==== InitGui Is Called Once During Each Custom Geom Construction ====//

    AddGui( GDEV_TAB, "Design"  );

    AddGui( GDEV_YGAP );

    AddGui( GDEV_DIVIDER_BOX, "Design" );

    AddGui( GDEV_SLIDER_ADJ_RANGE_INPUT, "Length", "Length", "Design"  );

    AddGui( GDEV_YGAP );

    AddGui( GDEV_SLIDER_ADJ_RANGE_INPUT, "Diameter", "Diameter", "Design"  );

    \endcode
    \sa GDEV
    \param [in] type GUI element type enum (i.e. GDEV_SLIDER_ADJ_RANGE)
    \param [in] label Optional GUI device label
    \param [in] parm_name Optional Parm name
    \param [in] group_name Optional Parm group name
    \param [in] range Optional GUI element range
    \return GUI element index
*/

int CustomGeomMgrSingleton::AddGui(int type, const string &label, const string &parm_name, const string &group_name, double range)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);

        GuiDef gd;
        gd.m_Type = type;
        gd.m_Label = label;
        gd.m_ParmName = parm_name;
        gd.m_GroupName = group_name;
        gd.m_Range = range;

        return custom_geom->AddGui(gd);
    }
    return -1;
}

/*!
    Update the GUI element with an associated Parm
    /code{.cpp}
    //==== UpdateGui Is Called Every Time The Gui is Updated ====//

    string geom_id = GetCurrCustomGeom();

    UpdateGui( WidthSlider, GetParm( geom_id, "Width", "Design" ) );

    UpdateGui( SeatHeightSlider, GetParm( geom_id, "SeatHeight", "Design" ) );

    UpdateGui( SeatLengthSlider, GetParm( geom_id, "SeatLength", "Design" ) );

    UpdateGui( BackHeightSlider, GetParm( geom_id, "BackHeight", "Design" ) );
    /endcode
    \param [in] gui_id Index of the GUI element
    \param [in] parm_id Parm ID
*/

void CustomGeomMgrSingleton::AddUpdateGui(int gui_id, const string &parm_id)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);
    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);

        GuiUpdate gu;
        gu.m_GuiID = gui_id;
        gu.m_ParmID = parm_id;

        custom_geom->AddUpdateGui(gu);
    }
}

/*! Get GUI def vec.*/
// Todo better explanation
vector<GuiDef> CustomGeomMgrSingleton::GetGuiDefVec(const string &geom_id)
{
    SetCurrCustomGeom(geom_id);
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);

        return custom_geom->GetGuiDefVec();
    }
    vector<GuiDef> defvec;
    return defvec;
}

/*!
    Check and clear a trigger event for a custom Geom GUI element
    \param [in] gui_id GUI element index
    \return True if successful, false otherwise
*/

bool CustomGeomMgrSingleton::CheckClearTriggerEvent(int gui_id)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);
    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        return custom_geom->CheckClearTriggerEvent(gui_id);
    }
    return false;
}

/* Build Update Gui Instruction Vector */
vector<GuiUpdate> CustomGeomMgrSingleton::GetGuiUpdateVec()
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);
    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);

        return custom_geom->GetGuiUpdateVec();
    }
    vector<GuiUpdate> defvec;
    return defvec;
}

/*!
    Add an XSecSurf to the current custom Geom
    /code{.cpp}
    //==== Add Cross Sections  =====//
    string seat_surf = AddXSecSurf();

    AppendCustomXSec( seat_surf, XS_POINT);

    AppendCustomXSec( seat_surf, XS_ROUNDED_RECTANGLE);

    AppendCustomXSec( seat_surf, XS_ROUNDED_RECTANGLE);

    AppendCustomXSec( seat_surf, XS_POINT);
    /endcode
    \return XSecSurf ID
*/

//==== Add XSec Surface To Current Geom - Return ID =====//
string CustomGeomMgrSingleton::AddXSecSurf()
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        return custom_geom->AddXSecSurf();
    }
    return string();
}

/*!
    Remove an XSecSurf from the current custom Geom
    \code{.cpp}
    //==== Add Temp Spine Surf ====//
    string spine_surf = AddXSecSurf();

    string spine_xsec = AppendXSec( spine_surf, XS_GENERAL_FUSE );

    SetCustomXSecRot( spine_xsec, vec3d( 0, 0, 90 ) );

    string geom_id = GetCurrCustomGeom();

    //==== Get The XSec Surf ====//
    string xsec_surf = GetXSecSurf( geom_id, 0 );

    //=== Remove Spine Surf ====//
    RemoveXSecSurf( spine_surf );
    \endcode
    \param [in] xsec_id XSecSurf ID
*/

void CustomGeomMgrSingleton::RemoveXSecSurf(const string &id)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        custom_geom->RemoveXSecSurf(id);
    }
}

/*!
    Clear all XSecSurf from the current custom Geom
    /code{.cpp}
    //==== Add Cross Sections  =====//
    string surf1 = AddXSecSurf();

    string surf2 = AddXSecSurf();

    ClearXSecSurfs();
    /endcode
*/

void CustomGeomMgrSingleton::ClearXSecSurfs()
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        return custom_geom->ClearXSecSurfs();
    }
}

/*!
    Perform a skinning update for the current custom Geom. This is typically the last call in the UpdateSurf
    function that gets called every time the Geom is updated.
    \code{.cpp}
    //==== UpdateSurf Is Called Every Time The Geom is Updated ====//

    string geom_id = GetCurrCustomGeom();

    //==== Get The XSec Surf ====//
    string xsec_surf = GetXSecSurf( geom_id, 0 );

    //==== Define The First/Last XSec Placement ====//
    string xsec3 = GetXSec( xsec_surf, 3 );

    SetCustomXSecLoc( xsec3, vec3d( 10.0, 0, 5.0 ) );

    SkinXSecSurf();

    \endcode
    \param [in] closed_flag Flag to set the last XSec equal to the first
*/

void CustomGeomMgrSingleton::SkinXSecSurf(bool closed_flag)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        custom_geom->SkinXSecSurf(closed_flag);
    }
}

/*!
    Make a copy of the current custom Geom's main surface at given index and apply a transformation
    \code{.cpp}
    string geom_id = GetCurrCustomGeom();

    Matrix4d mat;

    double x = 2.0;
    double y = 5.0;
    double z = 0.0;

    mat.translatef( x, y, z );

    CloneSurf( 0, mat );
    \endcode
    \param [in] index Main surface index
    \param [in] mat Transformation matrix
*/

void CustomGeomMgrSingleton::CloneSurf(int index, Matrix4d &mat)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        custom_geom->CloneSurf(index, mat);
    }
}

/*!
    Perform a transformation for the main surface at input index for the current custom Geom
    \param [in] index Main surface index
    \param [in] mat Transformation matrix
*/
void CustomGeomMgrSingleton::TransformSurf(int index, Matrix4d &mat)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        custom_geom->TransformSurf(index, mat);
    }
}

/*!
    Set the surface type for the current custom Geom at given surface index
    \code{.cpp}
    //==== UpdateSurf Is Called Every Time The Geom is Updated ====//

    SetVspSurfType( DISK_SURF, -1 );

    SetVspSurfCfdType( CFD_TRANSPARENT, -1 );

    SkinXSecSurf();

    \endcode
    \sa VSP_SURF_TYPE
    \param [in] type Surface type enum (i.e DISK_SURF)
    \param [in] surf_index Main surface index. The default value of -1 is used to indicate all main surfaces are the same type.
*/
void CustomGeomMgrSingleton::SetVspSurfType(int type, int surf_id)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        custom_geom->SetVspSurfType(type, surf_id);
    }
}

/*!
    Set the surface CFD type for the current custom Geom at given surface index
    \code{.cpp}
    //==== UpdateSurf Is Called Every Time The Geom is Updated ====//

    SetVspSurfType( DISK_SURF, -1 );

    SetVspSurfCfdType( CFD_TRANSPARENT, -1 );

    SkinXSecSurf();

    \endcode
    \sa VSP_SURF_CFD_TYPE
    \param [in] type CFD surface type enum (i.e. CFD_TRANSPARENT)
    \param [in] surf_index Main surface index. The default value of -1 is used to indicate all main surfaces are the same type.
*/

void CustomGeomMgrSingleton::SetVspSurfCfdType(int type, int surf_id)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        custom_geom->SetVspSurfCfdType(type, surf_id);
    }
}

/*!
    Add a CFD Mesh default source for the current custom Geom. Note, certain input params may not be used depending on the source type.
    \code{.cpp}

    //==== Add Cross Sections  =====//
    string xsec_surf = AddXSecSurf();

    AppendCustomXSec( xsec_surf, XS_POINT);

    AppendCustomXSec( xsec_surf, XS_CIRCLE);

    //==== Add A Default Point Source At Nose ====//
    SetupCustomDefaultSource( POINT_SOURCE, 0, 0.1, 1.0, 1.0, 1.0 );

    \endcode
    \sa CFD_MESH_SOURCE_TYPE
    \param [in] type CFD Mesh source type enum (i.e. BOX_SOURCE)
    \param [in] surf_index Main surface index
    \param [in] l1 Source first edge length
    \param [in] r1 Source first radius
    \param [in] u1 Source first U location
    \param [in] w1 Source first W location
    \param [in] l2 Source second edge length
    \param [in] r2 Source second radius
    \param [in] u2 Source second U location
    \param [in] w2 Source second W location
*/

void CustomGeomMgrSingleton::SetupCustomDefaultSource(int type, int surf_index,
                                                      double l1, double r1, double u1, double w1,
                                                      double l2, double r2, double u2, double w2)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);

        SourceData sd;
        sd.m_Type = type;
        sd.m_SurfIndex = surf_index;
        sd.m_Len1 = l1;
        sd.m_Rad1 = r1;
        sd.m_U1 = u1;
        sd.m_W1 = w1;
        sd.m_Len2 = l2;
        sd.m_Rad2 = r2;
        sd.m_U2 = u2;
        sd.m_W2 = w2;

        custom_geom->SetUpDefaultSource(sd);
    }
}

/*!
    Clear all default sources for the current custom Geom
    \code{.cpp}
    SetupCustomDefaultSource( POINT_SOURCE, 0, 0.1, 1.0, 1.0, 1.0 );

    ClearAllCustomDefaultSources();
    \endcode
*/

void CustomGeomMgrSingleton::ClearAllCustomDefaultSources()
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        custom_geom->ClearAllDefaultSources();
    }
}

/*!
    Set the center point of the current custom Geom
    \code{.cpp}
    string geom_id = GetCurrCustomGeom();

    double ht= GetParmVal( GetParm( geom_id, "Height", "Design" ) );

    double origin = GetParmVal( GetParm( geom_id, "Origin", "XForm" ) );

    SetCustomCenter( ht*origin, 0, 0 );
    \endcode
    \param [in] x X coordinate
    \param [in] y Y coordinate
    \param [in] z Z coordinate
*/

void CustomGeomMgrSingleton::SetCustomCenter(double x, double y, double z)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        custom_geom->SetCenter(x, y, z);
    }
}

/*!
    Set the location of an XSec for the current custom Geom
    \code{.cpp}
    string geom_id = GetCurrCustomGeom();

    //==== Get The XSec Surf ====//
    string xsec_surf = GetXSecSurf( geom_id, 0 );

    string xsec0 = GetXSec( xsec_surf, 0 );

    SetCustomXSecLoc( xsec0, vec3d( 0.0, 0.0, 5.0 ) );
    \endcode
    \param [in] xsec_id XSec ID
    \param [in] loc 3D location
*/

void CustomGeomMgrSingleton::SetCustomXSecLoc(const string &xsec_id, const vec3d &loc)
{
    ParmContainer *pc = ParmMgr.FindParmContainer(xsec_id);

    if (!pc)
        return;

    CustomXSec *cxs = dynamic_cast<CustomXSec *>(pc);
    if (!cxs)
        return;

    cxs->SetLoc(loc);
}

/*!
    Get the location of an XSec for the current custom Geom
    \code{.cpp}
    string geom_id = GetCurrCustomGeom();

    //==== Get The XSec Surf ====//
    string xsec_surf = GetXSecSurf( geom_id, 0 );

    string xsec0 = GetXSec( xsec_surf, 0 );

    SetCustomXSecLoc( xsec0, vec3d( 1.0, 2.0, 3.0 ) );

    vec3d loc =  GetCustomXSecLoc( xsec0 );

    Print( "Custom XSec Location: ", false );

    Print( loc );
    \endcode
    \param [in] xsec_id XSec ID
    \return 3D location
*/

vec3d CustomGeomMgrSingleton::GetCustomXSecLoc(const string &xsec_id)
{
    ParmContainer *pc = ParmMgr.FindParmContainer(xsec_id);

    if (!pc)
        return vec3d();

    CustomXSec *cxs = dynamic_cast<CustomXSec *>(pc);
    if (!cxs)
        return vec3d();

    return cxs->GetLoc();
}

/*!
    Set the rotation of an XSec for the current custom Geom
    \code{.cpp}
    //==== Add Spine Surf ====//
    string spine_surf = AddXSecSurf();

    string spine_xsec = AppendXSec( spine_surf, XS_GENERAL_FUSE );

    SetCustomXSecRot( spine_xsec, vec3d( 0, 0, 90 ) );
    \endcode
    \param [in] xsec_id XSec ID
    \param [in] rot Angle of rotation about the X, Y and Z axes (degrees)
*/

void CustomGeomMgrSingleton::SetCustomXSecRot(const string &xsec_id, const vec3d &rot)
{
    ParmContainer *pc = ParmMgr.FindParmContainer(xsec_id);
    if (!pc)
        return;

    CustomXSec *cxs = dynamic_cast<CustomXSec *>(pc);
    if (!cxs)
        return;

    cxs->SetRot(rot);
}

/*!
    Get the rotation of an XSec for the current custom Geom
    \code{.cpp}
    //==== Add Spine Surf ====//
    string spine_surf = AddXSecSurf();

    string spine_xsec = AppendXSec( spine_surf, XS_GENERAL_FUSE );

    SetCustomXSecRot( spine_xsec, vec3d( 0, 0, 90 ) );

    string geom_id = GetCurrCustomGeom();

    //==== Get The XSec Surf ====//
    string xsec_surf = GetXSecSurf( geom_id, 0 );

    string xsec0 = GetXSec( xsec_surf, 0 );

    vec3d rot =  GetCustomXSecRot( xsec0 );

    Print( "Custom XSec Rotation: ", false );

    Print( rot );
    \endcode
    \param [in] xsec_id XSec ID
    \return 3D rotation (degrees)
*/

vec3d CustomGeomMgrSingleton::GetCustomXSecRot(const string &xsec_id)
{
    ParmContainer *pc = ParmMgr.FindParmContainer(xsec_id);
    if (!pc)
        return vec3d();

    CustomXSec *cxs = dynamic_cast<CustomXSec *>(pc);
    if (!cxs)
        return vec3d();

    return cxs->GetRot();
}

/*!
    Append an XSec to the current custom Geom. This function is identical to AppendCustomXSec.
    \code{.cpp}
    //==== Add Cross Sections  =====//
    string xsec_surf = AddXSecSurf();

    AppendXSec( xsec_surf, XS_POINT);

    AppendXSec( xsec_surf, XS_CIRCLE );

    AppendXSec( xsec_surf, XS_CIRCLE );

    AppendXSec( xsec_surf, XS_ELLIPSE );

    AppendXSec( xsec_surf, XS_POINT);
    \endcode
    \sa XSEC_CRV_TYPE, AppendCustomXSec
    \param [in] xsec_surf_id XSecSurf ID
    \param [in] type XSec type enum (i.e. XSEC_WEDGE)
    \return XSec ID
*/

string CustomGeomMgrSingleton::AppendCustomXSec(const string &xsec_surf_id, int type)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);

        for (int i = 0; i < custom_geom->GetNumXSecSurfs(); i++)
        {
            XSecSurf *xs_surf = custom_geom->GetXSecSurf(i);
            if (xs_surf && xs_surf->GetID() == xsec_surf_id)
            {
                return xs_surf->AddXSec(type);
            }
        }
    }

    return string();
}

/*!
    Remove an XSec from the current custom Geom and keep it in memory
    \sa PasteCustomXSec
    \param [in] xsec_surf_id XSecSurf ID
    \param [in] index XSec index
*/

void CustomGeomMgrSingleton::CutCustomXSec(const string &xsec_surf_id, int index)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        int num = custom_geom->GetNumXSecSurfs();
        for (int i = 0; i < num; i++)
        {
            XSecSurf *xs_surf = custom_geom->GetXSecSurf(i);
            if (xs_surf && xs_surf->GetID() == xsec_surf_id)
            {
                xs_surf->CutXSec(index);
                // Set up flag so Update() knows to regenerate surface.
                // Insert / split cases don't need this because Parms are added,
                // which implicitly triggers this flag.
                // However, cut deletes Parms - requiring an explicit flag.
                gptr->m_SurfDirty = true;
            }
        }
    }
}

/*!
    Copy an XSec from the current custom Geom and keep it in memory
    \sa PasteCustomXSec
    \param [in] xsec_surf_id XSecSurf ID
    \param [in] index XSec index
*/

void CustomGeomMgrSingleton::CopyCustomXSec(const string &xsec_surf_id, int index)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        int num = custom_geom->GetNumXSecSurfs();
        for (int i = 0; i < num; i++)
        {
            XSecSurf *xs_surf = custom_geom->GetXSecSurf(i);
            if (xs_surf && xs_surf->GetID() == xsec_surf_id)
            {
                xs_surf->CopyXSec(index);
            }
        }
    }
}

/*!
    Paste the XSec currently held in memory for the current custom Geom at given index
    \sa CutCustomXSec, CopyCustomXSec
    \param [in] xsec_surf_id XSecSurf ID
    \param [in] index XSec index
*/

void CustomGeomMgrSingleton::PasteCustomXSec(const string &xsec_surf_id, int index)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        for (int i = 0; i < custom_geom->GetNumXSecSurfs(); i++)
        {
            XSecSurf *xs_surf = custom_geom->GetXSecSurf(i);
            if (xs_surf && xs_surf->GetID() == xsec_surf_id)
            {
                xs_surf->PasteXSec(index);
            }
        }
    }
}

/*!
    Insert a new XSec at the given index for the currently selected custom Geom
    \sa XSEC_CRV_TYPE
    \param [in] xsec_surf_id XSecSurf ID
    \param [in] type XSec type enum (i.e. XSEC_WEDGE)
    \param [in] index XSec index
*/

string CustomGeomMgrSingleton::InsertCustomXSec(const string &xsec_surf_id, int type, int index)
{
    Geom *gptr = VehicleMgr.GetVehicle()->FindGeom(m_CurrGeom);

    //==== Check If Geom is Valid and Correct Type ====//
    if (gptr && gptr->GetType().m_Type == CUSTOM_GEOM_TYPE)
    {
        CustomGeom *custom_geom = dynamic_cast<CustomGeom *>(gptr);
        for (int i = 0; i < custom_geom->GetNumXSecSurfs(); i++)
        {
            XSecSurf *xs_surf = custom_geom->GetXSecSurf(i);
            if (xs_surf && xs_surf->GetID() == xsec_surf_id)
            {
                return xs_surf->InsertXSec(type, index);
            }
        }
    }
    return string();
}

/*! Get All Custom Script Module Name */

vector<string> CustomGeomMgrSingleton::GetCustomScriptModuleNames()
{
    vector<string> module_name_vec;

    map<string, string>::iterator iter;
    for (iter = m_ModuleGeomIDMap.begin(); iter != m_ModuleGeomIDMap.end(); ++iter)
    {
        module_name_vec.push_back(iter->first);
    }
    return module_name_vec;
}

/*! Save script content to file*/

int CustomGeomMgrSingleton::SaveScriptContentToFile(const string &module_name, const string &file_name)
{
    return ScriptMgr.SaveScriptContentToFile(module_name, file_name);
}

//==================================================================================================//
//==================================================================================================//
//==================================================================================================//
//==================================================================================================//

/* Constructor */
CustomXSec::CustomXSec(XSecCurve *xsc) : SkinXSec(xsc)
{
    m_Type = vsp::XSEC_CUSTOM;
}

/*! Update */
void CustomXSec::Update()
{

    m_Type = XSEC_CUSTOM;

    m_LateUpdateFlag = false;

    XSecSurf *xsecsurf = (XSecSurf *)GetParentContainerPtr();

    // apply the needed transformation to get section into body orientation
    Matrix4d mat;
    xsecsurf->GetBasicTransformation(m_XSCurve->GetWidth(), mat);

    VspCurve baseCurve = GetUntransformedCurve();

    baseCurve.Transform(mat);

    //==== Apply Transform ====//
    m_TransformedCurve = baseCurve;

    Matrix4d tran_mat;
    tran_mat.translatef(m_Loc.x(), m_Loc.y(), m_Loc.z());

    Matrix4d rotate_mat;
    rotate_mat.rotateX(m_Rot.x());
    rotate_mat.rotateY(m_Rot.y());
    rotate_mat.rotateZ(m_Rot.z());

    Matrix4d cent_mat;
    cent_mat.translatef(-m_Loc.x(), -m_Loc.y(), -m_Loc.z());

    Matrix4d inv_cent_mat;
    inv_cent_mat.translatef(m_Loc.x(), m_Loc.y(), m_Loc.z());

    m_Transform.loadIdentity();

    m_Transform.postMult(tran_mat.data());
    m_Transform.postMult(cent_mat.data());
    m_Transform.postMult(rotate_mat.data());
    m_Transform.postMult(inv_cent_mat.data());

    m_Transform.postMult(xsecsurf->GetGlobalXForm().data());

    m_TransformedCurve.Transform(m_Transform);
}

/*! Copy Base Pos*/
void CustomXSec::CopyBasePos(XSec *xs)
{
    if (xs)
    {
        CustomXSec *cxs = dynamic_cast<CustomXSec *>(xs);
        if (cxs)
        {
            m_Loc = cxs->m_Loc;
            m_Rot = cxs->m_Rot;
        }
    }
}

/*! Set XSec Location - Not Using Parms To Avoid Exposing Dependant Vars */
void CustomXSec::SetLoc(const vec3d &loc)
{
    m_Loc = loc;
    m_LateUpdateFlag = true;
}

/*! Set XSec Rotation - Not Using Parms To Avoid Exposing Dependant Vars */
void CustomXSec::SetRot(const vec3d &rot)
{
    m_Rot = rot;
    m_LateUpdateFlag = true;
}

/*! Get Scale, but likely TODO as it just returns 1.0*/
double CustomXSec::GetScale()
{
    return 1.0;
}

//==================================================================================================//
//==================================================================================================//
//==================================================================================================//
//==================================================================================================//

/*! Constructor */
CustomGeom::CustomGeom(Vehicle *vehicle_ptr) : Geom(vehicle_ptr)
{
    m_InitGeomFlag = false;
    m_Name = "CustomGeom";
    m_Type.m_Name = "Custom";
    m_Type.m_Type = CUSTOM_GEOM_TYPE;
    m_VspSurfType = vsp::NORMAL_SURF;
    m_VspSurfCfdType = vsp::CFD_NORMAL;
    m_ConformalFlag = false;
}

/*! Destructor */
CustomGeom::~CustomGeom()
{
    Clear();
}

/*! Clear params and xsecsurfs */
void CustomGeom::Clear()
{
    //==== Clear Parms ====//
    for (int i = 0; i < (int)m_ParmVec.size(); i++)
    {
        delete m_ParmVec[i];
    }
    m_ParmVec.clear();

    //==== Clear XSec Surfs====//
    ClearXSecSurfs();
}

/*! Init Geometry */
void CustomGeom::InitGeom()
{
    Clear();
    m_InitGeomFlag = true;
    ScriptMgr.ExecuteScript(GetScriptModuleName().c_str(), "void Init()");
    ScriptMgr.ExecuteScript(GetScriptModuleName().c_str(), "void InitGui()");
    SetName(GetScriptModuleName());
    Update();
}

/*! Add Gui Definition */
int CustomGeom::AddGui(const GuiDef &gd)
{
    m_GuiDefVec.push_back(gd);
    return m_GuiDefVec.size() - 1;
}

/*! Add Parm->Gui Match */
void CustomGeom::AddUpdateGui(const GuiUpdate &gu)
{
    m_UpdateGuiVec.push_back(gu);
}

/*! Match Parm to GUI by Executing UpdateGui Script */
vector<GuiUpdate> CustomGeom::GetGuiUpdateVec()
{
    m_UpdateGuiVec.clear();

    //==== Load Predefined UpdateGui Parm Matches ====//
    for (int i = 0; i < (int)m_GuiDefVec.size(); i++)
    {
        if (m_GuiDefVec[i].m_ParmName.size() > 0 && m_GuiDefVec[i].m_GroupName.size())
        {
            string parm_id = GetParm(m_ID, m_GuiDefVec[i].m_ParmName, m_GuiDefVec[i].m_GroupName);

            GuiUpdate gu;
            gu.m_GuiID = i;
            gu.m_ParmID = parm_id;
            m_UpdateGuiVec.push_back(gu);
        }
    }

    //==== Call Script ====//
    ScriptMgr.ExecuteScript(GetScriptModuleName().c_str(), "void UpdateGui()");

    return m_UpdateGuiVec;
}

/*! Add A Gui Trigger Event */
void CustomGeom::AddGuiTriggerEvent(int index)
{
    if (m_TriggerVec.size() != m_GuiDefVec.size())
    {
        m_TriggerVec.resize(m_GuiDefVec.size(), 0);
    }

    if (index >= 0 && index < (int)m_TriggerVec.size())
    {
        m_TriggerVec[index] = 1;
    }
}

/*! Check and Clear A Trigger Event */
bool CustomGeom::CheckClearTriggerEvent(int index)
{
    bool trigger = false;
    if (index >= 0 && index < (int)m_TriggerVec.size())
    {
        if (m_TriggerVec[index] == 1)
        {
            trigger = true;
            m_TriggerVec[index] = 0;
            ForceUpdate();
        }
    }
    return trigger;
}

/*! Update Surf By Executing Update Surf Script */
void CustomGeom::UpdateSurf()
{
    if (!m_InitGeomFlag)
    {
        return;
    }

    CustomGeomMgr.SetCurrCustomGeom(GetID());

    /*! Call Script */
    ScriptMgr.ExecuteScript(GetScriptModuleName().c_str(), "void UpdateSurf()");
}

/*! Update flags*/
void CustomGeom::UpdateFlags()
{
    //==== Set Surf Type ====//
    for (int i = 0; i < (int)m_MainSurfVec.size(); i++)
    {
        m_MainSurfVec[i].SetSurfType(m_VspSurfType);
        m_MainSurfVec[i].SetSurfCfdType(m_VspSurfCfdType);

        if (m_NegativeVolumeFlag.Get() && m_VspSurfCfdType != vsp::CFD_TRANSPARENT)
        {
            m_MainSurfVec[i].SetSurfCfdType(vsp::CFD_NEGATIVE);
        }
    }

    map<int, int>::const_iterator iter;
    for (iter = m_VspSurfTypeMap.begin(); iter != m_VspSurfTypeMap.end(); ++iter)
    {
        int index = iter->first;
        if (index >= 0 && index < (int)m_MainSurfVec.size())
        {
            m_MainSurfVec[index].SetSurfType(iter->second);
        }
    }

    for (iter = m_VspSurfCfdTypeMap.begin(); iter != m_VspSurfCfdTypeMap.end(); ++iter)
    {
        int index = iter->first;
        if (index >= 0 && index < (int)m_MainSurfVec.size())
        {
            m_MainSurfVec[index].SetSurfCfdType(iter->second);
        }
    }
}

/*! Create Parm and Add To Vector Of Parms */
string CustomGeom::AddParm(int type, const string &name, const string &group)
{
    Parm *p = ParmMgr.CreateParm(type);

    if (p)
    {
        p->Init(name, group, this, 0.0, -1.0e6, 1.0e6);
        p->SetDescript("Custom Descript");
        m_ParmVec.push_back(p);
        return p->GetID();
    }
    return string();
}

/*! Find Parm String Based On Index */
string CustomGeom::FindParmID(int index)
{
    if (index >= 0 && index < (int)m_ParmVec.size())
    {
        return m_ParmVec[index]->GetID();
    }
    return string();
}

/*! Remove All XSec Surfs */
void CustomGeom::ClearXSecSurfs()
{
    //==== Clear XSec Surfs ====//
    for (int i = 0; i < (int)m_XSecSurfVec.size(); i++)
    {
        delete m_XSecSurfVec[i];
    }
    m_XSecSurfVec.clear();

    m_VspSurfType = vsp::NORMAL_SURF;
    m_VspSurfTypeMap.clear();
    m_VspSurfCfdType = vsp::CFD_NORMAL;
    m_VspSurfCfdTypeMap.clear();
}

/*! Add XSec Surface Return ID */
string CustomGeom::AddXSecSurf()
{
    XSecSurf *xsec_surf = new XSecSurf();
    xsec_surf->SetXSecType(XSEC_CUSTOM);
    xsec_surf->SetBasicOrientation(X_DIR, Y_DIR, XS_SHIFT_MID, false);
    xsec_surf->SetParentContainer(GetID());
    m_XSecSurfVec.push_back(xsec_surf);

    return xsec_surf->GetID();
}

/*! Remove XSec Surface */
void CustomGeom::RemoveXSecSurf(const string &id)
{
    vector<XSecSurf *> new_vec;
    for (int i = 0; i < (int)m_XSecSurfVec.size(); i++)
    {
        if (m_XSecSurfVec[i]->GetID() == id)
        {
            delete m_XSecSurfVec[i];
        }
        else
        {
            new_vec.push_back(m_XSecSurfVec[i]);
        }
    }

    m_XSecSurfVec = new_vec;
}

/*! Get XSecSurf At Index */
XSecSurf *CustomGeom::GetXSecSurf(int index)
{
    if (index < 0 || index >= (int)m_XSecSurfVec.size())
    {
        return NULL;
    }
    return m_XSecSurfVec[index];
}

/*! Skin XSec Surfs */
void CustomGeom::SkinXSecSurf(bool closed_flag)
{
    if (m_ConformalFlag)
    {
        ApplyConformalOffset(m_ConformalOffset);
        m_ConformalFlag = false;
    }

    m_MainSurfVec.resize(m_XSecSurfVec.size());
    assert(m_XSecSurfVec.size() == m_MainSurfVec.size());

    for (int i = 0; i < (int)m_XSecSurfVec.size(); i++)
    {
        //==== Remove Duplicate XSecs ====//
        vector<CustomXSec *> xsec_vec;
        for (int j = 0; j < m_XSecSurfVec[i]->NumXSec(); j++)
        {
            VspCurve last_crv;
            CustomXSec *xs = dynamic_cast<CustomXSec *>(m_XSecSurfVec[i]->FindXSec(j));
            if (xs)
            {
                if (j == 0)
                {
                    xsec_vec.push_back(xs);
                    last_crv = xs->GetCurve();
                }
                else
                {
                    VspCurve crv = xs->GetCurve();
                    if (!last_crv.IsEqual(crv))
                    {
                        xsec_vec.push_back(xs);
                        last_crv = crv;
                    }
                }
            }
        }
        //===== Make Sure Last XS is Exact ====//
        if (xsec_vec.size() > 2 && closed_flag)
        {
            xsec_vec[xsec_vec.size() - 1] = xsec_vec[0];
        }

        //==== Cross Section Curves & joint info ====//
        vector<rib_data_type> rib_vec;

        //==== Update XSec Location/Rotation ====//
        for (int j = 0; j < (int)xsec_vec.size(); j++)
        {
            CustomXSec *xs = xsec_vec[j];
            xs->SetLateUpdateFlag(true);

            VspCurve crv = xs->GetCurve();

            //==== Load Ribs ====//
            if (j == 0)
                rib_vec.push_back(xs->GetRib(true, false));
            else if (j == m_XSecSurfVec[i]->NumXSec() - 1)
                rib_vec.push_back(xs->GetRib(false, true));
            else
                rib_vec.push_back(xs->GetRib(false, false));
        }

        if (xsec_vec.size() >= 2)
        {
            m_MainSurfVec[i].SkinRibs(rib_vec, false);
            m_MainSurfVec[i].SetMagicVParm(false);
        }
    }
}

/*! Make A Copy Of MainSurf at Index and Apply XForm */
void CustomGeom::CloneSurf(int index, Matrix4d &mat)
{
    if (index >= 0 && index < (int)m_MainSurfVec.size())
    {
        VspSurf clone = m_MainSurfVec[index];
        clone.Transform(mat);
        clone.SetClone(index, mat);
        m_MainSurfVec.push_back(clone);
    }
}

/*! Apply Transformation for a Main Surface */
void CustomGeom::TransformSurf(int index, Matrix4d &mat)
{
    if (index >= 0 && index < (int)m_MainSurfVec.size())
    {
        m_MainSurfVec[index].Transform(mat);
    }
}

/*! Set VSP Surf Type */
void CustomGeom::SetVspSurfType(int type, int surf_id)
{
    if (surf_id == -1)
        m_VspSurfType = type;
    else
    {
        m_VspSurfTypeMap[surf_id] = type;
    }
}

/*! Set VSP Surf CFD Type */
void CustomGeom::SetVspSurfCfdType(int type, int surf_id)
{
    if (surf_id == -1)
        m_VspSurfCfdType = type;
    else
    {
        m_VspSurfCfdTypeMap[surf_id] = type;
    }
}

/*! Encode Data Into XML Data Struct */
xmlNodePtr CustomGeom::EncodeXml(xmlNodePtr &node)
{
    xmlNodePtr custom_node = xmlNewChild(node, NULL, BAD_CAST "CustomGeom", NULL);
    if (custom_node)
    {
        string file_contents = ScriptMgr.FindModuleContent(GetScriptModuleName());
        string incl_contents = ScriptMgr.ReplaceIncludes(file_contents, "");

        string safe_file_contents = XmlUtil::ConvertToXMLSafeChars(incl_contents);

        for (int i = 0; i < (int)m_ParmVec.size(); i++)
        {
            m_ParmVec[i]->EncodeXml(custom_node);
        }

        XmlUtil::AddStringNode(custom_node, "ScriptFileModule", GetScriptModuleName());
        XmlUtil::AddStringNode(custom_node, "ScriptFileContents", safe_file_contents);
    }
    Geom::EncodeXml(node);

    return custom_node;
}

/*! Encode Data Into XML Data Struct */
xmlNodePtr CustomGeom::DecodeXml(xmlNodePtr &node)
{

    xmlNodePtr custom_node = XmlUtil::GetNode(node, "CustomGeom", 0);
    if (custom_node)
    {
        string module_name = XmlUtil::FindString(custom_node, "ScriptFileModule", GetScriptModuleName());
        string safe_file_contents = XmlUtil::FindString(custom_node, "ScriptFileContents", string());
        string file_contents = XmlUtil::ConvertFromXMLSafeChars(safe_file_contents);

        string new_module_name = ScriptMgr.ReadScriptFromMemory(module_name, file_contents);
        CustomGeomMgr.InitGeom(GetID(), new_module_name, module_name);

        for (int i = 0; i < (int)m_ParmVec.size(); i++)
        {
            m_ParmVec[i]->DecodeXml(custom_node);
        }
    }
    Geom::DecodeXml(node);

    return custom_node;
}

/*! Add All Default Sources Currently in Vec */
void CustomGeom::AddDefaultSources(double base_len)
{
    for (int i = 0; i < (int)m_DefaultSourceVec.size(); i++)
    {
        SourceData sd = m_DefaultSourceVec[i];
        vsp::AddCFDSource(sd.m_Type, GetID(), sd.m_SurfIndex, sd.m_Len1, sd.m_Rad1, sd.m_U1, sd.m_W1,
                          sd.m_Len2, sd.m_Rad2, sd.m_U2, sd.m_W2);
    }
}

/*! Compute Center of Rotation */
void CustomGeom::ComputeCenter()
{
    //==== Try Calling Script Function First ====//
    if (!m_InitGeomFlag)
    {
        return;
    }

    CustomGeomMgr.SetCurrCustomGeom(GetID());

    //==== Call Script ====//
    int success = ScriptMgr.ExecuteScript(GetScriptModuleName().c_str(), "void ComputeCenter()");

    if (success == 0)
        return;

    //==== No Custom Script - Use Default ====//
    if (m_XSecSurfVec.size() < 1)
        return;

    int index = m_XSecSurfVec[0]->NumXSec() - 1;
    CustomXSec *xs = dynamic_cast<CustomXSec *>(m_XSecSurfVec[0]->FindXSec(index - 1));

    if (xs)
    {
        m_Center = vec3d(0, 0, 0);
        m_Center.set_x(m_Origin() * xs->GetLoc().x());
    }
}

/*! Optional Scale - If Script Does Not Exist Nothing Happens */
void CustomGeom::Scale()
{
    if (!m_InitGeomFlag)
    {
        return;
    }

    CustomGeomMgr.SetCurrCustomGeom(GetID());

    double curr_scale = m_Scale() / m_LastScale();

    //==== Call Script ====//
    ScriptMgr.ExecuteScript(GetScriptModuleName().c_str(), "void Scale(double s)", true, curr_scale);

    m_LastScale = m_Scale();
}

/*! Trigger Conformal XSec Offset */
void CustomGeom::OffsetXSecs(double off)
{
    m_ConformalFlag = true;
    m_ConformalOffset = off;
}

/*! Apply Conformal Offset */
void CustomGeom::ApplyConformalOffset(double off)
{

    for (int i = 0; i < (int)m_XSecSurfVec.size(); i++)
    {

        int nxsec = m_XSecSurfVec[i]->NumXSec();
        for (int j = 0; j < nxsec; j++)
        {
            XSec *xs = m_XSecSurfVec[i]->FindXSec(j);
            if (xs)
            {
                XSecCurve *xsc = xs->GetXSecCurve();
                if (xsc)
                {
                    xsc->OffsetCurve(off);
                }
            }
        }
    }
}
