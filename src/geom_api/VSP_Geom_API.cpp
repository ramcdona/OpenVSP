//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// API.h: interface for the Vehicle Class and Vehicle Mgr Singleton.
// J.R Gloudemans
//
//////////////////////////////////////////////////////////////////////

#include "VSP_Geom_API.h"
#include "VehicleMgr.h"
#include "Vehicle.h"
#include "ParmMgr.h"
#include "LinkMgr.h"
#include "AnalysisMgr.h"
#include "SurfaceIntersectionMgr.h"
#include "CfdMeshMgr.h"
#include "VspUtil.h"
#include "DesignVarMgr.h"
#include "VarPresetMgr.h"
#include "ParasiteDragMgr.h"
#include "WingGeom.h"
#include "PropGeom.h"
#include "BORGeom.h"
#include "VSPAEROMgr.h"
#include "MeasureMgr.h"
#include "SubSurfaceMgr.h"
#include "VKTAirfoil.h"
#include "StructureMgr.h"
#include "FeaMeshMgr.h"
#include "main.h"

#include "eli/mutil/quad/simpson.hpp"

#ifdef VSP_USE_FLTK
#include "GuiInterface.h"
#endif

namespace vsp
{
    //===================================================================//
    //===============       Helper Functions            =================//
    //===================================================================//
    //  Get the pointer to Vehicle - this is a helper function for the other API
    //  functions
    Vehicle *GetVehicle()
    {
        VSPCheckSetup();

        //==== Check For Valid Vehicle Ptr ====//
        Vehicle *veh = VehicleMgr.GetVehicle();
        if (!veh)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetVehicle::Invalid Vehicle Ptr");
            return veh;
        }
        return veh;
    }

    // Find the pointer to a XSecSurf given its id
    XSecSurf *FindXSecSurf(const string &id)
    {
        Vehicle *veh = GetVehicle();
        //   vector< Geom* > geom_vec = veh->FindGeomVec( veh->GetGeomVec() );
        vector<Geom *> geom_vec = veh->GetGeomStoreVec();

        for (int i = 0; i < (int)geom_vec.size(); i++)
        {
            Geom *gptr = geom_vec[i];
            for (int j = 0; j < gptr->GetNumXSecSurfs(); j++)
            {
                XSecSurf *xsec_surf = gptr->GetXSecSurf(j);
                if (xsec_surf && (id == xsec_surf->GetID()))
                {
                    return xsec_surf;
                }
            }
        }
        return NULL;
    }

    // Find the pointer to a XSec given its id
    XSec *FindXSec(const string &id)
    {
        ParmContainer *pc = ParmMgr.FindParmContainer(id);

        if (!pc)
        {
            return NULL;
        }

        XSec *xs = dynamic_cast<XSec *>(pc);
        return xs;
    }

    //===================================================================//
    //===============       API Functions               =================//
    //===================================================================//

    /*!
        Check if OpenVSP has been initialized successfully. If not, the OpenVSP instance will be exited. This call should be placed at the
        beginning of all API scripts.
        \code{.cpp}

        VSPCheckSetup();

        // Continue to do things...

        \endcode
    */

    /// Check VehicleMgr for a valid vehicle pointer.  Create vehicle
    /// pointer on first call.  There is a check to prevent multiple calls.
    void VSPCheckSetup()
    {
        //==== Make Sure Init is Only Called Once ===//
        static bool once = false;
        if (once)
        {
            return;
        }
        once = true;

        //==== Check For Valid Vehicle Ptr ====//
        if (!VehicleMgr.GetVehicle())
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "VSPCheckSetup::Invalid Vehicle Ptr");
            exit(0);
        }

        // Please dont do this - messes up the batch script mode
        // #ifdef VSP_USE_FLTK
        //    GuiInterface::getInstance().InitGui( GetVehicle() );
        // #endif

        ErrorMgr.NoError();
    }

    /*!
        Clear and reinitialize OpenVSP to all default settings
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        SetParmVal( pod_id, "Y_Rel_Location", "XForm", 2.0 );

        VSPRenew();

        if ( FindGeoms().size() != 0 ) { Print( "ERROR: VSPRenew" ); }
        \endcode
    */

    void VSPRenew()
    {
        Vehicle *veh = GetVehicle();
        veh->Renew();
        ErrorMgr.NoError();
    }

    /*!
        Update the entire vehicle and all lower level children. An input, which is true by default, is available to specify
        if managers should be updated as well. The managers are typically updated by their respective GUI, so must be
        updated through the API as well to avoid unexpected behavior.
        \code{.cpp}
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        string xsec_surf = GetXSecSurf( fid, 0 );           // Get First (and Only) XSec Surf

        int num_xsecs = GetNumXSec( xsec_surf );

        //==== Set Tan Angles At Nose/Tail
        SetXSecTanAngles( GetXSec( xsec_surf, 0 ), XSEC_BOTH_SIDES, 90 );
        SetXSecTanAngles( GetXSec( xsec_surf, num_xsecs - 1 ), XSEC_BOTH_SIDES, -90 );

        Update();       // Force Surface Update
        \endcode
        \param update_managers Flag to indicate if managers should be updated
    */

    void Update(bool update_managers)
    {
        Vehicle *veh = GetVehicle();
        veh->Update();

        if (update_managers)
        {
            // Update Managers that may respond to changes in geometry
            // This is not needed in the GUI since the screens will update
            // each manager
            veh->UpdateManagers();
        }

        ErrorMgr.NoError();
    }

    /*!
        Exit the program with a specific error code
        \param [in] error_code Error code
    */

    void VSPExit(int error_code)
    {
        exit(error_code);
    }

    void RegisterCFDMeshAnalyses()
    {
        SurfaceIntersectionMgr.RegisterAnalysis();
        CfdMeshMgr.RegisterAnalysis();
        FeaMeshMgr.RegisterAnalysis();
    }

    //===================================================================//
    //===============       File I/O Functions        ===================//
    //===================================================================//

    /*!
        Load an OpenVSP project from a VSP3 file
        \code{.cpp}
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        string fname = "example_fuse.vsp3";

        SetVSP3FileName( fname );

        Update();

        //==== Save Vehicle to File ====//
        Print( "\tSaving vehicle file to: ", false );

        Print( fname );

        WriteVSPFile( GetVSPFileName(), SET_ALL );

        //==== Reset Geometry ====//
        Print( string( "--->Resetting VSP model to blank slate\n" ) );

        ClearVSPModel();

        ReadVSPFile( fname );
        \endcode
        \param [in] file_name *.vsp3 file name
    */

    void ReadVSPFile(const string &file_name)
    {
        Vehicle *veh = GetVehicle();
        int err = veh->ReadXMLFile(file_name);
        if (err != 0)
        {
            ErrorMgr.AddError(VSP_WRONG_FILE_TYPE, "ReadVSPFile::Error");
            return;
        }
        veh->SetVSP3FileName(file_name);
        ErrorMgr.NoError();
    }

    /*!
        Save the current OpenVSP project to a VSP3 file
        \code{.cpp}
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        string fname = "example_fuse.vsp3";

        SetVSP3FileName( fname );

        Update();

        //==== Save Vehicle to File ====//
        Print( "\tSaving vehicle file to: ", false );

        Print( fname );

        WriteVSPFile( GetVSPFileName(), SET_ALL );

        //==== Reset Geometry ====//
        Print( string( "--->Resetting VSP model to blank slate\n" ) );

        ClearVSPModel();

        ReadVSPFile( fname );
        \endcode
        \param [in] file_name *.vsp3 file name
        \param [in] set Set index to write (i.e. SET_ALL)
    */

    void WriteVSPFile(const string &file_name, int set)
    {
        Vehicle *veh = GetVehicle();
        if (!veh->WriteXMLFile(file_name, set))
        {
            ErrorMgr.AddError(VSP_FILE_WRITE_FAILURE, "WriteVSPFile::Failure Writing File " + file_name);
            return;
        }
        veh->SetVSP3FileName(file_name);
        ErrorMgr.NoError();
    }

    /*!
        Set the file name of a OpenVSP project
        \code{.cpp}
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        string fname = "example_fuse.vsp3";

        SetVSP3FileName( fname );

        Update();

        //==== Save Vehicle to File ====//
        Print( "\tSaving vehicle file to: ", false );

        Print( fname );

        WriteVSPFile( GetVSPFileName(), SET_ALL );

        //==== Reset Geometry ====//
        Print( string( "--->Resetting VSP model to blank slate\n" ) );

        ClearVSPModel();

        ReadVSPFile( fname );
        \endcode
        \param [in] file_name File name
    */

    void SetVSP3FileName(const string &file_name)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetVSP3FileName::Failure Getting Vehicle Ptr");
            return;
        }
        veh->SetVSP3FileName(file_name);
        ErrorMgr.NoError();
    }

    /*!
        Get the file name of the current OpenVSP project
        \code{.cpp}
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        string fname = "example_fuse.vsp3";

        SetVSP3FileName( fname );

        Update();

        //==== Save Vehicle to File ====//
        Print( "\tSaving vehicle file to: ", false );

        Print( fname );

        WriteVSPFile( GetVSPFileName(), SET_ALL );
        \endcode
        \return File name for the current OpenVSP project
    */

    string GetVSPFileName()
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
            return string("NULL");

        ErrorMgr.NoError();
        return veh->GetVSP3FileName();
    }

    /*!
        Clear the current OpenVSP model
        \code{.cpp}
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        //==== Reset Geometry ====//
        Print( string( "--->Resetting VSP model to blank slate\n" ) );
        ClearVSPModel();
        \endcode
    */

    void ClearVSPModel()
    {
        GetVehicle()->Renew();
        ErrorMgr.NoError();
    }

    /*!
        Insert an external OpenVSP project into the current project. All Geoms in the external project are placed as children of the specified parent.
        If no parent or an invalid parent is given, the Geoms are inserted at the top level.
        \param [in] file_name *.vsp3 filename
        \param [in] parent Parent geom ID (ignored with empty string)
    */

    void InsertVSPFile(const string &file_name, const string &parent)
    {
        Vehicle *veh = GetVehicle();

        Geom *parent_geom = NULL;
        if (parent.size() > 0)
        {
            parent_geom = veh->FindGeom(parent);
            if (!parent_geom)
            {
                ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "InsertVSPFile::Can't Find Parent " + parent);
            }
        }

        if (parent_geom)
        {
            veh->SetActiveGeom(parent);
        }
        else
        {
            veh->ClearActiveGeom();
        }

        int err = veh->ReadXMLFileGeomsOnly(file_name);
        if (err != 0)
        {
            ErrorMgr.AddError(VSP_WRONG_FILE_TYPE, "InsertVSPFile::Error" + file_name);
            return;
        }
        ErrorMgr.NoError();
    }

    /*!
        Import a file into OpenVSP. Many formats are available, such as NASCART, V2, and BEM). The imported Geom, mesh, or other object is inserted
        as a child of the specified parent. If no parent or an invalid parent is given, the import will be done at the top level.
        \sa IMPORT_TYPE
        \param [in] file_name Import file name
        \param [in] file_type File type enum (i.e. IMPORT_PTS)
        \param [in] parent Parent Geom ID (ignored with empty string)
    */

    string ImportFile(const string &file_name, int file_type, const string &parent)
    {
        Vehicle *veh = GetVehicle();
        Geom *parent_geom = NULL;
        if (parent.size() > 0)
        {
            parent_geom = veh->FindGeom(parent);
            if (!parent_geom)
            {
                ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "ImportFile::Can't Find Parent " + parent);
            }
        }

        if (parent_geom)
        {
            veh->SetActiveGeom(parent);
        }
        else
        {
            veh->ClearActiveGeom();
        }

        ErrorMgr.NoError();
        return veh->ImportFile(file_name, file_type);
    }

    /*!
        Export a file from OpenVSP. Many formats are available, such as STL, IGES, and SVG. If a mesh is generated for a particular export,
        the ID of the MeshGeom will be returned. If no mesh is generated an empty string will be returned.
        \code{.cpp}
        string wid = AddGeom( "WING" );             // Add Wing

        ExportFile( "Airfoil_Metadata.csv", SET_ALL, EXPORT_SELIG_AIRFOIL );

        string mesh_id = ExportFile( "Example_Mesh.msh", SET_ALL, EXPORT_GMSH );
        DeleteGeom( mesh_id ); // Delete the mesh generated by the GMSH export
        \endcode
        \sa EXPORT_TYPE
        \param [in] file_name Export file name
        \param [in] thick_set Set index to export (i.e. SET_ALL)
        \param [in] file_type File type enum (i.e. EXPORT_IGES)
        \param [in] file_type File type enum (i.e. EXPORT_VSPGEOM)
        \return Mesh Geom ID if the export generates a mesh
    */

    string ExportFile(const string &file_name, int thick_set, int file_type, int thin_set)
    {
        string mesh_id = GetVehicle()->ExportFile(file_name, thick_set, thin_set, file_type);

        ErrorMgr.NoError();
        return mesh_id;
    }

    /*!
        Set the ID of the propeller to be exported to a BEM file. Call this function before ExportFile.
        \code{.cpp}
        //==== Add Prop Geometry ====//
        string prop_id = AddGeom( "PROP" );

        SetBEMPropID( prop_id );

        ExportFile( "ExampleBEM.bem", SET_ALL, EXPORT_BEM );
        \endcode
        \sa EXPORT_TYPE, ExportFile
        \param [in] prop_id Propeller Geom ID
    */

    void SetBEMPropID(const string &prop_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(prop_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBEMPropID::Can't Find Geom " + prop_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != PROP_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBEMPropID::Geom is not a propeller " + prop_id);
            return;
        }

        PropGeom *prop_ptr = dynamic_cast<PropGeom *>(geom_ptr);
        if (prop_ptr)
        {
            veh->m_BEMPropID = prop_id;
        }
    }

    //===================================================================//
    //======================== Design Files =============================//
    //===================================================================//

    /*!
    Read in and apply a design file (*.des) to the current OpenVSP project
    \param [in] file_name *.des input file
    */

    void ReadApplyDESFile(const string &file_name)
    {
        DesignVarMgr.ReadDesVarsDES(file_name);
        ErrorMgr.NoError();
    }

    /*!
        Write all design variables to a design file (*.des)
        \param [in] file_name *.des output file
    */

    void WriteDESFile(const string &file_name)
    {
        DesignVarMgr.WriteDesVarsDES(file_name);
        ErrorMgr.NoError();
    }

    /*!
        Read in and apply a Cart3D XDDM file (*.xddm) to the current OpenVSP project
        \param [in] file_name *.xddm input file
    */

    void ReadApplyXDDMFile(const string &file_name)
    {
        DesignVarMgr.ReadDesVarsXDDM(file_name);
        ErrorMgr.NoError();
    }

    /*!
        Write all design variables to a Cart3D XDDM file (*.xddm)
        \param [in] file_name *.xddm output file
    */

    void WriteXDDMFile(const string &file_name)
    {
        DesignVarMgr.WriteDesVarsXDDM(file_name);
        ErrorMgr.NoError();
    }

    /*!
        Get the number of design variables
        \return int Number of design variables
    */

    int GetNumDesignVars()
    {
        int num = DesignVarMgr.GetNumVars();

        ErrorMgr.NoError();
        return num;
    }

    /*!
        Add a design variable
        \sa XDDM_QUANTITY_TYPE
        \param [in] parm_id Parm ID
        \param [in] type XDDM type enum (XDDM_VAR or XDDM_CONST)
    */

    void AddDesignVar(const string &parm_id, int type)
    {
        DesignVarMgr.AddVar(parm_id, type);
        ErrorMgr.NoError();
    }

    /*!
        Delete all design variables
    */

    void DeleteAllDesignVars()
    {
        DesignVarMgr.DelAllVars();
        ErrorMgr.NoError();
    }

    /*!
        Get the Parm ID of the specified design variable
        \param [in] index Index of design variable
        \return Parm ID
    */

    string GetDesignVar(int index)
    {
        string parm_id;

        DesignVar *dv = DesignVarMgr.GetVar(index);

        if (!dv)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetDesignVar::Design variable " + to_string(index) + " out of range.");
            return parm_id;
        }

        parm_id = dv->m_ParmID;

        ErrorMgr.NoError();
        return parm_id;
    }

    /*!
        Get the XDDM type of the specified design variable
        \sa XDDM_QUANTITY_TYPE
        \param [in] index Index of design variable
        \return XDDM type enum (XDDM_VAR or XDDM_CONST)
    */

    int GetDesignVarType(int index)
    {
        int dvtype = -1;

        DesignVar *dv = DesignVarMgr.GetVar(index);

        if (!dv)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetDesignVarType::Design variable index " + to_string(index) + " out of range.");
            return dvtype;
        }

        dvtype = dv->m_XDDM_Type;

        ErrorMgr.NoError();
        return dvtype;
    }

    //===================================================================//
    //===============      Computations               ===================//
    //===================================================================//

    /*!
    Get the file name of a specified file type. Note, this function cannot be used to set FEA Mesh file names.
    \code{.cpp}
    //==== Set File Name ====//
    SetComputationFileName( DEGEN_GEOM_CSV_TYPE, "TestDegenScript.csv" );

    //==== Run Degen Geom ====//
    ComputeDegenGeom( SET_ALL, DEGEN_GEOM_CSV_TYPE );
    \endcode
    \sa COMPUTATION_FILE_TYPE, SetFeaMeshFileName
    \param [in] file_type File type enum (i.e. CFD_TRI_TYPE, COMP_GEOM_TXT_TYPE)
    \param [in] file_name File name
    */
    void SetComputationFileName(int file_type, const string &file_name)
    {
        GetVehicle()->setExportFileName(file_type, file_name);

        if (file_type == CFD_STL_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_STL_FILE_NAME);
        if (file_type == CFD_POLY_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_POLY_FILE_NAME);
        if (file_type == CFD_TRI_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_TRI_FILE_NAME);
        if (file_type == CFD_FACET_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_FACET_FILE_NAME);
        if (file_type == CFD_OBJ_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_OBJ_FILE_NAME);
        if (file_type == CFD_DAT_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_DAT_FILE_NAME);
        if (file_type == CFD_KEY_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_KEY_FILE_NAME);
        if (file_type == CFD_GMSH_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_GMSH_FILE_NAME);
        if (file_type == CFD_SRF_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_SRF_FILE_NAME);
        if (file_type == CFD_TKEY_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_TKEY_FILE_NAME);
        if (file_type == CFD_CURV_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_CURV_FILE_NAME);
        if (file_type == CFD_PLOT3D_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_PLOT3D_FILE_NAME);
        if (file_type == CFD_VSPGEOM_TYPE)
            GetVehicle()->GetCfdSettingsPtr()->SetExportFileName(file_name, CFD_VSPGEOM_FILE_NAME);

        ErrorMgr.NoError();
    }

    /*!
    Compute mass properties for the components in the set. Alternatively can be run through the Analysis Manager with 'MassProp'.
    \code{.cpp}
    //==== Test Mass Props ====//
    string pid = AddGeom( "POD", "" );

    string mesh_id = ComputeMassProps( SET_ALL, 20 );

    string mass_res_id = FindLatestResultsID( "Mass_Properties" );

    array<double> @double_arr = GetDoubleResults( mass_res_id, "Total_Mass" );

    if ( double_arr.size() != 1 )                                    { Print( "---> Error: API ComputeMassProps" ); }
    \endcode
    \sa SetAnalysisInputDefaults, PrintAnalysisInputs, ExecAnalysis
    \param [in] set Set index (i.e. SET_ALL)
    \param [in] num_slices Number of slices
    \return MeshGeom ID
    */

    string ComputeMassProps(int set, int num_slices)
    {
        Update();

        string id = GetVehicle()->MassPropsAndFlatten(set, num_slices);

        if (id.size() == 0)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "ComputeMassProps::Invalid ID ");
        }
        else
        {
            ErrorMgr.NoError();
        }

        return id;
    }

    /// Compute the Union and Wetted Surface Area and Volumes
    /*!
    Mesh, intersect, and trim components in the set. Alternatively can be run through the Analysis Manager with 'CompGeom'.
    \code{.cpp}
    //==== Add Pod Geom ====//
    string pid = AddGeom( "POD", "" );

    //==== Run CompGeom And Get Results ====//
    string mesh_id = ComputeCompGeom( SET_ALL, false, 0 );                      // Half Mesh false and no file export

    string comp_res_id = FindLatestResultsID( "Comp_Geom" );                    // Find Results ID

    array<double> @double_arr = GetDoubleResults( comp_res_id, "Wet_Area" );    // Extract Results
    \endcode
    \sa SetAnalysisInputDefaults, PrintAnalysisInputs, ExecAnalysis, COMPUTATION_FILE_TYPE
    \param [in] set Set index (i.e. SET_ALL)
    \param [in] half_mesh Flag to ignore surfaces on the negative side of the XZ plane (e.g. symmetry)
    \param [in] file_export_types CompGeom file type to export (supports XOR i.e. COMP_GEOM_CSV_TYPE & COMP_GEOM_TXT_TYPE )
    \return MeshGeom ID
    */
    string ComputeCompGeom(int set, bool half_mesh, int file_export_types)
    {
        Update();
        Vehicle *veh = GetVehicle();

        veh->setExportCompGeomCsvFile(false);
        if (file_export_types & COMP_GEOM_CSV_TYPE)
        {
            veh->setExportCompGeomCsvFile(true);
        }

        string id = veh->CompGeomAndFlatten(set, half_mesh);

        if (id.size() == 0)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "ComputeCompGeom::Invalid ID ");
        }
        else
        {
            ErrorMgr.NoError();
        }

        return id;
    }

    /*!
        Compute the degenerate geometry representation for the components in the set. Alternatively can be run through the Analysis Manager with 'DegenGeom' or 'VSPAERODegenGeom'.
        \code{.cpp}
        //==== Set File Name ====//
        SetComputationFileName( DEGEN_GEOM_CSV_TYPE, "TestDegenScript.csv" );

        //==== Run Degen Geom ====//
        ComputeDegenGeom( SET_ALL, DEGEN_GEOM_CSV_TYPE );
        \endcode
        \sa SetAnalysisInputDefaults, PrintAnalysisInputs, ExecAnalysis, COMPUTATION_FILE_TYPE
        \param [in] set Set index (i.e. SET_ALL)
        \param [in] file_type DegenGeom file type to export (supports XOR i.e DEGEN_GEOM_M_TYPE & DEGEN_GEOM_CSV_TYPE)
    */

    void ComputeDegenGeom(int set, int file_export_types)
    {
        Update();
        Vehicle *veh = GetVehicle();

        veh->setExportDegenGeomMFile(false);
        if (file_export_types & DEGEN_GEOM_M_TYPE)
        {
            veh->setExportDegenGeomMFile(true);
        }

        veh->setExportDegenGeomCsvFile(false);
        if (file_export_types & DEGEN_GEOM_CSV_TYPE)
        {
            veh->setExportDegenGeomCsvFile(true);
        }

        veh->CreateDegenGeom(set);
        veh->WriteDegenGeomFile();
        ErrorMgr.NoError();
    }

    /*!
        Slice and mesh the components in the set. Alternatively can be run through the Analysis Manager with 'PlanarSlice'.
        \code{.cpp}
        //==== Add Pod Geom ====//
        string pid = AddGeom( "POD", "" );

        //==== Test Plane Slice ====//
        string slice_mesh_id = ComputePlaneSlice( 0, 6, vec3d( 0.0, 0.0, 1.0 ), true );

        string pslice_results = FindLatestResultsID( "Slice" );

        array<double> @double_arr = GetDoubleResults( pslice_results, "Slice_Area" );

        if ( double_arr.size() != 6 )                                    { Print( "---> Error: API ComputePlaneSlice" ); }
        \endcode
        \sa SetAnalysisInputDefaults, PrintAnalysisInputs, ExecAnalysis
        \param [in] set Set index (i.e. SET_ALL)
        \param [in] num_slices Number of slices
        \param [in] norm Normal axis for all slices
        \param [in] auto_bnd Flag to automatically set the start and end bound locations
        \param [in] start_bnd Location of the first slice along the normal axis (default: 0.0)
        \param [in] end_bnd Location of the last slice along the normal axis (default: 0.0)
        \return MeshGeom ID
    */

    string ComputePlaneSlice(int set, int num_slices, const vec3d &norm, bool auto_bnd, double start_bnd, double end_bnd)
    {
        Update();
        Vehicle *veh = GetVehicle();

        string id = veh->PSliceAndFlatten(set, num_slices, norm, auto_bnd, start_bnd, end_bnd);

        if (id.size() == 0)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "ComputePlaneSlice::Invalid ID ");
        }
        else
        {
            ErrorMgr.NoError();
        }

        return id;
    }

    /*!
        Set the value of a specific CFD Mesh option
        \code{.cpp}
        SetCFDMeshVal( CFD_MIN_EDGE_LEN, 1.0 );
        \endcode
        \sa CFD_CONTROL_TYPE
        \param [in] type CFD Mesh control type enum (i.e. CFD_GROWTH_RATIO)
        \param [in] val Value to set
    */
    void SetCFDMeshVal(int type, double val)
    {
        if (type == CFD_MIN_EDGE_LEN)
            GetVehicle()->GetCfdGridDensityPtr()->m_MinLen = val;
        else if (type == CFD_MAX_EDGE_LEN)
            GetVehicle()->GetCfdGridDensityPtr()->m_BaseLen = val;
        else if (type == CFD_MAX_GAP)
            GetVehicle()->GetCfdGridDensityPtr()->m_MaxGap = val;
        else if (type == CFD_NUM_CIRCLE_SEGS)
            GetVehicle()->GetCfdGridDensityPtr()->m_NCircSeg = val;
        else if (type == CFD_GROWTH_RATIO)
            GetVehicle()->GetCfdGridDensityPtr()->m_GrowRatio = val;
        else if (type == CFD_LIMIT_GROWTH_FLAG)
            GetVehicle()->GetCfdGridDensityPtr()->SetRigorLimit(ToBool(val));
        else if (type == CFD_INTERSECT_SUBSURFACE_FLAG)
            GetVehicle()->GetCfdSettingsPtr()->m_IntersectSubSurfs = ToBool(val);
        else if (type == CFD_HALF_MESH_FLAG)
            GetVehicle()->GetCfdSettingsPtr()->SetHalfMeshFlag(ToBool(val));
        else if (type == CFD_FAR_FIELD_FLAG)
            GetVehicle()->GetCfdSettingsPtr()->SetFarMeshFlag(ToBool(val));
        else if (type == CFD_FAR_MAX_EDGE_LEN)
            GetVehicle()->GetCfdGridDensityPtr()->m_FarMaxLen = val;
        else if (type == CFD_FAR_MAX_GAP)
            GetVehicle()->GetCfdGridDensityPtr()->m_FarMaxGap = val;
        else if (type == CFD_FAR_NUM_CIRCLE_SEGS)
            GetVehicle()->GetCfdGridDensityPtr()->m_FarNCircSeg = val;
        else if (type == CFD_FAR_SIZE_ABS_FLAG)
            GetVehicle()->GetCfdSettingsPtr()->SetFarAbsSizeFlag(ToBool(val));
        else if (type == CFD_FAR_LENGTH)
            GetVehicle()->GetCfdSettingsPtr()->m_FarLength = val;
        else if (type == CFD_FAR_WIDTH)
            GetVehicle()->GetCfdSettingsPtr()->m_FarWidth = val;
        else if (type == CFD_FAR_HEIGHT)
            GetVehicle()->GetCfdSettingsPtr()->m_FarHeight = val;
        else if (type == CFD_FAR_X_SCALE)
            GetVehicle()->GetCfdSettingsPtr()->m_FarXScale = val;
        else if (type == CFD_FAR_Y_SCALE)
            GetVehicle()->GetCfdSettingsPtr()->m_FarYScale = val;
        else if (type == CFD_FAR_Z_SCALE)
            GetVehicle()->GetCfdSettingsPtr()->m_FarZScale = val;
        else if (type == CFD_FAR_LOC_MAN_FLAG)
            GetVehicle()->GetCfdSettingsPtr()->SetFarManLocFlag(ToBool(val));
        else if (type == CFD_FAR_LOC_X)
            GetVehicle()->GetCfdSettingsPtr()->m_FarXLocation = val;
        else if (type == CFD_FAR_LOC_Y)
            GetVehicle()->GetCfdSettingsPtr()->m_FarYLocation = val;
        else if (type == CFD_FAR_LOC_Z)
            GetVehicle()->GetCfdSettingsPtr()->m_FarZLocation = val;
        else if (type == CFD_SRF_XYZ_FLAG)
            GetVehicle()->GetCfdSettingsPtr()->m_XYZIntCurveFlag = ToBool(val);
        else
        {
            ErrorMgr.AddError(VSP_CANT_FIND_TYPE, "SetCFDMeshVal::Can't Find Type " + to_string((long long)type));
            return;
        }

        ErrorMgr.NoError();
    }

    /*!
     Activate or deactivate the CFD Mesh wake for a particular Geom. Note, the wake flag is only applicable for wing-type surfaces.
     Also, this function is simply an alternative to setting the value of the Parm with the available Parm setting API functions.
     \code{.cpp}
     //==== Add Wing Geom ====//
     string wid = AddGeom( "WING", "" );

     SetCFDWakeFlag( wid, true );
     // This is equivalent to SetParmValUpdate( wid, "Wake", "Shape", 1.0 );
     // To change the scale: SetParmValUpdate( wid, "WakeScale", "WakeSettings", 10.0 );
     // To change the angle: SetParmValUpdate( wid, "WakeAngle", "WakeSettings", -5.0 );
     \endcode
     \sa SetParmVal, SetParmValUpdate
     \param [in] geom_id Geom ID
     \param [in] flag True to activate, false to deactivate
    */

    void SetCFDWakeFlag(const string &geom_id, bool flag)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetCFDWakeFlag::Can't Find Geom " + geom_id);
            return;
        }

        geom_ptr->SetWakeActiveFlag(flag);

        if (!geom_ptr->HasWingTypeSurfs())
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "SetCFDWakeFlag::Geom is not a wing-type surface");
        }
        else
        {
            ErrorMgr.NoError();
        }
    }

    /*!
    Add a CFD Mesh default source for the indicated Geom. Note, certain input params may not be used depending on the source type
    \code{.cpp}
    //==== Add Pod Geom ====//
    string pid = AddGeom( "POD", "" );

    AddCFDSource( POINT_SOURCE, pid, 0, 0.25, 2.0, 0.5, 0.5 );      // Add A Point Source
    \endcode
    \sa CFD_MESH_SOURCE_TYPE
    \param [in] type CFD Mesh source type( i.e.BOX_SOURCE )
    \param [in] geom_id Geom ID
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
    void AddCFDSource(int type, const string &geom_id, int surf_index,
                      double l1, double r1, double u1, double w1,
                      double l2, double r2, double u2, double w2)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddCFDSource::Can't Find Geom " + geom_id);
            return;
        }

        CfdMeshMgr.SetCurrSourceGeomID(geom_id);
        CfdMeshMgr.SetCurrMainSurfIndx(surf_index);
        BaseSource *source = CfdMeshMgr.AddSource(type);

        if (!source)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddCFDSource::Can't Find Type");
            return;
        }

        source->m_Len = l1;
        source->m_Rad = r1;
        if (source->GetType() == POINT_SOURCE)
        {
            PointSource *ps = dynamic_cast<PointSource *>(source);
            ps->m_ULoc = u1;
            ps->m_WLoc = w1;
        }
        else if (source->GetType() == LINE_SOURCE)
        {
            LineSource *ls = dynamic_cast<LineSource *>(source);
            ls->m_Len2 = l2;
            ls->m_Rad2 = r2;
            ls->m_ULoc1 = u1;
            ls->m_WLoc1 = w1;
            ls->m_ULoc2 = u2;
            ls->m_WLoc2 = w2;
        }
        else if (source->GetType() == BOX_SOURCE)
        {
            BoxSource *bs = dynamic_cast<BoxSource *>(source);
            bs->m_ULoc1 = u1;
            bs->m_WLoc1 = w1;
            bs->m_ULoc2 = u2;
            bs->m_WLoc2 = w2;
        }
        else if (source->GetType() == ULINE_SOURCE)
        {
            ULineSource *bs = dynamic_cast<ULineSource *>(source);
            bs->m_Val = u1;
        }
        else if (source->GetType() == WLINE_SOURCE)
        {
            WLineSource *bs = dynamic_cast<WLineSource *>(source);
            bs->m_Val = w1;
        }
        ErrorMgr.NoError();
    }

    /*!
    Delete all CFD Mesh sources for all Geoms
    \code{.cpp}
    //==== Add Pod Geom ====//
    string pid = AddGeom( "POD", "" );

    AddCFDSource( POINT_SOURCE, pid, 0, 0.25, 2.0, 0.5, 0.5 );      // Add A Point Source

    DeleteAllCFDSources();
    \endcode
    */

    void DeleteAllCFDSources()
    {
        CfdMeshMgr.DeleteAllSources();
        ErrorMgr.NoError();
    }

    /*!
    Add default CFD Mesh sources for all Geoms
    \code{.cpp}
    //==== Add Pod Geom ====//
    string pid = AddGeom( "POD", "" );

    AddDefaultSources(); // 3 Sources: Def_Fwd_PS, Def_Aft_PS, Def_Fwd_Aft_LS
    \endcode
    */

    void AddDefaultSources()
    {
        CfdMeshMgr.AddDefaultSources();
        ErrorMgr.NoError();
    }

    /*!
    Create a CFD Mesh for the components in the set. This analysis cannot be run through the Analysis Manager.
    \code{.cpp}
    //==== CFDMesh Method Facet Export =====//
    SetComputationFileName( CFD_FACET_TYPE, "TestCFDMeshFacet_API.facet" );

   Print( "\tComputing CFDMesh..." );

    ComputeCFDMesh( SET_ALL, CFD_FACET_TYPE );
    \endcode
    \sa COMPUTATION_FILE_TYPE
    \param [in] set Set index (i.e. SET_ALL)
    \param [in] file_type CFD Mesh file type to export (supports XOR i.e CFD_SRF_TYPE & CFD_STL_TYPE)
    */

    void ComputeCFDMesh(int set, int file_export_types)
    {
        Update();
        Vehicle *veh = GetVehicle();

        veh->GetCfdSettingsPtr()->SetAllFileExportFlags(false);

        if (file_export_types & CFD_STL_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_STL_FILE_NAME, true);
        if (file_export_types & CFD_POLY_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_POLY_FILE_NAME, true);
        if (file_export_types & CFD_TRI_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_TRI_FILE_NAME, true);
        if (file_export_types & CFD_FACET_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_FACET_FILE_NAME, true);
        if (file_export_types & CFD_OBJ_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_OBJ_FILE_NAME, true);
        if (file_export_types & CFD_DAT_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_DAT_FILE_NAME, true);
        if (file_export_types & CFD_KEY_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_KEY_FILE_NAME, true);
        if (file_export_types & CFD_GMSH_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_GMSH_FILE_NAME, true);
        if (file_export_types & CFD_SRF_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_SRF_FILE_NAME, true);
        if (file_export_types & CFD_TKEY_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_TKEY_FILE_NAME, true);
        if (file_export_types & CFD_CURV_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_CURV_FILE_NAME, true);
        if (file_export_types & CFD_PLOT3D_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_PLOT3D_FILE_NAME, true);
        if (file_export_types & CFD_VSPGEOM_TYPE)
            veh->GetCfdSettingsPtr()->SetFileExportFlag(CFD_VSPGEOM_FILE_NAME, true);

        veh->GetCfdSettingsPtr()->m_SelectedSetIndex = set;
        CfdMeshMgr.GenerateMesh();
        ErrorMgr.NoError();
    }

    /*!
        Get ID of the current VSPAERO reference Geom
        \return Reference Geom ID
    */

    string GetVSPAERORefWingID()
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetVSPAERORefWingID::Can't Find Vehicle");
            return string();
        }

        if (VSPAEROMgr.m_RefFlag.Get() != vsp::COMPONENT_REF)
        {
            return string();
        }

        Geom *geom_ptr = veh->FindGeom(VSPAEROMgr.m_RefGeomID);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetVSPAERORefWingID::Can't Find Geom");
            return string();
        }

        return VSPAEROMgr.m_RefGeomID;
    }

    /*!
        Set the current VSPAERO reference Geom ID
        \code{.cpp}
        //==== Add Wing Geom and set some parameters =====//
        string wing_id = AddGeom( "WING" );

        SetGeomName( wing_id, "MainWing" );

        //==== Add Vertical tail and set some parameters =====//
        string vert_id = AddGeom( "WING" );

        SetGeomName( vert_id, "Vert" );

        SetParmValUpdate( vert_id, "TotalArea", "WingGeom", 10.0 );
        SetParmValUpdate( vert_id, "X_Rel_Location", "XForm", 8.5 );
        SetParmValUpdate( vert_id, "X_Rel_Rotation", "XForm", 90 );

        //==== Set VSPAERO Reference lengths & areas ====//
        SetVSPAERORefWingID( wing_id ); // Set as reference wing for VSPAERO

        Print( "VSPAERO Reference Wing ID: ", false );

        Print( GetVSPAERORefWingID() );
        \endcode
        \param [in] geom_id Reference Geom ID
    */

    string SetVSPAERORefWingID(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetVSPAERORefWingID::Can't Find Vehicle");
            return string();
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetVSPAERORefWingID::Can't Find Geom");
            return string();
        }

        VSPAEROMgr.m_RefGeomID = geom_id;
        VSPAEROMgr.m_RefFlag = vsp::COMPONENT_REF;

        ErrorMgr.NoError();

        return VSPAEROMgr.m_RefGeomID;
    }

    /*!
        Creates the initial default grouping for the control surfaces.
        The initial grouping collects all surface copies of the sub-surface into a single group.
        For example if a wing is defined with an aileron and that wing is symmetrical about the
        xz plane there will be a surface copy of the master wing surface as well as a copy of
        the sub-surface. The two sub-surfaces may get deflected differently during analysis
        routines and can be identified uniquely by their full name.
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL );                      // Add Control Surface Sub-Surface

        //==== Add Vertical tail and set some parameters =====//
        string vert_id = AddGeom( "WING" );

        SetGeomName( vert_id, "Vert" );

        SetParmValUpdate( vert_id, "TotalArea", "WingGeom", 10.0 );
        SetParmValUpdate( vert_id, "X_Rel_Location", "XForm", 8.5 );
        SetParmValUpdate( vert_id, "X_Rel_Rotation", "XForm", 90 );

        string rudder_id = AddSubSurf( vert_id, SS_CONTROL );                      // Add Control Surface Sub-Surface

        AutoGroupVSPAEROControlSurfaces();

        Update();

        Print( "COMPLETE\n" );
        string control_group_settings_container_id = FindContainer( "VSPAEROSettings", 0 );   // auto grouping produces parm containers within VSPAEROSettings

        //==== Set Control Surface Group Deflection Angle ====//
        Print( "\tSetting control surface group deflection angles..." );

        //  setup asymmetric deflection for aileron
        string deflection_gain_id;

        // subsurfaces get added to groups with "CSGQualities_[geom_name]_[control_surf_name]"
        // subsurfaces gain parm name is "Surf[surfndx]_Gain" starting from 0 to NumSymmetricCopies-1

        deflection_gain_id = FindParm( control_group_settings_container_id, "Surf_" + aileron_id + "_0_Gain", "ControlSurfaceGroup_0" );
        deflection_gain_id = FindParm( control_group_settings_container_id, "Surf_" + aileron_id + "_1_Gain", "ControlSurfaceGroup_0" );

        //  deflect aileron
        string deflection_angle_id = FindParm( control_group_settings_container_id, "DeflectionAngle", "ControlSurfaceGroup_0" );
        \endcode
        \sa CreateVSPAEROControlSurfaceGroup
    */

    void AutoGroupVSPAEROControlSurfaces()
    {
        VSPAEROMgr.Update();
        VSPAEROMgr.InitControlSurfaceGroups();
    }

    /*!
        Add a new VSPAERO control surface group using the default naming convention. The control surface group will not contain any
        control surfaces until they are added.
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL );                      // Add Control Surface Sub-Surface

        int group_index = CreateVSPAEROControlSurfaceGroup(); // Empty control surface group

        int num_group = GetNumControlSurfaceGroups();

        if ( num_group != 1 ) { Print( "Error: CreateVSPAEROControlSurfaceGroup" ); }
        \endcode
        \sa AddSelectedToCSGroup
        \return Index of the new VSPAERO control surface group
    */

    int CreateVSPAEROControlSurfaceGroup()
    {
        VSPAEROMgr.Update();
        VSPAEROMgr.AddControlSurfaceGroup();
        return VSPAEROMgr.GetCurrentCSGroupIndex();
    }

    /*!
        Add all available control surfaces to the control surface group at the specified index
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL );                      // Add Control Surface Sub-Surface

        int group_index = CreateVSPAEROControlSurfaceGroup(); // Empty control surface group

        AddAllToVSPAEROControlSurfaceGroup( group_index );
        \endcode
        \param [in] CSGroupIndex Index of the control surface group
    */

    void AddAllToVSPAEROControlSurfaceGroup(int CSGroupIndex)
    {
        if (CSGroupIndex < 0 || CSGroupIndex > GetNumControlSurfaceGroups())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "AddAllToVSPAEROControlSurfaceGroup::CSGroupIndex " + to_string(CSGroupIndex) + " out of range");
            return;
        }

        VSPAEROMgr.SetCurrentCSGroupIndex(CSGroupIndex);

        VSPAEROMgr.Update();

        VSPAEROMgr.AddAllToCSGroup();
    }

    /*!
        Remove all used control surfaces from the control surface group at the specified index
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL );                      // Add Control Surface Sub-Surface

        int group_index = CreateVSPAEROControlSurfaceGroup(); // Empty control surface group

        AddAllToVSPAEROControlSurfaceGroup( group_index );

        RemoveAllFromVSPAEROControlSurfaceGroup( group_index ); // Empty control surface group
        \endcode
        \param [in] CSGroupIndex Index of the control surface group
    */

    void RemoveAllFromVSPAEROControlSurfaceGroup(int CSGroupIndex)
    {
        if (CSGroupIndex < 0 || CSGroupIndex > GetNumControlSurfaceGroups())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "RemoveAllFromVSPAEROControlSurfaceGroup::CSGroupIndex " + to_string(CSGroupIndex) + " out of range");
            return;
        }

        VSPAEROMgr.SetCurrentCSGroupIndex(CSGroupIndex);

        VSPAEROMgr.Update();

        VSPAEROMgr.RemoveAllFromCSGroup();
    }

    /*!
        Get the names of each active (used) control surface in the control surface group at the specified index
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL );                      // Add Control Surface Sub-Surface

        int group_index = CreateVSPAEROControlSurfaceGroup(); // Empty control surface group

        AddAllToVSPAEROControlSurfaceGroup( group_index );

        array<string> @cs_name_vec = GetActiveCSNameVec( group_index );

        Print( "Active CS in Group Index #", false );
        Print( group_index );

        for ( int i = 0; i < int( cs_name_vec.size() ); i++ )
        {
            Print( cs_name_vec[i] );
        }
        \endcode
        \param [in] CSGroupIndex Index of the control surface group
        \return Array of active control surface names
    */

    std::vector<std::string> GetActiveCSNameVec(int CSGroupIndex)
    {
        if (CSGroupIndex < 0 || CSGroupIndex > GetNumControlSurfaceGroups())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetActiveCSNameVec::CSGroupIndex " + to_string(CSGroupIndex) + " out of range");
            return {};
        }

        VSPAEROMgr.SetCurrentCSGroupIndex(CSGroupIndex);

        VSPAEROMgr.Update();

        vector<VspAeroControlSurf> active_cs_vec = VSPAEROMgr.GetActiveCSVec();
        vector<string> return_vec(active_cs_vec.size());

        for (size_t i = 0; i < return_vec.size(); i++)
        {
            return_vec[i] = active_cs_vec[i].fullName;
        }

        return return_vec;
    }

    /*!
        Get the names of all control surfaces. Some may be active (used) while others may be available.
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL );                      // Add Control Surface Sub-Surface

        int group_index = CreateVSPAEROControlSurfaceGroup(); // Empty control surface group

        array<string> @cs_name_vec = GetCompleteCSNameVec();

        Print( "All Control Surfaces: ", false );

        for ( int i = 0; i < int( cs_name_vec.size() ); i++ )
        {
            Print( cs_name_vec[i] );
        }
        \endcode
        \return Array of all control surface names
    */

    std::vector<std::string> GetCompleteCSNameVec()
    {
        VSPAEROMgr.Update();

        vector<VspAeroControlSurf> complete_cs_vec = VSPAEROMgr.GetCompleteCSVec();
        vector<string> return_vec(complete_cs_vec.size());

        for (size_t i = 0; i < return_vec.size(); i++)
        {
            return_vec[i] = complete_cs_vec[i].fullName;
        }

        return return_vec;
    }

    /*!
        Get the names of each available (not used) control surface in the control surface group at the specified index
        \code{.cpp}
        string wid = AddGeom( "WING", "" ); // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL ); // Add Control Surface Sub-Surface

        int group_index = CreateVSPAEROControlSurfaceGroup(); // Empty control surface group

        array<string> @cs_name_vec = GetAvailableCSNameVec( group_index );

        array < int > cs_ind_vec(1);
        cs_ind_vec[0] = 1;

        AddSelectedToCSGroup( cs_ind_vec, group_index ); // Add the first available control surface to the group
        \endcode
        \param [in] CSGroupIndex Index of the control surface group
        \return Array of active control surface names
    */

    std::vector<std::string> GetAvailableCSNameVec(int CSGroupIndex)
    {
        if (CSGroupIndex < 0 || CSGroupIndex > GetNumControlSurfaceGroups())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetAvailableCSNameVec::CSGroupIndex " + to_string(CSGroupIndex) + " out of range");
            return {};
        }

        VSPAEROMgr.SetCurrentCSGroupIndex(CSGroupIndex);

        VSPAEROMgr.Update();

        vector<VspAeroControlSurf> avail_cs_vec = VSPAEROMgr.GetAvailableCSVec();
        vector<string> return_vec(avail_cs_vec.size());

        for (size_t i = 0; i < return_vec.size(); i++)
        {
            return_vec[i] = avail_cs_vec[i].fullName;
        }

        return return_vec;
    }

    /*!
        Set the name for the control surface group at the specified index
        \code{.cpp}
        string wid = AddGeom( "WING", "" ); // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL ); // Add Control Surface Sub-Surface

        int group_index = CreateVSPAEROControlSurfaceGroup(); // Empty control surface group

        SetVSPAEROControlGroupName( "Example_CS_Group", group_index );

        Print( "CS Group name: ", false );

        Print( GetVSPAEROControlGroupName( group_index ) );
        \endcode
        \param [in] name Name to set for the control surface group
        \param [in] CSGroupIndex Index of the control surface group
    */

    void SetVSPAEROControlGroupName(const string &name, int CSGroupIndex)
    {
        if (CSGroupIndex < 0 || CSGroupIndex > GetNumControlSurfaceGroups())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "SetVSPAEROControlGroupName::CSGroupIndex " + to_string(CSGroupIndex) + " out of range");
            return;
        }

        VSPAEROMgr.SetCurrentCSGroupIndex(CSGroupIndex);

        VSPAEROMgr.Update();

        VSPAEROMgr.SetCurrentCSGroupName(name);
    }

    /*!
        Get the name of the control surface group at the specified index
        \code{.cpp}
        string wid = AddGeom( "WING", "" ); // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL ); // Add Control Surface Sub-Surface

        int group_index = CreateVSPAEROControlSurfaceGroup(); // Empty control surface group

        SetVSPAEROControlGroupName( "Example_CS_Group", group_index );

        Print( "CS Group name: ", false );

        Print( GetVSPAEROControlGroupName( group_index ) );
        \endcode
        \param [in] CSGroupIndex Index of the control surface group
    */

    string GetVSPAEROControlGroupName(int CSGroupIndex)
    {
        if (CSGroupIndex < 0 || CSGroupIndex > GetNumControlSurfaceGroups())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetVSPAEROControlGroupName::CSGroupIndex " + to_string(CSGroupIndex) + " out of range");
            return string();
        }

        VSPAEROMgr.SetCurrentCSGroupIndex(CSGroupIndex);

        VSPAEROMgr.Update();

        return VSPAEROMgr.GetCurrentCSGGroupName();
    }

    /*!
        Add each control surfaces in the array of control surface indexes to the control surface group at the specified index.

        \warning The indexes in input "selected" must be matched with available control surfaces identified by GetAvailableCSNameVec.
        The "selected" input uses one- based indexing to associate available control surfaces.

        \code{.cpp}
        string wid = AddGeom( "WING", "" ); // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL ); // Add Control Surface Sub-Surface

        int group_index = CreateVSPAEROControlSurfaceGroup(); // Empty control surface group

        array < string > cs_name_vec = GetAvailableCSNameVec( group_index );

        array < int > cs_ind_vec( cs_name_vec.size() );

        for ( int i = 0; i < int( cs_name_vec.size() ); i++ )
        {
            cs_ind_vec[i] = i + 1;
        }

        AddSelectedToCSGroup( cs_ind_vec, group_index ); // Add all available control surfaces to the group
        \endcode
        \sa GetAvailableCSNameVec
        \param [in] selected Array of control surface indexes to add to the group. Note, the integer values are one based.
        \param [in] CSGroupIndex Index of the control surface group
    */

    void AddSelectedToCSGroup(vector<int> selected, int CSGroupIndex)
    {
        if (CSGroupIndex < 0 || CSGroupIndex > GetNumControlSurfaceGroups())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "AddSelectedToCSGroup::CSGroupIndex " + to_string(CSGroupIndex) + " out of range");
            return;
        }

        VSPAEROMgr.SetCurrentCSGroupIndex(CSGroupIndex);
        int max_cs_index = VSPAEROMgr.GetAvailableCSVec().size();

        if (selected.size() == 0 || selected.size() > max_cs_index)
        {
            ErrorMgr.AddError(VSP_INVALID_INPUT_VAL, "AddSelectedToCSGroup::selected out of range");
            return;
        }

        for (size_t i = 0; i < selected.size(); i++)
        {
            if (selected[i] <= 0 || selected[i] > max_cs_index)
            {
                ErrorMgr.AddError(VSP_INVALID_INPUT_VAL, "AddSelectedToCSGroup::component of selected out of range (indexing must be one based)");
                return;
            }
        }

        VSPAEROMgr.m_SelectedUngroupedCS = selected;

        VSPAEROMgr.Update();

        VSPAEROMgr.AddSelectedToCSGroup();
    }

    /*!
        Remove each control surfaces in the array of control surface indexes from the control surface group at the specified index.

        \warning The indexes in input "selected" must be matched with active control surfaces identified by GetActiveCSNameVec. The
        "selected" input uses one-based indexing to associate available control surfaces.

        \code{.cpp}
        string wid = AddGeom( "WING", "" ); // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL ); // Add Control Surface Sub-Surface

        int group_index = CreateVSPAEROControlSurfaceGroup(); // Empty control surface group

        array < string > cs_name_vec = GetAvailableCSNameVec( group_index );

        array < int > cs_ind_vec( cs_name_vec.size() );

        for ( int i = 0; i < int( cs_name_vec.size() ); i++ )
        {
            cs_ind_vec[i] = i + 1;
        }

        AddSelectedToCSGroup( cs_ind_vec, group_index ); // Add the available control surfaces to the group

        array < int > remove_cs_ind_vec( 1 );
        remove_cs_ind_vec[0] = 1;

        RemoveSelectedFromCSGroup( remove_cs_ind_vec, group_index ); // Remove the first control surface
        \endcode
        \sa GetActiveCSNameVec
        \param [in] selected Array of control surface indexes to remove from the group. Note, the integer values are one based.
        \param [in] CSGroupIndex Index of the control surface group
    */

    // TODO "//FIXME: RemoveSelectedFromCSGroup not working" was in ScriptMgr.cpp, so maybe needs fixing.

    void RemoveSelectedFromCSGroup(vector<int> selected, int CSGroupIndex)
    {
        if (CSGroupIndex < 0 || CSGroupIndex > GetNumControlSurfaceGroups())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "RemoveSelectedFromCSGroup::CSGroupIndex " + to_string(CSGroupIndex) + " out of range");
            return;
        }

        VSPAEROMgr.SetCurrentCSGroupIndex(CSGroupIndex);
        int max_cs_index = VSPAEROMgr.GetActiveCSVec().size();

        if (selected.size() == 0 || selected.size() > max_cs_index)
        {
            ErrorMgr.AddError(VSP_INVALID_INPUT_VAL, "RemoveSelectedFromCSGroup::selected out of range");
            return;
        }

        for (size_t i = 0; i < selected.size(); i++)
        {
            if (selected[i] <= 0 || selected[i] > max_cs_index)
            {
                ErrorMgr.AddError(VSP_INVALID_INPUT_VAL, "RemoveSelectedFromCSGroup::component of selected out of range (indexing must be one based)");
                return;
            }
        }

        VSPAEROMgr.m_SelectedGroupedCS = selected;

        VSPAEROMgr.Update();

        VSPAEROMgr.RemoveSelectedFromCSGroup();
    }

    /*!
        Get the total number of control surface groups
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string aileron_id = AddSubSurf( wid, SS_CONTROL );                      // Add Control Surface Sub-Surface

        //==== Add Horizontal tail and set some parameters =====//
        string horiz_id = AddGeom( "WING", "" );

        SetGeomName( horiz_id, "Vert" );

        SetParmValUpdate( horiz_id, "TotalArea", "WingGeom", 10.0 );
        SetParmValUpdate( horiz_id, "X_Rel_Location", "XForm", 8.5 );

        string elevator_id = AddSubSurf( horiz_id, SS_CONTROL );                      // Add Control Surface Sub-Surface

        AutoGroupVSPAEROControlSurfaces();

        int num_group = GetNumControlSurfaceGroups();

        if ( num_group != 2 ) { Print( "Error: GetNumControlSurfaceGroups" ); }
        \endcode
        \return Number of control surface groups
    */

    int GetNumControlSurfaceGroups()
    {
        return VSPAEROMgr.GetControlSurfaceGroupVec().size();
    }

    //===================================================================//
    //=========       VSPAERO Unsteady Group Functions        ===========//
    //===================================================================//

    /*!
        Get the ID of the VSPAERO unsteady group at the specified index. An empty string is returned if
        the index is out of range.
        \code{.cpp}
        string wing_id = AddGeom( "WING" );
        string pod_id = AddGeom( "POD" );

        // Create an actuator disk
        string prop_id = AddGeom( "PROP", "" );
        SetParmVal( prop_id, "PropMode", "Design", PROP_BLADES );

        Update();

        // Setup the unsteady group VSPAERO parms
        string disk_id = FindUnsteadyGroup( 1 ); // fixed components are in group 0 (wing & pod)

        SetParmVal( FindParm( disk_id, "RPM", "UnsteadyGroup" ), 1234.0 );
        \endcode
        \sa PROP_MODE
        \param [in] group_index Unsteady group index for the current VSPAERO set
        \return Unsteady group ID
    */

    string FindUnsteadyGroup(int group_index)
    {
        VSPAEROMgr.UpdateUnsteadyGroups();

        if (!VSPAEROMgr.ValidUnsteadyGroupInd(group_index))
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "FindUnsteadyGroup::group_index " + to_string(group_index) + " out of range");
            return string();
        }

        UnsteadyGroup *group = VSPAEROMgr.GetUnsteadyGroup(group_index);
        VSPAEROMgr.SetCurrentUnsteadyGroupIndex(group_index); // Need if RPM is uniform
        return group->GetID();
    }

    /*!
        Get the name of the unsteady group at the specified index.
        \code{.cpp}
        // Add a pod and wing
        string pod_id = AddGeom( "POD", "" );
        string wing_id = AddGeom( "WING", pod_id );

        SetParmVal( wing_id, "X_Rel_Location", "XForm", 2.5 );
        Update();

        Print( GetUnsteadyGroupName( 0 ) );
        \endcode
        \sa SetUnsteadyGroupName
        \param [in] group_index Unsteady group index for the current VSPAERO set
        \return Unsteady group name
    */

    string GetUnsteadyGroupName(int group_index)
    {
        VSPAEROMgr.UpdateUnsteadyGroups();

        if (!VSPAEROMgr.ValidUnsteadyGroupInd(group_index))
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetUnsteadyGroupName::group_index " + to_string(group_index) + " out of range");
            return string();
        }

        UnsteadyGroup *group = VSPAEROMgr.GetUnsteadyGroup(group_index);
        VSPAEROMgr.SetCurrentUnsteadyGroupIndex(group_index); // Need if RPM is uniform
        return group->GetName();
    }

    /*!
        Get an array of IDs for all components in the unsteady group at the specified index.
        \code{.cpp}
        // Add a pod and wing
        string pod_id = AddGeom( "POD", "" );
        string wing_id = AddGeom( "WING", pod_id ); // Default with symmetry on -> 2 surfaces

        SetParmVal( wing_id, "X_Rel_Location", "XForm", 2.5 );
        Update();

        array < string > comp_ids = GetUnsteadyGroupCompIDs( 0 );

        if ( comp_ids.size() != 3 )
        {
            Print( "ERROR: GetUnsteadyGroupCompIDs" );
        }
        \endcode
        \sa GetUnsteadyGroupSurfIndexes
        \param [in] group_index Unsteady group index for the current VSPAERO set
        \return Array of component IDs
    */

    vector<string> GetUnsteadyGroupCompIDs(int group_index)
    {
        vector<string> ret_vec;
        VSPAEROMgr.UpdateUnsteadyGroups();

        if (!VSPAEROMgr.ValidUnsteadyGroupInd(group_index))
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetUnsteadyGroupCompIDs::group_index " + to_string(group_index) + " out of range");
            return ret_vec;
        }

        UnsteadyGroup *group = VSPAEROMgr.GetUnsteadyGroup(group_index);

        vector<pair<string, int>> comp_vec = group->GetCompSurfPairVec();
        ret_vec.resize(comp_vec.size());

        for (size_t i = 0; i < comp_vec.size(); i++)
        {
            ret_vec[i] = comp_vec[i].first;
        }

        VSPAEROMgr.SetCurrentUnsteadyGroupIndex(group_index); // Need if RPM is uniform
        return ret_vec;
    }

    /*!
        Get an array of surface indexes for all components in the unsteady group at the specified index.
        \code{.cpp}
        // Add a pod and wing
        string pod_id = AddGeom( "POD", "" );
        string wing_id = AddGeom( "WING", pod_id ); // Default with symmetry on -> 2 surfaces

        SetParmVal( wing_id, "X_Rel_Location", "XForm", 2.5 );
        Update();

        array < int > surf_indexes = GetUnsteadyGroupSurfIndexes( 0 );

        if ( surf_indexes.size() != 3 )
        {
            Print( "ERROR: GetUnsteadyGroupSurfIndexes" );
        }
        \endcode
        \sa GetUnsteadyGroupCompIDs
        \param [in] group_index Unsteady group index for the current VSPAERO set
        \return Array of surface indexes
    */

    vector<int> GetUnsteadyGroupSurfIndexes(int group_index)
    {
        vector<int> ret_vec;
        VSPAEROMgr.UpdateUnsteadyGroups();

        if (!VSPAEROMgr.ValidUnsteadyGroupInd(group_index))
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetUnsteadyGroupSurfIndexes::group_index " + to_string(group_index) + " out of range");
            return ret_vec;
        }

        UnsteadyGroup *group = VSPAEROMgr.GetUnsteadyGroup(group_index);

        vector<pair<string, int>> comp_vec = group->GetCompSurfPairVec();
        ret_vec.resize(comp_vec.size());

        for (size_t i = 0; i < comp_vec.size(); i++)
        {
            ret_vec[i] = comp_vec[i].second;
        }

        VSPAEROMgr.SetCurrentUnsteadyGroupIndex(group_index); // Need if RPM is uniform
        return ret_vec;
    }

    /*!
        Get the number of unsteady groups in the current VSPAERO set. Each propeller is placed in its own unsteady group. All symmetric copies
        of propellers are also placed in an unsteady group. All other component types are placed in a single fixed component unsteady group.
        \code{.cpp}
        // Set VSPAERO set index to SET_ALL
        SetParmVal( FindParm( FindContainer( "VSPAEROSettings", 0 ), "GeomSet", "VSPAERO" ), SET_ALL );

        // Add a propeller
        string prop_id = AddGeom( "PROP" );
        SetParmValUpdate( prop_id, "PropMode", "Design", PROP_DISK );

        int num_group = GetNumUnsteadyGroups(); // Should be 0

        SetParmValUpdate( prop_id, "PropMode", "Design", PROP_BLADES );

        num_group = GetNumUnsteadyGroups(); // Should be 1

        string wing_id = AddGeom( "WING" );

        num_group = GetNumUnsteadyGroups(); // Should be 2 (includes fixed component group)
        \endcode
        \sa PROP_MODE, GetNumUnsteadyRotorGroups
        \return Number of unsteady groups in the current VSPAERO set
    */

    int GetNumUnsteadyGroups()
    {
        VSPAEROMgr.UpdateUnsteadyGroups();

        return VSPAEROMgr.NumUnsteadyGroups();
    }

    /*!
        Get the number of unsteady rotor groups in the current VSPAERO set. This is equivalent to the total number of propeller Geoms,
        including each symmetric copy, in the current VSPAERO set. While all fixed components (wings, fuseleage, etc.) are placed in
        their own unsteady group, this function does not consider them.
        \code{.cpp}
        // Set VSPAERO set index to SET_ALL
        SetParmVal( FindParm( FindContainer( "VSPAEROSettings", 0 ), "GeomSet", "VSPAERO" ), SET_ALL );

        // Add a propeller
        string prop_id = AddGeom( "PROP" );
        SetParmValUpdate( prop_id, "PropMode", "Design", PROP_DISK );

        int num_group = GetNumUnsteadyRotorGroups(); // Should be 0

        SetParmValUpdate( prop_id, "PropMode", "Design", PROP_BLADES );

        num_group = GetNumUnsteadyRotorGroups(); // Should be 1

        string wing_id = AddGeom( "WING" );

        num_group = GetNumUnsteadyRotorGroups(); // Should be 1 still (fixed group not included)
        \endcode
        \sa PROP_MODE, GetNumUnsteadyGroups
        \return Number of unsteady rotor groups in the current VSPAERO set
    */

    int GetNumUnsteadyRotorGroups()
    {
        VSPAEROMgr.UpdateUnsteadyGroups();

        return VSPAEROMgr.NumUnsteadyRotorGroups();
    }

    //===================================================================//
    //=========       VSPAERO Actuator Disk Functions        ============//
    //===================================================================//

    /*!
        Get the ID of a VSPAERO actuator disk at the specified index. An empty string is returned if
        the index is out of range.
        \code{.cpp}
        // Add a propeller
        string prop_id = AddGeom( "PROP", "" );
        SetParmVal( prop_id, "PropMode", "Design", PROP_DISK );
        SetParmVal( prop_id, "Diameter", "Design", 6.0 );

        Update();

        // Setup the actuator disk VSPAERO parms
        string disk_id = FindActuatorDisk( 0 );

        SetParmVal( FindParm( disk_id, "RotorRPM", "Rotor" ), 1234.0 );
        SetParmVal( FindParm( disk_id, "RotorCT", "Rotor" ), 0.35 );
        SetParmVal( FindParm( disk_id, "RotorCP", "Rotor" ), 0.55 );
        SetParmVal( FindParm( disk_id, "RotorHubDiameter", "Rotor" ), 1.0 );
        \endcode
        \sa PROP_MODE
        \param [in] disk_index Actuator disk index for the current VSPAERO set
        \return Actuator disk ID
    */

    string FindActuatorDisk(int disk_index)
    {
        VSPAEROMgr.UpdateRotorDisks();

        if (!VSPAEROMgr.ValidRotorDiskIndex(disk_index))
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "FindActuatorDisk::disk_index " + to_string(disk_index) + " out of range");
            return string();
        }

        RotorDisk *disk = VSPAEROMgr.GetRotorDisk(disk_index);
        return disk->GetID();
    }

    /*!
        Get the number of actuator disks in the current VSPAERO set. This is equivalent to the number of disk surfaces in the VSPAERO set.
        \code{.cpp}
        // Set VSPAERO set index to SET_ALL
        SetParmVal( FindParm( FindContainer( "VSPAEROSettings", 0 ), "GeomSet", "VSPAERO" ), SET_ALL );

        // Add a propeller
        string prop_id = AddGeom( "PROP", "" );
        SetParmValUpdate( prop_id, "PropMode", "Design", PROP_BLADES );

        int num_disk = GetNumActuatorDisks(); // Should be 0

        SetParmValUpdate( prop_id, "PropMode", "Design", PROP_DISK );

        num_disk = GetNumActuatorDisks(); // Should be 1
        \endcode
        \sa PROP_MODE
        \return Number of actuator disks in the current VSPAERO set
    */

    int GetNumActuatorDisks()
    {
        VSPAEROMgr.UpdateRotorDisks();

        return VSPAEROMgr.GetRotorDiskVec().size();
    }

    //===================================================================//
    //===============       Analysis Functions        ===================//
    //===================================================================//

    /*!
        Get the number of analysis types available in the Analysis Manager
        \code{.cpp}
        int nanalysis = GetNumAnalysis();

        Print( "Number of registered analyses: " + nanalysis );
        \endcode
        \return Number of analyses
    */

    int GetNumAnalysis()
    {
        return AnalysisMgr.GetNumAnalysis();
    }

    /*!
        Get the name of every available analysis in the Analysis Manager
        \code{.cpp}
        array< string > @analysis_array = ListAnalysis();

        Print( "List of Available Analyses: " );

        for ( int i = 0; i < int( analysis_array.size() ); i++ )
        {
            Print( "    " + analysis_array[i] );
        }
        \endcode
        \return Array of analysis names
    */

    vector<string> ListAnalysis()
    {
        return AnalysisMgr.ListAnalysis();
    }

    /*!
        Get the name of every available input for a particular analysis
        \code{.cpp}
        string analysis_name = "VSPAEROComputeGeometry";

        array<string>@ in_names =  GetAnalysisInputNames( analysis_name );

        Print("Analysis Inputs: ");

        for ( int i = 0; i < int( in_names.size() ); i++)
        {
            Print( ( "\t" + in_names[i] + "\n" ) );
        }
        \endcode
        \param [in] analysis Snalysis name
        \return Array of input names
    */

    vector<string> GetAnalysisInputNames(const string &analysis)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetAnalysisInputNames::Invalid Analysis ID " + analysis);
            vector<string> ret;
            return ret;
        }

        Analysis *a = AnalysisMgr.FindAnalysis(analysis);

        return a->m_Inputs.GetAllDataNames();
    }

    /*!
        Execute an analysis through the Analysis Manager
        \code{.cpp}
        string analysis_name = "VSPAEROComputeGeometry";

        string res_id = ExecAnalysis( analysis_name );
        \endcode
        \param [in] analysis Snalysis name
        \return Result ID
    */

    string ExecAnalysis(const string &analysis)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "ExecAnalysis::Invalid Analysis ID " + analysis);
            string ret;
            return ret;
        }

        return AnalysisMgr.ExecAnalysis(analysis);
    }

    /*!
        Get the number of input data for the particular analysis type and input
        \param [in] analysis Analysis name
        \param [in] name Input name
        \return Number of input data
    */

    int GetNumAnalysisInputData(const string &analysis, const string &name)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetNumAnalysisInputData::Invalid Analysis ID " + analysis);
            return 0;
        }
        ErrorMgr.NoError();

        return AnalysisMgr.GetNumInputData(analysis, name);
    }

    /*!
        Get the data type for a particulat analysis type and input
        \code{.cpp}
        string analysis = "VSPAEROComputeGeometry";

        array < string > @ inp_array = GetAnalysisInputNames( analysis );

        for ( int j = 0; j < int( inp_array.size() ); j++ )
        {
            int typ = GetAnalysisInputType( analysis, inp_array[j] );
        }
        \endcode
        \sa RES_DATA_TYPE
        \param [in] analysis Analysis name
        \param [in] name Input name
        \return int Data type enum (i.e. DOUBLE_DATA)
    */

    int GetAnalysisInputType(const string &analysis, const string &name)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetAnalysisInputType::Invalid Analysis ID " + analysis);
            return vsp::INVALID_TYPE;
        }
        ErrorMgr.NoError();

        return AnalysisMgr.GetAnalysisInputType(analysis, name);
    }

    /*!
        Get the current integer values for the particular analysis, input, and data index
        \code{.cpp}
        //==== Analysis: VSPAero Compute Geometry ====//
        string analysis_name = "VSPAEROComputeGeometry";

        // Set to panel method
        array< int > analysis_method = GetIntAnalysisInput( analysis_name, "AnalysisMethod" );

        analysis_method[0] = ( VSPAERO_ANALYSIS_METHOD::VORTEX_LATTICE );

        SetIntAnalysisInput( analysis_name, "AnalysisMethod", analysis_method );
        \endcode
        \sa RES_DATA_TYPE, SetIntAnalysisInput
        \param [in] analysis Analysis name
        \param [in] name Input name
        \param [in] index Data index
        \return Array of analysis input values
    */

    const vector<int> &GetIntAnalysisInput(const string &analysis, const string &name, int index)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetIntAnalysisInput::Invalid Analysis ID " + analysis);
        }
        else if (!AnalysisMgr.ValidAnalysisInputDataIndex(analysis, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetIntAnalysisInput::Can't Find Name " + name);
        }
        else
        {
            ErrorMgr.NoError();
        }

        return AnalysisMgr.GetIntInputData(analysis, name, index);
    }

    /*!
        Get the current double values for the particular analysis, input, and data index
        \code{.cpp}
        array<double> vinfFCinput = GetDoubleAnalysisInput( "ParasiteDrag", "Vinf" );

        vinfFCinput[0] = 629;

        SetDoubleAnalysisInput( "ParasiteDrag", "Vinf", vinfFCinput );
        \endcode
        \sa RES_DATA_TYPE, SetDoubleAnalysisInput
        \param [in] analysis Analysis name
        \param [in] name Input name
        \param [in] index Data index
        \return Array of analysis input values
    */

    const std::vector<double> &GetDoubleAnalysisInput(const string &analysis, const string &name, int index)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetDoubleAnalysisInput::Invalid Analysis ID " + analysis);
        }
        else if (!AnalysisMgr.ValidAnalysisInputDataIndex(analysis, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetDoubleAnalysisInput::Can't Find Name " + name);
        }
        else
        {
            ErrorMgr.NoError();
        }

        return AnalysisMgr.GetDoubleInputData(analysis, name, index);
    }

    /*!
        Get the current string values for the particular analysis, input, and data index
        \code{.cpp}
        array<string> fileNameInput = GetStringAnalysisInput( "ParasiteDrag", "FileName" );

        fileNameInput[0] = "ParasiteDragExample";

        SetStringAnalysisInput( "ParasiteDrag", "FileName", fileNameInput );
        \endcode
        \sa RES_DATA_TYPE, SetStringAnalysisInput
        \param [in] analysis Analysis name
        \param [in] name Input name
        \param [in] index Data index
        \return Array of analysis input values
    */

    const std::vector<std::string> &GetStringAnalysisInput(const string &analysis, const string &name, int index)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetStringAnalysisInput::Invalid Analysis ID " + analysis);
        }
        else if (!AnalysisMgr.ValidAnalysisInputDataIndex(analysis, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetStringAnalysisInput::Can't Find Name " + name);
        }
        else
        {
            ErrorMgr.NoError();
        }

        return AnalysisMgr.GetStringInputData(analysis, name, index);
    }

    /*!
        Get the current vec3d values for the particular analysis, input, and data index
        \code{.cpp}
        // PlanarSlice
        array<vec3d> norm = GetVec3dAnalysisInput( "PlanarSlice", "Norm" );

        norm[0].set_xyz( 0.23, 0.6, 0.15 );

        SetVec3dAnalysisInput( "PlanarSlice", "Norm", norm );
        \endcode
        \sa RES_DATA_TYPE, SetVec3dAnalysisInput
        \param [in] analysis Analysis name
        \param [in] name Input name
        \param [in] index Data index
        \return Array of analysis input values
    */

    const std::vector<vec3d> &GetVec3dAnalysisInput(const string &analysis, const string &name, int index)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetVec3dAnalysisInput::Invalid Analysis ID " + analysis);
        }
        else if (!AnalysisMgr.ValidAnalysisInputDataIndex(analysis, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetVec3dAnalysisInput::Can't Find Name " + name);
        }
        else
        {
            ErrorMgr.NoError();
        }

        return AnalysisMgr.GetVec3dInputData(analysis, name, index);
    }

    /*!
       Set all input values to their defaults for a specific analysis
        \code{.cpp}
        //==== Analysis: VSPAero Compute Geometry ====//
        string analysis_name = "VSPAEROComputeGeometry";

        // Set defaults
        SetAnalysisInputDefaults( analysis_name );
        \endcode
        \param [in] analysis Analysis name
    */

    void SetAnalysisInputDefaults(const string &analysis)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "SetAnalysisInputDefaults::Invalid Analysis ID " + analysis);
        }
        else
        {
            ErrorMgr.NoError();
        }

        AnalysisMgr.SetAnalysisInputDefaults(analysis);
    }

    /*!
        Set the value of a particular analysis input of integer type
        \code{.cpp}
        //==== Analysis: VSPAero Compute Geometry ====//
        string analysis_name = "VSPAEROComputeGeometry";

        // Set to panel method
        array< int > analysis_method = GetIntAnalysisInput( analysis_name, "AnalysisMethod" );

        analysis_method[0] = ( VSPAERO_ANALYSIS_METHOD::VORTEX_LATTICE );

        SetIntAnalysisInput( analysis_name, "AnalysisMethod", analysis_method );
        \endcode
        \sa GetIntAnalysisInput
        \param [in] analysis Analysis name
        \param [in] name Input name
        \param [in] indata_arr Array of integer values to set the input to
        \param [in] index Data index
    */

    void SetIntAnalysisInput(const string &analysis, const string &name, const std::vector<int> &indata, int index)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "SetIntAnalysisInput::Invalid Analysis ID " + analysis);
        }
        else if (!AnalysisMgr.ValidAnalysisInputDataIndex(analysis, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "SetIntAnalysisInput::Can't Find Name " + name);
        }
        else
        {
            ErrorMgr.NoError();
        }

        AnalysisMgr.SetIntAnalysisInput(analysis, name, indata, index);
    }

    /*!
        Set the value of a particular analysis input of double type
        \code{.cpp}
        //==== Analysis: CpSlicer ====//
        string analysis_name = "CpSlicer";

        // Setup cuts
        array < double > ycuts;
        ycuts.push_back( 2.0 );
        ycuts.push_back( 4.5 );
        ycuts.push_back( 8.0 );

        SetDoubleAnalysisInput( analysis_name, "YSlicePosVec", ycuts, 0 );
        \endcode
        \sa GetDoubleAnalysisInput
        \param [in] analysis Analysis name
        \param [in] name Input name
        \param [in] indata_arr Array of double values to set the input to
        \param [in] index Data index
    */

    void SetDoubleAnalysisInput(const string &analysis, const string &name, const std::vector<double> &indata, int index)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "SetDoubleAnalysisInput::Invalid Analysis ID " + analysis);
        }
        else if (!AnalysisMgr.ValidAnalysisInputDataIndex(analysis, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "SetDoubleAnalysisInput::Can't Find Name " + name);
        }
        else
        {
            ErrorMgr.NoError();
        }

        AnalysisMgr.SetDoubleAnalysisInput(analysis, name, indata, index);
    }

    /*!
        Set the value of a particular analysis input of string type
        \code{.cpp}
        array<string> fileNameInput = GetStringAnalysisInput( "ParasiteDrag", "FileName" );

        fileNameInput[0] = "ParasiteDragExample";

        SetStringAnalysisInput( "ParasiteDrag", "FileName", fileNameInput );
        \endcode
        \sa GetStringAnalysisInput
        \param [in] analysis Analysis name
        \param [in] name Input name
        \param [in] indata_arr Array of string values to set the input to
        \param [in] index Data index
    */

    void SetStringAnalysisInput(const string &analysis, const string &name, const std::vector<std::string> &indata, int index)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "SetStringAnalysisInput::Invalid Analysis ID " + analysis);
        }
        else if (!AnalysisMgr.ValidAnalysisInputDataIndex(analysis, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "SetStringAnalysisInput::Can't Find Name " + name);
        }
        else
        {
            ErrorMgr.NoError();
        }

        AnalysisMgr.SetStringAnalysisInput(analysis, name, indata, index);
    }

    /*!
        Set the value of a particular analysis input of vec3d type
        \code{.cpp}
        // PlanarSlice
        array<vec3d> norm = GetVec3dAnalysisInput( "PlanarSlice", "Norm" );

        norm[0].set_xyz( 0.23, 0.6, 0.15 );

        SetVec3dAnalysisInput( "PlanarSlice", "Norm", norm );
        \endcode
        \sa GetVec3dAnalysisInput
        \param [in] analysis Analysis name
        \param [in] name Input name
        \param [in] indata_arr Array of vec3d values to set the input to
        \param [in] index int Data index
    */

    void SetVec3dAnalysisInput(const string &analysis, const string &name, const std::vector<vec3d> &indata, int index)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "SetVec3dAnalysisInput::Invalid Analysis ID " + analysis);
        }
        else if (!AnalysisMgr.ValidAnalysisInputDataIndex(analysis, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "SetVec3dAnalysisInput::Can't Find Name " + name);
        }
        else
        {
            ErrorMgr.NoError();
        }

        AnalysisMgr.SetVec3dAnalysisInput(analysis, name, indata, index);
    }

    /*!
        Print to stdout all current input values for a specific analysis
        \code{.cpp}
        //==== Analysis: VSPAero Compute Geometry ====//
        string analysis_name = "VSPAEROComputeGeometry";

        // list inputs, type, and current values
        PrintAnalysisInputs( analysis_name );
        \endcode
        \param [in] analysis Analysis name
    */

    void PrintAnalysisInputs(const string &analysis_name)
    {
        if (!AnalysisMgr.ValidAnalysisName(analysis_name))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "PrintAnalysisInputs::Invalid Analysis ID " + analysis_name);
        }

        AnalysisMgr.PrintAnalysisInputs(analysis_name);
    }

    //===================================================================//
    //===============       Results Functions         ===================//
    //===================================================================//

    /*!
        Get the name of all results in the Results Manager
        \code{.cpp}
        //==== Write Some Fake Test Results =====//
        WriteTestResults();

        array< string > @results_array = GetAllResultsNames();

        for ( int i = 0; i < int( results_array.size() ); i++ )
        {
            string resid = FindLatestResultsID( results_array[i] );
            PrintResults( resid );
        }
        \endcode
        \return Array of result names
    */

    vector<string> GetAllResultsNames()
    {
        return ResultsMgr.GetAllResultsNames();
    }

    /*!
        Get the name of all results in the Results Manager
        \code{.cpp}
        //==== Write Some Fake Test Results =====//
        WriteTestResults();

        array< string > @results_array = GetAllResultsNames();

        for ( int i = 0; i < int( results_array.size() ); i++ )
        {
            string resid = FindLatestResultsID( results_array[i] );
            PrintResults( resid );
        }
        \endcode
        \return Array of result names
    */

    vector<string> GetAllDataNames(const string &results_id)
    {
        if (!ResultsMgr.ValidResultsID(results_id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetAllDataNames::Invalid ID " + results_id);
            vector<string> ret_vec;
            return ret_vec;
        }
        return ResultsMgr.GetAllDataNames(results_id);
    }

    /*!
        Get the number of results for a particular result name
        \code{.cpp}
        //==== Write Some Fake Test Results =====//
        WriteTestResults();

        if ( GetNumResults( "Test_Results" ) != 2 )                { Print( "---> Error: API GetNumResults" ); }
        \endcode
        \param [in] name Input name
        \return Number of results
    */

    int GetNumResults(const string &name)
    {
        return ResultsMgr.GetNumResults(name);
    }

    /*!
        Get the name of a result given its ID
        \code{.cpp}
        //==== Analysis: VSPAero Compute Geometry ====//
        string analysis_name = "VSPAEROComputeGeometry";

        // Set defaults
        SetAnalysisInputDefaults( analysis_name );

        string res_id = ( ExecAnalysis( analysis_name ) );

        Print( "Results Name: ", false );

        Print( GetResultsName( res_id ) );
        \endcode
        \param [in] results_id Result ID
        \return Result name
    */

    string GetResultsName(const string &results_id)
    {
        if (!ResultsMgr.ValidResultsID(results_id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetResultName::Invalid ID " + results_id);
            string ret_str;
            return ret_str;
        }

        return ResultsMgr.FindResultsPtr(results_id)->GetName();
    }

    /*!
        Find a results ID given its name and index
        \code{.cpp}
        //==== Write Some Fake Test Results =====//
        WriteTestResults();

        string res_id = FindResultsID( "Test_Results" );

        if ( res_id.size() == 0 )                                { Print( "---> Error: API FindResultsID" ); }
        \endcode
        \param [in] name Result name
        \param [in] index Result index
        \return Result ID
    */

    string FindResultsID(const string &name, int index)
    {
        string id = ResultsMgr.FindResultsID(name, index);
        if (id.size() == 0)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "FindResultsID::Can't Find Name " + name + " at index " + to_string(index));
            return id;
        }
        ErrorMgr.NoError();
        return id;
    }

    /*!
        Find the latest results ID for particular result name
        \code{.cpp}
        //==== Write Some Fake Test Results =====//
        WriteTestResults();

        array< string > @results_array = GetAllResultsNames();

        for ( int i = 0; i < int( results_array.size() ); i++ )
        {
            string resid = FindLatestResultsID( results_array[i] );
            PrintResults( resid );
        }
        \endcode
        \param [in] name Result name
        \return Result ID
    */

    string FindLatestResultsID(const string &name)
    {
        string id = ResultsMgr.FindLatestResultsID(name);
        if (id.size() == 0)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "FindLatestResultsID::Can't Find Name " + name);
            return id;
        }
        ErrorMgr.NoError();
        return id;
    }

    /*!
        Get the number of data values for a given result ID and data name
        \code{.cpp}
        //==== Write Some Fake Test Results =====//
        WriteTestResults();

        string res_id = FindResultsID( "Test_Results" );

        if ( GetNumData( res_id, "Test_Int" ) != 2 )            { Print( "---> Error: API GetNumData " ); }

        array<int> @int_arr = GetIntResults( res_id, "Test_Int", 0 );

        if ( int_arr[0] != 1 )                                    { Print( "---> Error: API GetIntResults" ); }

        int_arr = GetIntResults( res_id, "Test_Int", 1 );

        if ( int_arr[0] != 2 )                                    { Print( "---> Error: API GetIntResults" ); }
        \endcode
        \param [in] results_id Result ID
        \param [in] data_name Data name
        \return Number of data values
    */

    extern int GetNumData(const string &results_id, const string &data_name)
    {
        if (!ResultsMgr.ValidResultsID(results_id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetNumData::Invalid ID " + results_id);
            return 0;
        }
        ErrorMgr.NoError();
        return ResultsMgr.GetNumData(results_id, data_name);
    }

    /*!
        Get the data type for a given result ID and data name
        \code{.cpp}
        //==== Write Some Fake Test Results =====//
        WriteTestResults();

        string res_id = FindResultsID( "Test_Results" );

        array < string > @ res_array = GetAllDataNames( res_id );

        for ( int j = 0; j < int( res_array.size() ); j++ )
        {
            int typ = GetResultsType( res_id, res_array[j] );
        }
        \endcode
        \sa RES_DATA_TYPE
        \param [in] results_id Result ID
        \param [in] data_name Data name
        \return Data type enum (i.e. DOUBLE_DATA)
    */

    extern int GetResultsType(const string &results_id, const string &data_name)
    {
        if (!ResultsMgr.ValidResultsID(results_id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetResultsType::Invalid ID " + results_id);
            return vsp::INVALID_TYPE;
        }
        ErrorMgr.NoError();
        return ResultsMgr.GetResultsType(results_id, data_name);
    }

    /*!
        Get all integer values for a particular result, name, and index
        \code{.cpp}
        //==== Write Some Fake Test Results =====//
        WriteTestResults();

        string res_id = FindResultsID( "Test_Results" );

        if ( GetNumData( res_id, "Test_Int" ) != 2 )            { Print( "---> Error: API GetNumData " ); }
        //TODO angelscript vs c++ difference
        array<int> @int_arr = GetIntResults( res_id, "Test_Int", 0 );

        if ( int_arr[0] != 1 )                                    { Print( "---> Error: API GetIntResults" ); }

        int_arr = GetIntResults( res_id, "Test_Int", 1 );

        if ( int_arr[0] != 2 )                                    { Print( "---> Error: API GetIntResults" ); }
        \endcode
        \param [in] id Result ID
        \param [in] name Data name
        \param [in] index Data index
        \return Array of data values
    */

    const vector<int> &GetIntResults(const string &id, const string &name, int index)
    {
        if (!ResultsMgr.ValidResultsID(id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetIntResults::Invalid ID " + id);
        }
        else if (!ResultsMgr.ValidDataNameIndex(id, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetIntResults::Can't Find Name " + name + " at index " + to_string(index));
        }
        else
        {
            ErrorMgr.NoError();
        }

        return ResultsMgr.GetIntResults(id, name, index);
    }

    /*!
    Get all double values for a particular result, name, and index
    \code{.cpp}
    //==== Add Pod Geom ====//
    string pid = AddGeom( "POD", "" );

    //==== Run CompGeom And View Results ====//
    string mesh_id = ComputeCompGeom( SET_ALL, false, 0 );                      // Half Mesh false and no file export

    string comp_res_id = FindLatestResultsID( "Comp_Geom" );                    // Find Results ID

    array<double> @double_arr = GetDoubleResults( comp_res_id, "Wet_Area" );    // Extract Results
    \endcode
    \param [in] id Result ID
    \param [in] name Data name
    \param [in] index Data index
    \return Array of data values
*/
    const vector<double> &GetDoubleResults(const string &id, const string &name, int index)
    {
        if (!ResultsMgr.ValidResultsID(id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetDoubleResults::Invalid ID " + id);
        }
        else if (!ResultsMgr.ValidDataNameIndex(id, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetDoubleResults::Can't Find Name " + name + " at index " + to_string(index));
        }
        else
        {
            ErrorMgr.NoError();
        }

        return ResultsMgr.GetDoubleResults(id, name, index);
    }

    /*!
        Get all matrix (vector<vector<double>>) values for a particular result, name, and index
        \param [in] id Result ID
        \param [in] name Data name
        \param [in] index Data index
        \return 2D array of data values
    */

    const vector<vector<double>> &GetDoubleMatResults(const string &id, const string &name, int index)
    {
        if (!ResultsMgr.ValidResultsID(id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetDoubleMatResults::Invalid ID " + id);
        }
        else if (!ResultsMgr.ValidDataNameIndex(id, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetDoubleMatResults::Can't Find Name " + name + " at index " + to_string(index));
        }
        else
        {
            ErrorMgr.NoError();
        }

        return ResultsMgr.GetDoubleMatResults(id, name, index);
    }

    /*!
        Get all string values for a particular result, name, and index
        \code{.cpp}
        //==== Write Some Fake Test Results =====//
        WriteTestResults();

        string res_id = FindResultsID( "Test_Results" );

        array<string> @str_arr = GetStringResults( res_id, "Test_String" );

        if ( str_arr[0] != "This Is A Test" )                    { Print( "---> Error: API GetStringResults" ); }
        \endcode
        \param [in] id Result ID
        \param [in] name Data name
        \param [in] index Data index
        \return Array of data values
    */

    const vector<string> &GetStringResults(const string &id, const string &name, int index)
    {
        if (!ResultsMgr.ValidResultsID(id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetStringResults::Invalid ID " + id);
        }
        else if (!ResultsMgr.ValidDataNameIndex(id, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetStringResults::Can't Find Name " + name + " at index " + to_string(index));
        }
        else
        {
            ErrorMgr.NoError();
        }

        return ResultsMgr.GetStringResults(id, name, index);
    }

    /*!
        Get all vec3d values for a particular result, name, and index
        \code{.cpp}
        //==== Write Some Fake Test Results =====//

        double tol = 0.00001;

        WriteTestResults();

        string res_id = FindLatestResultsID( "Test_Results" );

        array<vec3d> @vec3d_vec = GetVec3dResults( res_id, "Test_Vec3d" );

        Print( "X: ", false );
        Print( vec3d_vec[0].x(), false );

        Print( "\tY: ", false );
        Print( vec3d_vec[0].y(), false );

        Print( "\tZ: ", false );
        Print( vec3d_vec[0].z() );
        \endcode
        \param [in] id Result ID
        \param [in] name Data name
        \param [in] index Data index
        \return Array of data values
    */

    const vector<vec3d> &GetVec3dResults(const string &id, const string &name, int index)
    {
        if (!ResultsMgr.ValidResultsID(id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetVec3dResults::Invalid ID " + id);
        }
        else if (!ResultsMgr.ValidDataNameIndex(id, name, index))
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetVec3dResults::Can't Find Name " + name + " at index " + to_string(index));
        }
        else
        {
            ErrorMgr.NoError();
        }

        return ResultsMgr.GetVec3dResults(id, name, index);
    }

    /*!
        Create a new result for a Geom
        \code{.cpp}
        //==== Test Comp Geom ====//
        string gid1 = AddGeom( "POD", "" );

        string mesh_id = ComputeCompGeom( 0, false, 0 );

        //==== Test Comp Geom Mesh Results ====//
        string mesh_geom_res_id = CreateGeomResults( mesh_id, "Comp_Mesh" );

        array<int> @int_arr = GetIntResults( mesh_geom_res_id, "Num_Tris" );

        if ( int_arr[0] < 4 )                                            { Print( "---> Error: API CreateGeomResults" ); }
        \endcode
        \param [in] geom_id Geom ID
        \param [in] name Result name
        \return Result ID
    */

    extern string CreateGeomResults(const string &geom_id, const string &name)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CreateGeomResults::Can't Find GeomID " + geom_id);
            return string();
        }

        string res_id = ResultsMgr.CreateGeomResults(geom_id, name);

        if (!ResultsMgr.ValidResultsID(res_id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "CreateGeomResults::Invalid Results " + res_id);
        }
        else
        {
            ErrorMgr.NoError();
        }

        return res_id;
    }

    /*!
        Delete all results
        \code{.cpp}
        //==== Test Comp Geom ====//
        string gid1 = AddGeom( "POD", "" );

        string mesh_id = ComputeCompGeom( 0, false, 0 );

        //==== Test Comp Geom Mesh Results ====//
        string mesh_geom_res_id = CreateGeomResults( mesh_id, "Comp_Mesh" );

        DeleteAllResults();

        if ( GetNumResults( "Comp_Mesh" ) != 0 )                { Print( "---> Error: API DeleteAllResults" ); }
        \endcode
    */

    void DeleteAllResults()
    {
        ResultsMgr.DeleteAllResults();
        ErrorMgr.NoError();
    }

    /*!
        Delete a particular result
        \code{.cpp}
        //==== Test Comp Geom ====//
        string gid1 = AddGeom( "POD", "" );

        string mesh_id = ComputeCompGeom( 0, false, 0 );

        //==== Test Comp Geom Mesh Results ====//
        string mesh_geom_res_id = CreateGeomResults( mesh_id, "Comp_Mesh" );

        DeleteResult( mesh_geom_res_id );

        if ( GetNumResults( "Comp_Mesh" ) != 0 )                { Print( "---> Error: API DeleteResult" ); }
        \endcode
        \param [in] id Result ID
    */

    void DeleteResult(const string &id)
    {
        if (!ResultsMgr.ValidResultsID(id))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "DeleteResult::Invalid ID " + id);
        }
        else
        {
            ErrorMgr.NoError();
        }

        ResultsMgr.DeleteResult(id);
    }

    /*!
        Export a result to CSV
        \code{.cpp}
        // Add Pod Geom
        string pid = AddGeom( "POD" );

        string analysis_name = "VSPAEROComputeGeometry";

        string rid = ExecAnalysis( analysis_name );

        WriteResultsCSVFile( rid, "CompGeomRes.csv" );
        \endcode
        \param [in] id Rsult ID
        \param [in] file_name CSV output file name
    */

    void WriteResultsCSVFile(const string &id, const string &file_name)
    {
        Results *resptr = ResultsMgr.FindResultsPtr(id);

        if (!resptr)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "WriteResultsCSVFile::Invalid ID " + id);
            return;
        }
        resptr->WriteCSVFile(file_name);
        ErrorMgr.NoError();
    }

    /*!
        Print a result's name value pairs to stdout
        \code{.cpp}
        // Add Pod Geom
        string pid = AddGeom( "POD" );

        string analysis_name = "VSPAEROComputeGeometry";

        string rid = ExecAnalysis( analysis_name );

        // Get & Display Results
        PrintResults( rid );
        \endcode
        \param [in] id Result ID
    */

    void PrintResults(const string &results_id)
    {
        ResultsMgr.PrintResults(results_id);
    }

    //===================================================================//
    //===============        GUI Functions            ===================//
    //===================================================================//

    extern void StartGui()
    {
#ifdef VSP_USE_FLTK
        GuiInterface::getInstance().StartGuiAPI();
#endif
    }

    /*!
        Capture the specified screen and save to file. Note, VSP_USE_FLTK must be defined
        \code{.cpp}
        int screenw = 2000;                                             // Set screenshot width and height
        int screenh = 2000;

        string fname = "test_screen_grab.png";

        ScreenGrab( fname, screenw, screenh, true, true );                // Take PNG screenshot
        \endcode
        \param [in] file_name Output file name
        \param [in] w Width of screen grab
        \param [in] h Height of screen grab
        \param [in] transparentBG Transparent background flag
        \param [in] autocrop Automatically crop transparent background flag
    */

    void ScreenGrab(const string &fname, int w, int h, bool transparentBG, bool autocrop)
    {
#ifdef VSP_USE_FLTK
        GuiInterface::getInstance().ScreenGrab(fname, w, h, transparentBG, autocrop);
#endif
    }

    /*!
        Toggle viewing the axis
        \code{.cpp}
        SetViewAxis( false );                                           // Turn off axis marker in corner of viewscreen
        \endcode
        \param [in] vaxis True to show the axis, false to hide the axis
    */

    void SetViewAxis(bool vaxis)
    {
#ifdef VSP_USE_FLTK
        GuiInterface::getInstance().SetViewAxis(vaxis);
#endif
    }

    /*!
        Toggle viewing the border frame
        \code{.cpp}
        SetShowBorders( false );                                        // Turn off red/black border on active window
        \endcode
        \param [in] brdr True to show the border frame, false to hide the border frame
    */
    void SetShowBorders(bool brdr)
    {
#ifdef VSP_USE_FLTK
        GuiInterface::getInstance().SetShowBorders(brdr);
#endif
    }

    /*!
        Set the draw type of the specified goemetry
        \code{.cpp}
        string pid = AddGeom( "POD", "" );                             // Add Pod for testing

        SetGeomDrawType( pid, GEOM_DRAW_SHADE );                       // Make pod appear as shaded
        \endcode
        \sa DRAW_TYPE
        \param [in] geom_id Geom ID
        \param [in] type Draw type enum (i.e. GEOM_DRAW_SHADE)
    */

    void SetGeomDrawType(const string &geom_id, int type)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetGeomDrawType::Can't Find Geom " + geom_id);
            return;
        }
        geom_ptr->m_GuiDraw.SetDrawType(type);

        ErrorMgr.NoError();
    }

    /*!
        Set the display type of the specified goemetry
        \code{.cpp}
        string pid = AddGeom( "POD" );                             // Add Pod for testing

        SetGeomDisplayType( pid, DISPLAY_DEGEN_PLATE );                       // Make pod appear as Bezier plate (Degen Geom)
        \endcode
        \sa DISPLAY_TYPE
        \param [in] geom_id Geom ID
        \param [in] type Display type enum (i.e. DISPLAY_BEZIER)
    */

    void SetGeomDisplayType(const string &geom_id, int type)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetGeomDisplayType::Can't Find Geom " + geom_id);
            return;
        }
        geom_ptr->m_GuiDraw.SetDisplayType(type);

        ErrorMgr.NoError();
    }

    /*!
        Set the background color
        \code{.cpp}
        SetBackground( 1.0, 1.0, 1.0 );                                 // Set background to bright white
        \endcode
        \param [in] r Red 8-bit unsigned integer (range: 0-255)
        \param [in] g Green 8-bit unsigned integer (range: 0-255)
        \param [in] b Blue 8-bit unsigned integer (range: 0-255)
    */

    void SetBackground(double r, double g, double b)
    {
#ifdef VSP_USE_FLTK
        GuiInterface::getInstance().SetBackground(r, g, b);
#endif
    }

    //===================================================================//
    //===============       Geom Functions            ===================//
    //===================================================================//

    /// Get a vector of geometry types. The types will include user defined components
    /// if available.  Fixed geom types are: "BLANK", "POD", "FUSELAGE"

    /*!
        Get an array of all Geom types (i.e FUSELAGE, POD, etc.)
        \code{.cpp}
            //==== Add Pod Geometries ====//
        string pod1 = AddGeom( "POD", "" );
        string pod2 = AddGeom( "POD", "" );

        array< string > @type_array = GetGeomTypes();

        if ( type_array[0] != "POD" )                { Print( "---> Error: API GetGeomTypes  " ); }
        \endcode
        \return Array of Geom type names
    */

    vector<string> GetGeomTypes()
    {
        Vehicle *veh = GetVehicle();

        //==== Load All Type Names ====//
        vector<string> ret_vec;
        for (int i = 0; i < veh->GetNumGeomTypes(); i++)
        {
            ret_vec.push_back(veh->GetGeomType(i).m_Name);
        }

        ErrorMgr.NoError();
        return ret_vec;
    }

    /// Add a geometry of given type to the vehicle.  The geometry will be a child of
    /// optional parent geometry.  The ID string of the geometry will be returned.

    /*!
        Add a new Geom of given type as a child of the specified parent. If no parent or an invalid parent is given, the Geom is placed at the top level
        \code{.cpp}
        //==== Add Wing Geometry ====//
        string wing_id = AddGeom( "WING" );
        \endcode
        \param [in] type Geom type (i.e FUSELAGE, POD, etc.)
        \param [in] parent Parent Geom ID
        \return Geom ID
    */
    string AddGeom(const string &type, const string &parent)
    {
        Vehicle *veh = GetVehicle();

        string ret_id;

        //==== Find Type Index ===//
        int type_index = -1;
        for (int i = 0; i < veh->GetNumGeomTypes(); i++)
        {
            if (veh->GetGeomType(i).m_Name == type)
            {
                type_index = i;
                break;
            }
        }

        if (type_index == -1)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_TYPE, "AddGeom::Can't Find Type Name " + type);
            return ret_id;
        }

        Geom *parent_geom = NULL;
        if (parent.size() > 0)
        {
            parent_geom = veh->FindGeom(parent);
            if (!parent_geom)
            {
                ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "AddGeom::Can't Find Parent " + parent);
                return ret_id;
            }
        }

        if (parent_geom)
        {
            veh->SetActiveGeom(parent);
        }
        else
        {
            veh->ClearActiveGeom();
        }

        ret_id = veh->AddGeom(veh->GetGeomType(type_index));

        Geom *added_geom = veh->FindGeom(ret_id);

        if (!added_geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "AddGeom::Failed To Add Geom");
            return ret_id;
        }

        ErrorMgr.NoError();
        return ret_id;
    }

    /*!
        Perform an update for the specified Geom
        \code{.cpp}
        //==== Add Wing Geometry ====//
        string wing_id = AddGeom( "WING" );

        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        SetParmVal( pod_id, "X_Rel_Location", "XForm", 5.0 );

        UpdateGeom( pod_id ); // Faster than updating the whole vehicle
        \endcode
        \sa Update()
        \param [in] geom_id Geom ID
    */

    void UpdateGeom(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();

        veh->UpdateGeom(geom_id);

        ErrorMgr.NoError();
    }

    /*!
        Delete a particular Geom
        \code{.cpp}
        //==== Add Wing Geometry ====//
        string wing_id = AddGeom( "WING" );

        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        DeleteGeom( wing_id );
        \endcode
        \param [in] geom_id Geom ID
    */

    void DeleteGeom(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();

        veh->DeleteGeom(geom_id);

        ErrorMgr.NoError();
    }

    /*!
        Delete multiple Geoms
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pid = AddGeom( "POD", "" );

        string rid = ExecAnalysis( "CompGeom" );

        array<string>@ mesh_id_vec = GetStringResults( rid, "Mesh_GeomID" );

        DeleteGeomVec( mesh_id_vec );
        \endcode
        \param [in] del_arr Array of Geom IDs
    */

    void DeleteGeomVec(const vector<string> &del_vec)
    {
        Vehicle *veh = GetVehicle();

        veh->DeleteGeomVec(del_vec);

        ErrorMgr.NoError();
    }

    /// Cut geometry and place it in the clipboard.  The clipboard is cleared before
    /// the cut geom is placed there.

    /*!
        Cut Geom from current location and store on clipboard
        \code{.cpp}
        //==== Add Pod Geometries ====//
        string pid1 = AddGeom( "POD", "" );
        string pid2 = AddGeom( "POD", "" );

        CutGeomToClipboard( pid1 );

        PasteGeomClipboard( pid2 ); // Paste Pod 1 as child of Pod 2

        array< string > @geom_ids = FindGeoms();

        if ( geom_ids.size() != 2 )                { Print( "---> Error: API Cut/Paste Geom  " ); }
        \endcode
        \sa PasteGeomClipboard
        \param [in] geom_id Geom ID
    */

    void CutGeomToClipboard(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();

        veh->SetActiveGeom(geom_id);
        veh->CutActiveGeomVec();

        ErrorMgr.NoError();
    }

    /// Copy geometry and place it in the clipboard.  The clipboard is cleared before
    /// the geometry is placed there.

    /*!
        Copy Geom from current location and store on clipboard
        \code{.cpp}
        //==== Add Pod Geometries ====//
        string pid1 = AddGeom( "POD", "" );
        string pid2 = AddGeom( "POD", "" );

        CopyGeomToClipboard( pid1 );

        PasteGeomClipboard( pid2 ); // Paste Pod 1 as child of Pod 2

        array< string > @geom_ids = FindGeoms();

        if ( geom_ids.size() != 3 )                { Print( "---> Error: API Copy/Paste Geom  " ); }
        \endcode
        \sa PasteGeomClipboard
        \param [in] geom_id Geom ID
    */

    void CopyGeomToClipboard(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();

        veh->SetActiveGeom(geom_id);
        veh->CopyActiveGeomVec();

        ErrorMgr.NoError();
    }

    /// Paste the geometry in the clipboard to the vehicle.  The geometry is inserted
    /// as a child of the optional parent id

    /*!
        Paste Geom from clipboard into the model. The Geom is pasted as a child of the specified parent, but will be placed at top level if no parent or an invalid one is provided.
        \code{.cpp}
        //==== Add Pod Geometries ====//
        string pid1 = AddGeom( "POD", "" );
        string pid2 = AddGeom( "POD", "" );

        CutGeomToClipboard( pid1 );

        PasteGeomClipboard( pid2 ); // Paste Pod 1 as child of Pod 2

        array< string > @geom_ids = FindGeoms();

        if ( geom_ids.size() != 2 )                { Print( "---> Error: API Cut/Paste Geom  " ); }
        \endcode
        \param [in] parent_id Parent Geom ID
        \return Array of pasted Geom IDs
    */

    vector<string> PasteGeomClipboard(const string &parent)
    {
        Vehicle *veh = GetVehicle();

        Geom *parent_geom = NULL;
        if (parent.size() > 0)
        {
            parent_geom = veh->FindGeom(parent);
            if (!parent_geom)
            {
                ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "PasteGeomClipboard::Can't Find Parent " + parent);
            }
        }

        if (parent_geom)
        {
            veh->SetActiveGeom(parent);
        }
        else
        {
            veh->ClearActiveGeom();
        }

        vector<string> pasted_ids = veh->PasteClipboard();
        ErrorMgr.NoError();

        return pasted_ids;
    }

    /*!
        Find and return all Geom IDs in the model
        \code{.cpp}
        //==== Add Pod Geometries ====//
        string pod1 = AddGeom( "POD", "" );
        string pod2 = AddGeom( "POD", "" );

        //==== There Should Be Two Geoms =====//
        array< string > @geom_ids = FindGeoms();

        if ( geom_ids.size() != 2 )                        { Print( "---> Error: API FindGeoms " ); }
        \endcode
        \return Array of all Geom IDs
    */

    vector<string> FindGeoms()
    {
        Vehicle *veh = GetVehicle();

        vector<string> ret_vec = veh->GetGeomVec();

        ErrorMgr.NoError();
        return ret_vec;
    }

    /*!
        Find and return all Geom IDs with the specified name
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pid = AddGeom( "POD", "" );

        SetGeomName( pid, "ExamplePodName" );

        array< string > @geom_ids = FindGeomsWithName( "ExamplePodName" );

        if ( geom_ids.size() != 1 )
        {
            Print( "---> Error: API FindGeomsWithName " );
        }
        \endcode
        \sa FindGeom
        \param [in] name Geom name
        \return Array of Geom IDs
    */

    vector<string> FindGeomsWithName(const string &name)
    {
        vector<string> ret_vec;
        Vehicle *veh = GetVehicle();

        vector<string> geom_id_vec = veh->GetGeomVec();
        for (int i = 0; i < (int)geom_id_vec.size(); i++)
        {
            Geom *gptr = veh->FindGeom(geom_id_vec[i]);
            if (gptr && gptr->GetName() == name)
            {
                ret_vec.push_back(geom_id_vec[i]);
            }
        }
        ErrorMgr.NoError();
        return ret_vec;
    }

    /*!
        Find and return the Geom ID with the specified name at given index. Equivalent to FindGeomsWithName( name )[index].
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pid = AddGeom( "POD", "" );

        SetGeomName( pid, "ExamplePodName" );

        string geom_id = FindGeom( "ExamplePodName", 0 );

        array< string > @geom_ids = FindGeomsWithName( "ExamplePodName" );

        if ( geom_ids[0] != geom_id )
        {
            Print( "---> Error: API FindGeom & FindGeomsWithName" );
        }
        \endcode
        \sa FindGeomsWithName
        \param [in] name Geom name
        \param [in] index
        \return Geom ID with name at specified index
    */

    string FindGeom(const string &name, int index)
    {
        vector<string> id_vec;
        Vehicle *veh = GetVehicle();

        vector<string> geom_id_vec = veh->GetGeomVec();
        for (int i = 0; i < (int)geom_id_vec.size(); i++)
        {
            Geom *gptr = veh->FindGeom(geom_id_vec[i]);
            if (gptr && gptr->GetName() == name)
            {
                id_vec.push_back(geom_id_vec[i]);
            }
        }

        if (index < 0 || index >= (int)id_vec.size())
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "FindGeom::Can't Find Name " + name + " or Index" + to_string((long long)index));
            return string();
        }
        ErrorMgr.NoError();
        return id_vec[index];
    }

    /*!
        Set the name of the specified Geom
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pid = AddGeom( "POD", "" );

        SetGeomName( pid, "ExamplePodName" );

        array< string > @geom_ids = FindGeomsWithName( "ExamplePodName" );

        if ( geom_ids.size() != 1 )
        {
            Print( "---> Error: API FindGeomsWithName " );
        }
        \endcode
        \param [in] geom_id Geom ID
        \param [in] name Geom name
    */

    void SetGeomName(const string &geom_id, const string &name)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetGeomName::Can't Find Geom " + geom_id);
            return;
        }
        geom_ptr->SetName(name);
        ErrorMgr.NoError();
    }

    /*!
        Get the name of a specific Geom
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pid = AddGeom( "POD", "" );

        SetGeomName( pid, "ExamplePodName" );

        string name_str = "Geom Name: " + GetGeomName( pid );

        Print( name_str );
        \endcode
        \param [in] geom_id Geom ID
        \return Geom name
    */

    string GetGeomName(const string &geom_id)
    {
        string ret_name;
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetGeomName::Can't Find Geom " + geom_id);
            return string();
        }
        ret_name = geom_ptr->GetName();
        ErrorMgr.NoError();
        return ret_name;
    }

    /*!
        Get the VSP surface type of the specified Geom (i.e DISK_SURF)
        \code{.cpp}
        //==== Add Wing Geometry ====//
        string wing_id = AddGeom( "WING" );

        if ( GetGeomVSPSurfType( wing_id ) != WING_SURF )
        {
            Print( "---> Error: API GetGeomVSPSurfType " );
        }
        \endcode
        \sa VSP_SURF_TYPE
        \param [in] geom_id Geom ID
        \param [in] main_surf_ind Main surface index
        \return VSP surface type enum (i.e. DISK_SURF)
    */

    int GetGeomVSPSurfType(const string &geom_id, int main_surf_ind)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetGeomVSPSurfType::Can't Find Geom " + geom_id);
            return -1;
        }

        int nms = geom_ptr->GetNumMainSurfs();

        if (main_surf_ind < 0 || main_surf_ind >= nms)
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetGeomVSPSurfType::Main Surf Index " + to_string(main_surf_ind) + " Out of Range");
        }

        return geom_ptr->GetMainSurfType(main_surf_ind);
    }

    /*!
        Get the VSP surface CFD type of the specified Geom (i.e TRANSPARENT_SURF)
        \code{.cpp}
        //==== Add Wing Geometry ====//
        string wing_id = AddGeom( "WING" );

        if ( GetGeomVSPSurfCfdType( wing_id ) != CFD_NORMAL )
        {
            Print( "---> Error: API GetGeomVSPSurfCfdType " );
        }
        \endcode
        \sa VSP_SURF_CFD_TYPE
        \param [in] geom_id Geom ID
        \param [in] main_surf_ind Main surface index
        \return VSP surface CFD type enum (i.e. CFD_TRANSPARENT)
    */
    int GetGeomVSPSurfCfdType(const string &geom_id, int main_surf_ind)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetGeomVSPSurfCfdType::Can't Find Geom " + geom_id);
            return -1;
        }

        if (main_surf_ind < 0 || main_surf_ind >= geom_ptr->GetNumMainSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetGeomVSPSurfCfdType::Main Surf Index " + to_string(main_surf_ind) + " Out of Range");
        }

        return geom_ptr->GetMainCFDSurfType(main_surf_ind);
    }

    /*!
        Get all Parm IDs associated with this Geom Parm container
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pid = AddGeom( "POD", "" );

        Print( string( "---> Test Get Parm Arrays" ) );

        array< string > @parm_array = GetGeomParmIDs( pid );

        if ( parm_array.size() < 1 )            { Print( "---> Error: API GetGeomParmIDs " ); }
        \endcode
        \param [in] geom_id Geom ID
        \return Array of Parm IDs
    */

    vector<string> GetGeomParmIDs(const string &geom_id)
    {
        vector<string> parm_vec;

        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetGeomParmIDs::Can't Find Geom " + geom_id);
            return parm_vec;
        }

        geom_ptr->AddLinkableParms(parm_vec);

        ErrorMgr.NoError();
        return parm_vec;
    }

    /*!
        Get the type name of specified Geom (i.e. FUSELAGE)
        \code{.cpp}
        //==== Add Wing Geometry ====//
        string wing_id = AddGeom( "WING" );

        Print( "Geom Type Name: ", false );

        Print( GetGeomTypeName( wing_id ) );
        \endcode
        \param [in] geom_id Geom ID
        \return Geom type name
    */

    string GetGeomTypeName(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GeomGeomTypeName::Can't Find Geom " + geom_id);
            return string();
        }

        string typ = string(geom_ptr->GetType().m_Name);
        return typ;
    }

    /*!
        Get Parm ID
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pid = AddGeom( "POD" );

        string lenid = GetParm( pid, "Length", "Design" );

        if ( !ValidParm( lenid ) )                { Print( "---> Error: API GetParm  " ); }
        \endcode
        \param [in] container_id Container ID
        \param [in] name Parm name
        \param [in] group Parm group name
        \return Array of Parm ID
    */

    /// Get the parm id given container id, parm name, and group name
    string GetParm(const string &container_id, const string &name, const string &group)
    {
        Vehicle *veh = GetVehicle();

        if (ParmMgr.GetDirtyFlag())
        {
            LinkMgr.BuildLinkableParmData(); // Make Sure Name/Group Get Mapped To Parms
        }

        string parm_id;

        ParmContainer *pc = ParmMgr.FindParmContainer(container_id);
        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetParm::Can't Find Container " + container_id);
            return parm_id;
        }

        parm_id = pc->FindParm(name, group);

        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetParm::Can't Find Parm " + container_id + ":" + group + ":" + name);
            return parm_id;
        }
        ErrorMgr.NoError();
        return parm_id;
    }

    /*!
        Get the parent Geom ID for the input child Geom. "NONE" is returned if the Geom has no parent.
        \code{.cpp}
        //==== Add Parent and Child Geometry ====//
        string pod1 = AddGeom( "POD" );

        string pod2 = AddGeom( "POD", pod1 );

        Print( "Parent ID of Pod #2: ", false );

        Print( GetGeomParent( pod2 ) );
        \endcode
        \param [in] geom_id Geom ID
        \return Parent Geom ID
    */

    string GetGeomParent(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetGeomParent::Can't Find Geom " + geom_id);
            return string();
        }

        return geom_ptr->GetParentID();
    }

    /*!
        Get the IDs for each child of the input parent Geom.
        \code{.cpp}
        //==== Add Parent and Child Geometry ====//
        string pod1 = AddGeom( "POD" );

        string pod2 = AddGeom( "POD", pod1 );

        string pod3 = AddGeom( "POD", pod2 );

        Print( "Children of Pod #1: " );

        array<string> children = GetGeomChildren( pod1 );

        for ( int i = 0; i < int( children.size() ); i++ )
        {
            Print( "\t", false );
            Print( children[i] );
        }
        \endcode
        \param [in] geom_id Geom ID
        \return Array of child Geom IDs
    */

    vector<string> GetGeomChildren(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetGeomChildren::Can't Find Geom " + geom_id);
            return vector<string>{};
        }

        return geom_ptr->GetChildIDVec();
    }

    /*!
        Get the number of XSecSurfs for the specified Geom
        \code{.cpp}
        //==== Add Fuselage Geometry ====//
        string fuseid = AddGeom( "FUSELAGE", "" );

        int num_xsec_surfs = GetNumXSecSurfs( fuseid );

        if ( num_xsec_surfs != 1 )                { Print( "---> Error: API GetNumXSecSurfs  " ); }
        \endcode
        \param [in] geom_id Geom ID
        \return Number of XSecSurfs
    */

    int GetNumXSecSurfs(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetNumXSecSurfs::Can't Find Geom " + geom_id);
            return 0;
        }

        ErrorMgr.NoError();
        return geom_ptr->GetNumXSecSurfs();
    }

    /*!
        Get the number of main surfaces for the specified Geom. Multiple main surfaces may exist for CustoGeoms, propellors, etc., but
        does not include surfaces created due to symmetry.
        \code{.cpp}
        //==== Add Prop Geometry ====//
        string prop_id = AddGeom( "PROP" );

        int num_surf = 0;

        num_surf = GetNumMainSurfs( prop_id ); // Should be the same as the number of blades

        Print( "Number of Propeller Surfaces: ", false );

        Print( num_surf );
        \endcode
        \param [in] geom_id Geom ID
        \return Number of main surfaces
    */

    int GetNumMainSurfs(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetNumMainSurfs::Can't Find Geom " + geom_id);
            return 0;
        }

        ErrorMgr.NoError();
        return geom_ptr->GetNumMainSurfs();
    }

    /*!
        Get the total number of surfaces for the specified Geom. This is equivalent to the number of main surface multiplied
        by the number of symmetric copies.
        \code{.cpp}
        //==== Add Wing Geometry ====//
        string wing_id = AddGeom( "WING" );

        int num_surf = 0;

        num_surf = GetTotalNumSurfs( wing_id ); // Wings default with XZ symmetry on -> 2 surfaces

        Print( "Total Number of Wing Surfaces: ", false );

        Print( num_surf );
        \endcode
        \param [in] geom_id Geom ID
        \return Number of main surfaces
    */

    int GetTotalNumSurfs(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetTotalNumSurfs::Can't Find Geom " + geom_id);
            return 0;
        }

        ErrorMgr.NoError();
        return geom_ptr->GetNumTotalSurfs();
    }

    /*!
        Get the the maximum coordinate of the bounding box of a Geom with given main surface index. The Geom bounding
        box may be specified in absolute or body reference frame.
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pid = AddGeom( "POD" );

        SetParmVal( FindParm( pid, "Y_Rotation", "XForm" ), 45 );
        SetParmVal( FindParm( pid, "Z_Rotation", "XForm" ), 25 );

        Update();

        vec3d max_pnt = GetGeomBBoxMax( pid, 0, false );
        \endcode
        \sa GetGeomBBoxMin
        \param [in] geom_id Geom ID
        \param [in] main_surf_ind Main surface index
        \param [in] ref_frame_is_absolute Flag to specify absolute or body reference frame
        \return Maximum coordinate of the bounding box
    */

    vec3d GetGeomBBoxMax(const string &geom_id, int main_surf_ind, bool ref_frame_is_absolute)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetGeomBBoxMax::Can't Find Geom " + geom_id);
            return vec3d();
        }

        vector<VspSurf> surf_vec;
        surf_vec = geom_ptr->GetSurfVecConstRef();

        if (main_surf_ind < 0 || main_surf_ind >= surf_vec.size())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetGeomBBoxMax::Main Surf Index " + to_string(main_surf_ind) + " Out of Range");
        }

        VspSurf current_surf = surf_vec[main_surf_ind];

        // Determine BndBox dimensions prior to rotating and translating
        Matrix4d model_matrix = geom_ptr->getModelMatrix();
        model_matrix.affineInverse();

        VspSurf orig_surf = current_surf;
        orig_surf.Transform(model_matrix);

        BndBox bbox;

        if (!ref_frame_is_absolute)
        {
            orig_surf.GetBoundingBox(bbox);
        }
        else
        {
            current_surf.GetBoundingBox(bbox);
        }

        return bbox.GetMax();
    }

    /*!
        Get the the minimum coordinate of the bounding box of a Geom with given main surface index. The Geom bounding
        box may be specified in absolute or body reference frame.
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pid = AddGeom( "POD" );

        SetParmVal( FindParm( pid, "Y_Rotation", "XForm" ), 45 );
        SetParmVal( FindParm( pid, "Z_Rotation", "XForm" ), 25 );

        Update();

        vec3d min_pnt = GetGeomBBoxMin( pid, 0, false );
        \endcode
        \sa GetGeomBBoxMax
        \param [in] geom_id Geom ID
        \param [in] main_surf_ind Main surface index
        \param [in] ref_frame_is_absolute Flag to specify absolute or body reference frame
        \return Minimum coordinate of the bounding box
    */

    vec3d GetGeomBBoxMin(const string &geom_id, int main_surf_ind, bool ref_frame_is_absolute)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetGeomBBoxMin::Can't Find Geom " + geom_id);
            return vec3d();
        }

        vector<VspSurf> surf_vec;
        surf_vec = geom_ptr->GetSurfVecConstRef();

        if (main_surf_ind < 0 || main_surf_ind >= surf_vec.size())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetGeomBBoxMin::Main Surf Index " + to_string(main_surf_ind) + " Out of Range");
        }

        VspSurf current_surf = surf_vec[main_surf_ind];

        // Determine BndBox dimensions prior to rotating and translating
        Matrix4d model_matrix = geom_ptr->getModelMatrix();
        model_matrix.affineInverse();

        VspSurf orig_surf = current_surf;
        orig_surf.Transform(model_matrix);

        BndBox bbox;

        if (!ref_frame_is_absolute)
        {
            orig_surf.GetBoundingBox(bbox);
        }
        else
        {
            current_surf.GetBoundingBox(bbox);
        }

        return bbox.GetMin();
    }

    /*!
        Add a sub-surface to the specified Geom
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        // Note: Parm Group for SubSurfaces in the form: "SS_" + type + "_" + count (initialized at 1)
        string ss_line_id = AddSubSurf( wid, SS_LINE );                      // Add Sub Surface Line

        SetParmVal( wid, "Const_Line_Value", "SubSurface_1", 0.4 );     // Change Location
        \endcode
        \sa SUBSURF_TYPE
        \param [in] geom_id Geom ID
        \param [in] type Sub-surface type enum (i.e. SS_RECTANGLE)
        \param [in] surfindex Main surface index (default: 0)
        \return Sub-surface ID
    */

    string AddSubSurf(const string &geom_id, int type, int surfindex)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddSubSurf::Can't Find Geom " + geom_id);
            return string();
        }

        SubSurface *ssurf = NULL;
        ssurf = geom_ptr->AddSubSurf(type, surfindex);
        if (!ssurf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddSubSurf::Invalid Sub Surface Ptr ");
            return string();
        }
        ssurf->Update();
        ErrorMgr.NoError();
        return ssurf->GetID();
    }

    /*!
        Get the ID of the specified sub-surface
        \code{.cpp}
        string wid = AddGeom( "WING", "" ); // Add Wing

        string ss_rec_1 = AddSubSurf( wid, SS_RECTANGLE ); // Add Sub Surface Rectangle #1

        string ss_rec_2 = AddSubSurf( wid, SS_RECTANGLE ); // Add Sub Surface Rectangle #2

        Print( ss_rec_2, false );

        Print( " = ", false );

        Print( GetSubSurf( wid, 1 ) );
        \endcode
        \param [in] geom_id Geom ID
        \param [in] index Sub-surface index
        \return Sub-surface ID
    */

    string GetSubSurf(const string &geom_id, int index)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurf::Can't Find Geom " + geom_id);
            return string();
        }
        SubSurface *ssurf = NULL;
        ssurf = geom_ptr->GetSubSurf(index);
        if (!ssurf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurf::Invalid Sub Surface Ptr ");
            return string();
        }
        ErrorMgr.NoError();
        return ssurf->GetID();
    }

    /*!
        Get all sub-surface IDs with specified parent Geom and name
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        rec_name = GetSubSurfName( ss_rec_id );

        id_vec = GetSubSurf( wid, rec_name );

        string ID_str = string("IDs of subsurfaces named \"") + rec_name + string("\": ") + id_vec[0];

        Print( ID_str );
        \endcode
        \param [in] geom_id Geom ID
        \param [in] name Sub-surface name
        \return Array of sub-surface IDs
    */

    std::vector<std::string> GetSubSurf(const string &geom_id, const string &name)
    {
        vector<string> ID_vec;
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurf::Can't Find Geom " + geom_id);
            return ID_vec;
        }
        vector<SubSurface *> ss_vec = geom_ptr->GetSubSurfVec();
        for (size_t i = 0; i < ss_vec.size(); i++)
        {
            if (strcmp(ss_vec[i]->GetName().c_str(), name.c_str()) == 0)
            {
                ID_vec.push_back(ss_vec[i]->GetID());
            }
        }

        if (ID_vec.size() == 0)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetSubSurf::Can't Find Sub Surface with Name " + name);
            return ID_vec;
        }
        ErrorMgr.NoError();
        return ID_vec;
    }

    /*!
        Delete the specified sub-surface
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_line_id = AddSubSurf( wid, SS_LINE );                      // Add Sub Surface Line
        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        Print("Delete SS_Line\n");

        DeleteSubSurf( wid, ss_line_id );

        int num_ss = GetNumSubSurf( wid );

        string num_str = string("Number of SubSurfaces: ") + formatInt( num_ss, '' ) + string("\n");

        Print( num_str );
        \endcode
        \param [in] geom_id Geom ID
        \param [in] sub_id Sub-surface ID
    */

    // TODO: Why are there two DeleteSubSurf if Geom ID isn't needed?
    void DeleteSubSurf(const string &geom_id, const string &sub_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteSubSurf::Can't Find Geom " + geom_id);
            return;
        }
        int index = geom_ptr->GetSubSurfIndex(sub_id);
        if (index == -1)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteSubSurf::Can't Find SubSurf " + sub_id);
            return;
        }
        geom_ptr->DelSubSurf(index);
        ErrorMgr.NoError();
        return;
    }

    /*!
        Delete the specified sub-surface
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_line_id = AddSubSurf( wid, SS_LINE );                      // Add Sub Surface Line
        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        Print("Delete SS_Line\n");

        DeleteSubSurf( ss_line_id );

        int num_ss = GetNumSubSurf( wid );

        string num_str = string("Number of SubSurfaces: ") + formatInt( num_ss, '' ) + string("\n");

        Print( num_str );
        \endcode
        \param [in] sub_id Sub-surface ID
    */

    void DeleteSubSurf(const string &sub_id)
    {
        SubSurface *ss_ptr = SubSurfaceMgr.GetSubSurf(sub_id);
        if (!ss_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteSubSurf::Can't Find SubSurf " + sub_id);
            return;
        }
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(ss_ptr->GetCompID());
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteSubSurf::Can't Find Geom " + ss_ptr->GetCompID());
            return;
        }
        int index = geom_ptr->GetSubSurfIndex(sub_id);
        if (index == -1)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteSubSurf::Can't Find SubSurf " + sub_id);
            return;
        }
        geom_ptr->DelSubSurf(index);
        ErrorMgr.NoError();
        return;
    }

    /*!
        Set the name of the specified sub-surface
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        string new_name = string("New_SS_Rec_Name");

        SetSubSurfName( wid, ss_rec_id, new_name );
        \endcode
        \param [in] geom_id Geom ID
        \param [in] sub_id Sub-surface ID
        \param [in] name Sub-surface name
    */

    // TODO: Why are there two SetSubSurfName if Geom ID isn't needed?
    void SetSubSurfName(const std::string &geom_id, const std::string &sub_id, const std::string &name)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetSubSurfName::Can't Find Geom " + geom_id);
            return;
        }
        SubSurface *ssurf = NULL;
        ssurf = geom_ptr->GetSubSurf(sub_id);
        if (!ssurf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetSubSurfName::Invalid Sub Surface Ptr " + sub_id);
            return;
        }
        ssurf->SetName(name);
        ErrorMgr.NoError();
        return;
    }

    /*!
        Set the name of the specified sub-surface
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        string new_name = string("New_SS_Rec_Name");

        SetSubSurfName( ss_rec_id, new_name );
        \endcode
        \param [in] sub_id Sub-surface ID
        \param [in] name Sub-surface name
    */

    void SetSubSurfName(const std::string &sub_id, const std::string &name)
    {
        SubSurface *ss_ptr = SubSurfaceMgr.GetSubSurf(sub_id);
        if (!ss_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetSubSurfName::Invalid Sub Surface Ptr " + sub_id);
            return;
        }
        ss_ptr->SetName(name);
        ErrorMgr.NoError();
        return;
    }

    /*!
        Get the name of the specified sub-surface
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        string rec_name = GetSubSurfName( wid, ss_rec_id );

        string name_str = string("Current Name of SS_Rectangle: ") + rec_name + string("\n");

        Print( name_str );
        \endcode
        \param [in] geom_id Geom ID
        \param [in] sub_id Sub-surface ID
        \return Sub-surface name
    */

    std::string GetSubSurfName(const std::string &geom_id, const std::string &sub_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurfName::Can't Find Geom " + geom_id);
            return string();
        }
        SubSurface *ssurf = NULL;
        ssurf = geom_ptr->GetSubSurf(sub_id);
        if (!ssurf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurfName::Invalid Sub Surface Ptr " + sub_id);
            return string();
        }
        ErrorMgr.NoError();
        return ssurf->GetName();
    }

    /*!
        Get the name of the specified sub-surface
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        string rec_name = GetSubSurfName( ss_rec_id );

        string name_str = string("Current Name of SS_Rectangle: ") + rec_name + string("\n");

        Print( name_str );
        \endcode
        \param [in] sub_id Sub-surface ID
        \return Sub-surface name
    */

    std::string GetSubSurfName(const std::string &sub_id)
    {
        SubSurface *ssurf = SubSurfaceMgr.GetSubSurf(sub_id);
        if (!ssurf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurfName::Invalid Sub Surface Ptr " + sub_id);
            return string();
        }
        ErrorMgr.NoError();
        return ssurf->GetName();
    }

    /*!
        Get the index of the specified sub-surface in its parent Geom's sub-surface vector
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_line_id = AddSubSurf( wid, SS_LINE );                      // Add Sub Surface Line
        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        int ind = GetSubSurfIndex( ss_rec_id );

        string ind_str = string("Index of SS_Rectangle: ") + ind + string("\n");

        Print( ind_str );
        \endcode
        \param [in] sub_id Sub-surface ID
        \return Sub-surface index
    */

    int GetSubSurfIndex(const std::string &sub_id)
    {
        SubSurface *ss_ptr = SubSurfaceMgr.GetSubSurf(sub_id);
        if (!ss_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurfIndex::Invalid Sub Surface Ptr " + sub_id);
            return -1;
        }
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(ss_ptr->GetCompID());
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurfIndex::Can't Find Geom " + ss_ptr->GetCompID());
            return -1;
        }

        int ss_ind = geom_ptr->GetSubSurfIndex(sub_id);

        ErrorMgr.NoError();
        return ss_ind;
    }

    /*!
        Get a vector of all sub-surface IDs for the specified geometry
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_line_id = AddSubSurf( wid, SS_LINE );                      // Add Sub Surface Line
        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        array<string> id_vec = GetSubSurfIDVec( wid );

        string id_type_str = string( "SubSurface IDs and Type Indexes -> ");

        for ( uint i = 0; i < uint(id_vec.length()); i++ )
        {
            id_type_str += id_vec[i];

            id_type_str += string(": ");

            id_type_str += GetSubSurfType(id_vec[i]);

            id_type_str += string("\t");
        }

        id_type_str += string("\n");

        Print( id_type_str );
        \endcode
        \param [in] geom_id Geom ID
        \return Array of sub-surface IDs
    */

    std::vector<std::string> GetSubSurfIDVec(const std::string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurfIDVec::Can't Find Geom " + geom_id);
            return vector<string>();
        }

        vector<SubSurface *> ss_vec = geom_ptr->GetSubSurfVec();
        vector<string> ID_vec;
        ID_vec.resize(ss_vec.size());

        for (size_t i = 0; i < ss_vec.size(); i++)
        {
            ID_vec[i] = ss_vec[i]->GetID();
        }

        ErrorMgr.NoError();
        return ID_vec;
    }

    /*!
        Get a vector of all sub-surface IDs for the entire model
        \return Array of sub-surface IDs
    */

    std::vector<std::string> GetAllSubSurfIDs()
    {
        vector<SubSurface *> ss_vec = SubSurfaceMgr.GetSubSurfs();
        vector<string> ID_vec;
        ID_vec.resize(ss_vec.size());

        for (size_t i = 0; i < ss_vec.size(); i++)
        {
            ID_vec[i] = ss_vec[i]->GetID();
        }

        ErrorMgr.NoError();
        return ID_vec;
    }

    /*!
        Get the number of sub-surfaces for the specified Geom
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_line_id = AddSubSurf( wid, SS_LINE );                      // Add Sub Surface Line
        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        int num_ss = GetNumSubSurf( wid );

        string num_str = string("Number of SubSurfaces: ") + num_ss + string("\n");

        Print( num_str );
        \endcode
        \param [in] geom_id Geom ID
        \return Number of Sub-surfaces
    */

    int GetNumSubSurf(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetNumSubSurf::Can't Find Geom " + geom_id);
            return -1;
        }
        ErrorMgr.NoError();
        return geom_ptr->NumSubSurfs();
    }

    /*!
        Get the type for the specified sub-surface (i.e. SS_RECTANGLE)
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_line_id = AddSubSurf( wid, SS_LINE );                      // Add Sub Surface Line
        string ss_rec_id = AddSubSurf( wid, SS_RECTANGLE );                        // Add Sub Surface Rectangle

        array<string> id_vec = GetSubSurfIDVec( wid );

        string id_type_str = string( "SubSurface IDs and Type Indexes -> ");

        for ( uint i = 0; i < uint(id_vec.length()); i++ )
        {
            id_type_str += id_vec[i];

            id_type_str += string(": ");

            id_type_str += GetSubSurfType(id_vec[i]);

            id_type_str += string("\t");
        }

        id_type_str += string("\n");

        Print( id_type_str );
        \endcode
        \sa SUBSURF_TYPE
        \param [in] sub_id Sub-surface ID
        \return Sub-surface type enum (i.e. SS_RECTANGLE)
    */

    int GetSubSurfType(const string &sub_id)
    {
        SubSurface *ssurf = SubSurfaceMgr.GetSubSurf(sub_id);
        if (!ssurf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurfType::Invalid Sub Surface Ptr " + sub_id);
            return -1;
        }
        ErrorMgr.NoError();
        return ssurf->GetType();
    }

    /*!
        Get the vector of Parm IDs for specified sub-surface
        \code{.cpp}
        string wid = AddGeom( "WING", "" );                             // Add Wing

        string ss_line_id = AddSubSurf( wid, SS_LINE );                      // Add Sub Surface Line

        // Get and list all Parm info for SS_Line
        array<string> parm_id_vec = GetSubSurfParmIDs( ss_line_id );

        for ( uint i = 0; i < uint(parm_id_vec.length()); i++ )
        {
            string id_name_str = string("\tName: ") + GetParmName( parm_id_vec[i] ) + string(", Group: ") + GetParmDisplayGroupName( parm_id_vec[i] ) +
                string(", ID: ") + parm_id_vec[i] + string("\n");

            Print( id_name_str );
        }
        \endcode
        \param [in] sub_id Sub-surface ID
        \return Array of Parm IDs
    */

    std::vector<std::string> GetSubSurfParmIDs(const string &sub_id)
    {
        vector<string> parm_vec;

        Vehicle *veh = GetVehicle();
        SubSurface *ss_ptr = SubSurfaceMgr.GetSubSurf(sub_id);
        if (!ss_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSubSurfParmIDs::Can't Find SubSurface " + sub_id);
            return parm_vec;
        }

        ss_ptr->AddLinkableParms(parm_vec);

        ErrorMgr.NoError();
        return parm_vec;
    }

    //**********************************************************************//
    //*****************     FEA Mesh API Functions     *********************//
    //**********************************************************************//

    /*!
        Add an FEA Structure to a specified Geom
        \warning init_skin should ALWAYS be set to true.
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] init_skin Flag to initialize the FEA Structure by creating an FEA Skin from the parent Geom's OML at surfindex
        \param [in] surfindex Main surface index for the FEA Structure
        \return FEA Structure index
    */

    /// Add an FeaStructure, return FeaStructure index
    int AddFeaStruct(const string &geom_id, bool init_skin, int surfindex)
    {
        StructureMgr.InitFeaProperties();

        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return -1;
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddFeaStruct::Can't Find Geom " + geom_id);
            return -1;
        }

        FeaStructure *feastruct = NULL;
        feastruct = geom_ptr->AddFeaStruct(init_skin, surfindex);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddFeaStruct::Invalid FeaStructure Ptr");
            return -1;
        }
        ErrorMgr.NoError();
        return (geom_ptr->NumGeomFeaStructs() - 1);
    }

    /*!
        Sets FeaMeshMgr m_FeaMeshStructIndex member using passed in index of a FeaStructure
        \code{.cpp}

        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        SetFeaMeshStructIndex( struct_ind );

        if ( FindGeoms().size() != 0 ) { Print( "ERROR: VSPRenew" ); }
        \endcode
    */

    void SetFeaMeshStructIndex(int struct_index)
    {
        vector<FeaStructure *> structVec = StructureMgr.GetAllFeaStructs();

        if (struct_index <= (int)structVec.size() && struct_index >= 0)
        {
            FeaMeshMgr.SetFeaMeshStructIndex(struct_index);
            ErrorMgr.NoError();
        }
        else
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "SetFeaMeshStructIndex::Index Out of Range");
        }
    }

    /*!
        Delete an FEA Structure and all FEA Parts and FEA SubSurfaces associated with it
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind_1 = AddFeaStruct( pod_id );

        int struct_ind_2 = AddFeaStruct( pod_id );

        DeleteFeaStruct( pod_id, struct_ind_1 );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
    */

    void DeleteFeaStruct(const string &geom_id, int fea_struct_ind)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return;
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteFeaStruct::Can't Find Geom " + geom_id);
            return;
        }
        if (!geom_ptr->ValidGeomFeaStructInd(fea_struct_ind))
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteFeaStruct::Can't Find FeaStructure at index " + to_string(fea_struct_ind));
            return;
        }
        geom_ptr->DeleteFeaStruct(fea_struct_ind);
        ErrorMgr.NoError();
        return;
    }

    /*!
        Get the ID of an FEA Structure
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        string struct_id = GetFeaStructID( pod_id, struct_ind );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
        \return FEA Structure ID
    */

    string GetFeaStructID(const string &geom_id, int fea_struct_ind)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return string();
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetFeaStructID::Can't Find Geom " + geom_id);
            return string();
        }

        FeaStructure *struct_ptr = geom_ptr->GetFeaStruct(fea_struct_ind);
        if (!struct_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetFeaStructID::Can't Find FeaStructure " + to_string((long long)fea_struct_ind));
            return string();
        }
        ErrorMgr.NoError();
        return struct_ptr->GetID();
    }

    /*!
        Get the index of an FEA Structure in its Parent Geom's vector of Structures
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind_1 = AddFeaStruct( pod_id );

        int struct_ind_2 = AddFeaStruct( pod_id );

        string struct_id_2 = GetFeaStructID( pod_id, struct_ind_2 );

        DeleteFeaStruct( pod_id, struct_ind_1 );

        int struct_ind_2_new = GetFeaStructIndex( struct_id_2 );
        \endcode
        \param [in] struct_id FEA Structure ID
        \return FEA Structure index
    */

    int GetFeaStructIndex(const string &struct_id)
    {
        return StructureMgr.GetGeomFeaStructIndex(struct_id);
    }

    /*!
        Get the Parent Geom ID for an FEA Structure
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        string struct_id = GetFeaStructID( pod_id, struct_ind );

        //==== Get Parent Geom ID and Index ====//
        string parent_id = GetFeaStructParentGeomID( struct_id );
        \endcode
        \param [in] struct_id FEA Structure ID
        \return Parent Geom ID
    */

    string GetFeaStructParentGeomID(const string &struct_id)
    {
        return StructureMgr.GetFeaStructParentID(struct_id);
    }

    /*!
        Get the name of an FEA Structure. The FEA Structure name functions as the the Parm Container name
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Get Structure Name ====//
        string parm_container_name = GetFeaStructName( pod_id, struct_ind );

        string display_name = string("Current Structure Parm Container Name: ") + parm_container_name + string("\n");

        Print( display_name );
        \endcode
        \sa FindContainer, SetFeaStructName
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
        \return Name for the FEA Structure
    */

    string GetFeaStructName(const string &geom_id, int fea_struct_ind)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return string();
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetFeaStructName::Can't Find Geom " + geom_id);
            return string();
        }

        FeaStructure *struct_ptr = geom_ptr->GetFeaStruct(fea_struct_ind);
        if (!struct_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetFeaStructName::Can't Find FeaStructure " + to_string((long long)fea_struct_ind));
            return string();
        }
        ErrorMgr.NoError();
        return struct_ptr->GetName();
    }

    /*!
        Set the name of an FEA Structure
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Change the Structure Name ====//
        SetFeaStructName( pod_id, struct_ind, "Example_Struct" );

        string parm_container_id = FindContainer( "Example_Struct", struct_ind );

        string display_id = string("New Structure Parm Container ID: ") + parm_container_id + string("\n");

        Print( display_id );
        \endcode
        \sa GetFeaStructName
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
        \param [in] name New name for the FEA Structure
    */

    void SetFeaStructName(const string &geom_id, int fea_struct_ind, const string &name)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return;
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetFeaStructName::Can't Find Geom " + geom_id);
            return;
        }

        FeaStructure *struct_ptr = geom_ptr->GetFeaStruct(fea_struct_ind);
        if (!struct_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetFeaStructName::Can't Find FeaStructure " + to_string((long long)fea_struct_ind));
            return;
        }
        struct_ptr->SetName(name);
        ErrorMgr.NoError();
        return;
    }

    /*!
        Set the name of an FEA Part
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Add Bulkead ====//
        string bulkhead_id = AddFeaPart( pod_id, struct_ind, FEA_SLICE );

        SetFeaPartName( bulkhead_id, "Bulkhead" );
        \endcode
        \sa GetFeaPartName
        \param [in] part_id FEA Part ID
        \param [in] name New name for the FEA Part
    */

    void SetFeaPartName(const string &part_id, const string &name)
    {
        FeaPart *part = StructureMgr.GetFeaPart(part_id);
        if (!part)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetFeaPartName::Can't Find FEA Part " + part_id);
            return;
        }

        part->SetName(name);
        ErrorMgr.NoError();
        return;
    }

    /*!
        Get the IDs of all FEA Structures in the vehicle
        \code{.cpp}
        //==== Add Geometries ====//
        string pod_id = AddGeom( "POD" );
        string wing_id = AddGeom( "WING" );

        //==== Add FeaStructures ====//
        int pod_struct_ind = AddFeaStruct( pod_id );
        int wing_struct_ind = AddFeaStruct( wing_id );

        array < string > struct_id_vec = GetFeaStructIDVec();
        \endcode
        \sa NumFeaStructures
        \return Array of FEA Structure IDs
    */

    vector<string> GetFeaStructIDVec()
    {
        vector<string> ret_vec;
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return ret_vec;
        }

        vector<FeaStructure *> struct_vec = StructureMgr.GetAllFeaStructs();
        ret_vec.resize(struct_vec.size());

        for (size_t i = 0; i < struct_vec.size(); i++)
        {
            ret_vec[i] = struct_vec[i]->GetID();
        }

        ErrorMgr.NoError();
        return ret_vec;
    }

    /*!
        Add an FEA Part to a Structure
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Add Bulkead ====//
        string bulkhead_id = AddFeaPart( pod_id, struct_ind, FEA_SLICE );

        SetParmVal( FindParm( bulkhead_id, "IncludedElements", "FeaPart" ), FEA_SHELL_AND_BEAM );

        SetParmVal( FindParm( bulkhead_id, "RelCenterLocation", "FeaPart" ), 0.15 );
        \endcode
        \sa FEA_PART_TYPE
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
        \param [in] type FEA Part type enum (i.e. FEA_RIB)
        \return FEA Part ID
    */

    /// Add a FeaPart, return FeaPart ID
    string AddFeaPart(const string &geom_id, int fea_struct_ind, int type)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return string();
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddFeaPart::Can't Find Geom " + geom_id);
            return string();
        }

        FeaStructure *feastruct = NULL;
        feastruct = geom_ptr->GetFeaStruct(fea_struct_ind);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddFeaPart::Invalid FeaStructure Ptr at Index " + to_string((long long)fea_struct_ind));
            return string();
        }

        FeaPart *feapart = NULL;
        feapart = feastruct->AddFeaPart(type);
        if (!feapart)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddFeaPart::Invalid FeaPart Ptr");
            return string();
        }
        feastruct->Update();
        ErrorMgr.NoError();
        return feapart->GetID();
    }

    /*!
        Delete an FEA Part from a Structure
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Add Bulkead ====//
        string bulkhead_id = AddFeaPart( pod_id, struct_ind, FEA_SLICE );

        //==== Add Fixed Point ====//
        string fixed_id = AddFeaPart( pod_id, struct_ind, FEA_FIX_POINT );

        //==== Delete Bulkead ====//
        DeleteFeaPart( pod_id, struct_ind, bulkhead_id );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
        \param [in] part_id FEA Part ID
    */

    void DeleteFeaPart(const string &geom_id, int fea_struct_ind, const string &part_id)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return;
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteFeaPart::Can't Find Geom " + geom_id);
            return;
        }

        FeaStructure *feastruct = NULL;
        feastruct = geom_ptr->GetFeaStruct(fea_struct_ind);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteFeaPart::Invalid FeaStructure Ptr at index " + to_string((long long)fea_struct_ind));
            return;
        }

        int index = StructureMgr.GetFeaPartIndex(part_id);
        if (index == -1)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteFeaPart::Can't Find FeaPart " + part_id);
            return;
        }
        feastruct->DelFeaPart(index);
        ErrorMgr.NoError();
        return;
    }

    /*!
        Get the Parm ID of an FEA Part, identified from a FEA Structure Parm ID and FEA Part index.
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        string struct_id = GetFeaStructID( pod_id, struct_ind );

        //==== Add Bulkead ====//
        string bulkhead_id = AddFeaPart( pod_id, struct_ind, FEA_SLICE );

        Update();

        if ( bulkhead_id != GetFeaPartID( struct_id, 1 ) ) // These should be equivalent (index 0 is skin)
        {
            Print( "Error: GetFeaPartID" );
        }
        \endcode
        \param [in] fea_struct_id FEA Structure ID
        \param [in] fea_part_index FEA Part index
        \return FEA Part ID
    */

    string GetFeaPartID(const string &fea_struct_id, int fea_part_index)
    {
        FeaStructure *feastruct = NULL;
        feastruct = StructureMgr.GetFeaStruct(fea_struct_id);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetFeaPartID::Invalid FeaStructure ID");
            return string();
        }

        FeaPart *part = feastruct->GetFeaPart(fea_part_index);

        if (!part)
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetFeaPartID::Invalid FeaPart Index");
            return string();
        }

        ErrorMgr.NoError();
        return part->GetID();
    }

    /*!
        Get the name of an FEA Part
        \code{.cpp}
        //==== Add Fuselage Geometry ====//
        string fuse_id = AddGeom( "FUSELAGE" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( fuse_id );

        //==== Add Bulkead ====//
        string bulkhead_id = AddFeaPart( fuse_id, struct_ind, FEA_SLICE );

        string name = "example_name";
        SetFeaPartName( bulkhead_id, name );

        if ( name != GetFeaPartName( bulkhead_id ) ) // These should be equivalent
        {
            Print( "Error: GetFeaPartName" );
        }
        \endcode
        \sa SetFeaPartName
        \param [in] part_id FEA Part ID
        \return FEA Part name
    */

    string GetFeaPartName(const string &part_id)
    {
        FeaPart *part = StructureMgr.GetFeaPart(part_id);
        if (!part)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetFeaPartName::Can't Find FEA Part " + part_id);
            return string();
        }

        ErrorMgr.NoError();
        return part->GetName();
    }

    /*!
        Get the type of an FEA Part
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Add Slice ====//
        string slice_id = AddFeaPart( pod_id, struct_ind, FEA_SLICE );

        if ( FEA_SLICE != GetFeaPartType( slice_id ) ) // These should be equivalent
        {
            Print( "Error: GetFeaPartType" );
        }
        \endcode
        \sa FEA_PART_TYPE
        \param [in] part_id FEA Part ID
        \return FEA Part type enum
    */

    int GetFeaPartType(const string &part_id)
    {
        FeaPart *part = StructureMgr.GetFeaPart(part_id);
        if (!part)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetFeaPartType::Can't Find FEA Part " + part_id);
            return -1;
        }

        ErrorMgr.NoError();
        return part->GetType();
    }

    /*!
        Set the ID of the perpendicular spar for an FEA Rib or Rib Array. Note, the FEA Rib or Rib Array should have "SPAR_NORMAL"
        set for the "PerpendicularEdgeType" Parm. If it is not, the ID will still be set, but the orientation of the Rib or Rib
        Array will not change.
        \code{.cpp}
        //==== Add Wing Geometry ====//
        string wing_id = AddGeom( "WING" );

        //==== Add FeaStructure to Wing ====//
        int struct_ind = AddFeaStruct( wing_id );

        //==== Add Rib ====//
        string rib_id = AddFeaPart( wing_id, struct_ind, FEA_RIB );

        //==== Add Spars ====//
        string spar_id_1 = AddFeaPart( wing_id, struct_ind, FEA_SPAR );
        string spar_id_2 = AddFeaPart( wing_id, struct_ind, FEA_SPAR );

        SetParmVal( FindParm( spar_id_1, "RelCenterLocation", "FeaPart" ), 0.25 );
        SetParmVal( FindParm( spar_id_2, "RelCenterLocation", "FeaPart" ), 0.75 );

        //==== Set Perpendicular Edge type to SPAR ====//
        SetParmVal( FindParm( rib_id, "PerpendicularEdgeType", "FeaRib" ), SPAR_NORMAL );

        SetFeaPartPerpendicularSparID( rib_id, spar_id_2 );

        if ( spar_id_2 != GetFeaPartPerpendicularSparID( rib_id ) )
        {
            Print( "Error: SetFeaPartPerpendicularSparID" );
        }
        \endcode
        \sa FEA_RIB_NORMAL, GetFeaPartPerpendicularSparID
        \param [in] part_id FEA Part ID (Rib or Rib Array Type)
        \param [in] perpendicular_spar_id FEA Spar ID
    */

    void SetFeaPartPerpendicularSparID(const string &part_id, const string &perpendicular_spar_id)
    {
        FeaPart *part = StructureMgr.GetFeaPart(part_id);
        if (!part)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetFeaPartPerpendicularSparID::Can't Find FEA Part " + part_id);
            return;
        }

        bool rib_type = part->GetType() == FEA_RIB;
        bool rib_array_type = part->GetType() == FEA_RIB_ARRAY;

        if (!(rib_type || rib_array_type))
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "SetFeaPartPerpendicularSparID::FEA Part is not Rib or Rib Array Type");
            return;
        }

        // Check if Spar exists (not really necessary, but should be helpful)
        FeaPart *spar = StructureMgr.GetFeaPart(perpendicular_spar_id);
        if (!spar || spar->GetType() != FEA_SPAR)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetFeaPartPerpendicularSparID::Can't Find FEA Spar " + perpendicular_spar_id + ". ID will still be set.");
        }

        if (rib_type)
        {
            FeaRib *rib = dynamic_cast<FeaRib *>(part);
            assert(rib);

            rib->SetPerpendicularEdgeID(perpendicular_spar_id);
        }
        else if (rib_array_type)
        {
            FeaRibArray *rib_array = dynamic_cast<FeaRibArray *>(part);
            assert(rib_array);

            rib_array->SetPerpendicularEdgeID(perpendicular_spar_id);
        }
    }

    /*!
        Get the ID of the perpendicular spar for an FEA Rib or Rib Array. Note, the FEA Rib or Rib Array doesn't have to have "SPAR_NORMAL"
        set for the "PerpendicularEdgeType" Parm for this function to still return a value.
        \code{.cpp}
        //==== Add Wing Geometry ====//
        string wing_id = AddGeom( "WING" );

        //==== Add FeaStructure to Wing ====//
        int struct_ind = AddFeaStruct( wing_id );

        //==== Add Rib ====//
        string rib_id = AddFeaPart( wing_id, struct_ind, FEA_RIB );

        //==== Add Spars ====//
        string spar_id_1 = AddFeaPart( wing_id, struct_ind, FEA_SPAR );
        string spar_id_2 = AddFeaPart( wing_id, struct_ind, FEA_SPAR );

        SetParmVal( FindParm( spar_id_1, "RelCenterLocation", "FeaPart" ), 0.25 );
        SetParmVal( FindParm( spar_id_2, "RelCenterLocation", "FeaPart" ), 0.75 );

        //==== Set Perpendicular Edge type to SPAR ====//
        SetParmVal( FindParm( rib_id, "PerpendicularEdgeType", "FeaRib" ), SPAR_NORMAL );

        SetFeaPartPerpendicularSparID( rib_id, spar_id_2 );

        if ( spar_id_2 != GetFeaPartPerpendicularSparID( rib_id ) )
        {
            Print( "Error: GetFeaPartPerpendicularSparID" );
        }
        \endcode
        \sa FEA_RIB_NORMAL, SetFeaPartPerpendicularSparID
        \param [in] part_id FEA Part ID (Rib or Rib Array Type)
        \return Perpendicular FEA Spar ID
    */

    string GetFeaPartPerpendicularSparID(const string &part_id)
    {
        FeaPart *part = StructureMgr.GetFeaPart(part_id);
        string ret_str = string();
        if (!part)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetFeaPartPerpendicularSparID::Can't Find FEA Part " + part_id);
            return ret_str;
        }

        bool rib_type = part->GetType() == FEA_RIB;
        bool rib_array_type = part->GetType() == FEA_RIB_ARRAY;

        if (!(rib_type || rib_array_type))
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "GetFeaPartPerpendicularSparID::FEA Part is not Rib or Rib Array Type");
            return ret_str;
        }

        if (rib_type)
        {
            FeaRib *rib = dynamic_cast<FeaRib *>(part);
            assert(rib);
            ret_str = rib->GetPerpendicularEdgeID();
        }
        else if (rib_array_type)
        {
            FeaRibArray *rib_array = dynamic_cast<FeaRibArray *>(part);
            assert(rib_array);
            ret_str = rib_array->GetPerpendicularEdgeID();
        }

        return ret_str;
    }

    /*!
        Add an FEA SubSurface to a Structure
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Add LineArray ====//
        string line_array_id = AddFeaSubSurf( pod_id, struct_ind, SS_LINE_ARRAY );

        SetParmVal( FindParm( line_array_id, "ConstLineType", "SS_LineArray" ), 1 ); // Constant W

        SetParmVal( FindParm( line_array_id, "Spacing", "SS_LineArray" ), 0.25 );
        \endcode
        \sa SUBSURF_TYPE
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
        \param [in] type FEA SubSurface type enum (i.e. SS_ELLIPSE)
        \return FEA SubSurface ID
    */

    /// Add a FeaSubSurface, return FeaSubSurface ID
    string AddFeaSubSurf(const string &geom_id, int fea_struct_ind, int type)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return string();
        }
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddFeaSubSurf::Can't Find Geom " + geom_id);
            return string();
        }

        FeaStructure *feastruct = NULL;
        feastruct = geom_ptr->GetFeaStruct(fea_struct_ind);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddFeaSubSurf::Invalid FeaStructure Ptr at index " + to_string((long long)fea_struct_ind));
            return string();
        }

        SubSurface *feasubsurf = NULL;
        feasubsurf = feastruct->AddFeaSubSurf(type);
        if (!feasubsurf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddFeaSubSurf::Invalid FeaSubSurface Ptr");
            return string();
        }
        feastruct->Update();
        ErrorMgr.NoError();
        return feasubsurf->GetID();
    }

    /*!
        Delete an FEA SubSurface from a Structure
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Add LineArray ====//
        string line_array_id = AddFeaSubSurf( pod_id, struct_ind, SS_LINE_ARRAY );

        //==== Add Rectangle ====//
        string rect_id = AddFeaSubSurf( pod_id, struct_ind, SS_RECTANGLE );

        //==== Delete LineArray ====//
        DeleteFeaSubSurf( pod_id, struct_ind, line_array_id );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
        \param [in] ss_id FEA SubSurface ID
    */

    void DeleteFeaSubSurf(const string &geom_id, int fea_struct_ind, const string &ss_id)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return;
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteFeaSubSurf::Can't Find Geom " + geom_id);
            return;
        }

        FeaStructure *feastruct = NULL;
        feastruct = geom_ptr->GetFeaStruct(fea_struct_ind);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteFeaSubSurf::Invalid FeaStructure Ptr at index " + to_string((long long)fea_struct_ind));
            return;
        }

        int index = StructureMgr.GetFeaSubSurfIndex(ss_id);
        if (index == -1)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DeleteFeaSubSurf::Can't Find FeaSubSurf " + ss_id);
            return;
        }
        feastruct->DelFeaSubSurf(index);
        ErrorMgr.NoError();
        return;
    }

    /*!
        Get the index of an FEA SubSurface give the SubSurface ID
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Add Slice ====//
        string slice_id = AddFeaPart( pod_id, struct_ind, FEA_SLICE );

        //==== Add LineArray ====//
        string line_array_id = AddFeaSubSurf( pod_id, struct_ind, SS_LINE_ARRAY );

        //==== Add Rectangle ====//
        string rect_id = AddFeaSubSurf( pod_id, struct_ind, SS_RECTANGLE );

        if ( 1 != GetFeaSubSurfIndex( rect_id ) ) // These should be equivalent
        {
            Print( "Error: GetFeaSubSurfIndex" );
        }
        \endcode
        \param [in] ss_id FEA SubSurface ID
        \return FEA SubSurface Index
    */

    int GetFeaSubSurfIndex(const string &ss_id)
    {
        int index = StructureMgr.GetFeaSubSurfIndex(ss_id);
        if (index < 0)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetFeaSubSurfIndex::Can't Find FeaSubSurf " + ss_id);
            return index;
        }
        ErrorMgr.NoError();
        return index;
    }

    /*!
        Get the total number of FEA Subsurfaces in the vehicle
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string wing_id = AddGeom( "WING" );

        //==== Add FeaStructure to Pod ====//
        int struct_1 = AddFeaStruct( wing_id );
        int struct_2 = AddFeaStruct( wing_id );

        if ( NumFeaStructures() != 2 )
        {
            Print( "Error: NumFeaStructures" );
        }
        \endcode
        \sa GetFeaStructIDVec
        \return Total Number of FEA Structures
    */

    int NumFeaStructures()
    {
        return StructureMgr.NumFeaStructures();
    }

    /*!
        Get the number of FEA Parts for a particular FEA Structure
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        string struct_id = GetFeaStructID( pod_id, struct_ind );

        //==== Add FEA Parts ====//
        string slice_id = AddFeaPart( pod_id, struct_ind, FEA_SLICE );
        string dome_id = AddFeaPart( pod_id, struct_ind, FEA_DOME );

        if ( NumFeaParts( struct_id ) != 3 ) // Includes FeaSkin
        {
            Print( "Error: NumFeaParts" );
        }
        \endcode
        \sa GetFeaPartIDVec
        \param [in] fea_struct_id FEA Structure ID
        \return Number of FEA Parts
    */

    int NumFeaParts(const string &fea_struct_id)
    {
        FeaStructure *feastruct = StructureMgr.GetFeaStruct(fea_struct_id);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "NumFeaParts::Invalid FeaStructure ID " + fea_struct_id);
            return -1;
        }

        ErrorMgr.NoError();
        return feastruct->NumFeaParts();
    }

    /*!
        Get the number of FEA Subsurfaces for a particular FEA Structure
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string wing_id = AddGeom( "WING" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( wing_id );

        string struct_id = GetFeaStructID( wing_id, struct_ind );

        //==== Add SubSurfaces ====//
        string line_array_id = AddFeaSubSurf( wing_id, struct_ind, SS_LINE_ARRAY );
        string rectangle_id = AddFeaSubSurf( wing_id, struct_ind, SS_RECTANGLE );

        if ( NumFeaSubSurfs( struct_id ) != 2 )
        {
            Print( "Error: NumFeaSubSurfs" );
        }
        \endcode
        \sa GetFeaSubSurfIDVec
        \param [in] fea_struct_id FEA Structure ID
        \return Number of FEA SubSurfaces
    */

    int NumFeaSubSurfs(const string &fea_struct_id)
    {
        FeaStructure *feastruct = StructureMgr.GetFeaStruct(fea_struct_id);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "NumFeaSubSurfs::Invalid FeaStructure ID " + fea_struct_id);
            return -1;
        }

        ErrorMgr.NoError();
        return feastruct->NumFeaSubSurfs();
    }

    /*!
        Get the IDs of all FEA Parts in the given FEA Structure
        \code{.cpp}
        //==== Add Geometries ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        string struct_id = GetFeaStructID( pod_id, struct_ind );

        //==== Add FEA Parts ====//
        string slice_id = AddFeaPart( pod_id, struct_ind, FEA_SLICE );
        string dome_id = AddFeaPart( pod_id, struct_ind, FEA_DOME );

        array < string > part_id_vec = GetFeaPartIDVec( struct_id ); // Should include slice_id & dome_id
        \endcode
        \sa NumFeaParts
        \param [in] fea_struct_id FEA Structure ID
        \return Array of FEA Part IDs
    */

    vector<string> GetFeaPartIDVec(const string &fea_struct_id)
    {
        vector<string> ret_vec;
        FeaStructure *feastruct = StructureMgr.GetFeaStruct(fea_struct_id);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetFeaPartIDVec::Invalid FeaStructure ID");
            return ret_vec;
        }

        vector<FeaPart *> fea_part_vec = feastruct->GetFeaPartVec();
        ret_vec.resize(fea_part_vec.size());

        for (size_t i = 0; i < fea_part_vec.size(); i++)
        {
            ret_vec[i] = fea_part_vec[i]->GetID();
        }

        ErrorMgr.NoError();
        return ret_vec;
    }

    /*!
        Get the IDs of all FEA SubSurfaces in the given FEA Structure
        \code{.cpp}
        //==== Add Geometries ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        string struct_id = GetFeaStructID( pod_id, struct_ind );

        //==== Add SubSurfaces ====//
        string line_array_id = AddFeaSubSurf( pod_id, struct_ind, SS_LINE_ARRAY );
        string rectangle_id = AddFeaSubSurf( pod_id, struct_ind, SS_RECTANGLE );

        array < string > part_id_vec = GetFeaSubSurfIDVec( struct_id ); // Should include line_array_id & rectangle_id
        \endcode
        \sa NumFeaSubSurfs
        \param [in] fea_struct_id FEA Structure ID
        \return Array of FEA Part IDs
    */

    vector<string> GetFeaSubSurfIDVec(const string &fea_struct_id)
    {
        vector<string> ret_vec;
        FeaStructure *feastruct = StructureMgr.GetFeaStruct(fea_struct_id);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetFeaSubSurfIDVec::Invalid FeaStructure ID " + fea_struct_id);
            return ret_vec;
        }

        vector<SubSurface *> fea_ss_vec = feastruct->GetFeaSubSurfVec();
        ret_vec.resize(fea_ss_vec.size());

        for (size_t i = 0; i < fea_ss_vec.size(); i++)
        {
            ret_vec[i] = fea_ss_vec[i]->GetID();
        }

        ErrorMgr.NoError();
        return ret_vec;
    }

    /*!
        Add an FEA Material the FEA Mesh material library. Materials are available across all Geoms and Structures.
        \code{.cpp}
        //==== Create FeaMaterial ====//
        string mat_id = AddFeaMaterial();

        SetParmVal( FindParm( mat_id, "MassDensity", "FeaMaterial" ), 0.016 );
        \endcode
        \return FEA Material ID
    */

    /// Add an FeaMaterial, return FeaMaterial ID
    string AddFeaMaterial()
    {
        FeaMaterial *feamat = NULL;
        feamat = StructureMgr.AddFeaMaterial();
        if (!feamat)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddFeaMaterial::Invalid FeaMaterial Ptr");
            return string();
        }
        ErrorMgr.NoError();
        return feamat->GetID();
    }

    /*!
        Add aa FEA Property the FEA Mesh property library. Properties are available across all Geoms and Structures. Currently only beam and
        shell properties are available. Note FEA_SHELL_AND_BEAM is not a valid property type.
        \code{.cpp}
        //==== Create FeaProperty ====//
        string prop_id = AddFeaProperty();

        SetParmVal( FindParm( prop_id, "Thickness", "FeaProperty" ), 0.01 );
        \endcode
        \sa FEA_PART_ELEMENT_TYPE
        \param [in] property_type FEA Property type enum (i.e. FEA_SHELL).
        \return FEA Property ID
    */

    /// Add an FeaProperty, return FeaProperty ID. The default is shell property type
    string AddFeaProperty(int property_type)
    {
        FeaProperty *feaprop = NULL;
        feaprop = StructureMgr.AddFeaProperty(property_type);
        if (!feaprop)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "AddFeaProperty::Invalid FeaProperty Ptr");
            return string();
        }
        ErrorMgr.NoError();
        return feaprop->GetID();
    }

    /*!
        Set the value of a particular FEA Mesh option for the specified Structure. Note, FEA Mesh makes use of enums initially created for CFD Mesh
        but not all CFD Mesh options are available for FEA Mesh.
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Adjust FeaMeshSettings ====//
        SetFeaMeshVal( pod_id, struct_ind, CFD_MAX_EDGE_LEN, 0.75 );

        SetFeaMeshVal( pod_id, struct_ind, CFD_MIN_EDGE_LEN, 0.2 );
        \endcode
        \sa CFD_CONTROL_TYPE
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
        \param [in] type FEA Mesh option type enum (i.e. CFD_MAX_EDGE_LEN)
        \param [in] val Value the option is set to
    */

    void SetFeaMeshVal(const string &geom_id, int fea_struct_ind, int type, double val)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return;
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetFEAMeshVal::Can't Find Geom " + geom_id);
            return;
        }

        FeaStructure *feastruct = NULL;
        feastruct = geom_ptr->GetFeaStruct(fea_struct_ind);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetFEAMeshVal::Invalid FeaStructure Ptr at index " + to_string((long long)fea_struct_ind));
            return;
        }

        // Type as CFD or FEA Mesh Setting Enums?
        if (type == CFD_MIN_EDGE_LEN)
            feastruct->GetFeaGridDensityPtr()->m_MinLen = val;
        else if (type == CFD_MAX_EDGE_LEN)
            feastruct->GetFeaGridDensityPtr()->m_BaseLen = val;
        else if (type == CFD_MAX_GAP)
            feastruct->GetFeaGridDensityPtr()->m_MaxGap = val;
        else if (type == CFD_NUM_CIRCLE_SEGS)
            feastruct->GetFeaGridDensityPtr()->m_NCircSeg = val;
        else if (type == CFD_GROWTH_RATIO)
            feastruct->GetFeaGridDensityPtr()->m_GrowRatio = val;
        else if (type == CFD_LIMIT_GROWTH_FLAG)
            feastruct->GetFeaGridDensityPtr()->SetRigorLimit(ToBool(val));
        else if (type == CFD_HALF_MESH_FLAG)
            feastruct->GetStructSettingsPtr()->SetHalfMeshFlag(ToBool(val));
        else
        {
            ErrorMgr.AddError(VSP_CANT_FIND_TYPE, "SetFEAMeshVal::Can't Find Type " + to_string((long long)type));
            return;
        }

        ErrorMgr.NoError();
    }

    /*!
        Set the name of a particular FEA Mesh output file for a specified Structure
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        string struct_id = GetFeaStructID( pod_id, struct_ind );

        //=== Set Export File Name ===//
        string export_name = "FEAMeshTest_calculix.dat";

        //==== Get Parent Geom ID and Index ====//
        string parent_id = GetFeaStructParentGeomID( struct_id ); // same as pod_id

        SetFeaMeshFileName( parent_id, struct_ind, FEA_CALCULIX_FILE_NAME, export_name );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
        \param [in] file_type FEA output file type enum (i.e. FEA_EXPORT_TYPE)
        \param [in] file_name Name for the output file
    */

    void SetFeaMeshFileName(const string &geom_id, int fea_struct_ind, int file_type, const string &file_name)
    {
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return;
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetFeaMeshFileNames::Can't Find Geom " + geom_id);
            return;
        }

        FeaStructure *feastruct = NULL;
        feastruct = geom_ptr->GetFeaStruct(fea_struct_ind);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetFeaMeshFileNames::Invalid FeaStructure Ptr at index " + to_string((long long)fea_struct_ind));
            return;
        }

        feastruct->GetStructSettingsPtr()->SetExportFileName(file_name, file_type);

        ErrorMgr.NoError();
    }

    /*!
        Compute an FEA Mesh for a Structure. Only a single output file can be generated with this function.
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        string struct_id = GetFeaStructID( pod_id, struct_ind );

        //==== Generate FEA Mesh and Export ====//
        Print( string( "--> Generating FeaMesh " ) );

        //==== Get Parent Geom ID and Index ====//
        string parent_id = GetFeaStructParentGeomID( struct_id ); // same as pod_id

        ComputeFeaMesh( parent_id, struct_ind, FEA_CALCULIX_FILE_NAME );
        // Could also call ComputeFeaMesh ( struct_id, FEA_CALCULIX_FILE_NAME );
        \endcode
        \sa SetFeaMeshFileName, FEA_EXPORT_TYPE
        \param [in] geom_id Parent Geom ID
        \param [in] fea_struct_ind FEA Structure index
        \param [in] file_type FEA output file type enum (i.e. FEA_EXPORT_TYPE)
    */

    void ComputeFeaMesh(const string &geom_id, int fea_struct_ind, int file_type)
    {
        Update(); // Remove?
        Vehicle *veh = GetVehicle();
        if (!veh)
        {
            return;
        }

        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ComputeFEAMesh::Can't Find Geom " + geom_id);
            return;
        }

        FeaStructure *feastruct = geom_ptr->GetFeaStruct(fea_struct_ind);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ComputeFEAMesh::Invalid FeaStructure Ptr at index " + to_string((long long)fea_struct_ind));
            return;
        }

        feastruct->GetStructSettingsPtr()->SetAllFileExportFlags(false);
        feastruct->GetStructSettingsPtr()->SetFileExportFlag(file_type, true);

        FeaMeshMgr.SetFeaMeshStructIndex(StructureMgr.GetTotFeaStructIndex(feastruct));

        FeaMeshMgr.GenerateFeaMesh();
        ErrorMgr.NoError();
    }

    /*!
        Compute an FEA Mesh for a Structure. Only a single output file can be generated with this function.
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        string struct_id = GetFeaStructID( pod_id, struct_ind );

        //==== Generate FEA Mesh and Export ====//
        Print( string( "--> Generating FeaMesh " ) );

        ComputeFeaMesh ( struct_id, FEA_CALCULIX_FILE_NAME );
        // Could also call ComputeFeaMesh( parent_id, struct_ind, FEA_CALCULIX_FILE_NAME );
        \endcode
        \sa SetFeaMeshFileName, FEA_EXPORT_TYPE
        \param [in] struct_id FEA Structure ID
        \param [in] file_type FEA output file type enum (i.e. FEA_EXPORT_TYPE)
    */

    void ComputeFeaMesh(const string &struct_id, int file_type)
    {
        Update(); // Not sure if this is needed

        FeaStructure *feastruct = StructureMgr.GetFeaStruct(struct_id);
        if (!feastruct)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ComputeFEAMesh::Can't Find Structure " + struct_id);
            return;
        }

        feastruct->GetStructSettingsPtr()->SetAllFileExportFlags(false);
        feastruct->GetStructSettingsPtr()->SetFileExportFlag(file_type, true);

        FeaMeshMgr.SetFeaMeshStructIndex(StructureMgr.GetTotFeaStructIndex(feastruct));

        FeaMeshMgr.GenerateFeaMesh();
        ErrorMgr.NoError();
    }

    /*!
        Cut a cross-section from the specified geometry and maintain it in memory
        \code{.cpp}
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        //==== Insert, Cut, Paste Example ====//
        InsertXSec( fid, 1, XS_ROUNDED_RECTANGLE );         // Insert A Cross-Section

        CopyXSec( fid, 2 );                                 // Copy Just Created XSec To Clipboard

        PasteXSec( fid, 1 );                                // Paste Clipboard

        CutXSec( fid, 2 );                                  // Cut Created XSec
        \endcode
        \sa PasteXSec
        \param [in] geom_id Geom ID
        \param [in] index XSec index
    */

    void CutXSec(const string &geom_id, int index)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "CutXSec::Can't Find Geom " + geom_id);
            return;
        }

        geom_ptr->CutXSec(index);
        Update();

        ErrorMgr.NoError();
    }

    /*!
        Copy a cross-section from the specified geometry and maintain it in memory
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Copy XSec To Clipboard
        CopyXSec( sid, 1 );

        // Paste To XSec 3
        PasteXSec( sid, 3 );
        \endcode
        \sa PasteXSec
        \param [in] geom_id Geom ID
        \param [in] index XSec index
    */

    void CopyXSec(const string &geom_id, int index)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "CopyXSec::Can't Find Geom " + geom_id);
            return;
        }

        geom_ptr->CopyXSec(index);
        ErrorMgr.NoError();
    }

    /*!
        Paste the cross-section currently held in memory to the specified geometry
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Copy XSec To Clipboard
        CopyXSec( sid, 1 );

        // Paste To XSec 3
        PasteXSec( sid, 3 );
        \endcode
        \sa CutXSec, CopyXSec
        \param [in] geom_id Geom ID
        \param [in] index XSec index
    */

    void PasteXSec(const string &geom_id, int index)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PasteXSec::Can't Find Geom " + geom_id);
            return;
        }

        geom_ptr->PasteXSec(index);
        ErrorMgr.NoError();
    }

    /*!
        Insert a cross-section of particular type to the specified geometry after the given index
        \code{.cpp}
        string wing_id = AddGeom( "WING" );

        //===== Add XSec ====//
        InsertXSec( wing_id, 1, XS_SIX_SERIES );
        \endcode
        \sa XSEC_CRV_TYPE
        \param [in] geom_id Geom ID
        \param [in] index XSec index
        \param [in] type XSec type enum (i.e. XS_GENERAL_FUSE)
    */

    void InsertXSec(const string &geom_id, int index, int type)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "InsertXSec::Can't Find Geom " + geom_id);
            return;
        }

        geom_ptr->InsertXSec(index, type);
        ErrorMgr.NoError();
    }

    //===================================================================//
    //===============       Wing Section Functions     ==================//
    //===================================================================//

    /*!
        Set the driver group for a wing section or a XSecCurve. Care has to be taken when setting these driver groups to ensure a valid combination.
        \code{.cpp}
        //==== Add Wing Geometry and Set Parms ====//
        string wing_id = AddGeom( "WING", "" );

        //==== Set Wing Section Controls ====//
        SetDriverGroup( wing_id, 1, AR_WSECT_DRIVER, ROOTC_WSECT_DRIVER, TIPC_WSECT_DRIVER );

        Update();

        //==== Set Parms ====//
        SetParmVal( wing_id, "Root_Chord", "XSec_1", 2 );
        SetParmVal( wing_id, "Tip_Chord", "XSec_1", 1 );

        Update();
        \endcode
        \sa WING_DRIVERS, XSEC_DRIVERS
        \param [in] geom_id Geom ID
        \param [in] section_index Wing section index
        \param [in] driver_0 First driver enum (i.e. SPAN_WSECT_DRIVER)
        \param [in] driver_1 Second driver enum (i.e. ROOTC_WSECT_DRIVER)
        \param [in] driver_2 Third driver enum (i.e. TIPC_WSECT_DRIVER)
        */

    void SetDriverGroup(const string &geom_id, int section_index, int driver_0, int driver_1, int driver_2)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetDriverGroup::Can't Find Geom " + geom_id);
            return;
        }

        if (geom_ptr->GetType().m_Type == MS_WING_GEOM_TYPE)
        {
            WingGeom *wg = dynamic_cast<WingGeom *>(geom_ptr);
            WingSect *ws = wg->GetWingSect(section_index);
            if (!ws)
            {
                ErrorMgr.AddError(VSP_INVALID_PTR, "SetDriverGroup::Invalid Wing Section Index " + to_string((long long)section_index));
                return;
            }

            vector<int> prevchoices = ws->m_DriverGroup.GetChoices();

            ws->m_DriverGroup.SetChoice(0, driver_0);
            ws->m_DriverGroup.SetChoice(1, driver_1);
            ws->m_DriverGroup.SetChoice(2, driver_2);

            bool valid = ws->m_DriverGroup.ValidDrivers(ws->m_DriverGroup.GetChoices());
            if (!valid)
            {
                ErrorMgr.AddError(VSP_INVALID_DRIVERS, "SetDriverGroup::Invalid wing drivers.");
                ws->m_DriverGroup.SetChoices(prevchoices);
                return;
            }

            ErrorMgr.NoError();
            return;
        }
        else
        {
            GeomXSec *gxs = dynamic_cast<GeomXSec *>(geom_ptr);

            XSecCurve *xsc = NULL;
            if (gxs) // Proceed as GeomXSec
            {
                xsc = gxs->GetXSec(section_index)->GetXSecCurve();
            }
            else
            {
                BORGeom *bor = dynamic_cast<BORGeom *>(geom_ptr);
                if (bor) // Proceed as Body of Revolution
                {
                    xsc = bor->GetXSecCurve();
                }
            }

            if (xsc) // Succeeded in getting an XSecCurve
            {
                vector<int> prevchoices = xsc->m_DriverGroup->GetChoices();

                // Only driver 0 used for Circles.
                xsc->m_DriverGroup->SetChoice(0, driver_0);

                // Driver 1 used for other XSecCurve types.
                if (driver_1 > -1)
                    xsc->m_DriverGroup->SetChoice(1, driver_1);

                bool valid = xsc->m_DriverGroup->ValidDrivers(xsc->m_DriverGroup->GetChoices());
                if (!valid)
                {
                    ErrorMgr.AddError(VSP_INVALID_DRIVERS, "SetDriverGroup::Invalid XSecCurve drivers.");
                    xsc->m_DriverGroup->SetChoices(prevchoices);
                    return;
                }
                ErrorMgr.NoError();
                return;
            }
        }
        ErrorMgr.AddError(VSP_INVALID_PTR, "SetDriverGroup::Invalid Geom Type " + geom_id);
        return;
    }

    //===================================================================//
    //===============       XSecSurf Functions         ==================//
    //===================================================================//

    /*!
        Get the XSecSurf ID for a particular Geom and XSecSurf index
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );
        \endcode
        \param [in] geom_id Geom ID
        \param [in] index XSecSurf index
        \return XSecSurf ID
    */

    string GetXSecSurf(const string &geom_id, int index)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSecSurf::Can't Find Geom " + geom_id);
            return string();
        }
        XSecSurf *xsec_surf = geom_ptr->GetXSecSurf(index);

        if (!xsec_surf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSecSurf::Can't Find XSecSurf " + geom_id + ":" + to_string((long long)index));
            return string();
        }

        ErrorMgr.NoError();
        return xsec_surf->GetID();
    }

    /*!
        Get number of XSecs in an XSecSurf
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        // Flatten ends
        int num_xsecs = GetNumXSec( xsec_surf );

        for ( int i = 0 ; i < num_xsecs ; i++ )
        {
            string xsec = GetXSec( xsec_surf, i );

            SetXSecTanAngles( xsec, XSEC_BOTH_SIDES, 0 );       // Set Tangent Angles At Cross Section

            SetXSecTanStrengths( xsec, XSEC_BOTH_SIDES, 0.0 );  // Set Tangent Strengths At Cross Section
        }
        \endcode
        \param [in] xsec_surf_id XSecSurf ID
        \return Number of XSecs
    */

    int GetNumXSec(const string &xsec_surf_id)
    {
        XSecSurf *xsec_surf = FindXSecSurf(xsec_surf_id);
        if (!xsec_surf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetNumXSec::Can't Find XSecSurf " + xsec_surf_id);
            return 0;
        }

        ErrorMgr.NoError();
        return xsec_surf->NumXSec();
    }

    /*!
        Get Xsec ID for a particular XSecSurf at given index
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        // Identify XSec 1
        string xsec_1 = GetXSec( xsec_surf, 1 );
        \endcode
        \param [in] xsec_surf_id XSecSurf ID
        \param [in] xsec_index Xsec index
        \return Xsec ID
    */

    string GetXSec(const string &xsec_surf_id, int xsec_index)
    {
        XSecSurf *xsec_surf = FindXSecSurf(xsec_surf_id);
        if (!xsec_surf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSec::Can't Find XSecSurf " + xsec_surf_id);
            return string();
        }
        XSec *xsec = xsec_surf->FindXSec(xsec_index);
        if (!xsec)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSec::Can't Find XSec " + xsec_surf_id + ":" + to_string((long long)xsec_index));
            return string();
        }

        ErrorMgr.NoError();
        return xsec->GetID();
    }

    /*!
        Change the shape of a particular XSec, identified by an XSecSurf ID and XSec index
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        // Set XSec 1 & 2 to Edit Curve type
        ChangeXSecShape( xsec_surf, 1, XS_EDIT_CURVE );
        ChangeXSecShape( xsec_surf, 2, XS_EDIT_CURVE );

        string xsec_2 = GetXSec( xsec_surf, 2 );

        if ( GetXSecShape( xsec_2 ) != XS_EDIT_CURVE )
        {
            Print( "Error: ChangeXSecShape" );
        }
        \endcode
        \sa XSEC_CRV_TYPE
        \param [in] xsec_surf_id XSecSurf ID
        \param [in] xsec_index Xsec index
        \param [in] type Xsec type enum (i.e. XS_ELLIPSE)
    */

    void ChangeXSecShape(const string &xsec_surf_id, int xsec_index, int type)
    {
        XSecSurf *xsec_surf = FindXSecSurf(xsec_surf_id);
        if (!xsec_surf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ChangeXSecShape::Can't Find XSecSurf " + xsec_surf_id);
            return;
        }
        if (xsec_index < 0 || xsec_index >= xsec_surf->NumXSec())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ChangeXSecShape::XSec Index Out of Range " + xsec_surf_id + ":" + to_string((long long)xsec_index));
            return;
        }

        ErrorMgr.NoError();
        xsec_surf->ChangeXSecShape(xsec_index, type);
    }

    /*!
        Set the global surface transform matrix for given XSecSurf
        \param [in] xsec_surf_id XSecSurf ID
        \param [in] mat Transformation matrix
    */

    void SetXSecSurfGlobalXForm(const string &xsec_surf_id, const Matrix4d &mat)
    {
        XSecSurf *xsec_surf = FindXSecSurf(xsec_surf_id);
        if (!xsec_surf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecSurfGlobalXForm::Can't Find XSecSurf " + xsec_surf_id);
            return;
        }
        xsec_surf->SetGlobalXForm(mat);
    }

    /*!
        Get the global surface transform matrix for given XSecSurf
        \param [in] xsec_surf_id XSecSurf ID
        \return Transformation matrix
    */

    Matrix4d GetXSecSurfGlobalXForm(const string &xsec_surf_id)
    {
        XSecSurf *xsec_surf = FindXSecSurf(xsec_surf_id);
        if (!xsec_surf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSecSurfGlobalXForm::Can't Find XSecSurf " + xsec_surf_id);
            return Matrix4d();
        }
        return xsec_surf->GetGlobalXForm();
    }

    //===================================================================//
    //=================       XSec Functions         ====================//
    //===================================================================//

    /*!
        Get the shape of an XSec
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_EDIT_CURVE );

        string xsec = GetXSec( xsec_surf, 1 );

        if ( GetXSecShape( xsec ) != XS_EDIT_CURVE ) { Print( "ERROR: GetXSecShape" ); }
        \endcode
        \sa XSEC_CRV_TYPE
        \param [in] xsec_id XSec ID
        \return XSec type enum (i.e. XS_ELLIPSE)
    */

    int GetXSecShape(const string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSecShape::Can't Find XSec " + xsec_id);
            return XS_UNDEFINED;
        }

        ErrorMgr.NoError();
        return xs->GetXSecCurve()->GetType();
    }

    /*!
        Get the width of an XSec. Note that POINT type XSecs have a width and height of 0, regardless of what width and height it is set to.
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        string xsec = GetXSec( xsec_surf, GetNumXSec( xsec_surf ) - 2 ); // Get 2nd to last XSec

        SetXSecWidthHeight( xsec, 3.0, 6.0 );

        if ( abs( GetXSecWidth( xsec ) - 3.0 ) > 1e-6 )        { Print( "---> Error: API Get/Set Width " ); }
        \endcode
        \sa SetXSecWidth
        \param [in] xsec_id XSec ID
        \return Xsec width
    */

    double GetXSecWidth(const string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSecWidth::Can't Find XSec " + xsec_id);
            return 0;
        }
        ErrorMgr.NoError();
        return xs->GetXSecCurve()->GetWidth();
    }

    /*!
        Get the height of an XSec. Note that POINT type XSecs have a width and height of 0, regardless of what width and height it is set to.
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        string xsec = GetXSec( xsec_surf, GetNumXSec( xsec_surf ) - 2 ); // Get 2nd to last XSec

        SetXSecWidthHeight( xsec, 3.0, 6.0 );

        if ( abs( GetXSecHeight( xsec ) - 6.0 ) > 1e-6 )        { Print( "---> Error: API Get/Set Width " ); }
        \endcode
        \sa SetXSecHeight
        \param [in] xsec_id XSec ID
        \return Xsec height
    */

    double GetXSecHeight(const string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSecHeight::Can't Find XSec " + xsec_id);
            return 0;
        }
        ErrorMgr.NoError();
        return xs->GetXSecCurve()->GetHeight();
    }

    /*!
        Set the width and height of an XSec. Note, if the XSec is an EDIT_CURVE type and PreserveARFlag is true, the input width value will be
        ignored and instead set from on the input height and aspect ratio. Use SetXSecWidth and SetXSecHeight directly to avoid this.
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        // Identify XSec 2
        string xsec_2 = GetXSec( xsec_surf, 2 );

        SetXSecWidthHeight( xsec_2, 1.5, 1.5 );
        \endcode
        \sa SetXSecWidth, SetXSecHeight
        \param [in] xsec_id XSec ID
        \param [in] w Xsec width
        \param [in] h Xsec height
    */

    void SetXSecWidthHeight(const string &xsec_id, double w, double h)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecWidthHeight::Can't Find XSec " + xsec_id);
            return;
        }
        xs->GetXSecCurve()->SetWidthHeight(w, h);
        xs->ParmChanged(NULL, Parm::SET_FROM_DEVICE); // Force Update
        ErrorMgr.NoError();
    }

    /*!
        Set the width of an XSec. Note that POINT type XSecs have a width and height of 0, regardless of what is input to SetXSecWidth.
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        // Identify XSec 2
        string xsec_2 = GetXSec( xsec_surf, 2 );

        SetXSecWidth( xsec_2, 1.5 );
        \endcode
        \sa GetXSecWidth
        \param [in] xsec_id XSec ID
        \param [in] w Xsec width
    */

    void SetXSecWidth(const string &xsec_id, double w)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecWidth::Can't Find XSec " + xsec_id);
            return;
        }
        Parm *width_parm = ParmMgr.FindParm(xs->GetXSecCurve()->GetWidthParmID());
        if (!width_parm)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecWidth::Can't Find Width Parm " + xs->GetXSecCurve()->GetWidthParmID());
            return;
        }
        width_parm->SetFromDevice(w);
        ErrorMgr.NoError();
    }

    /*!
        Set the height of an XSec. Note that POINT type XSecs have a width and height of 0, regardless of what is input to SetXSecHeight.
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        // Identify XSec 2
        string xsec_2 = GetXSec( xsec_surf, 2 );

        SetXSecHeight( xsec_2, 1.5 );
        \endcode
        \sa GetXSecHeight
        \param [in] xsec_id XSec ID
        \param [in] h Xsec height
    */

    void SetXSecHeight(const string &xsec_id, double h)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecHeight::Can't Find XSec " + xsec_id);
            return;
        }
        Parm *height_parm = ParmMgr.FindParm(xs->GetXSecCurve()->GetHeightParmID());
        if (!height_parm)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecHeight::Can't Find Width Parm " + xs->GetXSecCurve()->GetHeightParmID());
            return;
        }
        height_parm->SetFromDevice(h);
        ErrorMgr.NoError();
    }

    /*!
        Get all Parm IDs for specified XSec Parm Container
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        string xsec = GetXSec( xsec_surf, GetNumXSec( xsec_surf ) - 1 );

        array< string > @parm_array = GetXSecParmIDs( xsec );

        if ( parm_array.size() < 1 )                        { Print( "---> Error: API GetXSecParmIDs " ); }
        \endcode
        \param [in] xsec_id XSec ID
        \return Array of Parm IDs
    */

    vector<string> GetXSecParmIDs(const string &xsec_id)
    {
        vector<string> parm_vec;

        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSecParmIDs::Can't Find XSec " + xsec_id);
            return parm_vec;
        }

        xs->AddLinkableParms(parm_vec);

        ErrorMgr.NoError();
        return parm_vec;
    }

    /*!
        Get a specific Parm ID from an Xsec
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, GetNumXSec( xsec_surf ) - 1, XS_ROUNDED_RECTANGLE );

        string xsec = GetXSec( xsec_surf, GetNumXSec( xsec_surf ) - 1 );

        string wid = GetXSecParm( xsec, "RoundedRect_Width" );

        if ( !ValidParm( wid ) )                            { Print( "---> Error: API GetXSecParm " ); }
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] name Parm name
        \return Parm ID
    */

    string GetXSecParm(const string &xsec_id, const string &name)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSecParm::Can't Find XSec " + xsec_id);
            return string();
        }

        //==== Valid XSec Parm - Return Parm ID ====//
        string xsparm = xs->FindParm(name);
        if (ValidParm(xsparm))
        {
            ErrorMgr.NoError();
            return xsparm;
        }

        //==== Check Curve For Name ====//
        XSecCurve *xsc = xs->GetXSecCurve();
        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetXSecParm::Can't Find XSecCurve " + xsec_id);
            return string();
        }

        string xscparm = xsc->FindParm(name);
        if (ValidParm(xscparm))
        {
            ErrorMgr.NoError();
            return xscparm;
        }

        ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetXSecParm::Can't Find Parm " + name);
        return string();
    }

    /*!
        Read in XSec shape from fuselage (*.fsx) file and set to the specified XSec. The XSec must be of type XS_FILE_FUSE.
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, 2, XS_FILE_FUSE );

        string xsec = GetXSec( xsec_surf, 2 );

        array< vec3d > @vec_array = ReadFileXSec( xsec, "TestXSec.fxs" );
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] file_name Fuselage XSec file name
        \return Array of coordinate points read from the file and set to the XSec
    */

    vector<vec3d> ReadFileXSec(const string &xsec_id, const string &file_name)
    {
        vector<vec3d> pnt_vec;
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ReadFileXSec::Can't Find XSec " + xsec_id);
            return pnt_vec;
        }

        if (xs->GetXSecCurve()->GetType() == XS_FILE_FUSE)
        {
            FileXSec *file_xs = dynamic_cast<FileXSec *>(xs->GetXSecCurve());
            assert(file_xs);
            if (file_xs->ReadXsecFile(file_name))
            {
                ErrorMgr.NoError();
                return file_xs->GetUnityFilePnts();
            }
            else
            {
                ErrorMgr.AddError(VSP_FILE_DOES_NOT_EXIST, "ReadFileXSec::Error reading fuselage file " + file_name);
                return pnt_vec;
            }
        }

        ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "ReadFileXSec::XSec Not XS_FILE_FUSE Type " + xsec_id);
        return pnt_vec;
    }

    /*!
        Set the coordinate points for a specific XSec. The XSec must be of type XS_FILE_FUSE.
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, 2, XS_FILE_FUSE );

        string xsec = GetXSec( xsec_surf, 2 );

        array< vec3d > @vec_array = ReadFileXSec( xsec, "TestXSec.fxs" );

        if ( vec_array.size() > 0 )
        {
            vec_array[1] = vec_array[1] * 2.0;
            vec_array[3] = vec_array[3] * 2.0;

            SetXSecPnts( xsec, vec_array );
        }
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] pnt_arr Array of XSec coordinate points
    */

    void SetXSecPnts(const string &xsec_id, vector<vec3d> &pnt_vec)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecPnts::Can't Find XSec " + xsec_id);
            return;
        }
        if (xs->GetXSecCurve()->GetType() != XS_FILE_FUSE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetXSecPnts::Wrong XSec Type");
            return;
        }

        FileXSec *file_xs = dynamic_cast<FileXSec *>(xs->GetXSecCurve());
        assert(file_xs);
        file_xs->SetPnts(pnt_vec);
        ErrorMgr.NoError();
    }

    /*!
        Compute 3D coordinate for a point on an XSec curve given the parameter value (U) along the curve
        \code{.cpp}
        //==== Add Geom ====//
        string stack_id = AddGeom( "STACK" );

        //==== Get The XSec Surf ====//
        string xsec_surf = GetXSecSurf( stack_id, 0 );

        string xsec = GetXSec( xsec_surf, 2 );

        double u_fract = 0.25;

        vec3d pnt = ComputeXSecPnt( xsec, u_fract );
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] fract Curve parameter value (range: 0 - 1)
        \return 3D coordinate point
    */

    vec3d ComputeXSecPnt(const string &xsec_id, double fract)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ComputePnt::Can't Find XSec " + xsec_id);
            return vec3d();
        }

        vec3d pnt = xs->GetCurve().CompPnt01(fract);
        ErrorMgr.NoError();

        return pnt;
    }

    /*!
        Compute the tangent vector of a point on an XSec curve given the parameter value (U) along the curve
        \code{.cpp}
        //==== Add Geom ====//
        string stack_id = AddGeom( "STACK" );

        //==== Get The XSec Surf ====//
        string xsec_surf = GetXSecSurf( stack_id, 0 );

        string xsec = GetXSec( xsec_surf, 2 );

        double u_fract = 0.25;

        vec3d tan = ComputeXSecTan( xsec, u_fract );
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] fract Curve parameter value (range: 0 - 1)
        \return Tangent vector
    */

    vec3d ComputeXSecTan(const string &xsec_id, double fract)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ComputeTan::Can't Find XSec " + xsec_id);
            return vec3d();
        }

        vec3d pnt = xs->GetCurve().CompTan01(fract);
        ErrorMgr.NoError();

        return pnt;
    }

    /*!
        Reset all skinning Parms for a specified XSec. Set top, bottom, left, and right strengths, slew, angle, and curvature to 0. Set all symmetry and equality conditions to false.
        \code{.cpp}
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        string xsec_surf = GetXSecSurf( fid, 0 );           // Get First (and Only) XSec Surf

        int num_xsecs = GetNumXSec( xsec_surf );

        string xsec = GetXSec( xsec_surf, 1 );

        SetXSecTanAngles( xsec, XSEC_BOTH_SIDES, 0.0 );       // Set Tangent Angles At Cross Section
        SetXSecContinuity( xsec, 1 );                       // Set Continuity At Cross Section

        ResetXSecSkinParms( xsec );
        \endcode
        \param [in] xsec_id XSec ID
    */

    void ResetXSecSkinParms(const string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ResetXSecSkinParms::Can't Find XSec " + xsec_id);
            return;
        }
        SkinXSec *skinxs = dynamic_cast<SkinXSec *>(xs);
        if (!skinxs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ResetXSecSkinParms::Can't Convert To Skin XSec " + xsec_id);
            return;
        }

        skinxs->Reset();
        ErrorMgr.NoError();
    }

    /*!
        Set C-type continuity enforcement for a particular XSec
        \code{.cpp}
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        string xsec_surf = GetXSecSurf( fid, 0 );           // Get First (and Only) XSec Surf

        int num_xsecs = GetNumXSec( xsec_surf );

        for ( int i = 0 ; i < num_xsecs ; i++ )
        {
            string xsec = GetXSec( xsec_surf, i );

            SetXSecContinuity( xsec, 1 );                       // Set Continuity At Cross Section
        }
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] cx Continuity level (0, 1, or 2)
    */

    void SetXSecContinuity(const string &xsec_id, int cx)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecContinuity::Can't Find XSec " + xsec_id);
            return;
        }
        SkinXSec *skinxs = dynamic_cast<SkinXSec *>(xs);
        if (!skinxs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecContinuity::Can't Convert To Skin XSec " + xsec_id);
            return;
        }

        skinxs->SetContinuity(cx);
        ErrorMgr.NoError();
    }

    /*!
        Set the tangent angles for the specified XSec
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        int num_xsecs = GetNumXSec( xsec_surf );

        for ( int i = 0 ; i < num_xsecs ; i++ )
        {
            string xsec = GetXSec( xsec_surf, i );

            SetXSecTanAngles( xsec, XSEC_BOTH_SIDES, 10.0 );       // Set Tangent Angles At Cross Section
        }
        \endcode
        \sa XSEC_SIDES_TYPE
        \param [in] xsec_id XSec ID
        \param [in] side Side type enum (i.e. XSEC_BOTH_SIDES)
        \param [in] top Top angle (degrees)
        \param [in] right Right angle (degrees)
        \param [in] bottom Bottom angle (degrees)
        \param [in] left Left angle (degrees)
    */

    void SetXSecTanAngles(const string &xsec_id, int side, double top, double right, double bottom, double left)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecTanAngles::Can't Find XSec " + xsec_id);
            return;
        }
        SkinXSec *skinxs = dynamic_cast<SkinXSec *>(xs);
        if (!skinxs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecTanAngles::Can't Convert To Skin XSec " + xsec_id);
            return;
        }

        skinxs->SetTanAngles(side, top, right, bottom, left);
        ErrorMgr.NoError();
    }

    /*!
        Set the tangent slew angles for the specified XSec
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        int num_xsecs = GetNumXSec( xsec_surf );

        for ( int i = 0 ; i < num_xsecs ; i++ )
        {
            string xsec = GetXSec( xsec_surf, i );

            SetXSecTanSlews( xsec, XSEC_BOTH_SIDES, 5.0 );       // Set Tangent Slews At Cross Section
        }
        \endcode
        \sa XSEC_SIDES_TYPE
        \param [in] xsec_id XSec ID
        \param [in] side Side type enum (i.e. XSEC_BOTH_SIDES)
        \param [in] top Top angle (degrees)
        \param [in] right Right angle (degrees)
        \param [in] bottom Bottom angle (degrees)
        \param [in] left Left angle (degrees)
    */

    void SetXSecTanSlews(const string &xsec_id, int side, double top, double right, double bottom, double left)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecTanSlews::Can't Find XSec " + xsec_id);
            return;
        }
        SkinXSec *skinxs = dynamic_cast<SkinXSec *>(xs);
        if (!skinxs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecTanSlews::Can't Convert To Skin XSec " + xsec_id);
            return;
        }

        skinxs->SetTanSlews(side, top, right, bottom, left);
        ErrorMgr.NoError();
    }

    /*!
        Set the tangent strengths for the specified XSec
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        // Flatten ends
        int num_xsecs = GetNumXSec( xsec_surf );

        for ( int i = 0 ; i < num_xsecs ; i++ )
        {
            string xsec = GetXSec( xsec_surf, i );

            SetXSecTanStrengths( xsec, XSEC_BOTH_SIDES, 0.8 );  // Set Tangent Strengths At Cross Section
        }
        \endcode
        \sa XSEC_SIDES_TYPE
        \param [in] xsec_id XSec ID
        \param [in] side Side type enum (i.e. XSEC_BOTH_SIDES)
        \param [in] top Top strength
        \param [in] right Right strength
        \param [in] bottom Bottom strength
        \param [in] left Left strength
    */

    void SetXSecTanStrengths(const string &xsec_id, int side, double top, double right, double bottom, double left)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecTanStrengths::Can't Find XSec " + xsec_id);
            return;
        }
        SkinXSec *skinxs = dynamic_cast<SkinXSec *>(xs);
        if (!skinxs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecTanStrengths::Can't Convert To Skin XSec " + xsec_id);
            return;
        }

        skinxs->SetTanStrengths(side, top, right, bottom, left);
        ErrorMgr.NoError();
    }

    /*!
        Set curvatures for the specified XSec
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        // Flatten ends
        int num_xsecs = GetNumXSec( xsec_surf );

        for ( int i = 0 ; i < num_xsecs ; i++ )
        {
            string xsec = GetXSec( xsec_surf, i );

            SetXSecCurvatures( xsec, XSEC_BOTH_SIDES, 0.2 );  // Set Tangent Strengths At Cross Section
        }
        \endcode
        \sa XSEC_SIDES_TYPE
        \param [in] xsec_id XSec ID
        \param [in] side Side type enum (i.e. XSEC_BOTH_SIDES)
        \param [in] top Top curvature
        \param [in] right Right curvature
        \param [in] bottom Bottom curvature
        \param [in] left Left curvature
    */

    void SetXSecCurvatures(const string &xsec_id, int side, double top, double right, double bottom, double left)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecCurvatures::Can't Find XSec " + xsec_id);
            return;
        }
        SkinXSec *skinxs = dynamic_cast<SkinXSec *>(xs);
        if (!skinxs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetXSecCurvatures::Can't Convert To Skin XSec " + xsec_id);
            return;
        }

        skinxs->SetCurvatures(side, top, right, bottom, left);
        ErrorMgr.NoError();
    }

    /*!
        Read in XSec shape from airfoil file and set to the specified XSec. The XSec must be of type XS_FILE_AIRFOIL. Airfoil files may be in Lednicer or Selig format with *.af or *.dat extensions.
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_FILE_AIRFOIL );

        string xsec = GetXSec( xsec_surf, 1 );

        ReadFileAirfoil( xsec, "airfoil/N0012_VSP.af" );
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] file_name Airfoil XSec file name
    */

    void ReadFileAirfoil(const string &xsec_id, const string &file_name)
    {
        vector<vec3d> pnt_vec;
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ReadFileAirfoil::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() == XS_FILE_AIRFOIL)
        {
            FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xs->GetXSecCurve());
            assert(file_xs);
            if (file_xs->ReadFile(file_name))
            {
                ErrorMgr.NoError();
                return;
            }
            else
            {
                ErrorMgr.AddError(VSP_FILE_DOES_NOT_EXIST, "ReadFileAirfoil::Error reading airfoil file " + file_name);
                return;
            }
        }

        ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "ReadFileAirfoil::XSec Not XS_FILE_AIRFOIL Type " + xsec_id);
        return;
    }

    /*!
        Set the upper points for an airfoil. The XSec must be of type XS_FILE_AIRFOIL.
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_FILE_AIRFOIL );

        string xsec = GetXSec( xsec_surf, 1 );

        ReadFileAirfoil( xsec, "airfoil/N0012_VSP.af" );

        array< vec3d > @up_array = GetAirfoilUpperPnts( xsec );

        for ( int i = 0 ; i < int( up_array.size() ) ; i++ )
        {
            up_array[i].scale_y( 2.0 );
        }

        SetAirfoilUpperPnts( xsec, up_array );
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] up_pnt_vec Array of points defining the upper surface of the airfoil
    */

    void SetAirfoilUpperPnts(const string &xsec_id, const std::vector<vec3d> &up_pnt_vec)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetAirfoilUpperPnts::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_FILE_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetAirfoilUpperPnts::XSec Not XS_FILE_AIRFOIL Type");
            return;
        }

        FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xs->GetXSecCurve());
        assert(file_xs);
        file_xs->SetAirfoilUpperPnts(up_pnt_vec);
        ErrorMgr.NoError();
    }

    /*!
        Set the ower points for an airfoil. The XSec must be of type XS_FILE_AIRFOIL.
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_FILE_AIRFOIL );

        string xsec = GetXSec( xsec_surf, 1 );

        ReadFileAirfoil( xsec, "airfoil/N0012_VSP.af" );

        array< vec3d > @low_array = GetAirfoilLowerPnts( xsec );

        for ( int i = 0 ; i < int( low_array.size() ) ; i++ )
        {
            low_array[i].scale_y( 0.5 );
        }

        SetAirfoilUpperPnts( xsec, up_array );
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] low_pnt_vec Array of points defining the lower surface of the airfoil
    */

    void SetAirfoilLowerPnts(const string &xsec_id, const std::vector<vec3d> &low_pnt_vec)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetAirfoilLowerPnts::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_FILE_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetAirfoilLowerPnts::XSec Not XS_FILE_AIRFOIL Type");
            return;
        }

        FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xs->GetXSecCurve());
        assert(file_xs);
        file_xs->SetAirfoilLowerPnts(low_pnt_vec);
        ErrorMgr.NoError();
    }

    /*!
        Set the upper and lower points for an airfoil. The XSec must be of type XS_FILE_AIRFOIL.
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_FILE_AIRFOIL );

        string xsec = GetXSec( xsec_surf, 1 );

        ReadFileAirfoil( xsec, "airfoil/N0012_VSP.af" );

        array< vec3d > @up_array = GetAirfoilUpperPnts( xsec );

        array< vec3d > @low_array = GetAirfoilLowerPnts( xsec );

        for ( int i = 0 ; i < int( up_array.size() ) ; i++ )
        {
            up_array[i].scale_y( 2.0 );

            low_array[i].scale_y( 0.5 );
        }

        SetAirfoilPnts( xsec, up_array, low_array );
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] up_pnt_vec Array of points defining the upper surface of the airfoil
        \param [in] low_pnt_vec Array of points defining the lower surface of the airfoil
    */

    void SetAirfoilPnts(const string &xsec_id, const std::vector<vec3d> &up_pnt_vec, const std::vector<vec3d> &low_pnt_vec)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetAirfoilPnts::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_FILE_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetAirfoilPnts::XSec Not XS_FILE_AIRFOIL Type");
            return;
        }

        FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xs->GetXSecCurve());
        assert(file_xs);
        file_xs->SetAirfoilPnts(up_pnt_vec, low_pnt_vec);
        ErrorMgr.NoError();
    }

    void WriteSeligAirfoilFile(const std::string &airfoil_name, std::vector<vec3d> &ordered_airfoil_pnts)
    {
        // Note, the input airfoil coordinate points must be ordered in the correct Selig format: Start at X = 1, proceed
        //  along the top of the airfoil to x = 0.0 at the leading edge, and return to X = 1 along the bottom surface

        //==== Open file ====//
        string file_name = airfoil_name + ".dat";
        FILE *af = fopen(file_name.c_str(), "w");
        if (!af)
        {
            ErrorMgr.AddError(VSP_FILE_WRITE_FAILURE, "WriteSeligAirfoilFile::Error writing airfoil file " + airfoil_name);
            return;
        }

        string header = airfoil_name + " AIRFOIL\n";
        fprintf(af, "%s", header.c_str());

        char buff[256];

        for (size_t i = 0; i < ordered_airfoil_pnts.size(); i++)
        {
            sprintf(buff, " %7.6f     %7.6f\n", ordered_airfoil_pnts[i].x(), ordered_airfoil_pnts[i].y());
            fprintf(af, "%s", buff);
        }

        fclose(af);
        ErrorMgr.NoError();
    }

    struct LLT_Data // Struct containing Lifting Line Theory data
    {
        vector<long double> y_span_vec; // y position across half span
        vector<long double> gamma_vec;  // circulation
        vector<long double> w_vec;      // downwash velocity
        vector<long double> cl_vec;     // lift coefficient
        vector<long double> cd_vec;     // induced drag coefficient
    };

    LLT_Data GetHersheyLLTData(const unsigned int &npts, const long double &alpha, const long double &Vinf, const long double &span)
    {
        LLT_Data llt_data;

        const long double alpha0 = 0.0; // zero lift angle of attack (rad)
        const long double c = 1.0;      // root/tip chord

        vector<long double> theta_vec, r_vec, a_vec;
        vector<int> odd_vec;
        theta_vec.resize(npts);
        odd_vec.resize(npts);
        r_vec.resize(npts);
        a_vec.resize(npts);

        llt_data.y_span_vec.resize(npts);
        llt_data.gamma_vec.resize(npts);
        llt_data.w_vec.resize(npts);
        llt_data.cl_vec.resize(npts);
        llt_data.cd_vec.resize(npts);

        Eigen::Matrix<long double, Eigen::Dynamic, Eigen::Dynamic> c_mat;
        c_mat.resize(npts, npts);

        for (size_t i = 0; i < npts; i++)
        {
            theta_vec[i] = ((double)i + 1.0l) * ((EIGEN_PI / 2.0l) / ((double)npts)); // [0 to pi/2]
            llt_data.y_span_vec[i] = cos(theta_vec[i]) * (span / 2.0l);               // [tip to root]
            odd_vec[i] = 2 * i + 1;
            r_vec[i] = EIGEN_PI * c / 4.0l / (span / 2.0l) * (alpha - alpha0) * sin(theta_vec[i]);
        }

        for (size_t i = 0; i < npts; i++)
        {
            for (size_t j = 0; j < npts; j++)
            {
                c_mat(i, j) = sin(theta_vec[j] * (double)odd_vec[i]) * (EIGEN_PI * c * odd_vec[i] / 4.0l / (span / 2.0l) + sin(theta_vec[j]));
            }
        }

        // Invert the matrix
        c_mat = c_mat.inverse();

        // Matrix multiplication: [N,N]x[N,1]
        for (size_t i = 0; i < npts; i++)
        {
            for (size_t j = 0; j < npts; j++)
            {
                a_vec[i] += c_mat(j, i) * r_vec[j];
            }
        }

        // Matrix multiplication: [N,N]x[N,1]
        for (size_t i = 0; i < npts; i++)
        {
            for (size_t j = 0; j < npts; j++)
            {
                llt_data.gamma_vec[i] += 4.0l * Vinf * (span / 2.0l) * sin(theta_vec[i] * (double)odd_vec[j]) * a_vec[j];
                llt_data.w_vec[i] += Vinf * (span / 2.0l) * (double)odd_vec[j] * a_vec[j] * sin(theta_vec[i] * (double)odd_vec[j]) / sin(theta_vec[i]);
            }

            llt_data.cl_vec[i] = 2.0l * llt_data.gamma_vec[i] / Vinf;
            llt_data.cd_vec[i] = 2.0l * llt_data.w_vec[i] * llt_data.gamma_vec[i] / (c * (span / 2.0l) * pow(Vinf, 2.0));
        }

        return llt_data;
    }

    /*!
        Get the theoretical lift (Cl) distribution for a Hershey Bar wing with unit chord length using Glauert's Method. This function was initially created to compare VSPAERO results to Lifting Line Theory.
        If full_span_flag is set to true symmetry is applied to the results.
        \code{.cpp}
        // Compute theoretical lift and drag distributions using 100 points
        double Vinf = 100;

        double halfAR = 20;

        double alpha_deg = 10;

        int n_pts = 100;

        array<vec3d> cl_dist_theo = GetHersheyBarLiftDist( int( n_pts ), Deg2Rad( alpha_deg ), Vinf, ( 2 * halfAR ), false );

        array<vec3d> cd_dist_theo = GetHersheyBarDragDist( int( n_pts ), Deg2Rad( alpha_deg ), Vinf, ( 2 * halfAR ), false );
        \endcode
        \param [in] npts Number of points along the span to assess
        \param [in] alpha Wing angle of attack (Radians)
        \param [in] Vinf Freestream velocity
        \param [in] span Hershey Bar full-span. Note, only half is used in the calculation
        \param [in] full_span_flag Flag to apply symmetry to results
        \return Theoretical coefficient of lift distribution array (size = 2*npts if full_span_flag = true)
    */

    std::vector<vec3d> GetHersheyBarLiftDist(const int &npts, const double &alpha, const double &Vinf, const double &span, bool full_span_flag)
    {
        // Calculation of lift distribution for a Hershey Bar wing with unit chord length using Glauert's Method
        //  Input span is the entire wing span, which half is used in the following calculations. If full_span_flag == true,
        //  symmetry is applied to the results. Input alpha must be in radians.

        LLT_Data llt_data = GetHersheyLLTData(npts, alpha, Vinf, span);

        vector<vec3d> y_cl_vec;
        if (full_span_flag)
        {
            y_cl_vec.resize(2 * npts);

            for (size_t i = 0; i < npts; i++)
            {
                y_cl_vec[i] = vec3d(-1 * llt_data.y_span_vec[i], llt_data.cl_vec[i], 0.0);
            }

            for (size_t i = 0; i < npts; i++)
            {
                y_cl_vec[(2 * npts - 1) - i] = vec3d(llt_data.y_span_vec[i], llt_data.cl_vec[i], 0.0); // Apply symmetry
            }
        }
        else
        {
            y_cl_vec.resize(npts);

            for (size_t i = 0; i < npts; i++)
            {
                y_cl_vec[i] = vec3d(llt_data.y_span_vec[i], llt_data.cl_vec[i], 0.0);
            }

            std::reverse(y_cl_vec.begin(), y_cl_vec.end());
        }

        return y_cl_vec;
    }

    /*!
        Get the theoretical drag (Cd) distribution for a Hershey Bar wing with unit chord length using Glauert's Method. This function was initially created to compare VSPAERO results to Lifting Line Theory.
        If full_span_flag is set to true symmetry is applied to the results.
        \code{.cpp}
        // Compute theoretical lift and drag distributions using 100 points
        double Vinf = 100;

        double halfAR = 20;

        double alpha_deg = 10;

        int n_pts = 100;

        array<vec3d> cl_dist_theo = GetHersheyBarLiftDist( int( n_pts ), Deg2Rad( alpha_deg ), Vinf, ( 2 * halfAR ), false );

        array<vec3d> cd_dist_theo = GetHersheyBarDragDist( int( n_pts ), Deg2Rad( alpha_deg ), Vinf, ( 2 * halfAR ), false );
        \endcode
        \param [in] npts Number of points along the span to assess
        \param [in] alpha Wing angle of attack (Radians)
        \param [in] Vinf Freestream velocity
        \param [in] span Hershey Bar full-span. Note, only half is used in the calculation
        \param [in] full_span_flag Flag to apply symmetry to results (default: false)
        \return Theoretical coefficient of drag distribution array (size = 2*npts if full_span_flag = true)
    */

    std::vector<vec3d> GetHersheyBarDragDist(const int &npts, const double &alpha, const double &Vinf, const double &span, bool full_span_flag)
    {
        // Calculation of drag distribution for a Hershey Bar wing with unit chord length using Glauert's Method.
        //  Input span is the entire wing span, which half is used in the following calculations. If full_span_flag == true,
        //  symmetry is applied to the results. Input alpha must be in radians.

        LLT_Data llt_data = GetHersheyLLTData(npts, alpha, Vinf, span);

        vector<vec3d> y_cd_vec;
        if (full_span_flag)
        {
            y_cd_vec.resize(2 * npts);

            for (size_t i = 0; i < npts; i++)
            {
                y_cd_vec[i] = vec3d(-1 * llt_data.y_span_vec[i], llt_data.cd_vec[i], 0.0);
            }

            for (size_t i = 0; i < npts; i++)
            {
                y_cd_vec[(2 * npts - 1) - i] = vec3d(llt_data.y_span_vec[i], llt_data.cd_vec[i], 0.0); // Apply symmetry
            }
        }
        else
        {
            y_cd_vec.resize(npts);

            for (size_t i = 0; i < npts; i++)
            {
                y_cd_vec[i] = vec3d(llt_data.y_span_vec[i], llt_data.cd_vec[i], 0.0);
            }

            std::reverse(y_cd_vec.begin(), y_cd_vec.end());
        }

        return y_cd_vec;
    }

    /*!
        Get the 2D coordinates an input number of points along a Von K�rm�n-Trefftz airfoil of specified shape
        \code{.cpp}
        const double pi = 3.14159265358979323846;

        const int npts = 122;

        const double alpha = 0.0;

        const double epsilon = 0.1;

        const double kappa = 0.1;

        const double tau = 10;

        array<vec3d> xyz_airfoil = GetVKTAirfoilPnts(npts, alpha, epsilon, kappa, tau*(pi/180) );

        array<double> cp_dist = GetVKTAirfoilCpDist( alpha, epsilon, kappa, tau*(pi/180), xyz_airfoil );
        \endcode
        \param [in] npts Number of points along the airfoil to return
        \param [in] alpha Airfoil angle of attack (Radians)
        \param [in] epsilon Airfoil thickness
        \param [in] kappa Airfoil camber
        \param [in] tau Airfoil trailing edge angle (Radians)
        \return Array of points on the VKT airfoil (size = npts)
    */

    std::vector<vec3d> GetVKTAirfoilPnts(const int &npts, const double &alpha, const double &epsilon, const double &kappa, const double &tau)
    {
        // alpha = Angle of attack( radian )
        // epsilon = Thickness
        // kappa = Camber
        // tau = Trailing edge angle( radian )
        // npts = # of nodes in the circumferential direction

        const double ell = 0.25; // chord length = 4 * ell

        vector<vec3d> xyzdata;
        xyzdata.resize(npts);

        double a = ell * sqrt((1.0 + epsilon) * (1.0 + epsilon) + kappa * kappa); // Radius of circle
        double beta = asin(ell * kappa / a);                                      // Angle of TE location (rad)
        double n = 2.0 - tau / EIGEN_PI;
        doublec mu = doublec(-ell * epsilon, ell * kappa); // Center of circle

        if ((ell * kappa / a) > 1.0)
        {
            ErrorMgr.AddError(VSP_INVALID_INPUT_VAL, "GetVKTAirfoilPnts: Camber parameter, kappa, is too large");
            return xyzdata;
        }

        int ile = 0;
        double dmax = -1.0;
        // Evaluate points and track furthest from TE as surrogate for LE.
        // Would be better to identify LE as tightest curvature or similar.
        for (size_t p = 0; p < npts; p++)
        {
            // Clockwise from TE
            double theta = 2.0 * EIGEN_PI * (1.0 - p * 1.0 / (npts - 1)); // rad

            double xi = a * cos(theta - beta) + mu.real();
            double eta = a * sin(theta - beta) + mu.imag();
            doublec zeta = doublec(xi, eta);

            // Karman-Trefftz transformation
            doublec temp = pow(zeta - ell, n) / pow(zeta + ell, n);
            doublec z = n * ell * (1.0 + temp) / (1.0 - temp);
            xyzdata[p].set_xyz(z.real(), z.imag(), 0.0);

            // Find point furthest from TE.  Declare that the LE.
            double d = dist(xyzdata[p], xyzdata[0]);
            if (d > dmax)
            {
                dmax = d;
                ile = p;
            }
        }

        xyzdata[npts - 1] = xyzdata[0]; // Ensure closure

        // Shift and scale airfoil such that xle=0 and xte=1.
        double scale = xyzdata[0].x() - xyzdata[ile].x();
        double xshift = xyzdata[ile].x();

        for (size_t j = 0; j < npts; j++)
        {
            xyzdata[j].offset_x(-1 * xshift);
            xyzdata[j] = xyzdata[j] / scale;
        }

        return xyzdata;
    }

    /*!
        Get the pressure coefficient (Cp) along a Von Kármán-Trefftz airfoil of specified shape at specified points along the airfoil
        \code{.cpp}
        const double pi = 3.14159265358979323846;

        const int npts = 122;

        const double alpha = 0.0;

        const double epsilon = 0.1;

        const double kappa = 0.1;

        const double tau = 10;

        array<vec3d> xyz_airfoil = GetVKTAirfoilPnts(npts, alpha, epsilon, kappa, tau*(pi/180) );

        array<double> cp_dist = GetVKTAirfoilCpDist( alpha, epsilon, kappa, tau*(pi/180), xyz_airfoil );
        \endcode
        \sa GetVKTAirfoilPnts
        \param [in] alpha Airfoil angle of attack (Radians)
        \param [in] epsilon Airfoil thickness
        \param [in] kappa Airfoil camber
        \param [in] tau Airfoil trailing edge angle (Radians)
        \param [in] xydata Array of points on the airfoil to evaluate
        \return Array of Cp values for each point in xydata
    */

    std::vector<double> GetVKTAirfoilCpDist(const double &alpha, const double &epsilon, const double &kappa, const double &tau, std::vector<vec3d> xyzdata)
    {
        // alpha = Angle of attack( radian )
        // epsilon = Thickness
        // kappa = Camber
        // tau = Trailing edge angle( radian )
        // xyzdata = output from vsp::GetVKTAirfoilPnts

        doublec i(0, 1);
        const double ell = 0.25; // chord length = 4 * ell

        const unsigned int npts = xyzdata.size();

        vector<double> cpdata;
        cpdata.resize(npts);

        double a = ell * sqrt((1.0 + epsilon) * (1.0 + epsilon) + kappa * kappa); // Radius of circle
        double beta = asin(ell * kappa / a);                                      // Angle of TE location (rad)
        double n = 2.0 - tau / EIGEN_PI;
        doublec mu = doublec(-ell * epsilon, ell * kappa); // Center of circle

        if ((ell * kappa / a) > 1.0)
        {
            ErrorMgr.AddError(VSP_INVALID_INPUT_VAL, "GetVKTAirfoilCpDist: Camber parameter, kappa, is too large");
            return cpdata;
        }

        int ile = 0;
        double dmax = -1.0;
        // Evaluate points and track furthest from TE as surrogate for LE.
        // Would be better to identify LE as tightest curvature or similar.
        for (size_t p = 0; p < npts; p++)
        {
            // Clockwise from TE
            double theta = 2.0 * EIGEN_PI * (1.0 - p * 1.0 / (npts - 1)); // rad

            double xi = a * cos(theta - beta) + mu.real();
            double eta = a * sin(theta - beta) + mu.imag();
            doublec zeta = doublec(xi, eta);

            // w(zeta): Complex velocity in the circle plane (a flow around a cylinder)
            doublec w = cmplx_velocity(zeta, alpha, beta, a, mu);

            // Compute the velocity in the airfoil plane : ( u, v ) = w / ( dZ / dzeta )
            // Derivative of the Karman - Trefftz transformation:
            doublec dzdzeta = derivative(zeta, ell, n);

            double u, v;

            if (std::abs(theta) <= FLT_EPSILON || std::abs(theta - 2.0 * EIGEN_PI) <= FLT_EPSILON) // Special treatment at the trailing edge (theta = 0.0 or 2*pi)
            {
                if (std::abs(tau) <= FLT_EPSILON) // Joukowski airfoil (cusped trailing edge: tau = 0.0 )
                {
                    doublec uv = (ell / a) * exp(2.0 * i * beta) * cos(alpha + beta);
                    u = uv.real();
                    v = -1 * uv.imag();
                }
                else // Karman-Trefftz airfoil (finite angle: tau > 0.0), TE must be a stagnation point.
                {
                    u = 0.0;
                    v = 0.0;
                }
            }
            else
            {
                doublec uv = w / dzdzeta;
                u = uv.real();
                v = -1 * uv.imag();
            }

            cpdata[p] = 1.0 - (pow(u, 2.0) + (pow(v, 2.0)));
        }

        return cpdata;
    }

    /*!
        Generate the surface coordinate points for a ellipsoid at specified center of input radius along each axis.
        Based on the MATLAB function ellipsoid (https://in.mathworks.com/help/matlab/ref/ellipsoid.html).
        \sa GetVKTAirfoilPnts
        \param [in] center 3D location of the ellipsoid center
        \param [in] abc_rad Radius along the A (X), B (Y), and C (Z) axes
        \param [in] u_npts Number of points in the U direction
        \param [in] w_npts Number of points in the W direction
        \return Array of coordinates describing the ellipsoid surface
    */

    std::vector<vec3d> GetEllipsoidSurfPnts(const vec3d &center, const vec3d &abc_rad, int u_npts, int w_npts)
    {
        // Generate the surface points for a ellipsoid of input abc radius vector at center. Based on the Matlab function ellipsoid.m
        if (u_npts < 20)
        {
            u_npts = 20;
        }
        if (w_npts < 20)
        {
            w_npts = 20;
        }

        vector<vec3d> surf_pnt_vec;

        vector<double> theta_vec, phi_vec;
        theta_vec.resize(u_npts);
        phi_vec.resize(w_npts);

        theta_vec[0] = 0.0; // theta: [0,2PI]
        phi_vec[0] = 0.0;   // phi: [0,PI]

        const double theta_step = 2 * EIGEN_PI / (u_npts - 1);
        const double phi_step = EIGEN_PI / (w_npts - 1);

        for (size_t i = 1; i < u_npts; i++)
        {
            theta_vec[i] = theta_vec[i - 1] + theta_step;
        }

        for (size_t i = 1; i < w_npts; i++)
        {
            phi_vec[i] = phi_vec[i - 1] + phi_step;
        }

        for (size_t u = 0; u < u_npts; u++)
        {
            for (size_t w = 0; w < w_npts; w++)
            {
                surf_pnt_vec.push_back(vec3d((abc_rad.x() * cos(theta_vec[u]) * sin(phi_vec[w]) + center.x()),
                                             (abc_rad.y() * sin(theta_vec[u]) * sin(phi_vec[w]) + center.y()),
                                             (abc_rad.z() * cos(phi_vec[w]) + center.z())));
            }
        }

        return surf_pnt_vec;
    }

    /*!
        Get the points along the feature lines of a particular Geom
        \param [in] geom_id Geom ID
        \return Array of points along the Geom's feature lines
    */

    std::vector<vec3d> GetFeatureLinePnts(const string &geom_id)
    {
        vector<vec3d> pnt_vec;

        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetFeatureLinePnts::Can't Find Geom " + geom_id);
            return pnt_vec;
        }

        vector<VspSurf> surf_vec;
        surf_vec = geom_ptr->GetSurfVecConstRef();

        double tol = 1e-2;

        for (size_t i = 0; i < surf_vec.size(); i++)
        {
            // U feature lines
            for (int j = 0; j < surf_vec[0].GetNumUFeature(); j++)
            {
                vector<vec3d> ptline;
                surf_vec[i].TessUFeatureLine(j, ptline, tol);

                for (size_t k = 0; k < ptline.size(); k++)
                {
                    pnt_vec.push_back(ptline[k]);
                }
            }

            // V feature lines
            for (int j = 0; j < surf_vec[0].GetNumWFeature(); j++)
            {
                vector<vec3d> ptline;
                surf_vec[i].TessWFeatureLine(j, ptline, tol);

                for (size_t k = 0; k < ptline.size(); k++)
                {
                    pnt_vec.push_back(ptline[k]);
                }
            }
        }

        ErrorMgr.NoError();
        return pnt_vec;
    }

    /*!
        Generate Analytical Solution for Potential Flow for specified ellipsoid shape at input surface points for input velocity vector.
        Based on Munk, M. M., 'Remarks on the Pressure Distribution over the Surface of an Ellipsoid, Moving Translationally Through a Perfect
        Fluid,' NACA TN-196, June 1924. Function initially created to compare VSPAERO results to theory.
        \code{.cpp}
        const double pi = 3.14159265358979323846;

        const int npts = 101;

        const vec3d abc_rad = vec3d(1.0, 2.0, 3.0);

        const double alpha = 5; // deg

        const double beta = 5; // deg

        const double V_inf = 100.0;

        array < vec3d > x_slice_pnt_vec(npts);
        array<double> theta_vec(npts);

        theta_vec[0] = 0;

        for ( int i = 1; i < npts; i++ )
        {
            theta_vec[i] = theta_vec[i-1] + (2 * pi / ( npts - 1) );
        }

        for ( int i = 0; i < npts; i++ )
        {
            x_slice_pnt_vec[i] = vec3d( 0, abc_rad[1] * cos( theta_vec[i] ), abc_rad[2] *sin( theta_vec[i] ) );
        }

        vec3d V_vec = vec3d( ( V_inf * cos( Deg2Rad( alpha ) ) * cos( Deg2Rad( beta ) ) ), ( V_inf * sin( Deg2Rad( beta ) ) ), ( V_inf * sin( Deg2Rad( alpha ) ) * cos( Deg2Rad( beta ) ) ) );

        array < double > cp_dist = GetEllipsoidCpDist( x_slice_pnt_vec, abc_rad, V_vec );
        \endcode
        \sa GetEllipsoidSurfPnts
        \param [in] surf_pnt_arr Array of points on the ellipsoid surface to assess
        \param [in] abc_rad Radius along the A (X), B (Y), and C (Z) axes
        \param [in] V_inf 3D components of freestream velocity
        \return Array of Cp results corresponding to each point in surf_pnt_arr
    */

    std::vector<double> GetEllipsoidCpDist(const std::vector<vec3d> &surf_pnt_vec, const vec3d &abc_rad, const vec3d &V_inf)
    {
        // Generate Analytical Solution for Potential Flow at input ellipsoid surface points for input velocity vector (V).
        //  Based on Munk, M. M., 'Remarks on the Pressure Distribution over the Surface of an Ellipsoid, Moving Translationally
        //  Through a Perfect Fluid,' NACA TN-196, June 1924.

        double alpha = abc_rad.x() * abc_rad.y() * abc_rad.z() * IntegrateEllipsoidFlow(abc_rad, 0);
        double beta = abc_rad.x() * abc_rad.y() * abc_rad.z() * IntegrateEllipsoidFlow(abc_rad, 1);
        double gamma = abc_rad.x() * abc_rad.y() * abc_rad.z() * IntegrateEllipsoidFlow(abc_rad, 2);

        double k1 = alpha / (2.0 - alpha);
        double k2 = beta / (2.0 - beta);
        double k3 = gamma / (2.0 - gamma);

        double A = k1 + 1;
        double B = k2 + 1;
        double C = k3 + 1;

        vector<vec3d> pot_vec, uvw_vec;
        vector<double> cp_vec;
        pot_vec.resize(surf_pnt_vec.size());
        uvw_vec.resize(surf_pnt_vec.size());
        cp_vec.resize(surf_pnt_vec.size());

        double Vmax_x = A * V_inf.x();
        double Vmax_y = B * V_inf.y();
        double Vmax_z = C * V_inf.z();

        for (size_t i = 0; i < surf_pnt_vec.size(); i++)
        {
            // Velocity potential
            pot_vec[i] = vec3d((Vmax_x * surf_pnt_vec[i].x()), (Vmax_y * surf_pnt_vec[i].y()), (Vmax_z * surf_pnt_vec[i].z()));

            // Normal vector
            vec3d norm((2.0 * surf_pnt_vec[i].x() / pow(abc_rad.x(), 2.0)),
                       (2.0 * surf_pnt_vec[i].y() / pow(abc_rad.y(), 2.0)),
                       (2.0 * surf_pnt_vec[i].z() / pow(abc_rad.z(), 2.0)));

            norm.normalize();

            // Vmax component in panel normal direction
            double Vnorm = Vmax_x * norm.x() + Vmax_y * norm.y() + Vmax_z * norm.z();

            // Surface velocity
            uvw_vec[i] = vec3d((Vmax_x - Vnorm * norm.x()), (Vmax_y - Vnorm * norm.y()), (Vmax_z - Vnorm * norm.z()));

            // Pressure Coefficient
            cp_vec[i] = 1.0 - pow((uvw_vec[i].mag() / V_inf.mag()), 2.0);
        }

        return cp_vec;
    }

    struct ellipsoid_flow_functor
    {
        double operator()(const double &t)
        {
            return (1.0 / ((pow(abc_rad[abc_index], 2.0) + t) * sqrt((pow(abc_rad.x(), 2.0) + t) * (pow(abc_rad.y(), 2.0) + t) * (pow(abc_rad.z(), 2.0) + t))));
        }
        vec3d abc_rad;
        int abc_index; // a: 0, b: 1, c: 2
    };

    double IntegrateEllipsoidFlow(const vec3d &abc_rad, const int &abc_index)
    {
        // Integration of Equations 6 and 7 for alpha, beta, and gamma in "Hydrodynamics" by Horace Lamb, Ch.5, Section 111, pg. 162.
        //  abc_index corresponds to a:0 for alpha, b:1 for beta, and c:2 for gamma
        ellipsoid_flow_functor fun;
        fun.abc_rad = abc_rad;
        fun.abc_index = abc_index;

        eli::mutil::quad::simpson<double> quad;

        return quad(fun, 0.0, 1.0e8); // Integrate from 0 to inf (Note: an upper limit greater than 1.0e8 will produce errors)
    }

    /*!
        Get the coordinate points for the upper surface of an airfoil. The XSec must be of type XS_FILE_AIRFOIL
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_FILE_AIRFOIL );

        string xsec = GetXSec( xsec_surf, 1 );

        ReadFileAirfoil( xsec, "airfoil/N0012_VSP.af" );

        array< vec3d > @up_array = GetAirfoilUpperPnts( xsec );
        \endcode
        \sa SetAirfoilPnts
        \param [in] xsec_id XSec ID
        \return Array of coordinate points for the upper airfoil surface
    */

    std::vector<vec3d> GetAirfoilUpperPnts(const string &xsec_id)
    {
        vector<vec3d> pnt_vec;

        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetAirfoilUpperPnts::Can't Find XSec " + xsec_id);
            return pnt_vec;
        }

        if (xs->GetXSecCurve()->GetType() != XS_FILE_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetAirfoilUpperPnts::XSec Not XS_FILE_AIRFOIL Type");
            return pnt_vec;
        }

        FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xs->GetXSecCurve());
        assert(file_xs);
        pnt_vec = file_xs->GetUpperPnts();
        ErrorMgr.NoError();
        return pnt_vec;
    }

    /*!
        Get the coordinate points for the lower surface of an airfoil. The XSec must be of type XS_FILE_AIRFOIL
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_FILE_AIRFOIL );

        string xsec = GetXSec( xsec_surf, 1 );

        ReadFileAirfoil( xsec, "airfoil/N0012_VSP.af" );

        array< vec3d > @low_array = GetAirfoilLowerPnts( xsec );
        \endcode
        \sa SetAirfoilPnts
        \param [in] xsec_id XSec ID
        \return Array of coordinate points for the lower airfoil surface
    */

    std::vector<vec3d> GetAirfoilLowerPnts(const string &xsec_id)
    {
        vector<vec3d> pnt_vec;

        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetAirfoilLowerPnts::Can't Find XSec " + xsec_id);
            return pnt_vec;
        }

        if (xs->GetXSecCurve()->GetType() != XS_FILE_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetAirfoilLowerPnts::XSec Not XS_FILE_AIRFOIL Type");
            return pnt_vec;
        }

        FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xs->GetXSecCurve());
        assert(file_xs);
        pnt_vec = file_xs->GetLowerPnts();
        ErrorMgr.NoError();
        return pnt_vec;
    }

    /*!
        Get the CST coefficients for the upper surface of an airfoil. The XSec must be of type XS_CST_AIRFOIL
        \sa SetUpperCST
        \param [in] xsec_id XSec ID
        \return Array of CST coefficients for the upper airfoil surface
    */

    std::vector<double> GetUpperCSTCoefs(const string &xsec_id)
    {
        vector<double> ret_vec;

        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetUpperCSTCoefs::Can't Find XSec " + xsec_id);
            return ret_vec;
        }

        if (xs->GetXSecCurve()->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetUpperCSTCoefs::XSec Not XS_CST_AIRFOIL Type");
            return ret_vec;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xs->GetXSecCurve());
        assert(cst_xs);

        ret_vec = cst_xs->GetUpperCST();
        ErrorMgr.NoError();
        return ret_vec;
    }

    /*!
        Get the CST coefficients for the lower surface of an airfoil. The XSec must be of type XS_CST_AIRFOIL
        \sa SetLowerCST
        \param [in] xsec_id XSec ID
        \return Array of CST coefficients for the lower airfoil surface
    */

    std::vector<double> GetLowerCSTCoefs(const string &xsec_id)
    {
        vector<double> ret_vec;

        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetLowerCSTCoefs::Can't Find XSec " + xsec_id);
            return ret_vec;
        }

        if (xs->GetXSecCurve()->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetLowerCSTCoefs::XSec Not XS_CST_AIRFOIL Type");
            return ret_vec;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xs->GetXSecCurve());
        assert(cst_xs);

        ret_vec = cst_xs->GetLowerCST();
        ErrorMgr.NoError();
        return ret_vec;
    }

    /*!
        Get the CST degree for the upper surface of an airfoil. The XSec must be of type XS_CST_AIRFOIL
        \sa SetUpperCST
        \param [in] xsec_id XSec ID
        \return CST Degree for upper airfoil surface
    */

    int GetUpperCSTDegree(const string &xsec_id)
    {
        int deg = -1;

        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetUpperCSTDegree::Can't Find XSec " + xsec_id);
            return deg;
        }

        if (xs->GetXSecCurve()->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetUpperCSTDegree::XSec Not XS_CST_AIRFOIL Type");
            return deg;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xs->GetXSecCurve());
        assert(cst_xs);

        deg = cst_xs->GetUpperDegree();
        ErrorMgr.NoError();
        return deg;
    }

    /*!
        Get the CST degree for the lower surface of an airfoil. The XSec must be of type XS_CST_AIRFOIL
        \sa SetLowerCST
        \param [in] xsec_id XSec ID
        \return CST Degree for lower airfoil surface
    */

    int GetLowerCSTDegree(const string &xsec_id)
    {
        int deg = -1;

        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetLowerCSTDegree::Can't Find XSec " + xsec_id);
            return deg;
        }

        if (xs->GetXSecCurve()->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetLowerCSTDegree::XSec Not XS_CST_AIRFOIL Type");
            return deg;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xs->GetXSecCurve());
        assert(cst_xs);

        deg = cst_xs->GetLowerDegree();
        ErrorMgr.NoError();
        return deg;
    }

    /*!
        Set the CST degree and coefficients for the upper surface of an airfoil. The number of coefficients should be one more than the CST degree. The XSec must be of type XS_CST_AIRFOIL
        \sa GetUpperCSTDegree, GetUpperCSTCoefs
        \param [in] xsec_id XSec ID
        \param [in] deg CST degree of upper airfoil surface
        \param [in] coeff_arr Array of CST coefficients for the upper airfoil surface
    */

    void SetUpperCST(const string &xsec_id, int deg, const std::vector<double> &coefs)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetUpperCST::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetUpperCST::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xs->GetXSecCurve());
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->SetUpperCST(deg, coefs);
    }

    /*!
        Set the CST degree and coefficients for the lower surface of an airfoil. The number of coefficients should be one more than the CST degree. The XSec must be of type XS_CST_AIRFOIL
        \sa GetLowerCSTDegree, GetLowerCSTCoefs
        \param [in] xsec_id XSec ID
        \param [in] deg CST degree of lower airfoil surface
        \param [in] coeff_arr Array of CST coefficients for the lower airfoil surface
    */

    void SetLowerCST(const string &xsec_id, int deg, const std::vector<double> &coefs)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetLowerCST::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetLowerCST::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xs->GetXSecCurve());
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->SetLowerCST(deg, coefs);
    }

    /*!
        Promote the CST for the upper airfoil surface. The XSec must be of type XS_CST_AIRFOIL
        \sa GetUpperCSTDegree
        \param [in] xsec_id XSec ID
    */

    void PromoteCSTUpper(const string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PromoteCSTUpper::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "PromoteCSTUpper::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xs->GetXSecCurve());
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->PromoteUpper();
    }

    /*!
        Promote the CST for the lower airfoil surface. The XSec must be of type XS_CST_AIRFOIL
        \sa GetLowerCSTDegree
        \param [in] xsec_id XSec ID
    */

    void PromoteCSTLower(const string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PromoteCSTLower::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "PromoteCSTLower::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xs->GetXSecCurve());
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->PromoteLower();
    }

    /*!
        Demote the CST for the upper airfoil surface. The XSec must be of type XS_CST_AIRFOIL
        \sa GetUpperCSTDegree
        \param [in] xsec_id XSec ID
    */

    void DemoteCSTUpper(const string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DemoteCSTUpper::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "DemoteCSTUpper::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xs->GetXSecCurve());
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->DemoteUpper();
    }

    /*!
        Demote the CST for the lower airfoil surface. The XSec must be of type XS_CST_AIRFOIL
        \sa GetLowerCSTDegree
        \param [in] xsec_id XSec ID
    */

    void DemoteCSTLower(const string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DemoteCSTLower::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "DemoteCSTLower::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xs->GetXSecCurve());
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->DemoteLower();
    }

    /*!
        Fit a CST airfoil for an existing airfoil of type XS_FOUR_SERIES, XS_SIX_SERIES, XS_FOUR_DIGIT_MOD, XS_FIVE_DIGIT, XS_FIVE_DIGIT_MOD, XS_ONE_SIX_SERIES, or XS_FILE_AIRFOIL.
        \param [in] xsec_surf_id XsecSurf ID
        \param [in] xsec_index XSec index
        \param [in] deg CST degree
    */

    void FitAfCST(const string &xsec_surf_id, int xsec_index, int deg)
    {
        XSecSurf *xsec_surf = FindXSecSurf(xsec_surf_id);
        if (!xsec_surf)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "FitAfCST::Can't Find XSecSurf " + xsec_surf_id);
            return;
        }
        XSec *xsec = xsec_surf->FindXSec(xsec_index);
        if (!xsec)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "FitAfCST::Can't Find XSec " + xsec_surf_id + ":" + to_string((long long)xsec_index));
            return;
        }

        if ((xsec->GetXSecCurve()->GetType() != XS_FOUR_SERIES) ||
            (xsec->GetXSecCurve()->GetType() != XS_SIX_SERIES) ||
            (xsec->GetXSecCurve()->GetType() != XS_FOUR_DIGIT_MOD) ||
            (xsec->GetXSecCurve()->GetType() != XS_FIVE_DIGIT) ||
            (xsec->GetXSecCurve()->GetType() != XS_FIVE_DIGIT_MOD) ||
            (xsec->GetXSecCurve()->GetType() != XS_ONE_SIX_SERIES) ||
            (xsec->GetXSecCurve()->GetType() != XS_FILE_AIRFOIL))
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "FitAfCST::XSec Not Fittable Airfoil Type");
            return;
        }

        XSecCurve *xsc = xsec->GetXSecCurve();
        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "FitAfCST::Can't Get XSecCurve");
            return;
        }

        Airfoil *af_xs = dynamic_cast<Airfoil *>(xsc);
        if (!af_xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "FitAfCST::Can't Get Airfoil");
            return;
        }

        VspCurve c = af_xs->GetOrigCurve();

        xsec_surf->ChangeXSecShape(xsec_index, XS_CST_AIRFOIL);

        XSec *newxsec = xsec_surf->FindXSec(xsec_index);
        if (!newxsec)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "FitAfCST::Can't Find New XSec " + xsec_surf_id + ":" + to_string((long long)xsec_index));
            return;
        }

        XSecCurve *newxsc = newxsec->GetXSecCurve();
        if (!newxsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "FitAfCST::Can't Get New XSecCurve");
            return;
        }

        if (newxsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "FitAfCST::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(newxsc);

        assert(cst_xs);
        cst_xs->FitCurve(c, deg);

        ErrorMgr.NoError();
    }

    /*!
        Write out the untwisted unit-length 2D Bezier curve for the specified airfoil in custom *.bz format. The output will describe the analytical shape of the airfoil. See BezierAirfoilExample.m and BezierCtrlToCoordPnts.m for examples of
        discretizing the Bezier curve and generating a Selig airfoil file.
        \code{.cpp}
        //==== Add Wing Geometry and Set Parms ====//
        string wing_id = AddGeom( "WING", "" );

        const double u = 0.5; // export airfoil at mid span location

        //==== Write Bezier Airfoil File ====//
        WriteBezierAirfoil( "Example_Bezier.bz", wing_id, u );
        \endcode
        \param [in] file_name Airfoil (*.bz) output file name
        \param [in] geom_id Geom ID
        \param [in] foilsurf_u U location (range: 0 - 1) along the surface. The foil surface does not include root and tip caps (i.e. 2 section wing -> XSec0 @ u=0, XSec1 @ u=0.5, XSec2 @ u=1.0)
    */

    void WriteBezierAirfoil(const std::string &file_name, const std::string &geom_id, const double &foilsurf_u)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "WriteBezierAirfoil::Can't Find Geom " + geom_id);
            return;
        }

        if (foilsurf_u < 0.0 || foilsurf_u > 1.0)
        {
            ErrorMgr.AddError(VSP_INVALID_INPUT_VAL, "WriteBezierAirfoil::Invalid u Location " + to_string(foilsurf_u) + " - Must be range [0,1].");
            return;
        }

        geom_ptr->WriteBezierAirfoil(file_name, foilsurf_u);
        ErrorMgr.NoError();
    }

    /*!
        Write out the untwisted unit-length 2D coordinate points for the specified airfoil in Selig format. Coordinate points follow the on-screen wire frame W tessellation.
        \code{.cpp}
        //==== Add Wing Geometry and Set Parms ====//
        string wing_id = AddGeom( "WING", "" );

        const double u = 0.5; // export airfoil at mid span location

        //==== Write Selig Airfoil File ====//
        WriteSeligAirfoil( "Example_Selig.dat", wing_id, u );
        \endcode
        \sa GetAirfoilCoordinates
        \param [in] file_name Airfoil (*.dat) output file name
        \param [in] geom_id Geom ID
        \param [in] foilsurf_u U location (range: 0 - 1) along the surface. The foil surface does not include root and tip caps (i.e. 2 section wing -> XSec0 @ u=0, XSec1 @ u=0.5, XSec2 @ u=1.0)
    */

    void WriteSeligAirfoil(const std::string &file_name, const std::string &geom_id, const double &foilsurf_u)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "WriteSeligAirfoil::Can't Find Geom " + geom_id);
            return;
        }

        if (foilsurf_u < 0.0 || foilsurf_u > 1.0)
        {
            ErrorMgr.AddError(VSP_INVALID_INPUT_VAL, "WriteSeligAirfoil::Invalid u Location " + to_string(foilsurf_u) + " - Must be range [0,1].");
            return;
        }

        geom_ptr->WriteSeligAirfoil(file_name, foilsurf_u);
        ErrorMgr.NoError();
    }

    /*!
        Get the untwisted unit-length 2D coordinate points for the specified airfoil
        \sa WriteSeligAirfoil
        \param [in] geom_id Geom ID
        \param [in] foilsurf_u U location (range: 0 - 1) along the surface. The foil surface does not include root and tip caps (i.e. 2 section wing -> XSec0 @ u=0, XSec1 @ u=0.5, XSec2 @ u=1.0)
    */

    vector<vec3d> GetAirfoilCoordinates(const std::string &geom_id, const double &foilsurf_u)
    {
        vector<vec3d> ordered_vec;
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetAirfoilCoordinates::Can't Find Geom " + geom_id);
            return ordered_vec;
        }

        if (foilsurf_u < 0.0 || foilsurf_u > 1.0)
        {
            ErrorMgr.AddError(VSP_INVALID_INPUT_VAL, "GetAirfoilCoordinates::Invalid u Location " + to_string(foilsurf_u) + " - Must be range [0,1].");
            return ordered_vec;
        }

        ordered_vec = geom_ptr->GetAirfoilCoordinates(foilsurf_u);
        ErrorMgr.NoError();
        return ordered_vec;
    }

    //===================================================================//
    //==================      BOR Functions        ======================//
    //===================================================================//

    //==== Specialized Geom Functions ====//

    /*!
        Set the XSec type for a BOR component
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        ChangeBORXSecShape( bor_id, XS_ROUNDED_RECTANGLE );

        if ( GetBORXSecShape( bor_id ) != XS_ROUNDED_RECTANGLE ) { Print( "ERROR: ChangeBORXSecShape" ); }
        \endcode
        \sa XSEC_CRV_TYPE
        \param [in] geom_id Geom ID
        \param [in] type XSec type enum (i.e. XS_ROUNDED_RECTANGLE)
    */

    void ChangeBORXSecShape(const string &geom_id, int type)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ChangeBORXSecShape::Can't Find Geom " + geom_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "ChangeBORXSecShape::Geom " + geom_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);
        bor_ptr->SetXSecCurveType(type);
        ErrorMgr.NoError();
    }

    /*!
        Get the XSec type for a BOR component
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        ChangeBORXSecShape( bor_id, XS_ROUNDED_RECTANGLE );

        if ( GetBORXSecShape( bor_id ) != XS_ROUNDED_RECTANGLE ) { Print( "ERROR: GetBORXSecShape" ); }
        \endcode
        \param [in] geom_id Geom ID
        \return XSec type enum (i.e. XS_ROUNDED_RECTANGLE)
    */

    int GetBORXSecShape(const string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORXSecShape::Can't Find Geom " + geom_id);
            return XS_UNDEFINED;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "GetBORXSecShape::Geom " + geom_id + " is not a body of revolution");
            return XS_UNDEFINED;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        ErrorMgr.NoError();
        return bor_ptr->GetXSecCurveType();
    }

    //==== Read XSec From File ====//

    /*!
        Set the coordinate points for a specific BOR. The BOR XSecCurve must be of type XS_FILE_FUSE.
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        ChangeBORXSecShape( bor_id, XS_FILE_FUSE );

        array< vec3d > @vec_array = ReadBORFileXSec( bor_id, "TestXSec.fxs" );
        \endcode
        \param [in] bor_id Geom ID of BOR
        \param [in] file_name Fuselage XSec file name
        \return Array of coordinate points read from the file and set to the XSec
    */

    vector<vec3d> ReadBORFileXSec(const string &bor_id, const string &file_name)
    {
        vector<vec3d> pnt_vec;
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ReadBORFileXSec::Can't Find Geom " + bor_id);
            return pnt_vec;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "ReadBORFileXSec::Geom " + bor_id + " is not a body of revolution");
            return pnt_vec;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ReadBORFileXSec::Can't Get XSecCurve");
            return pnt_vec;
        }

        if (xsc->GetType() == XS_FILE_FUSE)
        {
            FileXSec *file_xs = dynamic_cast<FileXSec *>(xsc);
            assert(file_xs);
            if (file_xs->ReadXsecFile(file_name))
            {
                ErrorMgr.NoError();
                return file_xs->GetUnityFilePnts();
            }
            else
            {
                ErrorMgr.AddError(VSP_FILE_DOES_NOT_EXIST, "ReadBORFileXSec::Error reading fuselage file " + file_name);
                return pnt_vec;
            }
        }

        ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "ReadBORFileXSec::XSec Not XS_FILE_FUSE Type ");
        return pnt_vec;
    }

    /*!
        Set the coordinate points for a specific BOR. The BOR XSecCurve must be of type XS_FILE_FUSE.
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        ChangeBORXSecShape( bor_id, XS_FILE_FUSE );

        array< vec3d > @vec_array = ReadBORFileXSec( bor_id, "TestXSec.fxs" );

        if ( vec_array.size() > 0 )
        {
            vec_array[1] = vec_array[1] * 2.0;
            vec_array[3] = vec_array[3] * 2.0;

            SetBORXSecPnts( bor_id, vec_array );
        }
        \endcode
        \param [in] bor_id Geom ID of BOR
        \param [in] pnt_arr Array of XSec coordinate points
    */

    void SetBORXSecPnts(const string &bor_id, vector<vec3d> &pnt_vec)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORXSecPnts::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "SetBORXSecPnts::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORXSecPnts::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() != XS_FILE_FUSE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetBORXSecPnts::Wrong XSec Type");
            return;
        }

        FileXSec *file_xs = dynamic_cast<FileXSec *>(xsc);
        assert(file_xs);
        file_xs->SetPnts(pnt_vec);
        ErrorMgr.NoError();
    }

    /*!
        Compute 3D coordinate for a point on a BOR XSecCurve given the parameter value (U) along the curve
        \code{.cpp}
        //==== Add Geom ====//
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        double u_fract = 0.25;

        vec3d pnt = ComputeBORXSecPnt( bor_id, u_fract );
        \endcode
        \param [in] bor_id Geom ID of BOR
        \param [in] fract Curve parameter value (range: 0 - 1)
        \return 3D coordinate point
    */

    vec3d ComputeBORXSecPnt(const string &bor_id, double fract)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ComputeBORXSecPnt::Can't Find Geom " + bor_id);
            return vec3d();
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "ComputeBORXSecPnt::Geom " + bor_id + " is not a body of revolution");
            return vec3d();
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ComputeBORXSecPnt::Can't Get XSecCurve");
            return vec3d();
        }

        vec3d pnt = xsc->GetCurve().CompPnt01(fract);
        ErrorMgr.NoError();

        return pnt;
    }

    /*!
        Compute the tangent vector of a point on a BOR XSecCurve given the parameter value (U) along the curve
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        double u_fract = 0.25;

        vec3d tan = ComputeBORXSecTan( bor_id, u_fract );
        \endcode
        \param [in] bor_id Geom ID of BOR
        \param [in] fract Curve parameter value (range: 0 - 1)
        \return Tangent vector
    */

    vec3d ComputeBORXSecTan(const string &bor_id, double fract)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ComputeBORXSecTan::Can't Find Geom " + bor_id);
            return vec3d();
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "ComputeBORXSecTan::Geom " + bor_id + " is not a body of revolution");
            return vec3d();
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ComputeBORXSecTan::Can't Get XSecCurve");
            return vec3d();
        }

        vec3d pnt = xsc->GetCurve().CompTan01(fract);
        ErrorMgr.NoError();

        return pnt;
    }

    /*!
        Read in shape from airfoil file and set to the specified BOR XSecCurve. The XSecCurve must be of type XS_FILE_AIRFOIL. Airfoil files may be in Lednicer or Selig format with *.af or *.dat extensions.
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        ChangeBORXSecShape( bor_id, XS_FILE_AIRFOIL );

        ReadBORFileAirfoil( bor_id, "airfoil/N0012_VSP.af" );
        \endcode
        \param [in] bor_id Geom ID of BOR
        \param [in] file_name Airfoil XSec file name
    */

    void ReadBORFileAirfoil(const string &bor_id, const string &file_name)
    {
        vector<vec3d> pnt_vec;
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ReadBORFileAirfoil::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "ReadBORFileAirfoil::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ReadBORFileAirfoil::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() == XS_FILE_AIRFOIL)
        {
            FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xsc);
            assert(file_xs);
            if (file_xs->ReadFile(file_name))
            {
                ErrorMgr.NoError();
                return;
            }
            else
            {
                ErrorMgr.AddError(VSP_FILE_DOES_NOT_EXIST, "ReadBORFileAirfoil::Error reading airfoil file " + file_name);
                return;
            }
        }

        ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "ReadBORFileAirfoil::XSec Not XS_FILE_AIRFOIL Type " + bor_id);
        return;
    }

    /*!
        Set the upper points for an airfoil on a BOR. The BOR XSecCurve must be of type XS_FILE_AIRFOIL.
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        ChangeBORXSecShape( bor_id, XS_FILE_AIRFOIL );

        ReadBORFileAirfoil( bor_id, "airfoil/N0012_VSP.af" );

        array< vec3d > @up_array = GetBORAirfoilUpperPnts( bor_id );

        for ( int i = 0 ; i < int( up_array.size() ) ; i++ )
        {
            up_array[i].scale_y( 2.0 );
        }

        SetBORAirfoilUpperPnts( bor_id, up_array );
        \endcode
        \param [in] bor_id Geom ID of BOR
        \param [in] up_pnt_vec Array of points defining the upper surface of the airfoil
    */

    void SetBORAirfoilUpperPnts(const string &bor_id, const std::vector<vec3d> &up_pnt_vec)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORAirfoilUpperPnts::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "SetBORAirfoilUpperPnts::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORAirfoilUpperPnts::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() != XS_FILE_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetBORAirfoilUpperPnts::XSec Not XS_FILE_AIRFOIL Type");
            return;
        }

        FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xsc);
        assert(file_xs);
        file_xs->SetAirfoilUpperPnts(up_pnt_vec);
        ErrorMgr.NoError();
    }

    /*!
        Set the lower points for an airfoil on a BOR. The BOR XSecCurve must be of type XS_FILE_AIRFOIL.
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        ChangeBORXSecShape( bor_id, XS_FILE_AIRFOIL );

        ReadBORFileAirfoil( bor_id, "airfoil/N0012_VSP.af" );

        array< vec3d > @low_array = GetBORAirfoilLowerPnts( bor_id );

        for ( int i = 0 ; i < int( low_array.size() ) ; i++ )
        {
            low_array[i].scale_y( 0.5 );
        }

        SetBORAirfoilLowerPnts( bor_id, low_array );
        \endcode
        \param [in] bor_id Geom ID of BOR
        \param [in] low_pnt_vec Array of points defining the lower surface of the airfoil
    */

    void SetBORAirfoilLowerPnts(const string &bor_id, const std::vector<vec3d> &low_pnt_vec)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORAirfoilLowerPnts::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "SetBORAirfoilLowerPnts::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORAirfoilLowerPnts::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() != XS_FILE_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetBORAirfoilLowerPnts::XSec Not XS_FILE_AIRFOIL Type");
            return;
        }

        FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xsc);
        assert(file_xs);
        file_xs->SetAirfoilLowerPnts(low_pnt_vec);
        ErrorMgr.NoError();
    }

    /*!
        Set the upper and lower points for an airfoil on a BOR. The BOR XSecCurve must be of type XS_FILE_AIRFOIL.
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        ChangeBORXSecShape( bor_id, XS_FILE_AIRFOIL );

        ReadBORFileAirfoil( bor_id, "airfoil/N0012_VSP.af" );

        array< vec3d > @up_array = GetBORAirfoilUpperPnts( bor_id );

        array< vec3d > @low_array = GetBORAirfoilLowerPnts( bor_id );

        for ( int i = 0 ; i < int( up_array.size() ) ; i++ )
        {
            up_array[i].scale_y( 2.0 );

            low_array[i].scale_y( 0.5 );
        }

        SetBORAirfoilPnts( bor_id, up_array, low_array );
        \endcode
        \param [in] bor_id Geom ID of BOR
        \param [in] up_pnt_vec Array of points defining the upper surface of the airfoil
        \param [in] low_pnt_vec Array of points defining the lower surface of the airfoil
    */

    void SetBORAirfoilPnts(const string &bor_id, const std::vector<vec3d> &up_pnt_vec, const std::vector<vec3d> &low_pnt_vec)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORAirfoilPnts::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "SetBORAirfoilPnts::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORAirfoilPnts::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() != XS_FILE_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetBORAirfoilPnts::XSec Not XS_FILE_AIRFOIL Type");
            return;
        }

        FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xsc);
        assert(file_xs);
        file_xs->SetAirfoilPnts(up_pnt_vec, low_pnt_vec);
        ErrorMgr.NoError();
    }

    /*!
        Get the coordinate points for the upper surface of an airfoil on a BOR. The BOR XSecCurve must be of type XS_FILE_AIRFOIL
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        ChangeBORXSecShape( bor_id, XS_FILE_AIRFOIL );

        ReadBORFileAirfoil( bor_id, "airfoil/N0012_VSP.af" );

        array< vec3d > @up_array = GetBORAirfoilUpperPnts( bor_id );
        \endcode
        \sa SetAirfoilPnts
        \param [in] bor_id Geom ID of BOR
        \return Array of coordinate points for the upper airfoil surface
    */

    std::vector<vec3d> GetBORAirfoilUpperPnts(const string &bor_id)
    {
        vector<vec3d> pnt_vec;
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORAirfoilUpperPnts::Can't Find Geom " + bor_id);
            return pnt_vec;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "GetBORAirfoilUpperPnts::Geom " + bor_id + " is not a body of revolution");
            return pnt_vec;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORAirfoilUpperPnts::Can't Get XSecCurve");
            return pnt_vec;
        }

        if (xsc->GetType() != XS_FILE_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetBORAirfoilUpperPnts::XSec Not XS_FILE_AIRFOIL Type");
            return pnt_vec;
        }

        FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xsc);
        assert(file_xs);
        pnt_vec = file_xs->GetUpperPnts();
        ErrorMgr.NoError();
        return pnt_vec;
    }

    /*!
        Get the coordinate points for the lower surface of an airfoil of a BOR. The XSecCurve must be of type XS_FILE_AIRFOIL
        \code{.cpp}
        // Add Body of Recolution
        string bor_id = AddGeom( "BODYOFREVOLUTION", "" );

        ChangeBORXSecShape( bor_id, XS_FILE_AIRFOIL );

        ReadBORFileAirfoil( bor_id, "airfoil/N0012_VSP.af" );

        array< vec3d > @low_array = GetBORAirfoilLowerPnts( bor_id );
        \endcode
        \sa SetAirfoilPnts
        \param [in] bor_id Geom ID of BOR
        \return Array of coordinate points for the lower airfoil surface
    */

    std::vector<vec3d> GetBORAirfoilLowerPnts(const string &bor_id)
    {
        vector<vec3d> pnt_vec;

        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORAirfoilLowerPnts::Can't Find Geom " + bor_id);
            return pnt_vec;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "GetBORAirfoilLowerPnts::Geom " + bor_id + " is not a body of revolution");
            return pnt_vec;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORAirfoilLowerPnts::Can't Get XSecCurve");
            return pnt_vec;
        }

        if (xsc->GetType() != XS_FILE_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetBORAirfoilLowerPnts::XSec Not XS_FILE_AIRFOIL Type");
            return pnt_vec;
        }

        FileAirfoil *file_xs = dynamic_cast<FileAirfoil *>(xsc);
        assert(file_xs);
        pnt_vec = file_xs->GetLowerPnts();
        ErrorMgr.NoError();
        return pnt_vec;
    }

    /*!
        Get the CST coefficients for the upper surface of an airfoil of a BOR. The XSecCurve must be of type XS_CST_AIRFOIL
        \sa SetUpperCST
        \param [in] bor_id Geom ID of BOR
        \return Array of CST coefficients for the upper airfoil surface
    */

    std::vector<double> GetBORUpperCSTCoefs(const string &bor_id)
    {
        vector<double> ret_vec;

        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORUpperCSTCoefs::Can't Find Geom " + bor_id);
            return ret_vec;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "GetBORUpperCSTCoefs::Geom " + bor_id + " is not a body of revolution");
            return ret_vec;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORUpperCSTCoefs::Can't Get XSecCurve");
            return ret_vec;
        }

        if (xsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetBORUpperCSTCoefs::XSec Not XS_CST_AIRFOIL Type");
            return ret_vec;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xsc);
        assert(cst_xs);

        ret_vec = cst_xs->GetUpperCST();
        ErrorMgr.NoError();
        return ret_vec;
    }

    /*!
        Get the CST coefficients for the lower surface of an airfoil of a BOR. The XSecCurve must be of type XS_CST_AIRFOIL
        \sa SetLowerCST
        \param [in] bor_id Geom ID of BOR
        \return Array of CST coefficients for the lower airfoil surface
    */

    std::vector<double> GetBORLowerCSTCoefs(const string &bor_id)
    {
        vector<double> ret_vec;

        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORLowerCSTCoefs::Can't Find Geom " + bor_id);
            return ret_vec;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "GetBORLowerCSTCoefs::Geom " + bor_id + " is not a body of revolution");
            return ret_vec;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORLowerCSTCoefs::Can't Get XSecCurve");
            return ret_vec;
        }

        if (xsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetBORLowerCSTCoefs::XSec Not XS_CST_AIRFOIL Type");
            return ret_vec;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xsc);
        assert(cst_xs);

        ret_vec = cst_xs->GetLowerCST();
        ErrorMgr.NoError();
        return ret_vec;
    }

    /*!
        Get the CST degree for the upper surface of an airfoil of a BOR. The XSecCurve must be of type XS_CST_AIRFOIL
        \sa SetUpperCST
        \param [in] bor_id Geom ID of BOR
        \return CST Degree for upper airfoil surface
    */

    int GetBORUpperCSTDegree(const string &bor_id)
    {
        int deg = -1;

        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORUpperCSTDegree::Can't Find Geom " + bor_id);
            return deg;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "GetBORUpperCSTDegree::Geom " + bor_id + " is not a body of revolution");
            return deg;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORUpperCSTDegree::Can't Get XSecCurve");
            return deg;
        }

        if (xsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetBORUpperCSTDegree::XSec Not XS_CST_AIRFOIL Type");
            return deg;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xsc);
        assert(cst_xs);

        deg = cst_xs->GetUpperDegree();
        ErrorMgr.NoError();
        return deg;
    }

    /*!
        Get the CST degree for the lower surface of an airfoil of a BOR. The XSecCurve must be of type XS_CST_AIRFOIL
        \sa SetLowerCST
        \param [in] bor_id Geom ID of BOR
        \return CST Degree for lower airfoil surface
    */

    int GetBORLowerCSTDegree(const string &bor_id)
    {
        int deg = -1;

        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORLowerCSTDegree::Can't Find Geom " + bor_id);
            return deg;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "GetBORLowerCSTDegree::Geom " + bor_id + " is not a body of revolution");
            return deg;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetBORLowerCSTDegree::Can't Get XSecCurve");
            return deg;
        }

        if (xsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetBORLowerCSTDegree::XSec Not XS_CST_AIRFOIL Type");
            return deg;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xsc);
        assert(cst_xs);

        deg = cst_xs->GetLowerDegree();
        ErrorMgr.NoError();
        return deg;
    }

    /*!
        Set the CST degree and coefficients for the upper surface of an airfoil of a BOR. The number of coefficients should be one more than the CST degree. The XSecCurve must be of type XS_CST_AIRFOIL
        \sa GetUpperCSTDegree, GetUpperCSTCoefs
        \param [in] bor_id Geom ID of BOR
        \param [in] deg CST degree of upper airfoil surface
        \param [in] coeff_arr Array of CST coefficients for the upper airfoil surface
    */

    void SetBORUpperCST(const string &bor_id, int deg, const std::vector<double> &coefs)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORUpperCST::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "SetBORUpperCST::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORUpperCST::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetBORUpperCST::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xsc);
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->SetUpperCST(deg, coefs);
    }

    /*!
        Set the CST degree and coefficients for the lower surface of an airfoil of a BOR. The number of coefficients should be one more than the CST degree. The XSecCurve must be of type XS_CST_AIRFOIL
        \sa GetLowerCSTDegree, GetLowerCSTCoefs
        \param [in] bor_id Geom ID of BOR
        \param [in] deg CST degree of lower airfoil surface
        \param [in] coeff_arr Array of CST coefficients for the lower airfoil surface
    */

    void SetBORLowerCST(const string &bor_id, int deg, const std::vector<double> &coefs)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORLowerCST::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "SetBORLowerCST::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetBORLowerCST::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetBORLowerCST::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xsc);
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->SetLowerCST(deg, coefs);
    }

    /*!
        Promote the CST for the upper airfoil surface of a BOR. The XSecCurve must be of type XS_CST_AIRFOIL
        \sa GetUpperCSTDegree
        \param [in] bor_id Geom ID of BOR
    */

    void PromoteBORCSTUpper(const string &bor_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PromoteBORCSTUpper::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "PromoteBORCSTUpper::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PromoteBORCSTUpper::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "PromoteBORCSTUpper::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xsc);
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->PromoteUpper();
    }

    /*!
        Promote the CST for the lower airfoil surface of a BOR. The XSecCurve must be of type XS_CST_AIRFOIL
        \sa GetLowerCSTDegree
        \param [in] bor_id Geom ID of BOR
    */

    void PromoteBORCSTLower(const string &bor_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PromoteBORCSTLower::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "PromoteBORCSTLower::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PromoteBORCSTLower::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "PromoteBORCSTLower::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xsc);
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->PromoteLower();
    }

    /*!
        Demote the CST for the upper airfoil surface of a BOR. The XSecCurve must be of type XS_CST_AIRFOIL
        \sa GetUpperCSTDegree
        \param [in] bor_id Geom ID of BOR
    */

    void DemoteBORCSTUpper(const string &bor_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DemoteBORCSTUpper::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "DemoteBORCSTUpper::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DemoteBORCSTUpper::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "DemoteBORCSTUpper::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xsc);
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->DemoteUpper();
    }

    /*!
        Demote the CST for the lower airfoil surface of a BOR. The XSecCurve must be of type XS_CST_AIRFOIL
        \sa GetLowerCSTDegree
        \param [in] bor_id Geom ID of BOR
    */

    void DemoteBORCSTLower(const string &bor_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DemoteBORCSTLower::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "DemoteBORCSTLower::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "DemoteBORCSTLower::Can't Get XSecCurve");
            return;
        }

        if (xsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "DemoteBORCSTLower::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(xsc);
        assert(cst_xs);

        ErrorMgr.NoError();
        cst_xs->DemoteLower();
    }

    /*!
        Fit a CST airfoil for an existing airfoil of a BOR of type XS_FOUR_SERIES, XS_SIX_SERIES, XS_FOUR_DIGIT_MOD, XS_FIVE_DIGIT, XS_FIVE_DIGIT_MOD, XS_ONE_SIX_SERIES, or XS_FILE_AIRFOIL.
        \param [in] bor_id Geom ID of BOR
        \param [in] deg CST degree
    */

    void FitBORAfCST(const string &bor_id, int deg)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(bor_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "FitBORAfCST::Can't Find Geom " + bor_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != BOR_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "FitBORAfCST::Geom " + bor_id + " is not a body of revolution");
            return;
        }

        BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);

        XSecCurve *xsc = bor_ptr->GetXSecCurve();

        if (!xsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "FitBORAfCST::Can't Get XSecCurve");
            return;
        }

        if ((xsc->GetType() != XS_FOUR_SERIES) ||
            (xsc->GetType() != XS_SIX_SERIES) ||
            (xsc->GetType() != XS_FOUR_DIGIT_MOD) ||
            (xsc->GetType() != XS_FIVE_DIGIT) ||
            (xsc->GetType() != XS_FIVE_DIGIT_MOD) ||
            (xsc->GetType() != XS_ONE_SIX_SERIES) ||
            (xsc->GetType() != XS_FILE_AIRFOIL))
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "FitBORAfCST::XSec Not Fittable Airfoil Type");
            return;
        }

        Airfoil *af_xs = dynamic_cast<Airfoil *>(xsc);
        if (!af_xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "FitBORAfCST::Can't Get Airfoil");
            return;
        }

        VspCurve c = af_xs->GetOrigCurve();

        bor_ptr->SetXSecCurveType(XS_CST_AIRFOIL);

        XSecCurve *newxsc = bor_ptr->GetXSecCurve();
        ;
        if (!newxsc)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "FitBORAfCST::Can't Get New XSecCurve");
            return;
        }

        if (newxsc->GetType() != XS_CST_AIRFOIL)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "FitBORAfCST::XSec Not XS_CST_AIRFOIL Type");
            return;
        }

        CSTAirfoil *cst_xs = dynamic_cast<CSTAirfoil *>(newxsc);

        assert(cst_xs);
        cst_xs->FitCurve(c, deg);

        ErrorMgr.NoError();
    }

    //===================================================================//
    //===============      Edit XSec Functions        ===================//
    //===================================================================//

    /*!
        Initialize the EditCurveXSec to the current value of m_ShapeType (i.e. EDIT_XSEC_ELLIPSE)
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        ChangeXSecShape( xsec_surf, 2, XS_EDIT_CURVE );

        // Identify XSec 2
        string xsec_2 = GetXSec( xsec_surf, 2 );

        // Set XSec 2 to linear
        EditXSecConvertTo( xsec_2, LINEAR );

        EditXSecInitShape( xsec_2 ); // Change back to default ellipse
        \endcode
        \sa INIT_EDIT_XSEC_TYPE
        \param [in] xsec_id XSec ID
    */

    void EditXSecInitShape(const std::string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "EditXSecInitShape::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "EditXSecInitShape::XSec Not XS_EDIT_CURVE Type");
            return;
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        ErrorMgr.NoError();

        edit_xs->InitShape();
    }

    /*!
        Convert the EditCurveXSec curve type to the specified new type. Note, EditCurveXSec uses the same enumerations for PCurve to identify curve type,
        but APPROX_CEDIT is not supported at this time.
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_EDIT_CURVE );

        // Identify XSec 1
        string xsec_1 = GetXSec( xsec_surf, 1 );

        // Set XSec 1 to Linear
        EditXSecConvertTo( xsec_1, LINEAR );
        \endcode
        \sa PCURV_TYPE
        \param [in] xsec_id XSec ID
        \param [in] newtype New curve type enum (i.e. CEDIT)
    */

    void EditXSecConvertTo(const std::string &xsec_id, const int &newtype)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "EditXSecConvertTo::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "EditXSecConvertTo::XSec Not XS_EDIT_CURVE Type");
            return;
        }

        if (newtype < 0 || newtype > vsp::CEDIT)
        {
            ErrorMgr.AddError(VSP_INVALID_TYPE, "EditXSecConvertTo::Invalid PCURV_TYPE");
            return;
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        ErrorMgr.NoError();

        edit_xs->ConvertTo(newtype);
    }

    /*!
        Get the U parameter vector for an EditCurveXSec. The vector will be in increasing order with a range of 0 - 1.
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        ChangeXSecShape( xsec_surf, 2, XS_EDIT_CURVE );

        // Identify XSec 2
        string xsec_2 = GetXSec( xsec_surf, 2 );

        // Set XSec 2 to linear
        EditXSecConvertTo( xsec_2, LINEAR );

        array < double > u_vec = GetEditXSecUVec( xsec_2 );

        if ( u_vec[1] - 0.25 > 1e-6 )
        {
            Print( "Error: GetEditXSecUVec" );
        }
        \endcode
        \param [in] xsec_id XSec ID
        \return Array of U parameter values
    */

    vector<double> GetEditXSecUVec(const std::string &xsec_id)
    {
        vector<double> ret_vec;

        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetEditXSecUVec::Can't Find XSec " + xsec_id);
            return ret_vec;
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetEditXSecUVec::XSec Not XS_EDIT_CURVE Type");
            return ret_vec;
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        ErrorMgr.NoError();

        return edit_xs->GetUVec();
    }

    /*!
        Get the control point vector for an EditCurveXSec. Note, the returned array of vec3d values will be represented in 2D with Z set to 0.
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_EDIT_CURVE );

        // Identify XSec 1
        string xsec_1 = GetXSec( xsec_surf, 1 );

        // Get the control points for the default shape
        array < vec3d > xsec1_pts = GetEditXSecCtrlVec( xsec_1, true ); // The returned control points will not be scaled by width and height

        Print( "Normalized Bottom Point of XSecCurve: " + xsec1_pts[3].x() + ", " + xsec1_pts[3].y() + ", " + xsec1_pts[3].z() );
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] non_dimensional True to get the points non-dimensionalized, False to get them scaled by m_Width and m_Height
        \return Array of control points
    */

    vector<vec3d> GetEditXSecCtrlVec(const std::string &xsec_id, const bool non_dimensional)
    {
        vector<vec3d> ret_vec;

        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetEditXSecCtrlVec::Can't Find XSec " + xsec_id);
            return ret_vec;
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetEditXSecCtrlVec::XSec Not XS_EDIT_CURVE Type");
            return ret_vec;
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        ErrorMgr.NoError();

        return edit_xs->GetCtrlPntVec(non_dimensional);
    }

    /*!
        Set the U parameter vector and the control point vector for an EditCurveXSec. The arrays must be of equal length, with the values for U defined in
        increasing order and range 0 - 1. The input control points to SetEditXSecPnts must be nondimensionalized in the approximate range of [-0.5, 0.5].
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        ChangeXSecShape( xsec_surf, 2, XS_EDIT_CURVE );

        // Identify XSec 2
        string xsec_2 = GetXSec( xsec_surf, 2 );

        // Set XSec 2 to linear
        EditXSecConvertTo( xsec_2, LINEAR );

        // Turn off R/L symmetry
        SetParmVal( GetXSecParm( xsec_2, "SymType"), SYM_NONE );

        // Define a square
        array < vec3d > xsec2_pts(5);

        xsec2_pts[0] = vec3d( 0.5, 0.5, 0.0 );
        xsec2_pts[1] = vec3d( 0.5, -0.5, 0.0 );
        xsec2_pts[2] = vec3d( -0.5, -0.5, 0.0 );
        xsec2_pts[3] = vec3d( -0.5, 0.5, 0.0 );
        xsec2_pts[4] = vec3d( 0.5, 0.5, 0.0 );

        // u vec must start at 0.0 and end at 1.0
        array < double > u_vec(5);

        u_vec[0] = 0.0;
        u_vec[1] = 0.25;
        u_vec[2] = 0.5;
        u_vec[3] = 0.75;
        u_vec[4] = 1.0;

        array < double > r_vec(5);

        r_vec[0] = 0.0;
        r_vec[1] = 0.0;
        r_vec[2] = 0.0;
        r_vec[3] = 0.0;
        r_vec[4] = 0.0;

        SetEditXSecPnts( xsec_2, u_vec, xsec2_pts, r_vec ); // Note: points are unscaled by the width and height parms

        array < vec3d > new_pnts = GetEditXSecCtrlVec( xsec_2, true ); // The returned control points will not be scaled by width and height

        if ( dist( new_pnts[3], xsec2_pts[3] ) > 1e-6 )
        {
            Print( "Error: SetEditXSecPnts");
        }
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] u_vec Array of U parameter values
        \param [in] r_vec Array of R parameter values
        \param [in] control_pts Nondimensionalized array of control points
    */

    void SetEditXSecPnts(const std::string &xsec_id, vector<double> u_vec, vector<vec3d> control_pts, vector<double> r_vec)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetEditXSecPnts::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetEditXSecPnts::XSec Not XS_EDIT_CURVE Type");
            return;
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        ErrorMgr.NoError();

        edit_xs->SetPntVecs(u_vec, control_pts, r_vec);
    }

    /*!
        Delete an EditCurveXSec control point. Note, cubic Bezier intermediate control points (those not on the curve) cannot be deleted.
        The previous and next Bezier control point will be deleted along with the point on the curve. Regardless of curve type, the first
        and last points may not be deleted.
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        ChangeXSecShape( xsec_surf, 2, XS_EDIT_CURVE );

        // Identify XSec 2
        string xsec_2 = GetXSec( xsec_surf, 2 );

        // Turn off R/L symmetry
        SetParmVal( GetXSecParm( xsec_2, "SymType"), SYM_NONE );

        array < vec3d > old_pnts = GetEditXSecCtrlVec( xsec_2, true ); // The returned control points will not be scaled by width and height

        EditXSecDelPnt( xsec_2, 3 ); // Remove control point at bottom of circle

        array < vec3d > new_pnts = GetEditXSecCtrlVec( xsec_2, true ); // The returned control points will not be scaled by width and height

        if ( old_pnts.size() - new_pnts.size() != 3  )
        {
            Print( "Error: EditXSecDelPnt");
        }
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] indx Control point index
    */

    void EditXSecDelPnt(const std::string &xsec_id, const int &indx)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "EditXSecDelPnt::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "EditXSecDelPnt::XSec Not XS_EDIT_CURVE Type");
            return;
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        ErrorMgr.NoError();

        return edit_xs->DeletePt(indx);
    }

    /*!
        Split the EditCurveXSec at the specified U value
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        ChangeXSecShape( xsec_surf, 2, XS_EDIT_CURVE );

        // Identify XSec 2
        string xsec_2 = GetXSec( xsec_surf, 2 );

        // Turn off R/L symmetry
        SetParmVal( GetXSecParm( xsec_2, "SymType"), SYM_NONE );

        array < vec3d > old_pnts = GetEditXSecCtrlVec( xsec_2, true ); // The returned control points will not be scaled by width and height

        int new_pnt_ind = EditXSecSplit01( xsec_2, 0.375 );

        array < vec3d > new_pnts = GetEditXSecCtrlVec( xsec_2, true ); // The returned control points will not be scaled by width and height

        if ( new_pnts.size() - old_pnts.size() != 3  )
        {
            Print( "Error: EditXSecSplit01");
        }
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] u U value to split the curve at (0 - 1)
        \return Index of the point added from the split
    */

    int EditXSecSplit01(const std::string &xsec_id, const double &u)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "EditXSecSplit01::Can't Find XSec " + xsec_id);
            return -1;
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "EditXSecSplit01::XSec Not XS_EDIT_CURVE Type");
            return -1;
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        ErrorMgr.NoError();

        return edit_xs->Split01(u);
    }

    /*!
        Move an EditCurveXSec control point. The XSec points are nondimensionalized by m_Width and m_Height and
        defined in 2D, so the Z value of the new coordinate point will be ignored.
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_EDIT_CURVE );

        // Identify XSec 1
        string xsec_1 = GetXSec( xsec_surf, 1 );

        // Turn off R/L symmetry
        SetParmVal( GetXSecParm( xsec_1, "SymType"), SYM_NONE );

        // Get the control points for the default shape
        array < vec3d > xsec1_pts = GetEditXSecCtrlVec( xsec_1, true ); // The returned control points will not be scaled by width and height

        // Identify a control point that lies on the curve and shift it in Y
        int move_pnt_ind = 3;

        vec3d new_pnt = vec3d( xsec1_pts[move_pnt_ind].x(), 2 * xsec1_pts[move_pnt_ind].y(), 0.0 );

        // Move the control point
        MoveEditXSecPnt( xsec_1, move_pnt_ind, new_pnt );

        array < vec3d > new_pnts = GetEditXSecCtrlVec( xsec_1, true ); // The returned control points will not be scaled by width and height

        if ( dist( new_pnt, new_pnts[move_pnt_ind] ) > 1e-6 )
        {
            Print( "Error: MoveEditXSecPnt" );
        }
        \endcode
        \param [in] xsec_id XSec ID
        \param [in] indx Control point index
        \param [in] new_pnt Coordinate of the new point
    */

    void MoveEditXSecPnt(const std::string &xsec_id, const int &indx, const vec3d &new_pnt)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "MoveEditXSecPnt::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "MoveEditXSecPnt::XSec Not XS_EDIT_CURVE Type");
            return;
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        if (indx < 0 || indx >= edit_xs->GetNumPts())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "MoveEditXSecPnt::Invalid point index " + std::to_string(indx));
            return;
        }

        // edit_xs->MovePnt also moves adjacent CEDIT points, so just set parm values directly
        edit_xs->m_XParmVec[indx]->Set(new_pnt.x());
        edit_xs->m_YParmVec[indx]->Set(new_pnt.y());
        edit_xs->m_ZParmVec[indx]->Set(new_pnt.z());

        edit_xs->ParmChanged(NULL, Parm::SET_FROM_DEVICE); // Force update

        ErrorMgr.NoError();
    }

    /*!
        Convert any XSec type into an EditCurveXSec. This function will work for BOR Geoms, in which case the input XSec index is ignored.
        \code{.cpp}
        // Add Stack
        string sid = AddGeom( "STACK", "" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( sid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_ROUNDED_RECTANGLE );

        // Convert Rounded Rectangle to Edit Curve type XSec
        ConvertXSecToEdit( sid, 1 );

        // Identify XSec 1
        string xsec_1 = GetXSec( xsec_surf, 1 );

        // Get the control points for the default shape
        array < vec3d > xsec1_pts = GetEditXSecCtrlVec( xsec_1, true ); // The returned control points will not be scaled by width and height
        \endcode
        \param [in] geom_id Geom ID
        \param [in] indx XSec index
    */

    void ConvertXSecToEdit(const std::string &geom_id, const int &indx)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ConvertXSecToEdit::Can't Find Geom " + geom_id);
            return;
        }

        if (geom_ptr->GetType().m_Type == BOR_GEOM_TYPE)
        {
            BORGeom *bor_ptr = dynamic_cast<BORGeom *>(geom_ptr);
            if (!bor_ptr)
            {
                ErrorMgr.AddError(VSP_INVALID_TYPE, "ConvertXSecToEdit::Geom " + geom_id + " is not a body of revolution");
                return;
            }

            bor_ptr->ConvertToEdit();
            ErrorMgr.NoError();
            return;
        }
        else
        {
            GeomXSec *geom_xsec = dynamic_cast<GeomXSec *>(geom_ptr);
            if (!geom_xsec)
            {
                ErrorMgr.AddError(VSP_INVALID_TYPE, "ConvertXSecToEdit::Geom " + geom_id + " is not a GeomXSec");
                return;
            }

            XSec *xs = geom_xsec->GetXSec(indx);

            if (!xs)
            {
                ErrorMgr.AddError(VSP_INVALID_PTR, "ConvertXSecToEdit::Can't Find XSec " + to_string((long long)indx));
                return;
            }

            xs->ConvertToEdit();
            ErrorMgr.NoError();
            return;
        }
    }

    /*!
        Get the vector of fixed U flags for each control point in an EditCurveXSec. The fixed U flag is used to hold the
        U parameter of the control point constant when performing an equal arc length reparameterization of the curve.
        \code{.cpp}
        // Add Wing
        string wid = AddGeom( "WING" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( wid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_EDIT_CURVE );

        // Identify XSec 1
        string xsec_1 = GetXSec( xsec_surf, 1 );

        array < bool > @ fixed_u_vec = GetEditXSecFixedUVec( xsec_1 );

        fixed_u_vec[3] = true; // change a flag

        SetEditXSecFixedUVec( xsec_1, fixed_u_vec );

        ReparameterizeEditXSec( xsec_1 );
        \endcode
        \sa SetEditXSecFixedUVec, ReparameterizeEditXSec
        \param [in] xsec_id XSec ID
        \return Array of bool values for each control point
    */

    vector<bool> GetEditXSecFixedUVec(const std::string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetEditXSecFixedUVec::Can't Find XSec " + xsec_id);
            return vector<bool>{};
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "GetEditXSecFixedUVec::XSec Not XS_EDIT_CURVE Type");
            return vector<bool>{};
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        ErrorMgr.NoError();
        return edit_xs->GetFixedUVec();
    }

    /*!
        Set the vector of fixed U flags for each control point in an EditCurveXSec. The fixed U flag is used to hold the
        U parameter of the control point constant when performing an equal arc length reparameterization of the curve.
        \code{.cpp}
        // Add Wing
        string wid = AddGeom( "WING" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( wid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_EDIT_CURVE );

        // Identify XSec 1
        string xsec_1 = GetXSec( xsec_surf, 1 );

        array < bool > @ fixed_u_vec = GetEditXSecFixedUVec( xsec_1 );

        fixed_u_vec[3] = true; // change a flag

        SetEditXSecFixedUVec( xsec_1, fixed_u_vec );

        ReparameterizeEditXSec( xsec_1 );
        \endcode
        \sa GetEditXSecFixedUVec, ReparameterizeEditXSec
        \param [in] xsec_id XSec ID
        \param [in] fixed_u_vec Array of fixed U flags
    */

    void SetEditXSecFixedUVec(const std::string &xsec_id, vector<bool> fixed_u_vec)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetEditXSecFixedUVec::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "SetEditXSecFixedUVec::XSec Not XS_EDIT_CURVE Type");
            return;
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        if (edit_xs->GetNumPts() != fixed_u_vec.size())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "SetEditXSecFixedUVec:Size of fixed_u_vec Not Equal to Number of Control Points");
            return;
        }

        for (size_t i = 0; i < fixed_u_vec.size(); i++)
        {
            edit_xs->m_FixedUVec[i]->Set(fixed_u_vec[i]);
        }
        ErrorMgr.NoError();
    }

    /*!
        Perform an equal arc length repareterization on an EditCurveXSec. The reparameterization is performed between
        specific U values if the Fixed U flag is true. This allows corners, such as at 0.25, 0.5, and 0.75 U, to be held
        constant while everything between them is reparameterized.
        \code{.cpp}
        // Add Wing
        string wid = AddGeom( "WING" );

        // Get First (and Only) XSec Surf
        string xsec_surf = GetXSecSurf( wid, 0 );

        ChangeXSecShape( xsec_surf, 1, XS_EDIT_CURVE );

        // Identify XSec 1
        string xsec_1 = GetXSec( xsec_surf, 1 );

        array < bool > @ fixed_u_vec = GetEditXSecFixedUVec( xsec_1 );

        fixed_u_vec[3] = true; // change a flag

        SetEditXSecFixedUVec( xsec_1, fixed_u_vec );

        ReparameterizeEditXSec( xsec_1 );
        \endcode
        \sa SetEditXSecFixedUVec, GetEditXSecFixedUVec
        \param [in] xsec_id XSec ID
    */

    void ReparameterizeEditXSec(const std::string &xsec_id)
    {
        XSec *xs = FindXSec(xsec_id);
        if (!xs)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ReparameterizeEditXSec::Can't Find XSec " + xsec_id);
            return;
        }

        if (xs->GetXSecCurve()->GetType() != XS_EDIT_CURVE)
        {
            ErrorMgr.AddError(VSP_WRONG_XSEC_TYPE, "ReparameterizeEditXSec::XSec Not XS_EDIT_CURVE Type");
            return;
        }

        EditCurveXSec *edit_xs = dynamic_cast<EditCurveXSec *>(xs->GetXSecCurve());
        assert(edit_xs);

        edit_xs->ReparameterizeEqualArcLength();
        ErrorMgr.NoError();
    }

    //===================================================================//
    //===============       Set Functions            ===================//
    //===================================================================//

    /// Get the total number of defined sets.  Named sets are used to group components
    /// and perform read/write or operations on them

    /*!
        Get the total number of defined sets. Named sets are used to group components and read/write on them. The number of named
        sets will be 10 for OpenVSP versions up to 3.17.1 and 20 for later versions.
        \code{.cpp}
        if ( GetNumSets() <= 0 )                            { Print( "---> Error: API GetNumSets " ); }
        \endcode
        \return Number of sets
    */

    int GetNumSets()
    {
        Vehicle *veh = GetVehicle();
        return veh->GetSetNameVec().size();
    }

    /*!
        Set the name of a set at specified index
        \code{.cpp}
        SetSetName( 3, "SetFromScript" );

        if ( GetSetName( 3 ) != "SetFromScript" )            { Print( "---> Error: API Get/Set Set Name " ); }
        \endcode
        \sa SET_TYPE
        \param [in] index Set index
        \param [in] name Set name
    */

    /// Set the set name at the provided index. Index between 0 and NumSets.
    void SetSetName(int index, const string &name)
    {
        Vehicle *veh = GetVehicle();
        veh->SetSetName(index, name);
    }

    /*!
        Get the name of a set at specified index
        \code{.cpp}
        SetSetName( 3, "SetFromScript" );

        if ( GetSetName( 3 ) != "SetFromScript" )            { Print( "---> Error: API Get/Set Set Name " ); }
        \endcode
        \sa SET_TYPE
        \param [in] index Set index
        \return Set name
    */

    string GetSetName(int index)
    {
        Vehicle *veh = GetVehicle();
        vector<string> name_vec = veh->GetSetNameVec();
        if (index < 0 || index >= (int)name_vec.size())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetSetName::Index Out Of Range " + to_string((long long)index));
            return string();
        }
        ErrorMgr.NoError();
        return name_vec[index];
    }

    /*!
        Get an array of Geom IDs for the specified set index
        \code{.cpp}
        SetSetName( 3, "SetFromScript" );

        array<string> @geom_arr1 = GetGeomSetAtIndex( 3 );

        array<string> @geom_arr2 = GetGeomSet( "SetFromScript" );

        if ( geom_arr1.size() != geom_arr2.size() )            { Print( "---> Error: API GetGeomSet " ); }
        \endcode
        \sa SET_TYPE
        \param [in] index Set index
        \return Array of Geom IDs
    */

    vector<string> GetGeomSetAtIndex(int index)
    {
        Vehicle *veh = GetVehicle();
        return veh->GetGeomSet(index);
    }

    /*!
        Get an array of Geom IDs for the specified set name
        \code{.cpp}
        SetSetName( 3, "SetFromScript" );

        array<string> @geom_arr1 = GetGeomSetAtIndex( 3 );

        array<string> @geom_arr2 = GetGeomSet( "SetFromScript" );

        if ( geom_arr1.size() != geom_arr2.size() )            { Print( "---> Error: API GetGeomSet " ); }
        \endcode
        \param [in] name const string set name
        \return array<string> array of Geom IDs
    */

    vector<string> GetGeomSet(const string &name)
    {
        Vehicle *veh = GetVehicle();
        vector<string> name_vec = veh->GetSetNameVec();
        int index = -1;
        for (int i = 0; i < (int)name_vec.size(); i++)
        {
            if (name == name_vec[i])
            {
                index = i;
            }
        }
        if (index == -1)
        {
            vector<string> ret_vec;
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetGeomSet::Can't Find Name " + name);
            return ret_vec;
        }
        ErrorMgr.NoError();
        return veh->GetGeomSet(index);
    }

    /*!
        Get the set index for the specified set name
        \code{.cpp}
        SetSetName( 3, "SetFromScript" );

        if ( GetSetIndex( "SetFromScript" ) != 3 ) { Print( "ERROR: GetSetIndex" ); }
        \endcode
        \param [in] name Set name
        \return Set index
    */

    int GetSetIndex(const string &name)
    {
        Vehicle *veh = GetVehicle();
        vector<string> name_vec = veh->GetSetNameVec();
        int index = -1;
        for (int i = 0; i < (int)name_vec.size(); i++)
        {
            if (name == name_vec[i])
            {
                index = i;
            }
        }
        if (index == -1)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_NAME, "GetSetIndex::Can't Find Name " + name);
            return index;
        }

        ErrorMgr.NoError();
        return index;
    }

    /*!
        Check if a Geom is in the set at the specified set index
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        SetSetFlag( fuseid, 3, true );

        if ( !GetSetFlag( fuseid, 3 ) )                        { Print( "---> Error: API Set/Get Set Flag " ); }
        \endcode
        \param [in] geom_id Geom ID
        \param [in] set_index Set index
        \return True if geom is in the set, false otherwise
    */

    bool GetSetFlag(const string &geom_id, int set_index)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "GetSetFlag::Can't Find Geom " + geom_id);
            return false;
        }
        ErrorMgr.NoError();
        return geom_ptr->GetSetFlag(set_index);
    }

    /*!
        Set whether or not a Geom is a member of the set at specified set index
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        SetSetFlag( fuseid, 3, true );

        if ( !GetSetFlag( fuseid, 3 ) )                        { Print( "---> Error: API Set/Get Set Flag " ); }
        \endcode
        \param [in] geom_id Geom ID
        \param [in] set_index Set index
        \param [in] flag Flag that indicates set membership
    */

    void SetSetFlag(const string &geom_id, int set_index, bool flag)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetSetFlag::Can't Find Geom " + geom_id);
            return;
        }
        if (set_index < 0 || set_index >= (int)veh->GetSetNameVec().size())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "SetSetFlag::Invalid Set Index " + to_string((long long)set_index));
            return;
        }

        ErrorMgr.NoError();

        geom_ptr->SetSetFlag(set_index, flag);
    }

    /*!
        Copies all the states of a geom set and pastes them into a specific set based on passed in indexs
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        //set fuseid's state for set 3 to true
        SetSetFlag( fuseid, 3, true );

        //Copy set 3 and Paste into set 4
        CopyPasteSet( 3, 4 );

        //get fuseid's state for set 4
        bool flag_value = GetSetFlag( fuseid, 4 );

        if ( flag_value != true)                      { Print( "---> Error: API CopyPasteSet " ); }
        \endcode
        \param [in] copyIndex Copy Index
        \param [in] pasteIndex Paste Index
    */

    void CopyPasteSet(int copy_index, int paste_index)
    {

        Vehicle *veh = GetVehicle();
        if (copy_index < 0 || copy_index >= (int)veh->GetSetNameVec().size())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CopyPasteSet::Invalid Copy Index " + to_string((long long)copy_index));
            return;
        }

        if (paste_index < 0 || paste_index >= (int)veh->GetSetNameVec().size())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CopyPasteSet::Invalid Paste Index " + to_string((long long)paste_index));
            return;
        }

        if (copy_index > SET_NOT_SHOWN && paste_index > SET_NOT_SHOWN)
        {
            ErrorMgr.NoError();

            veh->CopyPasteSet(copy_index, paste_index);
        }
    }

    //================================================================//
    //=============== Group Modifications for Sets ===================//
    //================================================================//

    /*!
        Apply a scale factor to a set
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE" );

        SetSetFlag( fuseid, 3, true );

        // Scale by a factor of 2
        ScaleSet( 3, 2.0 );
        \endcode
        \param [in] set_index Set index
        \param [in] scale Scale factor
    */

    void ScaleSet(int set_index, double scale)
    {
        Vehicle *veh = GetVehicle();
        GroupTransformations *group_trans = veh->GetGroupTransformationsPtr();
        if (!group_trans)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ScaleSet::Can't Get Group Transformation Pointer");
            return;
        }

        vector<string> geom_id_vec = veh->GetGeomSet(set_index);

        veh->ClearActiveGeom();
        veh->SetActiveGeomVec(geom_id_vec);
        group_trans->ReInitialize();

        group_trans->m_GroupScale.Set(scale);
        group_trans->ParmChanged(NULL, Parm::SET_FROM_DEVICE);

        veh->ClearActiveGeom();
        group_trans->ReInitialize();
    }

    /*!
        Rotate a set about the global X, Y, and Z axes
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE" );

        SetSetFlag( fuseid, 3, true );

        // Rotate 90 degrees about Y
        RotateSet( 3, 0, 90, 0 );
        \endcode
        \param [in] set_index Set index
        \param [in] x_rot_deg Rotation about the X axis (degrees)
        \param [in] y_rot_deg Rotation about the Y axis (degrees)
        \param [in] z_rot_deg Rotation about the Z axis (degrees)
    */

    void RotateSet(int set_index, double x_rot_deg, double y_rot_deg, double z_rot_deg)
    {
        Vehicle *veh = GetVehicle();
        GroupTransformations *group_trans = veh->GetGroupTransformationsPtr();
        if (!group_trans)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "RotateSet::Can't Get Group Transformation Pointer");
            return;
        }

        vector<string> geom_id_vec = veh->GetGeomSet(set_index);

        veh->ClearActiveGeom();
        veh->SetActiveGeomVec(geom_id_vec);
        group_trans->ReInitialize();

        group_trans->m_GroupXRot.Set(x_rot_deg);
        group_trans->m_GroupYRot.Set(y_rot_deg);
        group_trans->m_GroupZRot.Set(z_rot_deg);
        group_trans->ParmChanged(NULL, Parm::SET_FROM_DEVICE);

        veh->ClearActiveGeom();
        group_trans->ReInitialize();
    }

    /*!
        Translate a set along a given vector
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE" );

        SetSetFlag( fuseid, 3, true );

        // Translate 2 units in X and 3 units in Y
        TranslateSet( 3, vec3d( 2, 3, 0 ) );
        \endcode
        \param [in] set_index Set index
        \param [in] translation_vec Translation vector
    */

    void TranslateSet(int set_index, const vec3d &translation_vec)
    {
        Vehicle *veh = GetVehicle();
        GroupTransformations *group_trans = veh->GetGroupTransformationsPtr();
        if (!group_trans)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "TranslateSet::Can't Get Group Transformation Pointer");
            return;
        }

        vector<string> geom_id_vec = veh->GetGeomSet(set_index);

        veh->ClearActiveGeom();
        veh->SetActiveGeomVec(geom_id_vec);
        group_trans->ReInitialize();

        group_trans->m_GroupXLoc.Set(translation_vec.x());
        group_trans->m_GroupYLoc.Set(translation_vec.y());
        group_trans->m_GroupZLoc.Set(translation_vec.z());
        group_trans->ParmChanged(NULL, Parm::SET_FROM_DEVICE);

        veh->ClearActiveGeom();
        group_trans->ReInitialize();
    }

    /*!
        Apply translation, rotation, and scale transformations to a set
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE" );

        SetSetFlag( fuseid, 3, true );

        // Translate 2 units in X and 3 units in Y, rotate 90 degrees about Y, and scale by a factor of 2
        TransformSet( 3, vec3d( 2, 3, 0 ), 0, 90, 0, 2.0, true );
        \endcode
        \sa TranslateSet, RotateSet, ScaleSet
        \param [in] set_index Set index
        \param [in] translation_vec Translation vector
        \param [in] x_rot_deg Rotation about the X axis (degrees)
        \param [in] y_rot_deg Rotation about the Y axis (degrees)
        \param [in] z_rot_deg Rotation about the Z axis (degrees)
        \param [in] scale Scale factor
        \param [in] scale_translations_flag Flag to apply the scale factor to translations
    */

    void TransformSet(int set_index, const vec3d &translation_vec, double x_rot_deg, double y_rot_deg, double z_rot_deg, double scale, bool scale_translations_flag)
    {
        Vehicle *veh = GetVehicle();
        GroupTransformations *group_trans = veh->GetGroupTransformationsPtr();
        if (!group_trans)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "TransformSet::Can't Get Group Transformation Pointer");
            return;
        }

        vector<string> geom_id_vec = veh->GetGeomSet(set_index);

        veh->ClearActiveGeom();
        veh->SetActiveGeomVec(geom_id_vec);
        group_trans->ReInitialize();

        group_trans->m_GroupXLoc.Set(translation_vec.x());
        group_trans->m_GroupYLoc.Set(translation_vec.y());
        group_trans->m_GroupZLoc.Set(translation_vec.z());
        group_trans->m_GroupXRot.Set(x_rot_deg);
        group_trans->m_GroupYRot.Set(y_rot_deg);
        group_trans->m_GroupZRot.Set(z_rot_deg);
        group_trans->m_GroupScale.Set(scale);
        group_trans->m_scaleGroupTranslations.Set(scale_translations_flag);
        group_trans->ParmChanged(NULL, Parm::SET_FROM_DEVICE);

        veh->ClearActiveGeom();
        group_trans->ReInitialize();
    }

    //===================================================================//
    //===============       Parm Functions            ===================//
    //===================================================================//

    /*!
        Check if given Parm is valid
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pid = AddGeom( "POD" );

        string lenid = GetParm( pid, "Length", "Design" );

        if ( !ValidParm( lenid ) )                { Print( "---> Error: API GetParm  " ); }
        \endcode
        \param [in] id Parm ID
        \return True if Parm ID is valid, false otherwise
    */

    bool ValidParm(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            return false;
        }

        return true;
    }

    /*!
        Set the value of the specified Parm.
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, GetNumXSec( xsec_surf ) - 1, XS_ROUNDED_RECTANGLE );

        string xsec = GetXSec( xsec_surf, GetNumXSec( xsec_surf ) - 1 );

        string wid = GetXSecParm( xsec, "RoundedRect_Width" );

        SetParmVal( wid, 23.0 );

        if ( abs( GetParmVal( wid ) - 23 ) > 1e-6 )                { Print( "---> Error: API Parm Val Set/Get " ); }
        \endcode
        \sa SetParmValUpdate
        \param [in] parm_id Parm ID
        \param [in] val Parm value to set
        \return Value that the Parm was set to
    */

    /// Set the parm value.
    /// The final value of parm is returned.
    double SetParmVal(const string &parm_id, double val)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "SetParmVal::Can't Find Parm " + parm_id);
            return val;
        }
        ErrorMgr.NoError();
        return p->Set(val);
    }

    /// Set the parm value.  If update is true, the parm container is updated.
    /// The final value of parm is returned.
    double SetParmVal(const string &container_id, const string &name, const string &group, double val)
    {
        string parm_id = GetParm(container_id, name, group);
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "SetParmVal::Can't Find Parm " + container_id + ":" + group + ":" + name);
            return val;
        }
        ErrorMgr.NoError();
        return p->Set(val);
    }

    /*!
        Set the value along with the upper and lower limits of the specified Parm
        \code{.cpp}
        string pod_id = AddGeom( "POD" );

        string length = FindParm( pod_id, "Length", "Design" );

        SetParmValLimits( length, 10.0, 0.001, 1.0e12 );

        SetParmDescript( length, "Total Length of Geom" );
        \endcode
        \sa SetParmLowerLimit, SetParmUpperLimit
        \param [in] parm_id Parm ID
        \param [in] val Parm value to set
        \param [in] lower_limit Parm lower limit
        \param [in] upper_limit Parm upper limit
        \return Value that the Parm was set to
    */

    double SetParmValLimits(const string &parm_id, double val, double lower_limit, double upper_limit)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "SetParmValLimits::Can't Find Parm " + parm_id);
            return val;
        }
        ErrorMgr.NoError();

        p->SetLowerUpperLimits(lower_limit, upper_limit);
        return p->Set(val);
    }

    /*!
        Set the value of the specified Parm and force an Update.
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        string parm_id = GetParm( pod_id, "X_Rel_Location", "XForm" );

        SetParmValUpdate( parm_id, 5.0 );
        \endcode
        \sa SetParmVal
        \param [in] parm_id Parm ID
        \param [in] val Parm value to set
        \return Value that the Parm was set to
    */

    /// Set the parm value.
    /// The final value of parm is returned.
    double SetParmValUpdate(const string &parm_id, double val)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "SetParmValUpdate::Can't Find Parm " + parm_id);
            return val;
        }
        ErrorMgr.NoError();
        return p->SetFromDevice(val); // Force Update
    }

    /*!
    Set the value of the specified Parm and force an Update. This function includes a call to GetParm to identify the Parm ID given the Container ID, Parm name, and Parm group.
    \code{.cpp}
    //==== Add Pod Geometry ====//
    string pod_id = AddGeom( "POD" );

    SetParmValUpdate( pod_id, "X_Rel_Location", "XForm", 5.0 );
    \endcode
    \sa SetParmVal
    \param [in] container_id Container ID
    \param [in] parm_name Parm name
    \param [in] parm_group_name Parm group name
    \param [in] val Parm value to set
    \return Value that the Parm was set to
*/

    /// Set the parm value.  If update is true, the parm container is updated.
    /// The final value of parm is returned.
    double SetParmValUpdate(const string &container_id, const string &parm_name, const string &parm_group_name, double val)
    {
        string parm_id = GetParm(container_id, parm_name, parm_group_name);
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "SetParmValUpdate::Can't Find Parm " + container_id + ":" + parm_group_name + ":" + parm_name);
            return val;
        }
        ErrorMgr.NoError();
        return p->SetFromDevice(val); // Force Update
    }

    /*!
        Get the value of the specified Parm. The data type of the Parm value will be cast to a double
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, GetNumXSec( xsec_surf ) - 1, XS_ROUNDED_RECTANGLE );

        string xsec = GetXSec( xsec_surf, GetNumXSec( xsec_surf ) - 1 );

        string wid = GetXSecParm( xsec, "RoundedRect_Width" );

        SetParmVal( wid, 23.0 );

        if ( abs( GetParmVal( wid ) - 23 ) > 1e-6 )                { Print( "---> Error: API Parm Val Set/Get " ); }
        \endcode
        \param [in] parm_id Parm ID
        \return Parm value
    */

    double GetParmVal(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetParmVal::Can't Find Parm " + parm_id);
            return 0.0;
        }
        ErrorMgr.NoError();
        return p->Get();
    }

    /*!
        Get the value of the specified double type Parm. This function includes a call to GetParm to identify the Parm ID given the Container ID, Parm name, and Parm group.
        The data type of the Parm value will be cast to a double.
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        double length = GetParmVal( pod_id, "Length", "Design" );
        \endcode
        \param [in] container_id Container ID
        \param [in] name Parm name
        \param [in] group Parm group name
        \return Parm value
    */

    double GetParmVal(const string &container_id, const string &name, const string &group)
    {
        string parm_id = GetParm(container_id, name, group);
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetParmVal::Can't Find Parm " + container_id + ":" + group + ":" + name);
            return 0.0;
        }
        ErrorMgr.NoError();
        return p->Get();
    }

    /*!
        Get the value of the specified int type Parm
        \code{.cpp}
        //==== Add Prop Geometry ====//
        string prop_id = AddGeom( "PROP" );

        string num_blade_id = GetParm( prop_id, "NumBlade", "Design" );

        int num_blade = GetIntParmVal( num_blade_id );
        \endcode
        \param [in] parm_id Parm ID
        \return Parm value
    */

    int GetIntParmVal(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetIntParmVal::Can't Find Parm " + parm_id);
            return 0;
        }
        ErrorMgr.NoError();
        return (int)(p->Get() + 0.5);
    }

    /*!
        Get the value of the specified bool type Parm
        \code{.cpp}
        //==== Add Prop Geometry ====//
        string prop_id = AddGeom( "PROP" );

        string rev_flag_id = GetParm( prop_id, "ReverseFlag", "Design" );

        bool reverse_flag = GetBoolParmVal( rev_flag_id );
        \endcode
        \param [in] parm_id Parm ID
        \return Parm value
    */

    bool GetBoolParmVal(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetBoolParmVal::Can't Find Parm " + parm_id);
            return false;
        }
        if (p->GetType() != PARM_BOOL_TYPE)
        {
            return false;
        }
        ErrorMgr.NoError();

        BoolParm *bp = dynamic_cast<BoolParm *>(p);
        return bp->Get();
    }

    /*!
        Set the upper limit value for the specified Parm
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, GetNumXSec( xsec_surf ) - 1, XS_ROUNDED_RECTANGLE );

        string xsec = GetXSec( xsec_surf, GetNumXSec( xsec_surf ) - 1 );

        string wid = GetXSecParm( xsec, "RoundedRect_Width" );

        SetParmVal( wid, 23.0 );

        SetParmUpperLimit( wid, 13.0 );

        if ( abs( GetParmVal( wid ) - 13 ) > 1e-6 )                { Print( "---> Error: API SetParmUpperLimit " ); }
        \endcode
        \sa SetParmValLimits
        \param [in] parm_id Parm ID
        \param [in] val Parm upper limit
    */

    void SetParmUpperLimit(const string &parm_id, double val)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "SetParmUpperLimit::Can't Find Parm " + parm_id);
            return;
        }
        ErrorMgr.NoError();
        p->SetUpperLimit(val);
    }

    /*!
        Get the upper limit value for the specified Parm
        \code{.cpp}
        //==== Add Prop Geometry ====//
        string prop_id = AddGeom( "PROP" );

        string num_blade_id = GetParm( prop_id, "NumBlade", "Design" );

        double max_blade = GetParmUpperLimit( num_blade_id );
        \endcode
        \param [in] parm_id Parm ID
        \return Parm upper limit
    */

    double GetParmUpperLimit(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetParmUpperLimit::Can't Find Parm " + parm_id);
            return 0.0;
        }
        ErrorMgr.NoError();
        return p->GetUpperLimit();
    }

    /*!
        Set the lower limit value for the specified Parm
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, GetNumXSec( xsec_surf ) - 1, XS_ROUNDED_RECTANGLE );

        string xsec = GetXSec( xsec_surf, GetNumXSec( xsec_surf ) - 1 );

        string wid = GetXSecParm( xsec, "RoundedRect_Width" );

        SetParmVal( wid, 13.0 );

        SetParmLowerLimit( wid, 15.0 );

        if ( abs( GetParmVal( wid ) - 15 ) > 1e-6 )                { Print( "---> Error: API SetParmLowerLimit " ); }
        \endcode
        \sa SetParmValLimits
        \param [in] parm_id Parm ID
        \param [in] val Parm lower limit
    */

    void SetParmLowerLimit(const string &parm_id, double val)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "SetParmLowerLimit::Can't Find Parm " + parm_id);
            return;
        }
        ErrorMgr.NoError();
        p->SetLowerLimit(val);
    }

    /*!
        Get the lower limit value for the specified Parm
        \code{.cpp}
        //==== Add Prop Geometry ====//
        string prop_id = AddGeom( "PROP" );

        string num_blade_id = GetParm( prop_id, "NumBlade", "Design" );

        double min_blade = GetParmLowerLimit( num_blade_id );
        \endcode
        \param [in] parm_id Parm ID
        \return Parm lower limit
    */

    double GetParmLowerLimit(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetParmLowerLimit::Can't Find Parm " + parm_id);
            return 0.0;
        }
        ErrorMgr.NoError();
        return p->GetLowerLimit();
    }

    /*!
        Get the data type for the specified Parm
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, GetNumXSec( xsec_surf ) - 1, XS_ROUNDED_RECTANGLE );

        string xsec = GetXSec( xsec_surf, GetNumXSec( xsec_surf ) - 1 );

        string wid = GetXSecParm( xsec, "RoundedRect_Width" );

        if ( GetParmType( wid ) != PARM_DOUBLE_TYPE )        { Print( "---> Error: API GetParmType " ); }
        \endcode
        \sa PARM_TYPE
        \param [in] parm_id Parm ID
        \return Parm data type enum (i.e. PARM_BOOL_TYPE)
    */

    /// Get the parm type.
    /// 0 = Double
    /// 1 = Int
    /// 2 = Bool
    /// 3 = Fraction
    int GetParmType(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetParmType::Can't Find Parm " + parm_id);
            return PARM_DOUBLE_TYPE;
        }
        ErrorMgr.NoError();
        return p->GetType();
    }

    /*!
        Get the name for the specified Parm
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Get Structure Name and Parm Container ID ====//
        string parm_container_name = GetFeaStructName( pod_id, struct_ind );

        string parm_container_id = FindContainer( parm_container_name, struct_ind );

        //==== Get and List All Parms in the Container ====//
        array<string> parm_ids = FindContainerParmIDs( parm_container_id );

        for ( uint i = 0; i < uint(parm_ids.length()); i++ )
        {
            string name_id = GetParmName( parm_ids[i] ) + string(": ") + parm_ids[i] + string("\n");

            Print( name_id );
        }
        \endcode
        \param [in] parm_id Parm ID
        \return Parm name
    */

    string GetParmName(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetParmName::Can't Find Parm " + parm_id);
            return string();
        }
        ErrorMgr.NoError();
        return p->GetName();
    }

    /*!
        Get the group name for the specified Parm
        \code{.cpp}
        string veh_id = FindContainer( "Vehicle", 0 );

        //==== Get and List All Parms in the Container ====//
        array<string> parm_ids = FindContainerParmIDs( veh_id );

        Print( "Parm Groups and IDs in Vehicle Parm Container: " );

        for ( uint i = 0; i < uint(parm_ids.length()); i++ )
        {
            string group_str = GetParmGroupName( parm_ids[i] ) + string(": ") + parm_ids[i] + string("\n");

            Print( group_str );
        }
        \endcode
        \param [in] parm_id Parm ID
        \return Parm group name
    */

    string GetParmGroupName(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetParmGroupName::Can't Find Parm " + parm_id);
            return string();
        }
        ErrorMgr.NoError();
        return p->GetGroupName();
    }

    /*!
        Get the display group name for the specified Parm
        \code{.cpp}
        string veh_id = FindContainer( "Vehicle", 0 );

        //==== Get and List All Parms in the Container ====//
        array<string> parm_ids = FindContainerParmIDs( veh_id );

        Print( "Parm Group Display Names and IDs in Vehicle Parm Container: " );

        for ( uint i = 0; i < uint(parm_ids.length()); i++ )
        {
            string group_str = GetParmDisplayGroupName( parm_ids[i] ) + string(": ") + parm_ids[i] + string("\n");

            Print( group_str );
        }
        \endcode
        \param [in] parm_id Parm ID
        \return Parm display group name
    */

    string GetParmDisplayGroupName(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetParmDisplayGroupName::Can't Find Parm " + parm_id);
            return string();
        }
        ErrorMgr.NoError();
        return p->GetDisplayGroupName();
    }

    /*!
        Get Parm Container ID for the specified Parm
        \code{.cpp}
        // Add Fuselage Geom
        string fuseid = AddGeom( "FUSELAGE", "" );

        string xsec_surf = GetXSecSurf( fuseid, 0 );

        ChangeXSecShape( xsec_surf, GetNumXSec( xsec_surf ) - 1, XS_ROUNDED_RECTANGLE );

        string xsec = GetXSec( xsec_surf, GetNumXSec( xsec_surf ) - 1 );

        string wid = GetXSecParm( xsec, "RoundedRect_Width" );

        string cid = GetParmContainer( wid );

        if ( cid.size() == 0 )                                { Print( "---> Error: API GetParmContainer " ); }
        \endcode
        \param [in] parm_id Parm ID
        \return Parm Container ID
    */

    string GetParmContainer(const string &parm_id)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "GetParmContainer::Can't Find Parm " + parm_id);
            return string();
        }
        ErrorMgr.NoError();
        return p->GetContainerID();
    }

    /*!
        Set the description of the specified Parm
        \code{.cpp}
        string pod_id = AddGeom( "POD" );

        string length = FindParm( pod_id, "Length", "Design" );

        SetParmValLimits( length, 10.0, 0.001, 1.0e12 );

        SetParmDescript( length, "Total Length of Geom" );
        \endcode
        \param [in] parm_id Parm ID
        \param [in] desc Parm description
    */

    void SetParmDescript(const string &parm_id, const string &desc)
    {
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "SetParmDescript::Can't Find Parm " + parm_id);
            return;
        }
        ErrorMgr.NoError();
        return p->SetDescript(desc);
    }

    /*!
        Find a Parm ID given the Parm Container ID, Parm name, and Parm group
        \code{.cpp}
        //==== Add Wing Geometry ====//
        string wing_id = AddGeom( "WING" );

        //==== Turn Symmetry OFF ====//
        string sym_id = FindParm( wing_id, "Sym_Planar_Flag", "Sym");

        SetParmVal( sym_id, 0.0 ); // Note: bool input not supported in SetParmVal
        \endcode
        \param [in] parm_container_id Parm Container ID
        \param [in] parm_name Parm name
        \param [in] group_name Parm group name
        \return Parm ID
    */

    string FindParm(const string &parm_container_id, const string &parm_name, const string &group_name)
    {
        if (ParmMgr.GetDirtyFlag())
        {
            LinkMgr.BuildLinkableParmData(); // Make Sure Name/Group Get Mapped To Parms
        }

        ParmContainer *pc = ParmMgr.FindParmContainer(parm_container_id);

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "FindParm::Can't Find Parm Container " + parm_container_id);
            return string();
        }

        string parm_id = pc->FindParm(parm_name, group_name);
        Parm *p = ParmMgr.FindParm(parm_id);
        if (!p)
        {
            ErrorMgr.AddError(VSP_CANT_FIND_PARM, "FindParm::Can't Find Parm " + parm_id);
            return string();
        }
        ErrorMgr.NoError();

        return parm_id;
    }

    //===================================================================//
    //===============       Parm Container Functions       ==============//
    //===================================================================//

    /*!
        Get an array of all Parm Container IDs
        \code{.cpp}
        array<string> @ctr_arr = FindContainers();

        Print( "---> API Parm Container IDs: " );

        for ( int i = 0; i < int( ctr_arr.size() ); i++ )
        {
            string message = "\t" + ctr_arr[i] + "\n";

            Print( message );
        }
        \endcode
        \return Array of Parm Container IDs
    */

    vector<std::string> FindContainers()
    {
        vector<string> containerVec;
        if (ParmMgr.GetDirtyFlag())
        {
            LinkMgr.BuildLinkableParmData();
        }
        LinkMgr.GetAllContainerVec(containerVec);

        ErrorMgr.NoError();
        return containerVec;
    }

    /*!
        Get an array of Parm Container IDs for Containers with the specified name
        \code{.cpp}
        array<string> @ctr_arr = FindContainersWithName( "UserParms" );

        if ( ctr_arr.size() > 0 )            { Print( ( "UserParms Parm Container ID: " + ctr_arr[0] ) ); }
        \endcode
        \param [in] name Parm Container name
        \return Array of Parm Container IDs
    */

    vector<std::string> FindContainersWithName(const string &name)
    {
        vector<string> containerVec;
        vector<string> ret_vec;
        if (ParmMgr.GetDirtyFlag())
        {
            LinkMgr.BuildLinkableParmData();
        }
        LinkMgr.GetAllContainerVec(containerVec);

        for (int i = 0; i < (int)containerVec.size(); i++)
        {
            ParmContainer *pc = ParmMgr.FindParmContainer(containerVec[i]);

            if (pc && pc->GetName() == name)
            {
                ret_vec.push_back(containerVec[i]);
            }
        }
        ErrorMgr.NoError();
        return ret_vec;
    }

    /*!
        Get the ID of a Parm Container with specified name at input index
        \code{.cpp}
        //===== Get Vehicle Parm Container ID ====//
        string veh_id = FindContainer( "Vehicle", 0 );
        \endcode
        \sa FindContainersWithName
        \param [in] name Parm Container name
        \param [in] index Parm Container index
        \return Parm Container ID
    */

    string FindContainer(const string &name, int index)
    {
        vector<string> containerVec;
        vector<string> id_vec;
        if (ParmMgr.GetDirtyFlag())
        {
            LinkMgr.BuildLinkableParmData();
        }
        LinkMgr.GetAllContainerVec(containerVec);

        for (int i = 0; i < (int)containerVec.size(); i++)
        {
            ParmContainer *pc = ParmMgr.FindParmContainer(containerVec[i]);

            if (pc && pc->GetName() == name)
            {
                id_vec.push_back(containerVec[i]);
            }
        }

        if (index < 0 || index >= (int)id_vec.size())
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "FindContainer::Can't Find Name " + name + " or Index" + to_string((long long)index));
            return string();
        }
        ErrorMgr.NoError();
        return id_vec[index];
    }

    /*!
        Get the name of the specified Parm Container
        \code{.cpp}
        string veh_id = FindContainer( "Vehicle", 0 );

        if ( GetContainerName( veh_id ) != "Vehicle" )         { Print( "---> Error: API GetContainerName" ); }
        \endcode
        \param [in] parm_container_id Parm Container ID
        \return Parm Container name
    */

    string GetContainerName(const string &parm_container_id)
    {
        string ret_name;

        ParmContainer *pc = ParmMgr.FindParmContainer(parm_container_id);

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "GetContainerName::Can't Find Parm Container " + parm_container_id);
            return string();
        }

        ret_name = pc->GetName();
        ErrorMgr.NoError();
        return ret_name;
    }

    /*!
        Get an array of Parm group names included in the specified Container
        \code{.cpp}
        string user_ctr = FindContainer( "UserParms", 0 );

        array<string> @grp_arr = FindContainerGroupNames( user_ctr );

        Print( "---> UserParms Container Group IDs: " );
        for ( int i = 0; i < int( grp_arr.size() ); i++ )
        {
            string message = "\t" + grp_arr[i] + "\n";

            Print( message );
        }
        \endcode
        \param [in] parm_container_id Parm Container ID
        \return Array of Parm group names
    */

    vector<string> FindContainerGroupNames(const string &parm_container_id)
    {
        vector<string> ret_names;

        ParmContainer *pc = ParmMgr.FindParmContainer(parm_container_id);

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "FindContainerGroupNames::Can't Find Parm Container " + parm_container_id);
            return ret_names;
        }

        pc->GetGroupNames(ret_names);

        ErrorMgr.NoError();
        return ret_names;
    }

    /*!
        Get an array of Parm IDs included in the specified Container
        \code{.cpp}
        //==== Add Pod Geometry ====//
        string pod_id = AddGeom( "POD" );

        //==== Add FeaStructure to Pod ====//
        int struct_ind = AddFeaStruct( pod_id );

        //==== Get Structure Name and Parm Container ID ====//
        string parm_container_name = GetFeaStructName( pod_id, struct_ind );

        string parm_container_id = FindContainer( parm_container_name, struct_ind );

        //==== Get and List All Parms in the Container ====//
        array<string> parm_ids = FindContainerParmIDs( parm_container_id );

        for ( uint i = 0; i < uint(parm_ids.length()); i++ )
        {
            string name_id = GetParmName( parm_ids[i] ) + string(": ") + parm_ids[i] + string("\n");

            Print( name_id );
        }
        \endcode
        \param [in] parm_container_id Parm Container ID
        \return Array of Parm IDs
    */

    vector<string> FindContainerParmIDs(const string &parm_container_id)
    {
        vector<string> parm_vec;

        ParmContainer *pc = ParmMgr.FindParmContainer(parm_container_id);

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "FindContainerParmIDs::Can't Find Parm Container " + parm_container_id);
            return parm_vec;
        }

        pc->AddLinkableParms(parm_vec);

        ErrorMgr.NoError();
        return parm_vec;
    }

    /*!
        Get the ID of the Vehicle Parm Container
        \code{.cpp}
        //===== Get Vehicle Parm Container ID ====//
        string veh_id = GetVehicleID();
        \endcode
        \return Vehicle ID
    */

    string GetVehicleID()
    {
        Vehicle *veh = GetVehicle();

        ErrorMgr.NoError();
        return veh->GetID();
    }

    //===================================================================//
    //===============           Snap To Functions          ==============//
    //===================================================================//

    /*!
        Compute the minimum clearance distance for the specified geometry
        \code{.cpp}
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        string pid = AddGeom( "POD", "" );                     // Add Pod

        string x = GetParm( pid, "X_Rel_Location", "XForm" );

        SetParmVal( x, 3.0 );

        Update();

        double min_dist = ComputeMinClearanceDistance( pid, SET_ALL );
        \endcode
        \param [in] geom_id Geom ID
        \param [in] set Collision set enum (i.e. SET_ALL)
        \return Minimum clearance distance
    */

    double ComputeMinClearanceDistance(const string &geom_id, int set)
    {
        Vehicle *veh = GetVehicle();

        int old_set = veh->GetSnapToPtr()->m_CollisionSet;
        veh->GetSnapToPtr()->m_CollisionSet = set;

        vector<string> old_active_geom = veh->GetActiveGeomVec();
        veh->SetActiveGeom(geom_id);

        veh->GetSnapToPtr()->CheckClearance();
        double min_clearance_dist = veh->GetSnapToPtr()->m_CollisionMinDist;

        //==== Restore State ====//
        veh->GetSnapToPtr()->m_CollisionSet = old_set;
        veh->SetActiveGeomVec(old_active_geom);

        return min_clearance_dist;
    }

    /*!
        Snap the specified Parm to input target minimum clearance distance
        \code{.cpp}
        //Add Geoms
        string fid = AddGeom( "FUSELAGE", "" );             // Add Fuselage

        string pid = AddGeom( "POD", "" );                     // Add Pod

        string x = GetParm( pid, "X_Rel_Location", "XForm" );

        SetParmVal( x, 3.0 );

        Update();

        double min_dist = SnapParm( x, 0.1, true, SET_ALL );
        \endcode
        \param [in] parm_id Parm ID
        \param [in] target_min_dist Target minimum clearance distance
        \param [in] inc_flag Direction indication flag. If true, upper parm limit is used and direction is set to positive
        \param [in] set Collision set enum (i.e. SET_ALL)
        \return Minimum clearance distance
    */

    double SnapParm(const string &parm_id, double target_min_dist, bool inc_flag, int set)
    {
        Vehicle *veh = GetVehicle();

        int old_set = veh->GetSnapToPtr()->m_CollisionSet;
        veh->GetSnapToPtr()->m_CollisionSet = set;

        double old_min_dist = veh->GetSnapToPtr()->m_CollisionTargetDist();
        veh->GetSnapToPtr()->m_CollisionTargetDist = target_min_dist;

        veh->GetSnapToPtr()->AdjParmToMinDist(parm_id, inc_flag);
        double min_clearance_dist = veh->GetSnapToPtr()->m_CollisionMinDist;

        //==== Restore State ====//
        veh->GetSnapToPtr()->m_CollisionSet = old_set;
        veh->GetSnapToPtr()->m_CollisionTargetDist = old_min_dist;

        return min_clearance_dist;
    }

    //===================================================================//
    //===============     Variable Presets Functions       ==============//
    //===================================================================//

    /*!
        Add a Variable Presets group
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        if ( GetVarPresetGroupNames().size() != 1 )                    { Print( "---> Error: API AddVarPresetGroup" ); }
        \endcode
        \param [in] group_name Variable Presets group name
    */

    void AddVarPresetGroup(const string &group_name)
    {
        VarPresetMgr.AddGroup(group_name);
        VarPresetMgr.SavePreset();

        ErrorMgr.NoError();
    }

    /*!
        Add a setting to the currently active Variable Preset
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        if ( GetVarPresetSettingNamesWName( "Tess" ).size() != 1 )            { Print( "---> Error: API AddVarPresetSetting" ); }
        \endcode
        \param [in] setting_name Variable Presets setting name
    */

    void AddVarPresetSetting(const string &setting_name)
    {
        VarPresetMgr.AddSetting(setting_name);
        VarPresetMgr.SavePreset();

        ErrorMgr.NoError();
    }

    /*!
        Add a Parm to the currently active Variable Preset
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1 );
        \endcode
        \param [in] parm_ID Parm ID
    */

    void AddVarPresetParm(const string &parm_ID)
    {
        if (!VarPresetMgr.AddVar(parm_ID))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "AddVarPresetParm::Failed to add Variable Preset " + parm_ID);
        }
        VarPresetMgr.SavePreset();

        ErrorMgr.NoError();
    }

    /*!
        Add a Parm to the specified Variable Preset group
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1, "Tess" );
        \endcode
        \param [in] parm_ID Parm ID
        \param [in] group_name Variable Presets group name
    */

    void AddVarPresetParm(const string &parm_ID, const string &group_name)
    {
        VarPresetMgr.GroupChange(group_name);
        if (!VarPresetMgr.AddVar(parm_ID))
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "AddVarPresetParm::Failed to add Variable Preset " + parm_ID);
        }
        VarPresetMgr.SavePreset();

        ErrorMgr.NoError();
    }

    /*!
        Edit the value of a Parm in the currently active Variable Preset
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1 );

        EditVarPresetParm( p1, 5 );
        \endcode
        \param [in] parm_ID Parm ID
        \param [in] parm_val Parm value
    */

    void EditVarPresetParm(const string &parm_ID, double parm_val)
    {
        Parm *p = ParmMgr.FindParm(parm_ID);
        if (p)
        {
            p->Set(parm_val);
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "EditVarPresetParm::Can't Find Parm " + parm_ID);
        }
        VarPresetMgr.SavePreset();
    }

    /*!
        Edit the value of a Parm in the specified Variable Preset group and setting
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1 );

        AddVarPresetGroup( "New_Group" );

        EditVarPresetParm( p1, 5, "Tess", "Coarse" );
        \endcode
        \param [in] parm_ID Parm ID
        \param [in] parm_val Parm value
        \param [in] group_name Variable Presets group name
        \param [in] setting_name Variable Presets setting name
    */

    void EditVarPresetParm(const string &parm_ID, double parm_val, const string &group_name,
                           const string &setting_name)
    {
        SwitchVarPreset(group_name, setting_name);
        EditVarPresetParm(parm_ID, parm_val);
    }

    /*!
        Remove a Parm from the currently active Variable Preset group
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1 );

        DeleteVarPresetParm( p1 );
        \endcode
        \param [in] parm_ID Parm ID
    */

    void DeleteVarPresetParm(const string &parm_ID)
    {
        VarPresetMgr.SetWorkingParmID(parm_ID);
        VarPresetMgr.DelCurrVar();
        VarPresetMgr.SavePreset();

        ErrorMgr.NoError();
    }

    /*!
        Remove a Parm from a Variable Preset group
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1 );

        AddVarPresetGroup( "New_Group" );

        DeleteVarPresetParm( p1, "Tess" );
        \endcode
        \param [in] parm_ID Parm ID
        \param [in] group_name Variable Presets group name
    */

    void DeleteVarPresetParm(const string &parm_ID, const string &group_name)
    {
        VarPresetMgr.GroupChange(group_name);
        if (VarPresetMgr.GetActiveGroupText().compare(group_name) == 0)
        {
            ErrorMgr.NoError();
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_VARPRESET_GROUPNAME, "DeleteVarPresetParm::Can't Find Group " + group_name);
        }
        DeleteVarPresetParm(parm_ID);
    }

    /*!
        Change the currently active Variable Preset
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Config" );

        AddVarPresetSetting( "Default" );

        string p1 = FindParm( pod1, "Y_Rel_Rotation", "XForm" );
        string p2 = FindParm( pod1, "Z_Rel_Rotation", "XForm" );

        AddVarPresetParm( p1 );
        AddVarPresetParm( p2 );

        SwitchVarPreset( "Config", "Default" );
        \endcode
        \param [in] group_name Variable Presets group name
        \param [in] setting_name Variable Presets setting name
    */

    void SwitchVarPreset(const string &group_name, const string &setting_name)
    {
        VarPresetMgr.GroupChange(group_name);
        if (VarPresetMgr.GetActiveGroupText().compare(group_name) == 0)
        {
            ErrorMgr.NoError();
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_VARPRESET_GROUPNAME, "SwitchVarPreset::Can't Find Group " + group_name);
        }
        VarPresetMgr.SettingChange(setting_name);
        if (VarPresetMgr.GetActiveSettingText().compare(setting_name) == 0)
        {
            ErrorMgr.NoError();
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_VARPRESET_SETNAME, "SwitchSaveParmGroup::Can't Find Setting " + setting_name);
        }
        VarPresetMgr.ApplySetting();
    }

    /*!
        Delete a Variable Preset
        \code{.cpp}
        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Fine" );

        DeleteVarPresetSet( "Tess", "Fine" );

        if ( GetVarPresetSettingNamesWName( "Tess" ).size() != 0 )    { Print ( "---> Error: DeleteVarPresetSet" ); }
        \endcode
        \param [in] group_name Variable Presets group
        \param [in] setting_name Variable Presets setting name
        \return true is successful, false otherwise
    */

    bool DeleteVarPresetSet(const string &group_name, const string &setting_name)
    {
        if (VarPresetMgr.DeletePreset(group_name, setting_name))
        {
            ErrorMgr.NoError();
            return true;
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_VARPRESET_GROUPNAME, "DeleteVarPresetSet::Can't Find Group " + group_name);
            ErrorMgr.AddError(VSP_INVALID_VARPRESET_SETNAME, "DeleteVarPresetSet::Can't Find Setting " + setting_name);
            return false;
        }
    }

    /*!
        Get the currently active Variable Presets group name
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1 );

        AddVarPresetGroup( "New_Group" );

        AddVarPresetSetting( "New_Setting" );

        Print( "Current Group: " );

        Print( GetCurrentGroupName() );
        \endcode
        \return Variable Presets group name
    */

    string GetCurrentGroupName()
    {
        return VarPresetMgr.GetActiveGroupText();
    }

    /*!
        Get the currently active Variable Presets setting name
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1 );

        AddVarPresetGroup( "New_Group" );

        AddVarPresetSetting( "New_Setting" );

        Print( "Current Setting: " );

        Print( GetCurrentSettingName() );
        \endcode
        \return Variable Presets setting name
    */

    string GetCurrentSettingName()
    {
        return VarPresetMgr.GetActiveSettingText();
    }

    /*!
        Get all Variable Preset group names
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        if ( GetVarPresetGroupNames().size() != 1 )                    { Print( "---> Error: API AddVarPresetGroup" ); }
        \endcode
        \return Array of Variable Presets group names
    */

    vector<string> GetVarPresetGroupNames()
    {
        ErrorMgr.NoError();
        return VarPresetMgr.GetGroupNames();
    }

    /*!
        Get the name of each settings in the specified Variable Presets group name
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        if ( GetVarPresetSettingNamesWName( "Tess" ).size() != 1 )            { Print( "---> Error: API AddVarPresetSetting" ); }
        \endcode
        \param [in] group_name Variable Presets group name
        \return Array of Variable Presets setting names
    */

    vector<string> GetVarPresetSettingNamesWName(const string &group_name)
    {
        vector<string> vec;
        vec = VarPresetMgr.GetSettingNames(group_name);

        if (vec.empty())
        {
            ErrorMgr.AddError(VSP_INVALID_VARPRESET_GROUPNAME, "GetVarPresetSettingNamesWName::Can't Find Group " + group_name);
            return vec;
        }
        else
        {
            ErrorMgr.NoError();
            return vec;
        }
    }

    /*!
        Get the name of each settings in the specified Variable Presets group index
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1 );

        AddVarPresetGroup( "New_Group" );

        AddVarPresetSetting( "New_Setting_1" );
        AddVarPresetSetting( "New_Setting_2" );

        array < string > group_1_settings = GetVarPresetSettingNamesWIndex( 1 );

        if ( group_1_settings.size() != 2 )            { Print( "---> Error: API GetVarPresetSettingNamesWIndex" ); }
        \endcode
        \param [in] group_index Variable Presets group index
        \return Array of Variable Presets setting names
    */

    vector<string> GetVarPresetSettingNamesWIndex(int group_index)
    {
        vector<string> vec;
        vec = VarPresetMgr.GetSettingNames(group_index);

        if (vec.empty())
        {
            ErrorMgr.AddError(VSP_INVALID_VARPRESET_GROUPNAME, "GetVarPresetSettingNamesWIndex::Can't Find Group @ Index " + to_string(group_index));
            return vec;
        }
        else
        {
            ErrorMgr.NoError();
            return vec;
        }
    }

    /*!
        Get the value of each Parm in the currently active Variable Preset group and setting
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1 );

        EditVarPresetParm( p1, 5 );

        array <double> p_vals = GetVarPresetParmVals();

        if ( p_vals[0] != 5 )                                { Print ( "---> Error: API EditVarPresetParm" ); }
        \endcode
        \return Array of Variable Presets Parm values
    */

    vector<double> GetVarPresetParmVals()
    {
        ErrorMgr.NoError();
        return VarPresetMgr.GetCurrentParmVals();
    }

    /*!
        Get the value of each Parm in the specified Variable Preset group and setting
        param [in] group_name Variable Presets group name
        param [in] setting_name Variable Presets setting name
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "New_Group" );

        AddVarPresetSetting( "New_Setting_1" );
        AddVarPresetSetting( "New_Setting_2" );

        string p1 = FindParm( pod1, "Y_Rel_Rotation", "XForm" );
        string p2 = FindParm( pod1, "Z_Rel_Rotation", "XForm" );

        AddVarPresetParm( p1 );
        AddVarPresetParm( p2 );

        EditVarPresetParm( p2, 2, "New_Group", "New_Setting_2" );

        array < double > parm_vals = GetVarPresetParmValsWNames( "New_Group", "New_Setting_2" );

        if ( parm_vals.size() != 2 )            { Print( "---> Error: API GetVarPresetParmValsWNames" ); }
        \endcode
        \return Array of Variable Presets Parm values
    */

    vector<double> GetVarPresetParmValsWNames(const string &group_name, const string &setting_name)
    {
        ErrorMgr.NoError();
        return VarPresetMgr.GetParmVals(group_name, setting_name);
    }

    /*!
        Get the Parm IDs contained in the currently active Variable Presets group
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "Tess" );

        AddVarPresetSetting( "Coarse" );

        string p1 = FindParm( pod1, "Tess_U", "Shape" );

        AddVarPresetParm( p1 );

        array <string> p_IDs = GetVarPresetParmIDs();

        if ( p_IDs.size() != 1 )                                { Print( "---> Error: API AddVarPresetParm" ); }
        \endcode
        \return Array of Variable Presets Parm IDs
    */

    vector<string> GetVarPresetParmIDs()
    {
        ErrorMgr.NoError();
        return VarPresetMgr.GetCurrentParmIDs();
    }

    /*!
        Get the Parm IDs contained in the specitied Variable Presets group
        \code{.cpp}
        // Add Pod Geom
        string pod1 = AddGeom( "POD", "" );

        AddVarPresetGroup( "New_Group" );

        AddVarPresetSetting( "New_Setting_1" );
        AddVarPresetSetting( "New_Setting_2" );

        string p1 = FindParm( pod1, "Y_Rel_Rotation", "XForm" );
        string p2 = FindParm( pod1, "Z_Rel_Rotation", "XForm" );

        AddVarPresetParm( p1 );
        AddVarPresetParm( p2 );

        array < string > parm_ids = GetVarPresetParmIDsWName( "New_Group" );

        if ( parm_ids.size() != 2 )            { Print( "---> Error: API GetVarPresetParmIDsWName" ); }
        \endcode
        \param [in] group_name Variable Presets group name
        \return Array of Parm IDs
    */

    vector<string> GetVarPresetParmIDsWName(const string &group_name)
    {
        ErrorMgr.NoError();
        return VarPresetMgr.GetParmIDs(group_name);
    }

    //===================================================================//
    //===============     Parametric Curve Functions       ==============//
    //===================================================================//

    /*!
        Set the parameters, values, and curve type of a propeller blade curve (P Curve)
        \sa PCURV_TYPE
        \param [in] geom_id Parent Geom ID
        \param [in] pcurveid P Curve index
        \param [in] tvec Array of parameter values
        \param [in] valvec Array of values
        \param [in] newtype Curve type enum (i.e. CEDIT)
    */

    void SetPCurve(const string &geom_id, const int &pcurveid, const vector<double> &tvec,
                   const vector<double> &valvec, const int &newtype)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetPCurve::Can't Find Geom " + geom_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != PROP_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "SetPCurve::Geom doesn't support PCurves " + geom_id);
            return;
        }

        PropGeom *prop_ptr = dynamic_cast<PropGeom *>(geom_ptr);
        PCurve *pc = NULL;

        if (prop_ptr)
        {
            pc = prop_ptr->GetPCurve(pcurveid);
        }

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "SetPCurve::PCurve not found " + geom_id + " " + to_string(pcurveid));
            return;
        }

        pc->SetCurve(tvec, valvec, newtype);

        ErrorMgr.NoError();
    }

    /*!
        Change the type of a propeller blade curve (P Curve)
        \sa PCURV_TYPE
        \param [in] geom_id Parent Geom ID
        \param [in] pcurveid P Curve index
        \param [in] newtype Curve type enum (i.e. CEDIT)
    */

    void PCurveConvertTo(const string &geom_id, const int &pcurveid, const int &newtype)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveConvertTo::Can't Find Geom " + geom_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != PROP_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveConvertTo::Geom doesn't support PCurves " + geom_id);
            return;
        }

        PropGeom *prop_ptr = dynamic_cast<PropGeom *>(geom_ptr);
        PCurve *pc = NULL;

        if (prop_ptr)
        {
            pc = prop_ptr->GetPCurve(pcurveid);
        }

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "PCurveConvertTo::PCurve not found " + geom_id + " " + to_string(pcurveid));
            return;
        }

        pc->ConvertTo(newtype);

        ErrorMgr.NoError();
    }

    /*!
        Get the type of a propeller blade curve (P Curve)
        \sa PCURV_TYPE
        \param [in] geom_id Parent Geom ID
        \param [in] pcurveid P Curve index
        \return Curve type enum (i.e. CEDIT)
    */

    int PCurveGetType(const std::string &geom_id, const int &pcurveid)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveGetType::Can't Find Geom " + geom_id);
            return -1;
        }
        else if (geom_ptr->GetType().m_Type != PROP_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveGetType::Geom doesn't support PCurves " + geom_id);
            return -1;
        }

        PropGeom *prop_ptr = dynamic_cast<PropGeom *>(geom_ptr);
        PCurve *pc = NULL;

        if (prop_ptr)
        {
            pc = prop_ptr->GetPCurve(pcurveid);
        }

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "PCurveGetType::PCurve not found " + geom_id + " " + to_string(pcurveid));
            return -1;
        }

        ErrorMgr.NoError();

        return pc->m_CurveType();
    }

    /*!
        Get the parameters of a propeller blade curve (P Curve). Each parameter is a fraction of propeller radius.
        \param [in] geom_id Parent Geom ID
        \param [in] pcurveid P Curve index
        \return Array of parameters
    */

    vector<double> PCurveGetTVec(const string &geom_id, const int &pcurveid)
    {
        vector<double> retvec;

        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveGetTVec::Can't Find Geom " + geom_id);
            return retvec;
        }
        else if (geom_ptr->GetType().m_Type != PROP_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveGetTVec::Geom doesn't support PCurves " + geom_id);
            return retvec;
        }

        PropGeom *prop_ptr = dynamic_cast<PropGeom *>(geom_ptr);
        PCurve *pc = NULL;

        if (prop_ptr)
        {
            pc = prop_ptr->GetPCurve(pcurveid);
        }

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "PCurveGetTVec::PCurve not found " + geom_id + " " + to_string(pcurveid));
            return retvec;
        }

        retvec = pc->GetTVec();

        ErrorMgr.NoError();

        return retvec;
    }

    /*!
        Get the values of a propeller blade curve (P Curve). What the values represent id dependent on the curve type (i.e. twist, chord, etc.).
        \param [in] geom_id Parent Geom ID
        \param [in] pcurveid P Curve index
        \return Array of values
    */

    vector<double> PCurveGetValVec(const string &geom_id, const int &pcurveid)
    {
        vector<double> retvec;

        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveGetValVec::Can't Find Geom " + geom_id);
            return retvec;
        }
        else if (geom_ptr->GetType().m_Type != PROP_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveGetValVec::Geom doesn't support PCurves " + geom_id);
            return retvec;
        }

        PropGeom *prop_ptr = dynamic_cast<PropGeom *>(geom_ptr);
        PCurve *pc = NULL;

        if (prop_ptr)
        {
            pc = prop_ptr->GetPCurve(pcurveid);
        }

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "PCurveGetValVec::PCurve not found " + geom_id + " " + to_string(pcurveid));
            return retvec;
        }

        retvec = pc->GetValVec();

        ErrorMgr.NoError();

        return retvec;
    }

    /*!
        Delete a propeller blade curve (P Curve) point
        \param [in] geom_id Parent Geom ID
        \param [in] pcurveid P Curve index
        \param [in] indx Point index
    */

    void PCurveDeletePt(const string &geom_id, const int &pcurveid, const int &indx)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveDeletePt::Can't Find Geom " + geom_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != PROP_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveDeletePt::Geom doesn't support PCurves " + geom_id);
            return;
        }

        PropGeom *prop_ptr = dynamic_cast<PropGeom *>(geom_ptr);
        PCurve *pc = NULL;

        if (prop_ptr)
        {
            pc = prop_ptr->GetPCurve(pcurveid);
        }

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "PCurveDeletePt::PCurve not found " + geom_id + " " + to_string(pcurveid));
            return;
        }

        return pc->DeletePt(indx);

        ErrorMgr.NoError();
    }

    /*!
        Split a propeller blade curve (P Curve) at the specified 1D parameter
        \param [in] geom_id Parent Geom ID
        \param [in] pcurveid P Curve index
        \param [in] tsplit 1D parameter split location
        \return Index of new control point
    */

    int PCurveSplit(const string &geom_id, const int &pcurveid, const double &tsplit)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveSplit::Can't Find Geom " + geom_id);
            return -1;
        }
        else if (geom_ptr->GetType().m_Type != PROP_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "PCurveSplit::Geom doesn't support PCurves " + geom_id);
            return -1;
        }

        PropGeom *prop_ptr = dynamic_cast<PropGeom *>(geom_ptr);
        PCurve *pc = NULL;

        if (prop_ptr)
        {
            pc = prop_ptr->GetPCurve(pcurveid);
        }

        if (!pc)
        {
            ErrorMgr.AddError(VSP_INVALID_ID, "PCurveSplit::PCurve not found " + geom_id + " " + to_string(pcurveid));
            return -1;
        }

        return pc->Split(tsplit);

        ErrorMgr.NoError();
    }

    /*!
        Approximate all propeller blade curves with cubic Bezier curves.
        \code{.cpp}
        // Add Propeller
        string prop = AddGeom( "PROP", "" );

        ApproximateAllPropellerPCurves( prop );

        \endcode
        \param [in] geom_id Geom ID
    */

    void ApproximateAllPropellerPCurves(const std::string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ApproximateAllPropellerPCurves::Can't Find Geom " + geom_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != PROP_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ApproximateAllPropellerPCurves::Geom not a propeller " + geom_id);
            return;
        }

        PropGeom *prop_ptr = dynamic_cast<PropGeom *>(geom_ptr);

        if (prop_ptr)
        {
            prop_ptr->ApproxCubicAllPCurves();
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ApproximateAllPropellerPCurves::Geom not a propeller " + geom_id);
            return;
        }

        ErrorMgr.NoError();
    }

    /*!
        Reset propeller T/C curve to match basic thickness of file-type airfoils.  Typically only used for a propeller that
        has been constructed with file-type airfoils across the blade.  The new thickness curve will be a PCHIP curve
        with t/c matching the propeller's XSecs -- unless it is a file XSec, then the Base thickness is used.
        \code{.cpp}
        // Add Propeller
        string prop = AddGeom( "PROP", "" );

        ResetPropellerThicknessCurve( prop );

        \endcode
        \param [in] geom_id Geom ID
    */

    void ResetPropellerThicknessCurve(const std::string &geom_id)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ResetPropellerThicknessCurve::Can't Find Geom " + geom_id);
            return;
        }
        else if (geom_ptr->GetType().m_Type != PROP_GEOM_TYPE)
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ResetPropellerThicknessCurve::Geom not a propeller " + geom_id);
            return;
        }

        PropGeom *prop_ptr = dynamic_cast<PropGeom *>(geom_ptr);

        if (prop_ptr)
        {
            prop_ptr->ResetThickness();
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_PTR, "ResetPropellerThicknessCurve::Geom not a propeller " + geom_id);
            return;
        }

        ErrorMgr.NoError();
    }

    //===================================================================//
    //===============    Parasite Drag Tool Functions      ==============//
    //===================================================================//

    /*!
        Add an Excresence to the Parasite Drag Tool
        \code{.cpp}
        AddExcrescence( "Miscellaneous", EXCRESCENCE_COUNT, 8.5 );

        AddExcrescence( "Cowl Boattail", EXCRESCENCE_CD, 0.0003 );
        \endcode
        \sa EXCRES_TYPE
        \param [in] excresName Name of the Excressence
        \param [in] excresType Excressence type enum (i.e. EXCRESCENCE_PERCENT_GEOM)
        \param [in] excresVal Excressence value
    */

    void AddExcrescence(const std::string &excresName, const int &excresType, const double &excresVal)
    {
        ParasiteDragMgr.AddExcrescence(excresName, excresType, excresVal);

        ErrorMgr.NoError();
    }

    /*!
        Delete an Excresence from the Parasite Drag Tool
        \code{.cpp}
        AddExcrescence( "Miscellaneous", EXCRESCENCE_COUNT, 8.5 );

        AddExcrescence( "Cowl Boattail", EXCRESCENCE_CD, 0.0003 );

        AddExcrescence( "Percentage Example", EXCRESCENCE_PERCENT_GEOM, 5 );

        DeleteExcrescence( 2 ); // Last Index
        \endcode
        \param [in] excresName Name of the Excressence
    */

    void DeleteExcrescence(const int &index)
    {
        ParasiteDragMgr.DeleteExcrescence(index);

        ErrorMgr.NoError();
    }

    /*!
        Update any reference geometry, atmospheric properties, excressences, etc. in the Parasite Drag Tool
    */

    void UpdateParasiteDrag()
    {
        ParasiteDragMgr.Update();

        ErrorMgr.NoError();
    }

    /*!
        Calculate the atmospheric properties determined by a specified model for a preset array of altitudes ranging from 0 to 90000 m and
        write the results to a CSV output file
        \code{.cpp}
        Print( "Starting USAF Atmosphere 1966 Table Creation. \n" );

        WriteAtmosphereCSVFile( "USAFAtmosphere1966Data.csv", ATMOS_TYPE_HERRINGTON_1966 );
        \endcode
        \sa ATMOS_TYPE
        \param [in] file_name Output CSV file
        \param [in] atmos_type Atmospheric model enum (i.e. ATMOS_TYPE_HERRINGTON_1966)
    */

    void WriteAtmosphereCSVFile(const std::string &file_name, const int &atmos_type)
    {
        const static double arr[] = {0.0, 5000.0, 10000.0, 10999.0, 11001.0, 15000.0, 19999.0, 20000.0,
                                     20001.0, 25000.0, 30000.0, 31999.0, 32001.0, 35000.0, 40000.0, 45000.0, 46999.0, 47001.0, 50000.0,
                                     50999.0, 51001.0, 55000.0, 60000.0, 65000.0, 70000.0, 70999.0, 71001.0, 75000.0, 80000.0, 84851.0,
                                     84853.0, 85000.0, 90000.0}; // meters
        vector<double> AltTestPoints(arr, arr + sizeof(arr) / sizeof(arr[0]));

        double temp, pres, pres_ratio, rho_ratio;
        vector<double> temp_vec, pres_vec, pres_ratio_vec, rho_ratio_vec;

        for (size_t i = 0; i < AltTestPoints.size(); ++i)
        {
            vsp::CalcAtmosphere(AltTestPoints[i], 0.0, atmos_type,
                                temp, pres, pres_ratio, rho_ratio);
            temp_vec.push_back(temp);
            pres_vec.push_back(pres);
            pres_ratio_vec.push_back(pres_ratio);
            rho_ratio_vec.push_back(rho_ratio);
        }
        Results *res = ResultsMgr.CreateResults("Atmosphere");
        res->Add(NameValData("Alt", AltTestPoints));
        res->Add(NameValData("Temp", temp_vec));
        res->Add(NameValData("Pres", pres_vec));
        res->Add(NameValData("Pres_Ratio", pres_ratio_vec));
        res->Add(NameValData("Rho_Ratio", rho_ratio_vec));
        res->WriteCSVFile(file_name);
    }

    /*!
        Calculate the atmospheric properties determined by a specified model at input altitude and temperature deviation. This function may
        not be used for any manual atmospheric model types (i.e. ATMOS_TYPE_MANUAL_P_T). This function assumes freestream units are metric,
        temperature units are Kelvin, and pressure units are kPA.
        \code{.cpp}
        double temp, pres, pres_ratio, rho_ratio;

        double alt = 4000;

        double delta_temp = 0;

        CalcAtmosphere( alt, delta_temp, ATMOS_TYPE_US_STANDARD_1976, temp, pres, pres_ratio, rho_ratio );
        \endcode
        \sa ATMOS_TYPE
        \param [in] alt Altitude
        \param [in] delta_temp Deviation in temperature from the value specified in the atmospheric model
        \param [in] atmos_type Atmospheric model enum (i.e. ATMOS_TYPE_HERRINGTON_1966)
        \param [out] temp output Temperature
        \param [out] pres output Pressure
        \param [out] pres_ratio Output pressure ratio
        \param [out] rho_ratio Output density ratio
    */

    void CalcAtmosphere(const double &alt, const double &delta_temp, const int &atmos_type,
                        double &temp, double &pres, double &pres_ratio, double &rho_ratio)
    {
        Atmosphere atmos;

        switch (atmos_type)
        {
        case vsp::ATMOS_TYPE_US_STANDARD_1976:
            atmos.USStandardAtmosphere1976(alt, delta_temp, vsp::PD_UNITS_METRIC, vsp::TEMP_UNIT_K, vsp::PRES_UNIT_KPA);
            break;

        case vsp::ATMOS_TYPE_HERRINGTON_1966:
            atmos.USAF1966(alt, delta_temp, vsp::PD_UNITS_METRIC, vsp::TEMP_UNIT_K, vsp::PRES_UNIT_KPA);
            break;

        default:
            break;
        }

        temp = atmos.GetTemp();
        pres = atmos.GetPres();
        pres_ratio = atmos.GetPressureRatio();
        rho_ratio = atmos.GetDensityRatio();

        ErrorMgr.NoError();
    }

    /*!
        Calculate the form factor from each body FF equation (i.e. Hoerner Streamlined Body) and write the results to a CSV output file
        \code{.cpp}
        Print( "Starting Body Form Factor Data Creation. \n" );
        WriteBodyFFCSVFile( "BodyFormFactorData.csv" );
        \endcode
        \param [in] file_name Output CSV file
    */

    void WriteBodyFFCSVFile(const std::string &file_name)
    {
        Results *res = ResultsMgr.CreateResults("Body_Form_Factor");
        char str[256];
        vector<double> body_ff_vec;
        vector<double> dol_array = linspace(0.0, 0.3, 200);
        res->Add(NameValData("D_L", dol_array));

        for (size_t body_ff_case = 0; body_ff_case <= vsp::FF_B_JENKINSON_AFT_FUSE_NACELLE; ++body_ff_case)
        {
            for (size_t j = 0; j < dol_array.size(); ++j)
            {
                body_ff_vec.push_back(ParasiteDragMgr.CalcFFBody(1.0 / dol_array[j], body_ff_case));
            }
            sprintf(str, "%s", ParasiteDragMgr.AssignFFBodyEqnName(body_ff_case).c_str());
            res->Add(NameValData(str, body_ff_vec));
            body_ff_vec.clear();
        }
        res->WriteCSVFile(file_name);
    }

    /*!
        Calculate the form factor from each wing FF equation (i.e. Schemensky 4 Series Airfoil) and write the results to a CSV output file
        \code{.cpp}
        Print( "Starting Wing Form Factor Data Creation. \n" );
        WriteWingFFCSVFile( "WingFormFactorData.csv" );
        \endcode
        \param [in] file_name Output CSV file
    */

    void WriteWingFFCSVFile(const std::string &file_name)
    {
        Results *res = ResultsMgr.CreateResults("Wing_Form_Factor");
        char str[256];
        vector<double> wing_ff_vec;
        vector<double> toc_array = linspace(0.0, 0.205, 200);
        vector<double> perc_lam, sweep25, sweep50;
        perc_lam.push_back(0.0);
        sweep25.push_back(30.0 * EIGEN_PI / 180.0);
        sweep50.push_back(30.0 * EIGEN_PI / 180.0);
        ParasiteDragMgr.m_Atmos.SetMach(0.8);
        res->Add(NameValData("T_C", toc_array));
        for (size_t wing_ff_case = 0; wing_ff_case < vsp::FF_W_SCHEMENSKY_SUPERCRITICAL_AF; ++wing_ff_case)
        {
            for (size_t j = 0; j < toc_array.size(); ++j)
            {
                wing_ff_vec.push_back(ParasiteDragMgr.CalcFFWing(toc_array[j], wing_ff_case, perc_lam[0], sweep25[0], sweep50[0]));
            }
            sprintf(str, "%s", ParasiteDragMgr.AssignFFWingEqnName(wing_ff_case).c_str());
            res->Add(NameValData(str, wing_ff_vec));
            wing_ff_vec.clear();
        }
        res->WriteCSVFile(file_name);
    }

    /*!
        Calculate the coefficient of friction from each Cf equation (i.e. Power Law Blasius) and write the results to a CSV output file
        \code{.cpp}
        Print( "Starting Turbulent Friciton Coefficient Data Creation. \n" );
        WriteCfEqnCSVFile( "FrictionCoefficientData.csv" );
        \endcode
        \param [in] file_name Output CSV file
    */

    void WriteCfEqnCSVFile(const std::string &file_name)
    {
        Results *res = ResultsMgr.CreateResults("Friction_Coefficient");
        char str[256];
        vector<double> lam_cf_vec, turb_cf_vec, ref_leng;
        vector<double> ReyIn_array = logspace(3, 10, 500);
        vector<double> roughness, gamma, taw_tw_ratio, te_tw_ratio;
        roughness.push_back(0.01);
        gamma.push_back(1.4);
        taw_tw_ratio.push_back(1.0);
        te_tw_ratio.push_back(1.0);
        ref_leng.push_back(1.0);

        for (size_t cf_case = 0; cf_case <= vsp::DO_NOT_USE_CF_TURB_WHITE_CHRISTOPH_COMPRESSIBLE; ++cf_case)
        {
            if (!ParasiteDragMgr.IsTurbBlacklisted(cf_case))
            {
                for (size_t j = 0; j < ReyIn_array.size(); ++j)
                {
                    turb_cf_vec.push_back(ParasiteDragMgr.CalcTurbCf(ReyIn_array[j], ref_leng[0], cf_case, roughness[0], gamma[0], taw_tw_ratio[0], te_tw_ratio[0]));
                }
                sprintf(str, "%s", ParasiteDragMgr.AssignTurbCfEqnName(cf_case).c_str());
                res->Add(NameValData(str, turb_cf_vec));
                turb_cf_vec.clear();
            }
        }

        for (size_t cf_case = 0; cf_case < vsp::CF_LAM_BLASIUS_W_HEAT; ++cf_case)
        {
            for (size_t i = 0; i < ReyIn_array.size(); ++i)
            {
                lam_cf_vec.push_back(ParasiteDragMgr.CalcLamCf(ReyIn_array[i], cf_case));
            }
            sprintf(str, "%s", ParasiteDragMgr.AssignLamCfEqnName(cf_case).c_str());
            res->Add(NameValData(str, lam_cf_vec));
            lam_cf_vec.clear();
        }

        res->Add(NameValData("ReyIn", ReyIn_array));
        res->Add(NameValData("Ref_Leng", ref_leng));
        res->WriteCSVFile(file_name);
    }

    /*!
        Calculate the partial coefficient of friction and write the results to a CSV output file
        \code{.cpp}
        Print( "Starting Partial Friction Method Data Creation. \n" );
        WritePartialCfMethodCSVFile( "PartialFrictionMethodData.csv" );
        \endcode
        \param [in] file_name Output CSV file
    */

    void WritePartialCfMethodCSVFile(const std::string &file_name)
    {
        Results *res = ResultsMgr.CreateResults("Friction_Coefficient");
        vector<double> cf_vec, ref_leng;
        vector<double> lam_perc_array = linspace(0, 100, 1000);
        vector<double> ReyIn_array, reql_array;
        ReyIn_array.push_back(1.0e7);
        reql_array.push_back(1.0e7);
        vector<double> roughness, taw_tw_ratio, te_tw_ratio;
        roughness.push_back(0.0);
        taw_tw_ratio.push_back(1.0);
        te_tw_ratio.push_back(1.0);
        ref_leng.push_back(1.0);

        for (size_t i = 0; i < lam_perc_array.size(); ++i)
        {
            cf_vec.push_back(ParasiteDragMgr.CalcPartialTurbulence(lam_perc_array[i], ReyIn_array[0], ref_leng[0], reql_array[0],
                                                                   roughness[0], taw_tw_ratio[0], te_tw_ratio[0]));
        }

        res->Add(NameValData("LamPerc", lam_perc_array));
        res->Add(NameValData("Cf", cf_vec));
        res->Add(NameValData("ReyIn", ReyIn_array));
        res->Add(NameValData("Ref_Leng", ref_leng));
        res->Add(NameValData("Re/L", reql_array));
        res->Add(NameValData("Roughness", roughness));
        res->Add(NameValData("Taw/Tw", taw_tw_ratio));
        res->Add(NameValData("Te/Tw", te_tw_ratio));
        res->WriteCSVFile(file_name);
    }

    //============================================================================//

    /*!
        Calculate the 3D coordinate equivalent for the input surface coordinate point
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double u = 0.12345;
        double w = 0.67890;

        vec3d pnt = CompPnt01( geom_id, surf_indx, u, w );

        Print( "Point: ( " + pnt.x() + ', ' + pnt.y() + ', ' + pnt.z() + ' )' );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] u U (0 - 1) surface coordinate
        \param [in] w W (0 - 1) surface coordinate
        \return Normal vector3D coordinate point
    */

    vec3d CompPnt01(const std::string &geom_id, const int &surf_indx, const double &u, const double &w)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        vec3d ret;
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CompPnt01::Can't Find Geom " + geom_id);
            return ret;
        }

        if (surf_indx < 0 || surf_indx >= geom_ptr->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompPnt01::Invalid Surface Index " + to_string(surf_indx));
            return ret;
        }

        ret = geom_ptr->CompPnt01(surf_indx, clamp(u, 0.0, 1.0), clamp(w, 0.0, 1.0));

        ErrorMgr.NoError();
        return ret;
    }

    /*!
        Calculate the normal vector on the specified surface at input surface coordinate
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double u = 0.12345;
        double w = 0.67890;

        vec3d norm = CompNorm01( geom_id, surf_indx, u, w );

        Print( "Point: ( " + norm.x() + ', ' + norm.y() + ', ' + norm.z() + ' )' );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] u U (0 - 1) surface coordinate
        \param [in] w W (0 - 1) surface coordinate
        \return Normal vector
    */

    vec3d CompNorm01(const std::string &geom_id, const int &surf_indx, const double &u, const double &w)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        vec3d ret;
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CompNorm01::Can't Find Geom " + geom_id);
            return ret;
        }

        if (surf_indx < 0 || surf_indx >= geom_ptr->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompNorm01::Invalid Surface Index " + to_string(surf_indx));
            return ret;
        }

        VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);
        ret = surf->CompNorm01(clamp(u, 0.0, 1.0), clamp(w, 0.0, 1.0));

        ErrorMgr.NoError();
        return ret;
    }

    /*!
        Calculate the vector tangent to the specified surface at input surface coordinate in the U direction
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double u = 0.12345;
        double w = 0.67890;

        vec3d tanu = CompTanU01( geom_id, surf_indx, u, w );

        Print( "Point: ( " + tanu.x() + ', ' + tanu.y() + ', ' + tanu.z() + ' )' );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] u U (0 - 1) surface coordinate
        \param [in] w W (0 - 1) surface coordinate
        \return Tangent vector in U direction
    */

    vec3d CompTanU01(const std::string &geom_id, const int &surf_indx, const double &u, const double &w)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        vec3d ret;
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CompTanU01::Can't Find Geom " + geom_id);
            return ret;
        }

        if (surf_indx < 0 || surf_indx >= geom_ptr->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompTanU01::Invalid Surface Index " + to_string(surf_indx));
            return ret;
        }

        VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);
        ret = surf->CompTanU01(clamp(u, 0.0, 1.0), clamp(w, 0.0, 1.0));

        ErrorMgr.NoError();
        return ret;
    }

    /*!
        Calculate the vector tangent to the specified surface at input surface coordinate in the W direction
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double u = 0.12345;
        double w = 0.67890;

        vec3d tanw = CompTanW01( geom_id, surf_indx, u, w );

        Print( "Point: ( " + tanw.x() + ', ' + tanw.y() + ', ' + tanw.z() + ' )' );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] u U (0 - 1) surface coordinate
        \param [in] w W (0 - 1) surface coordinate
        \return Tangent vector in W direction
    */

    vec3d CompTanW01(const std::string &geom_id, const int &surf_indx, const double &u, const double &w)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        vec3d ret;
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CompTanW01::Can't Find Geom " + geom_id);
            return ret;
        }

        if (surf_indx < 0 || surf_indx >= geom_ptr->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompTanW01::Invalid Surface Index " + to_string(surf_indx));
            return ret;
        }

        VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);
        ret = surf->CompTanW01(clamp(u, 0.0, 1.0), clamp(w, 0.0, 1.0));

        ErrorMgr.NoError();
        return ret;
    }

    /*!
        Determine the curvature of a specified surface at the input surface coordinate point
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double k1, k2, ka, kg;

        double u, w;
        u = 0.25;
        w = 0.75;

        CompCurvature01( geom_id, surf_indx, u, w, k1, k2, ka, kg );

        Print( "Curvature : k1 " + k1 + " k2 " + k2 + " ka " + ka + " kg " + kg );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] u U (0 - 1) surface coordinate
        \param [in] w W (0 - 1) surface coordinate
        \param [out] k1 Output value of maximum principal curvature
        \param [out] k2 Output value of minimum principal curvature
        \param [out] ka Output value of mean curvature
        \param [out] kg Output value of Gaussian curvature
    */

    void CompCurvature01(const std::string &geom_id, const int &surf_indx, const double &u, const double &w, double &k1,
                         double &k2, double &ka, double &kg)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);

        k1 = 0.0;
        k2 = 0.0;
        ka = 0.0;
        kg = 0.0;

        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CompCurvature01::Can't Find Geom " + geom_id);
            return;
        }

        if (surf_indx < 0 || surf_indx >= geom_ptr->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompCurvature01::Invalid Surface Index " + to_string(surf_indx));
            return;
        }

        VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);
        surf->CompCurvature01(clamp(u, 0.0, 1.0), clamp(w, 0.0, 1.0), k1, k2, ka, kg);

        ErrorMgr.NoError();
    }

    /*!
        Determine the nearest surface coordinate for an input 3D coordinate point and calculate the distance between the
        3D point and the closest point of the surface.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double u = 0.12345;
        double w = 0.67890;

        vec3d pnt = CompPnt01( geom_id, surf_indx, u, w );

        vec3d norm = CompNorm01( geom_id, surf_indx, u, w );

        double uout, wout;

        // Offset point from surface
        pnt = pnt + norm;

        double d = ProjPnt01( geom_id, surf_indx, pnt, uout, wout );

        Print( "Dist " + d + " u " + uout + " w " + wout );
        \endcode
        \sa ProjPnt01Guess, ProjPnt01I
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] pt Input 3D coordinate point
        \param [out] u Output closest U (0 - 1) surface coordinate
        \param [out] w Output closest W (0 - 1) surface coordinate
        \return Distance between the 3D point and the closest point of the surface
    */

    double ProjPnt01(const std::string &geom_id, const int &surf_indx, const vec3d &pt, double &u, double &w)
    {
        Vehicle *vPtr = VehicleMgr.GetVehicle();
        Geom *geom = vPtr->FindGeom(geom_id);

        double dmin = (std::numeric_limits<double>::max)();

        if (!geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "ProjPnt01::Can't Find Geom " + geom_id);
            return dmin;
        }

        if (surf_indx < 0 || surf_indx >= geom->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ProjPnt01::Invalid Surface Index " + to_string(surf_indx));
            return dmin;
        }

        dmin = geom->GetSurfPtr(surf_indx)->FindNearest01(u, w, pt);

        ErrorMgr.NoError();

        return dmin;
    }

    /*!
        Determine the nearest surface coordinate and corresponding parent Geom main surface index for an input 3D coordinate point. Return the distance between
        the closest point and the input.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double u = 0.12345;
        double w = 0.67890;

        double d = 0;

        vec3d pnt = CompPnt01( geom_id, surf_indx, u, w );

        vec3d norm = CompNorm01( geom_id, surf_indx, u, w );

        double uout, wout;

        int surf_indx_out;

        // Offset point from surface
        pnt = pnt + norm;

        d = ProjPnt01I( geom_id, pnt, surf_indx_out, uout, wout );

        Print( "Dist " + d + " u " + uout + " w " + wout + " surf_index " + surf_indx_out );
        \endcode
        \sa ProjPnt01, ProjPnt01Guess
        \param [in] geom_id Parent Geom ID
        \param [in] pt Input 3D coordinate point
        \param [out] surf_indx Output main surface index from the parent Geom
        \param [out] u Output closest U (0 - 1) surface coordinat
        \param [out] w Output closest W (0 - 1) surface coordinat
        \return Distance between the 3D point and the closest point of the surface
    */

    double ProjPnt01I(const std::string &geom_id, const vec3d &pt, int &surf_indx,
                      double &u, double &w)
    {
        Vehicle *vPtr = VehicleMgr.GetVehicle();
        Geom *geom = vPtr->FindGeom(geom_id);

        double dmin = (std::numeric_limits<double>::max)();

        if (!geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "ProjPnt01I::Can't Find Geom " + geom_id);
            return dmin;
        }

        dmin = vPtr->ProjPnt01I(geom_id, pt, surf_indx, u, w);

        ErrorMgr.NoError();

        return dmin;
    }

    /*!
        Determine the nearest surface coordinate for an input 3D coordinate point and calculate the distance between the
        3D point and the closest point of the surface. This function takes an input surface coordinate guess for, offering
        a potential decrease in computation time compared to ProjPnt01.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double u = 0.12345;
        double w = 0.67890;

        double d = 0;

        vec3d pnt = CompPnt01( geom_id, surf_indx, u, w );

        vec3d norm = CompNorm01( geom_id, surf_indx, u, w );

        double uout, wout;

        // Offset point from surface
        pnt = pnt + norm;

        d = ProjPnt01Guess( geom_id, surf_indx, pnt, u + 0.1, w + 0.1, uout, wout );

        Print( "Dist " + d + " u " + uout + " w " + wout );
        \endcode
        \sa ProjPnt01, ProjPnt01I
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] pt Input 3D coordinate point
        \param [in] u0 Input U (0 - 1) surface coordinate guess
        \param [in] w0 Input W (0 - 1) surface coordinate guess
        \param [out] u Output closest U (0 - 1) surface coordinate
        \param [out] w Output closest W (0 - 1) surface coordinate
        \return Distance between the 3D point and the closest point of the surface
    */

    double ProjPnt01Guess(const std::string &geom_id, const int &surf_indx, const vec3d &pt, const double &u0, const double &w0, double &u, double &w)
    {
        Vehicle *vPtr = VehicleMgr.GetVehicle();
        Geom *geom = vPtr->FindGeom(geom_id);

        double dmin = (std::numeric_limits<double>::max)();

        if (!geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "ProjPnt01Guess::Can't Find Geom " + geom_id);
            return dmin;
        }

        if (surf_indx < 0 || surf_indx >= geom->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ProjPnt01Guess::Invalid Surface Index " + to_string(surf_indx));
            return dmin;
        }

        dmin = geom->GetSurfPtr(surf_indx)->FindNearest01(u, w, pt, clamp(u0, 0.0, 1.0), clamp(w0, 0.0, 1.0));

        ErrorMgr.NoError();

        return dmin;
    }

    /*!
        Project an input 3D coordinate point onto a surface along a specified axis.  If the axis-aligned ray from the point intersects the surface multiple times, the nearest intersection is returned.  If the axis-aligned ray from the point does not intersect the surface, the original point is returned and -1 is returned in the other output parameters.

        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double u = 0.12345;
        double w = 0.67890;

        vec3d surf_pt = CompPnt01( geom_id, surf_indx, u, w );
        vec3d pt = surf_pt;

        pt.offset_y( -5.0 );

        double u_out, w_out;
        vec3d p_out;

        double idist = AxisProjPnt01( geom_id, surf_indx, Y_DIR, pt, u_out, w_out, p_out);

        Print( "iDist " + idist + " u_out " + u_out + " w_out " + w_out );
        Print( "3D Offset ", false);
        Print( surf_pt - p_out );
        \endcode
        \sa AxisProjPnt01Guess, AxisProjPnt01I, AxisProjVecPnt01, AxisProjVecPnt01Guess
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] iaxis Axis direction to project point along (X_DIR, Y_DIR, or Z_DIR)
        \param [in] pt Input 3D coordinate point
        \param [out] u_out Output closest U (0 - 1) surface coordinate
        \param [out] w_out Output closest W (0 - 1) surface coordinate
        \param [out] p_out Output 3D coordinate point
        \return Axis aligned distance between the 3D point and the projected point on the surface
    */

    double AxisProjPnt01(const std::string &geom_id, const int &surf_indx, const int &iaxis, const vec3d &pt, double &u_out, double &w_out, vec3d &p_out)
    {
        Vehicle *vPtr = VehicleMgr.GetVehicle();
        Geom *geom = vPtr->FindGeom(geom_id);

        double idmin = -1;

        if (!geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "AxisProjPnt01::Can't Find Geom " + geom_id);
            return idmin;
        }

        if (surf_indx < 0 || surf_indx >= geom->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "AxisProjPnt01::Invalid Surface Index " + to_string(surf_indx));
            return idmin;
        }

        idmin = geom->GetSurfPtr(surf_indx)->ProjectPt01(pt, iaxis, u_out, w_out, p_out);

        ErrorMgr.NoError();

        return idmin;
    }

    /*!
        Project an input 3D coordinate point onto a Geom along a specified axis.  The intersecting surface index is also returned.  If the axis-aligned ray from the point intersects the Geom multiple times, the nearest intersection is returned.  If the axis-aligned ray from the point does not intersect the Geom, the original point is returned and -1 is returned in the other output parameters.

        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double u = 0.12345;
        double w = 0.67890;

        vec3d surf_pt = CompPnt01( geom_id, surf_indx, u, w );
        vec3d pt = surf_pt;

        pt.offset_y( -5.0 );

        double u_out, w_out;
        vec3d p_out;
        int surf_indx_out;

        double idist = AxisProjPnt01I( geom_id, Y_DIR, pt, surf_indx_out, u_out, w_out, p_out);

        Print( "iDist " + idist + " u_out " + u_out + " w_out " + w_out + " surf_index " + surf_indx_out );
        Print( "3D Offset ", false);
        Print( surf_pt - p_out );
        \endcode
        \sa AxisProjPnt01, AxisProjPnt01Guess, AxisProjVecPnt01, AxisProjVecPnt01Guess
        \param [in] geom_id Parent Geom ID
        \param [in] iaxis Axis direction to project point along (X_DIR, Y_DIR, or Z_DIR)
        \param [in] pt Input 3D coordinate point
        \param [out] surf_indx_out Output main surface index from the parent Geom
        \param [out] u_out Output closest U (0 - 1) surface coordinate
        \param [out] w_out Output closest W (0 - 1) surface coordinate
        \param [out] p_out Output 3D coordinate point
        \return Axis aligned distance between the 3D point and the projected point on the surface
    */

    double AxisProjPnt01I(const std::string &geom_id, const int &iaxis, const vec3d &pt, int &surf_indx_out, double &u_out, double &w_out, vec3d &p_out)
    {
        Vehicle *vPtr = VehicleMgr.GetVehicle();
        Geom *geom = vPtr->FindGeom(geom_id);

        double idmin = (std::numeric_limits<double>::max)();

        if (!geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "AxisProjPnt01I::Can't Find Geom " + geom_id);
            return idmin;
        }

        idmin = vPtr->AxisProjPnt01I(geom_id, iaxis, pt, surf_indx_out, u_out, w_out, p_out);

        ErrorMgr.NoError();

        return idmin;
    }

    /*!
        Project an input 3D coordinate point onto a surface along a specified axis given an initial guess of surface parameter.  If the axis-aligned ray from the point intersects the surface multiple times, the nearest intersection is returned.  If the axis-aligned ray from the point does not intersect the surface, the original point is returned and -1 is returned in the other output parameters.  The surface parameter guess should allow this call to be faster than calling AxisProjPnt01 without a guess.

        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double u = 0.12345;
        double w = 0.67890;



        vec3d surf_pt = CompPnt01( geom_id, surf_indx, u, w );
        vec3d pt = surf_pt;

        pt.offset_y( -5.0 );

        // Construct initial guesses near actual parameters
        double u0 = u + 0.01234;
        double w0 = w - 0.05678;

        double uout, wout;
        vec3d p_out;

        double d = AxisProjPnt01Guess( geom_id, surf_indx, Y_DIR, pt, u0, w0, uout, wout, p_out);

        Print( "Dist " + d + " u " + uout + " w " + wout );
        \endcode
        \sa AxisProjPnt01, AxisProjPnt01I, AxisProjVecPnt01, AxisProjVecPnt01Guess
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] iaxis Axis direction to project point along (X_DIR, Y_DIR, or Z_DIR)
        \param [in] pt Input 3D coordinate point
        \param [in] u0 Input U (0 - 1) surface coordinate guess
        \param [in] w0 Input W (0 - 1) surface coordinate guess
        \param [out] u_out Output closest U (0 - 1) surface coordinate
        \param [out] w_out Output closest W (0 - 1) surface coordinate
        \param [out] p_out Output 3D coordinate point
        \return Distance between the 3D point and the closest point of the surface
    */

    double AxisProjPnt01Guess(const std::string &geom_id, const int &surf_indx, const int &iaxis, const vec3d &pt, const double &u0, const double &w0, double &u_out, double &w_out, vec3d &p_out)
    {
        Vehicle *vPtr = VehicleMgr.GetVehicle();
        Geom *geom = vPtr->FindGeom(geom_id);

        double idmin = (std::numeric_limits<double>::max)();

        if (!geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "AxisProjPnt01Guess::Can't Find Geom " + geom_id);
            return idmin;
        }

        if (surf_indx < 0 || surf_indx >= geom->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "AxisProjPnt01Guess::Invalid Surface Index " + to_string(surf_indx));
            return idmin;
        }

        idmin = geom->GetSurfPtr(surf_indx)->ProjectPt01(pt, iaxis, clamp(u0, 0.0, 1.0), clamp(w0, 0.0, 1.0), u_out, w_out, p_out);

        ErrorMgr.NoError();

        return idmin;
    }

    /*!
        Test whether a given point is inside a specified surface.

        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double r = 0.12;
        double s = 0.34;
        double t = 0.56;

        vec3d pnt = CompPntRST( geom_id, surf_indx, r, s, t );

        bool res = InsideSurf( geom_id, surf_indx, pt );

        if ( res )
        {
            print( "Inside" );
        }
        else
        {
            print( "Outside" );
        }

        \endcode
        \sa VecInsideSurf
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] pt Input 3D coordinate point
        \return Boolean true if the point is inside the surface, false otherwise.
    */

    bool InsideSurf(const std::string &geom_id, const int &surf_indx, const vec3d &pt)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "InsideSurf::Can't Find Geom " + geom_id);
            return false;
        }

        if (surf_indx < 0 || surf_indx >= geom_ptr->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "InsideSurf::Invalid Surface Index " + to_string(surf_indx));
            return false;
        }

        bool ret = geom_ptr->GetSurfPtr(surf_indx)->IsInside(pt);

        ErrorMgr.NoError();
        return ret;
    }

    /*!
        Calculate the (X, Y, Z) coordinate for the input volume (R, S, T) coordinate point
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double r = 0.12;
        double s = 0.34;
        double t = 0.56;

        vec3d pnt = CompPntRST( geom_id, surf_indx, r, s, t );

        Print( "Point: ( " + pnt.x() + ', ' + pnt.y() + ', ' + pnt.z() + ' )' );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] r R (0 - 1) volume coordinate
        \param [in] s S (0 - 0.5) volume coordinate
        \param [in] t T (0 - 1) volume coordinate
        \return vec3d coordinate point
    */

    vec3d CompPntRST(const std::string &geom_id, const int &surf_indx, const double &r, const double &s, const double &t)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);
        vec3d ret;
        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CompPntRST::Can't Find Geom " + geom_id);
            return ret;
        }

        if (surf_indx < 0 || surf_indx >= geom_ptr->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompPntRST::Invalid Surface Index " + to_string(surf_indx));
            return ret;
        }

        ret = geom_ptr->CompPntRST(surf_indx, clamp(r, 0.0, 1.0), clamp(s, 0.0, 0.5), clamp(t, 0.0, 1.0));

        ErrorMgr.NoError();
        return ret;
    }

    /*!
        Determine the nearest (R, S, T) volume coordinate for an input (X, Y, Z) 3D coordinate point and calculate the distance between the
        3D point and the found volume point.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double r = 0.12;
        double s = 0.34;
        double t = 0.56;

        vec3d pnt = CompPntRST( geom_id, surf_indx, r, s, t );

        double rout, sout, tout;

        double d = FindRST( geom_id, surf_indx, pnt, rout, sout, tout );

        Print( "Dist " + d + " r " + rout + " s " + sout + " t " + tout );
        \endcode
        \sa FindRSTGuess
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] pt Input 3D coordinate point
        \param [out] r Output closest R (0 - 1.0) volume coordinate
        \param [out] s Output closest S (0 - 0.5) volume coordinate
        \param [out] t Output closest T (0 - 1.0) volume coordinate
        \return Distance between the 3D point and the closest point of the volume
    */

    double FindRST(const std::string &geom_id, const int &surf_indx, const vec3d &pt, double &r_out, double &s_out, double &t_out)
    {
        Vehicle *vPtr = VehicleMgr.GetVehicle();
        Geom *geom = vPtr->FindGeom(geom_id);

        double dist = (std::numeric_limits<double>::max)();

        if (!geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "FindRST::Can't Find Geom " + geom_id);
            return dist;
        }

        if (surf_indx < 0 || surf_indx >= geom->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "FindRST::Invalid Surface Index " + to_string(surf_indx));
            return dist;
        }

        dist = geom->GetSurfPtr(surf_indx)->FindRST(pt, r_out, s_out, t_out);

        ErrorMgr.NoError();

        return dist;
    }

    /*!
        Determine the nearest (R, S, T) volume coordinate for an input (X, Y, Z) 3D coordinate point given an initial guess of volume coordinates.  Also calculate the distance between the
        3D point and the found volume point.

        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double r = 0.12;
        double s = 0.34;
        double t = 0.56;

        vec3d pnt = CompPntRST( geom_id, surf_indx, r, s, t );

        double rout, sout, tout;

        double r0 = 0.1;
        double s0 = 0.3;
        double t0 = 0.5;

        double d = FindRSTGuess( geom_id, surf_indx, pnt, r0, s0, t0, rout, sout, tout );

        Print( "Dist " + d + " r " + rout + " s " + sout + " t " + tout );
        \endcode
        \sa FindRST
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] pt Input 3D coordinate point
        \param [in] r0 Input R (0 - 1.0) volume coordinate guess
        \param [in] s0 Input S (0 - 0.5) volume coordinate guess
        \param [in] t0 Input T (0 - 1.0) volume coordinate guess
        \param [out] r Output closest R (0 - 1.0) volume coordinate
        \param [out] s Output closest S (0 - 0.5) volume coordinate
        \param [out] t Output closest T (0 - 1.0) volume coordinate
        \return Distance between the 3D point and the closest point of the volume
    */

    double FindRSTGuess(const std::string &geom_id, const int &surf_indx, const vec3d &pt, const double &r0, const double &s0, const double &t0, double &r_out, double &s_out, double &t_out)
    {
        Vehicle *vPtr = VehicleMgr.GetVehicle();
        Geom *geom = vPtr->FindGeom(geom_id);

        double dist = (std::numeric_limits<double>::max)();

        if (!geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "FindRST::Can't Find Geom " + geom_id);
            return dist;
        }

        if (surf_indx < 0 || surf_indx >= geom->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "FindRST::Invalid Surface Index " + to_string(surf_indx));
            return dist;
        }

        dist = geom->GetSurfPtr(surf_indx)->FindRST(pt, clamp(r0, 0.0, 1.0), clamp(s0, 0.0, 0.5), clamp(t0, 0.0, 1.0), r_out, s_out, t_out);

        ErrorMgr.NoError();

        return dist;
    }

    /*!
        Convert RST volumetric coordinates to LMN coordinates.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double r = 0.12;
        double s = 0.34;
        double t = 0.56;
        double l, m, n;

        ConvertRSTtoLMN( geom_id, surf_indx, r, s, t, l, m, n );

        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] r R (0 - 1) volume coordinate
        \param [in] s S (0 - 0.5) volume coordinate
        \param [in] t T (0 - 1) volume coordinate
        \param [out] l L (0 - 1) linear volume coordinate
        \param [out] m M (0 - 1) linear volume coordinate
        \param [out] n N (0 - 1) linear volume coordinate
        \return void
    */

    void ConvertRSTtoLMN(const std::string &geom_id, const int &surf_indx, const double &r, const double &s, const double &t, double &l, double &m, double &n)
    {
        Vehicle *vPtr = VehicleMgr.GetVehicle();
        Geom *geom = vPtr->FindGeom(geom_id);

        if (!geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "ConvertRSTtoLMN::Can't Find Geom " + geom_id);
            return;
        }

        if (surf_indx < 0 || surf_indx >= geom->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ConvertRSTtoLMN::Invalid Surface Index " + to_string(surf_indx));
            return;
        }

        geom->GetSurfPtr(surf_indx)->ConvertRSTtoLMN(r, s, t, l, m, n);

        ErrorMgr.NoError();

        return;
    }

    /*!
        Convert LMN volumetric coordinates to RST coordinates.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        double l = 0.12;
        double m = 0.34;
        double n = 0.56;
        double r, s, t;

        ConvertLMNtoRST( geom_id, surf_indx, l, m, n, r, s, t );

        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] l L (0 - 1) linear volume coordinate
        \param [in] m M (0 - 1) linear volume coordinate
        \param [in] n N (0 - 1) linear volume coordinate
        \param [out] r R (0 - 1) volume coordinate
        \param [out] s S (0 - 0.5) volume coordinate
        \param [out] t T (0 - 1) volume coordinate
        \return void
    */

    void ConvertLMNtoRST(const std::string &geom_id, const int &surf_indx, const double &l, const double &m, const double &n, double &r, double &s, double &t)
    {
        Vehicle *vPtr = VehicleMgr.GetVehicle();
        Geom *geom = vPtr->FindGeom(geom_id);

        if (!geom)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "ConvertLMNtoRST::Can't Find Geom " + geom_id);
            return;
        }

        if (surf_indx < 0 || surf_indx >= geom->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ConvertLMNtoRST::Invalid Surface Index " + to_string(surf_indx));
            return;
        }

        geom->GetSurfPtr(surf_indx)->ConvertLMNtoRST(l, m, n, r, s, t);

        ErrorMgr.NoError();

        return;
    }

    /*!
        Determine 3D coordinate for each surface coordinate point in the input arrays
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int n = 5;

        array<double> uvec, wvec;

        uvec.resize( n );
        wvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            uvec[i] = (i+1)*1.0/(n+1);

            wvec[i] = (n-i)*1.0/(n+1);
        }

        array< vec3d > ptvec = CompVecPnt01( geom_id, 0, uvec, wvec );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] us Input array of U (0 - 1) surface coordinates
        \param [in] ws Input array of W (0 - 1) surface coordinates
        \return Array of 3D coordinate points
    */

    vector<vec3d> CompVecPnt01(const std::string &geom_id, const int &surf_indx, const vector<double> &us, const vector<double> &ws)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        vector<vec3d> pts;
        pts.resize(0);

        if (geom_ptr)
        {
            if (us.size() == ws.size())
            {
                VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

                if (surf)
                {
                    pts.resize(us.size());

                    for (int i = 0; i < us.size(); i++)
                    {
                        pts[i] = surf->CompPnt01(clamp(us[i], 0.0, 1.0), clamp(ws[i], 0.0, 1.0));
                    }
                }
                else
                {
                    ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompVecPnt01::Invalid surf index " + to_string(surf_indx));
                    return pts;
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompVecPnt01::Input size mismatch.");
                return pts;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CompVecPnt01::Can't Find Geom " + geom_id);
            return pts;
        }
        ErrorMgr.NoError();
        return pts;
    }

    /*!
        Determine the normal vector on a surface for each surface coordinate point in the input arrays
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int n = 5;

        array<double> uvec, wvec;

        uvec.resize( n );
        wvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            uvec[i] = (i+1)*1.0/(n+1);

            wvec[i] = (n-i)*1.0/(n+1);
        }

        array< vec3d > normvec = CompVecNorm01( geom_id, 0, uvec, wvec );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] us Input array of U (0 - 1) surface coordinates
        \param [in] ws Input array of W (0 - 1) surface coordinates
        \return Array of 3D normal vectors
    */

    vector<vec3d> CompVecNorm01(const std::string &geom_id, const int &surf_indx, const vector<double> &us, const vector<double> &ws)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        vector<vec3d> norms;
        norms.resize(0);

        if (geom_ptr)
        {
            if (us.size() == ws.size())
            {
                VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

                if (surf)
                {
                    norms.resize(us.size());

                    for (int i = 0; i < us.size(); i++)
                    {
                        norms[i] = surf->CompNorm01(clamp(us[i], 0.0, 1.0), clamp(ws[i], 0.0, 1.0));
                    }
                }
                else
                {
                    ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompNorm01::Invalid surf index " + to_string(surf_indx));
                    return norms;
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompNorm01::Input size mismatch.");
                return norms;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CompNorm01::Can't Find Geom " + geom_id);
            return norms;
        }
        ErrorMgr.NoError();
        return norms;
    }

    /*!
        Determine the curvature of a specified surface at each surface coordinate point in the input arrays
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int n = 5;

        array<double> uvec, wvec;

        uvec.resize( n );
        wvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            uvec[i] = (i+1)*1.0/(n+1);

            wvec[i] = (n-i)*1.0/(n+1);
        }

        array<double> k1vec, k2vec, kavec, kgvec;

        CompVecCurvature01( geom_id, 0, uvec, wvec, k1vec, k2vec, kavec, kgvec );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] us Input array of U (0 - 1) surface coordinates
        \param [in] ws Input array of W (0 - 1) surface coordinates
        \param [out] k1s Output array of maximum principal curvatures
        \param [out] k2s Output array of minimum principal curvatures
        \param [out] kas Output array of mean curvatures
        \param [out] kgs Output array of Gaussian curvatures
    */

    void CompVecCurvature01(const std::string &geom_id, const int &surf_indx, const vector<double> &us, const vector<double> &ws, vector<double> &k1s, vector<double> &k2s, vector<double> &kas, vector<double> &kgs)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        k1s.resize(0);
        k2s.resize(0);
        kas.resize(0);
        kgs.resize(0);

        if (geom_ptr)
        {
            if (us.size() == ws.size())
            {
                VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

                if (surf)
                {
                    k1s.resize(us.size());
                    k2s.resize(us.size());
                    kas.resize(us.size());
                    kgs.resize(us.size());

                    for (int i = 0; i < us.size(); i++)
                    {
                        surf->CompCurvature01(clamp(us[i], 0.0, 1.0), clamp(ws[i], 0.0, 1.0), k1s[i], k2s[i], kas[i], kgs[i]);
                    }
                }
                else
                {
                    ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompCurvature01::Invalid surf index " + to_string(surf_indx));
                    return;
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompCurvature01::Input size mismatch.");
                return;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CompCurvature01::Can't Find Geom " + geom_id);
            return;
        }
        ErrorMgr.NoError();
    }

    /*!
        Determine the nearest surface coordinates for an input array of 3D coordinate points and calculate the distance between each
        3D point and the closest point of the surface.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int n = 5;

        array<double> uvec, wvec;

        uvec.resize( n );
        wvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            uvec[i] = (i+1)*1.0/(n+1);

            wvec[i] = (n-i)*1.0/(n+1);
        }

        array< vec3d > ptvec = CompVecPnt01( geom_id, 0, uvec, wvec );

        array< vec3d > normvec = CompVecNorm01( geom_id, 0, uvec, wvec );

        for( int i = 0 ; i < n ; i++ )
        {
            ptvec[i] = ptvec[i] + normvec[i];
        }

        array<double> uoutv, woutv, doutv;

        ProjVecPnt01( geom_id, 0, ptvec, uoutv, woutv, doutv );
        \endcode
        \sa ProjVecPnt01Guess
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] pts Input array of 3D coordinate points
        \param [out] us Output array of the closest U (0 - 1) surface coordinate for each 3D input point
        \param [out] ws Output array of the closest W (0 - 1) surface coordinate for each 3D input point
        \param [out] ds Output array of distances for each 3D point and the closest point of the surface
    */

    void ProjVecPnt01(const std::string &geom_id, const int &surf_indx, const vector<vec3d> &pts, vector<double> &us, vector<double> &ws, vector<double> &ds)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        us.resize(0);
        ws.resize(0);
        ds.resize(0);

        if (geom_ptr)
        {
            VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

            if (surf)
            {
                us.resize(pts.size());
                ws.resize(pts.size());
                ds.resize(pts.size());

                for (int i = 0; i < pts.size(); i++)
                {
                    ds[i] = surf->FindNearest01(us[i], ws[i], pts[i]);
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ProjVecPnt01::Invalid surf index " + to_string(surf_indx));
                return;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "ProjVecPnt01::Can't Find Geom " + geom_id);
            return;
        }
        ErrorMgr.NoError();
    }

    /*!
        Determine the nearest surface coordinates for an input array of 3D coordinate points and calculate the distance between each
        3D point and the closest point of the surface. This function takes an input array of surface coordinate guesses for each 3D
        coordinate, offering a potential decrease in computation time compared to ProjVecPnt01.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int n = 5;

        array<double> uvec, wvec;

        uvec.resize( n );
        wvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            uvec[i] = (i+1)*1.0/(n+1);

            wvec[i] = (n-i)*1.0/(n+1);
        }

        array< vec3d > ptvec = CompVecPnt01( geom_id, 0, uvec, wvec );

        array< vec3d > normvec = CompVecNorm01( geom_id, 0, uvec, wvec );

        for( int i = 0 ; i < n ; i++ )
        {
            ptvec[i] = ptvec[i] + normvec[i];
        }

        array<double> uoutv, woutv, doutv, u0v, w0v;

        u0v.resize( n );
        w0v.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            u0v[i] = uvec[i] + 0.01234;

            w0v[i] = wvec[i] - 0.05678;
        }

        ProjVecPnt01Guess( geom_id, 0, ptvec, u0v,  w0v,  uoutv, woutv, doutv );
        \endcode
        \sa ProjVecPnt01,
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] pts Input array of 3D coordinate points
        \param [in] u0s Input array of U (0 - 1) surface coordinate guesses
        \param [in] w0s Input array of W (0 - 1) surface coordinate guesses
        \param [out] us Output array of the closest U (0 - 1) surface coordinate for each 3D input point
        \param [out] ws Output array of the closest W (0 - 1) surface coordinate for each 3D input point
        \param [out] ds Output array of distances for each 3D point and the closest point of the surface
    */

    void ProjVecPnt01Guess(const std::string &geom_id, const int &surf_indx, const vector<vec3d> &pts, const vector<double> &u0s, const vector<double> &w0s, vector<double> &us, vector<double> &ws, vector<double> &ds)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        us.resize(0);
        ws.resize(0);
        ds.resize(0);

        if (geom_ptr)
        {
            if (pts.size() == u0s.size() && pts.size() == w0s.size())
            {
                VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

                if (surf)
                {
                    us.resize(pts.size());
                    ws.resize(pts.size());
                    ds.resize(pts.size());

                    for (int i = 0; i < pts.size(); i++)
                    {
                        ds[i] = surf->FindNearest01(us[i], ws[i], pts[i], clamp(u0s[i], 0.0, 1.0), clamp(w0s[i], 0.0, 1.0));
                    }
                }
                else
                {
                    ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ProjVecPnt01Guess::Invalid surf index " + to_string(surf_indx));
                    return;
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ProjVecPnt01Guess::Input size mismatch.");
                return;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "ProjVecPnt01Guess::Can't Find Geom " + geom_id);
            return;
        }
        ErrorMgr.NoError();
    }

    /*!
        Project an input array of 3D coordinate points onto a surface along a specified axis.  If the axis-aligned ray from the point intersects the surface multiple times, the nearest intersection is returned.  If the axis-aligned ray from the point does not intersect the surface, the original point is returned and -1 is returned in the other output parameters.


        \code{.cpp}
           // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );
        int surf_indx = 0;

        int n = 5;

        array<double> uvec, wvec;

        uvec.resize( n );
        wvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            uvec[i] = (i+1)*1.0/(n+1);

            wvec[i] = (n-i)*1.0/(n+1);
        }

        array< vec3d > ptvec = CompVecPnt01( geom_id, surf_indx, uvec, wvec );

        for( int i = 0 ; i < n ; i++ )
        {
            ptvec[i].offset_y( -5.0 );
        }

        array<double> uoutv, woutv, doutv;
        array< vec3d > poutv;

        AxisProjVecPnt01( geom_id, surf_indx, Y_DIR, ptvec, uoutv, woutv, poutv, doutv );

        // Some of these outputs are expected to be non-zero because the projected point is on the opposite side of
        // the pod from the originally computed point.  I.e. there were multiple solutions and the original point
        // is not the closest intersection point.  We could offset those points in the +Y direction instead of -Y.
        for( int i = 0 ; i < n ; i++ )
        {
            Print( i, false );
            Print( "U delta ", false );
            Print( uvec[i] - uoutv[i], false );
            Print( "W delta ", false );
            Print( wvec[i] - woutv[i] );
        }

        \endcode
        \sa AxisProjPnt01, AxisProjPnt01Guess, AxisProjPnt01I, AxisProjVecPnt01Guess
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] iaxis Axis direction to project point along (X_DIR, Y_DIR, or Z_DIR)
        \param [in] pts Input array of 3D coordinate points
        \param [out] us Output array of the closest U (0 - 1) surface coordinate for each 3D input point
        \param [out] ws Output array of the closest W (0 - 1) surface coordinate for each 3D input point
        \param [out] ps_out Output array of 3D coordinate point
        \param [out] ds Output array of axis distances for each 3D point and the projected point of the surface
    */

    void AxisProjVecPnt01(const std::string &geom_id, const int &surf_indx, const int &iaxis, const std::vector<vec3d> &pts, std::vector<double> &u_out_vec, std::vector<double> &w_out_vec, std::vector<vec3d> &pt_out_vec, std::vector<double> &d_out_vec)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        u_out_vec.resize(0);
        w_out_vec.resize(0);
        pt_out_vec.resize(0);
        d_out_vec.resize(0);

        if (geom_ptr)
        {
            VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

            if (surf)
            {
                u_out_vec.resize(pts.size());
                w_out_vec.resize(pts.size());
                pt_out_vec.resize(pts.size());
                d_out_vec.resize(pts.size());

                for (int i = 0; i < pts.size(); i++)
                {
                    d_out_vec[i] = surf->ProjectPt01(pts[i], iaxis, u_out_vec[i], w_out_vec[i], pt_out_vec[i]);
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "AxisProjVecPnt01::Invalid surf index " + to_string(surf_indx));
                return;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "AxisProjVecPnt01::Can't Find Geom " + geom_id);
            return;
        }
        ErrorMgr.NoError();
    }

    /*!
        Project an input array of 3D coordinate points onto a surface along a specified axis given initial guess arrays of surface parameter.  If the axis-aligned ray from the point intersects the surface multiple times, the nearest intersection is returned.  If the axis-aligned ray from the point does not intersect the surface, the original point is returned and -1 is returned in the other output parameters.  The surface parameter guess should allow this call to be faster than calling AxisProjVecPnt01 without a guess.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );
        int surf_indx = 0;

        int n = 5;

        array<double> uvec, wvec;

        uvec.resize( n );
        wvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            uvec[i] = (i+1)*1.0/(n+1);

            wvec[i] = (n-i)*1.0/(n+1);
        }

        array< vec3d > ptvec = CompVecPnt01( geom_id, surf_indx, uvec, wvec );

        for( int i = 0 ; i < n ; i++ )
        {
            ptvec[i].offset_y( -5.0 );
        }

        array<double> uoutv, woutv, doutv, u0v, w0v;
        array< vec3d > poutv;

        u0v.resize( n );
        w0v.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            u0v[i] = uvec[i] + 0.01234;
            w0v[i] = wvec[i] - 0.05678;
        }

        AxisProjVecPnt01Guess( geom_id, surf_indx, Y_DIR, ptvec, u0v,  w0v,  uoutv, woutv, poutv, doutv );

        for( int i = 0 ; i < n ; i++ )
        {
            Print( i, false );
            Print( "U delta ", false );
            Print( uvec[i] - uoutv[i], false );
            Print( "W delta ", false );
            Print( wvec[i] - woutv[i] );
        }

        \endcode
        \sa AxisProjPnt01, AxisProjPnt01Guess, AxisProjPnt01I, AxisProjVecPnt01
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] iaxis Axis direction to project point along (X_DIR, Y_DIR, or Z_DIR)
        \param [in] pts Input array of 3D coordinate points
        \param [in] u0s Input array of U (0 - 1) surface coordinate guesses
        \param [in] w0s Input array of W (0 - 1) surface coordinate guesses
        \param [out] us Output array of the closest U (0 - 1) surface coordinate for each 3D input point
        \param [out] ws Output array of the closest W (0 - 1) surface coordinate for each 3D input point
        \param [out] ps_out Output array of 3D coordinate point
        \param [out] ds Output array of axis distances for each 3D point and the projected point of the surface
    */

    void AxisProjVecPnt01Guess(const std::string &geom_id, const int &surf_indx, const int &iaxis, const std::vector<vec3d> &pts, const std::vector<double> &u0s, const std::vector<double> &w0s, std::vector<double> &u_out_vec, std::vector<double> &w_out_vec, std::vector<vec3d> &pt_out_vec, std::vector<double> &d_out_vec)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        u_out_vec.resize(0);
        w_out_vec.resize(0);
        pt_out_vec.resize(0);
        d_out_vec.resize(0);

        if (geom_ptr)
        {
            if (pts.size() == u0s.size() && pts.size() == w0s.size())
            {
                VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

                if (surf)
                {
                    u_out_vec.resize(pts.size());
                    w_out_vec.resize(pts.size());
                    pt_out_vec.resize(pts.size());
                    d_out_vec.resize(pts.size());

                    for (int i = 0; i < pts.size(); i++)
                    {
                        d_out_vec[i] = surf->ProjectPt01(pts[i], iaxis, clamp(u0s[i], 0.0, 1.0), clamp(w0s[i], 0.0, 1.0), u_out_vec[i], w_out_vec[i], pt_out_vec[i]);
                    }
                }
                else
                {
                    ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "AxisProjVecPnt01Guess::Invalid surf index " + to_string(surf_indx));
                    return;
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "AxisProjVecPnt01Guess::Input size mismatch.");
                return;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "AxisProjVecPnt01Guess::Can't Find Geom " + geom_id);
            return;
        }
        ErrorMgr.NoError();
    }

    /*!
        Test whether a vector of points are inside a specified surface.

        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        int n = 5;

        array<double> rvec, svec, tvec;

        rvec.resize( n );
        svec.resize( n );
        tvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            rvec[i] = (i+1)*1.0/(n+1);

            svec[i] = (n-i)*0.5/(n+1);

            tvec[i] = (i+1)*1.0/(n+1);
        }

        array< vec3d > ptvec = CompVecPntRST( geom_id, 0, rvec, svec, tvec );

        array<bool> res;
        res = VecInsideSurf( geom_id, surf_indx, ptvec );

        \endcode
        \sa VecInsideSurf
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] pts Input array of 3D coordinate points
        \return Boolean vector for each point.  True if it is inside the surface, false otherwise.
    */

    std::vector<bool> VecInsideSurf(const std::string &geom_id, const int &surf_indx, const std::vector<vec3d> &pts)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        vector<bool> ret;
        ret.resize(0);

        if (geom_ptr)
        {
            VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

            if (surf)
            {
                ret.resize(pts.size(), false);

                for (int i = 0; i < pts.size(); i++)
                {
                    ret[i] = surf->IsInside(pts[i]);
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "VecInsideSurf::Invalid surf index " + to_string(surf_indx));
                return ret;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "VecInsideSurf::Can't Find Geom " + geom_id);
            return ret;
        }
        ErrorMgr.NoError();
        return ret;
    }

    /*!
        Determine 3D coordinate for each volume coordinate point in the input arrays
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int n = 5;

        array<double> rvec, svec, tvec;

        rvec.resize( n );
        svec.resize( n );
        tvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            rvec[i] = (i+1)*1.0/(n+1);

            svec[i] = (n-i)*0.5/(n+1);

            tvec[i] = (i+1)*1.0/(n+1);
        }

        array< vec3d > ptvec = CompVecPntRST( geom_id, 0, rvec, svec, tvec );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] rs Input array of R (0 - 1.0) volume coordinates
        \param [in] ss Input array of S (0 - 0.5) volume coordinates
        \param [in] ts Input array of T (0 - 1.0) volume coordinates
        \return Array of 3D coordinate points
    */

    std::vector<vec3d> CompVecPntRST(const std::string &geom_id, const int &surf_indx, const std::vector<double> &rs, const std::vector<double> &ss, const std::vector<double> &ts)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        vector<vec3d> pts;
        pts.resize(0);

        if (geom_ptr)
        {
            if (rs.size() == ss.size() && rs.size() == ts.size())
            {
                VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

                if (surf)
                {
                    pts.resize(rs.size());

                    for (int i = 0; i < rs.size(); i++)
                    {
                        pts[i] = surf->CompPntRST(clamp(rs[i], 0.0, 1.0), clamp(ss[i], 0.0, 0.5), clamp(ts[i], 0.0, 1.0));
                    }
                }
                else
                {
                    ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompVecPntRST::Invalid surf index " + to_string(surf_indx));
                    return pts;
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "CompVecPntRST::Input size mismatch.");
                return pts;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "CompVecPntRST::Can't Find Geom " + geom_id);
            return pts;
        }
        ErrorMgr.NoError();
        return pts;
    }

    /*!
        Determine the nearest volume coordinates for an input array of 3D coordinate points and calculate the distance between each
        3D point and the found point in the volume.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int n = 5;

        array<double> rvec, svec, tvec;

        rvec.resize( n );
        svec.resize( n );
        tvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            rvec[i] = (i+1)*1.0/(n+1);

            svec[i] = (n-i)*0.5/(n+1);

            tvec[i] = (i+1)*1.0/(n+1);
        }

        array< vec3d > ptvec = CompVecPntRST( geom_id, 0, rvec, svec, tvec );

        array<double> routv, soutv, toutv, doutv;

        FindRSTVec( geom_id, 0, ptvec, routv, soutv, toutv, doutv );
        \endcode
        \sa FindRSTVecGuess
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] pts Input array of 3D coordinate points
        \param [out] rs Output array of the closest R (0 - 1.0) volume coordinate for each 3D input point
        \param [out] ss Output array of the closest S (0 - 0.5) volume coordinate for each 3D input point
        \param [out] ts Output array of the closest T (0 - 1.0) volume coordinate for each 3D input point
        \param [out] ds Output array of distances for each 3D point and the closest point of the volume
    */

    void FindRSTVec(const std::string &geom_id, const int &surf_indx, const std::vector<vec3d> &pts, std::vector<double> &rs, std::vector<double> &ss, std::vector<double> &ts, std::vector<double> &ds)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        rs.resize(0);
        ss.resize(0);
        ts.resize(0);
        ds.resize(0);

        if (geom_ptr)
        {
            VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

            if (surf)
            {
                rs.resize(pts.size());
                ss.resize(pts.size());
                ts.resize(pts.size());
                ds.resize(pts.size());

                surf->FindRST(pts, rs, ss, ts, ds);
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "FindRSTVec::Invalid surf index " + to_string(surf_indx));
                return;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "FindRSTVec::Can't Find Geom " + geom_id);
            return;
        }
        ErrorMgr.NoError();
    }

    /*!
        Determine the nearest volume coordinates for an input array of 3D coordinate points and calculate the distance between each
        3D point and the closest point of the volume. This function takes an input array of volume coordinate guesses for each 3D
        coordinate, offering a potential decrease in computation time compared to FindRSTVec.
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int n = 5;

        array<double> rvec, svec, tvec;

        rvec.resize( n );
        svec.resize( n );
        tvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            rvec[i] = (i+1)*1.0/(n+1);

            svec[i] = (n-i)*0.5/(n+1);

            tvec[i] = (i+1)*1.0/(n+1);
        }

        array< vec3d > ptvec = CompVecPntRST( geom_id, 0, rvec, svec, tvec );

        array<double> routv, soutv, toutv, doutv;

        for( int i = 0 ; i < n ; i++ )
        {
            ptvec[i] = ptvec[i] * 0.9;
        }

        FindRSTVecGuess( geom_id, 0, ptvec, rvec, svec, tvec, routv, soutv, toutv, doutv );
        \endcode
        \sa FindRSTVec,
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] pts Input array of 3D coordinate points
        \param [in] r0s Input array of U (0 - 1.0) volume coordinate guesses
        \param [in] s0s Input array of S (0 - 0.5) volume coordinate guesses
        \param [in] t0s Input array of T (0 - 1.0) volume coordinate guesses
        \param [out] rs Output array of the closest R (0 - 1.0) volume coordinate for each 3D input point
        \param [out] ss Output array of the closest S (0 - 0.5) volume coordinate for each 3D input point
        \param [out] ts Output array of the closest T (0 - 1.0) volume coordinate for each 3D input point
        \param [out] ds Output array of distances for each 3D point and the closest point of the volume
    */

    void FindRSTVecGuess(const std::string &geom_id, const int &surf_indx, const std::vector<vec3d> &pts, const std::vector<double> &r0s, const std::vector<double> &s0s, const std::vector<double> &t0s, std::vector<double> &rs, std::vector<double> &ss, std::vector<double> &ts, std::vector<double> &ds)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        rs.resize(0);
        ss.resize(0);
        ts.resize(0);
        ds.resize(0);

        if (geom_ptr)
        {
            if (pts.size() == r0s.size() && pts.size() == s0s.size() && pts.size() == t0s.size())
            {
                VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

                if (surf)
                {
                    rs.resize(pts.size());
                    ss.resize(pts.size());
                    ts.resize(pts.size());
                    ds.resize(pts.size());

                    for (int i = 0; i < pts.size(); i++)
                    {
                        ds[i] = surf->FindRST(pts[i], clamp(r0s[i], 0.0, 1.0), clamp(s0s[i], 0.0, 0.5), clamp(t0s[i], 0.0, 1.0), rs[i], ss[i], ts[i]);
                    }
                }
                else
                {
                    ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "FindRSTVecGuess::Invalid surf index " + to_string(surf_indx));
                    return;
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "FindRSTVecGuess::Input size mismatch.");
                return;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "FindRSTVecGuess::Can't Find Geom " + geom_id);
            return;
        }
        ErrorMgr.NoError();
    }

    /*!
        Convert vector of RST volumetric coordinates to LMN coordinates.

        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int n = 5;

        array<double> rvec, svec, tvec;

        rvec.resize( n );
        svec.resize( n );
        tvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            rvec[i] = (i+1)*1.0/(n+1);
            svec[i] = 0.5 * (n-i)*1.0/(n+1);
            tvec[i] = (i+1)*1.0/(n+1);
        }

        array<double> lvec, mvec, nvec;

        ConvertRSTtoLMNVec( geom_id, 0, rvec, svec, tvec, lvec, mvec, nvec );

        \endcode
        \sa ConvertLMNtoRSTVec, ConvertRSTtoLMN, ConvertLMNtoRST
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] rs Input array of R (0 - 1) volumetric coordinate
        \param [in] ss Input array of S (0 - 0.5) volumetric coordinate
        \param [in] ts Input array of T (0 - 1) volumetric coordinate
        \param [out] ls Output array of L (0 - 1) linear volumetric coordinate
        \param [out] ms Output array of M (0 - 1) linear volumetric coordinate
        \param [out] ns Output array of N (0 - 1) linear volumetric coordinate
    */

    void ConvertRSTtoLMNVec(const std::string &geom_id, const int &surf_indx,
                            const std::vector<double> &r_vec, const std::vector<double> &s_vec, const std::vector<double> &t_vec,
                            std::vector<double> &l_out_vec, std::vector<double> &m_out_vec, std::vector<double> &n_out_vec)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        l_out_vec.resize(0);
        m_out_vec.resize(0);
        n_out_vec.resize(0);

        if (geom_ptr)
        {
            if (r_vec.size() == s_vec.size() && r_vec.size() == t_vec.size())
            {
                VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

                if (surf)
                {
                    l_out_vec.resize(r_vec.size());
                    m_out_vec.resize(r_vec.size());
                    n_out_vec.resize(r_vec.size());

                    for (int i = 0; i < r_vec.size(); i++)
                    {
                        surf->ConvertRSTtoLMN(r_vec[i], s_vec[i], t_vec[i], l_out_vec[i], m_out_vec[i], n_out_vec[i]);
                    }
                }
                else
                {
                    ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ConvertRSTtoLMN::Invalid surf index " + to_string(surf_indx));
                    return;
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ConvertRSTtoLMN::Input size mismatch.");
                return;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "ConvertRSTtoLMN::Can't Find Geom " + geom_id);
            return;
        }
        ErrorMgr.NoError();
    }

    /*!
        Convert vector of LMN volumetric coordinates to RST coordinates.

        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int n = 5;

        array<double> lvec, mvec, nvec;

        lvec.resize( n );
        mvec.resize( n );
        nvec.resize( n );

        for( int i = 0 ; i < n ; i++ )
        {
            lvec[i] = (i+1)*1.0/(n+1);
            mvec[i] = (n-i)*1.0/(n+1);
            nvec[i] = (i+1)*1.0/(n+1);
        }

        array<double> rvec, svec, tvec;

        ConvertLMNtoRSTVec( geom_id, 0, lvec, mvec, nvec, rvec, svec, tvec );

        \endcode
        \sa ConvertRSTtoLMNVec, ConvertRSTtoLMN, ConvertLMNtoRST
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [in] rs Input array of R (0 - 1) volumetric coordinate
        \param [in] ss Input array of S (0 - 0.5) volumetric coordinate
        \param [in] ts Input array of T (0 - 1) volumetric coordinate
        \param [out] ls Output array of L (0 - 1) linear volumetric coordinate
        \param [out] ms Output array of M (0 - 1) linear volumetric coordinate
        \param [out] ns Output array of N (0 - 1) linear volumetric coordinate
    */

    void ConvertLMNtoRSTVec(const std::string &geom_id, const int &surf_indx,
                            const std::vector<double> &l_vec, const std::vector<double> &m_vec, const std::vector<double> &n_vec,
                            std::vector<double> &r_out_vec, std::vector<double> &s_out_vec, std::vector<double> &t_out_vec)
    {
        Vehicle *veh = GetVehicle();

        Geom *geom_ptr = veh->FindGeom(geom_id);

        r_out_vec.resize(0);
        s_out_vec.resize(0);
        t_out_vec.resize(0);

        if (geom_ptr)
        {
            if (l_vec.size() == m_vec.size() && l_vec.size() == n_vec.size())
            {
                VspSurf *surf = geom_ptr->GetSurfPtr(surf_indx);

                if (surf)
                {
                    r_out_vec.resize(l_vec.size());
                    s_out_vec.resize(l_vec.size());
                    t_out_vec.resize(l_vec.size());

                    for (int i = 0; i < l_vec.size(); i++)
                    {
                        surf->ConvertRSTtoLMN(l_vec[i], m_vec[i], n_vec[i], r_out_vec[i], s_out_vec[i], t_out_vec[i]);
                    }
                }
                else
                {
                    ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ConvertLMNtoRST::Invalid surf index " + to_string(surf_indx));
                    return;
                }
            }
            else
            {
                ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "ConvertLMNtoRST::Input size mismatch.");
                return;
            }
        }
        else
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "ConvertLMNtoRST::Can't Find Geom " + geom_id);
            return;
        }
        ErrorMgr.NoError();
    }

    /*!
        Get the surface coordinate point of each intersection of the tessellated wireframe for a particular surface
        \code{.cpp}
        // Add Pod Geom
        string geom_id = AddGeom( "POD", "" );

        int surf_indx = 0;

        array<double> utess, wtess;

        GetUWTess01( geom_id, surf_indx, utess, wtess );
        \endcode
        \param [in] geom_id Parent Geom ID
        \param [in] surf_indx Main surface index from the parent Geom
        \param [out] us Output array of U (0 - 1) surface coordinates
        \param [out] ws Output array of W (0 - 1) surface coordinates
    */

    void GetUWTess01(const std::string &geom_id, const int &surf_indx, std::vector<double> &u_out_vec, std::vector<double> &w_out_vec)
    {
        Vehicle *veh = GetVehicle();
        Geom *geom_ptr = veh->FindGeom(geom_id);

        if (!geom_ptr)
        {
            ErrorMgr.AddError(VSP_INVALID_GEOM_ID, "GetUWTess01::Can't Find Geom " + geom_id);
            return;
        }

        if (surf_indx < 0 || surf_indx >= geom_ptr->GetNumTotalSurfs())
        {
            ErrorMgr.AddError(VSP_INDEX_OUT_RANGE, "GetUWTess01::Invalid Surface Index " + to_string(surf_indx));
            return;
        }

        geom_ptr->GetUWTess01(surf_indx, u_out_vec, w_out_vec);

        ErrorMgr.NoError();
        return;
    }

    /*!
        Create a new Ruler and add it to the Measure Tool
        \code{.cpp}
        string pid1 = AddGeom( "POD", "" );

        SetParmVal( pid1, "Y_Rel_Location", "XForm", 2.0 );

        string pid2 = AddGeom( "POD", "" );

        SetParmVal( pid2, "Z_Rel_Location", "XForm", 4.0 );

        string rid = AddRuler( pid1, 1, 0.2, 0.3, pid2, 0, 0.2, 0.3, "Ruler 1" );

        SetParmVal( FindParm( rid, "X_Offset", "Measure" ), 6.0 );
        \endcode
        \param [in] startgeomid Start parent Geom ID
        \param [in] startsurfindx Main surface index from the staring parent Geom
        \param [in] startu Surface u (0 - 1) start coordinate
        \param [in] startw Surface w (0 - 1) start coordinate
        \param [in] endgeomid End parent Geom ID
        \param [in] endsurfindx Main surface index on the end parent Geom
        \param [in] endu Surface u (0 - 1) end coordinate
        \param [in] endw Surface w (0 - 1) end coordinate
        \param [in] name Ruler name
        \return Ruler ID
    */

    string AddRuler(const string &startgeomid, int startsurfindx, double startu, double startw,
                    const string &endgeomid, int endsurfindx, double endu, double endw, const string &name)
    {
        return MeasureMgr.CreateAndAddRuler(startgeomid, startsurfindx, startu, startw,
                                            endgeomid, endsurfindx, endu, endw, name);
    }

    /*!
        Get an array of all Ruler IDs from the Measure Tool
        \code{.cpp}
        string pid1 = AddGeom( "POD", "" );

        SetParmVal( pid1, "Y_Rel_Location", "XForm", 2.0 );

        string pid2 = AddGeom( "POD", "" );

        SetParmVal( pid2, "Z_Rel_Location", "XForm", 4.0 );

        string rid1 = AddRuler( pid1, 1, 0.2, 0.3, pid2, 0, 0.2, 0.3, "Ruler 1" );

        string rid2 = AddRuler( pid1, 0, 0.4, 0.6, pid1, 1, 0.8, 0.9, "Ruler 2" );

        array< string > @ruler_array = GetAllRulers();

        Print("Two Rulers");

        for( int n = 0 ; n < int( ruler_array.length() ) ; n++ )
        {
            Print( ruler_array[n] );
        }
        \endcode
        \return Array of Ruler IDs
    */

    vector<string> GetAllRulers()
    {
        return MeasureMgr.GetAllRulers();
    }

    /*!
        Delete a particular Ruler from the Meaure Tool
        \code{.cpp}
        string pid1 = AddGeom( "POD", "" );

        SetParmVal( pid1, "Y_Rel_Location", "XForm", 2.0 );

        string pid2 = AddGeom( "POD", "" );

        SetParmVal( pid2, "Z_Rel_Location", "XForm", 4.0 );

        string rid1 = AddRuler( pid1, 1, 0.2, 0.3, pid2, 0, 0.2, 0.3, "Ruler 1" );

        string rid2 = AddRuler( pid1, 0, 0.4, 0.6, pid1, 1, 0.8, 0.9, "Ruler 2" );

        array< string > @ruler_array = GetAllRulers();

        DelRuler( ruler_array[0] );
        \endcode
        \param [in] id Ruler ID
    */

    void DelRuler(const string &id)
    {
        MeasureMgr.DelRuler(id);
    }

    /*!
        Delete all Rulers from the Meaure Tool
        \code{.cpp}
        string pid1 = AddGeom( "POD", "" );

        SetParmVal( pid1, "Y_Rel_Location", "XForm", 2.0 );

        string pid2 = AddGeom( "POD", "" );

        SetParmVal( pid2, "Z_Rel_Location", "XForm", 4.0 );

        string rid1 = AddRuler( pid1, 1, 0.2, 0.3, pid2, 0, 0.2, 0.3, "Ruler 1" );

        string rid2 = AddRuler( pid1, 0, 0.4, 0.6, pid1, 1, 0.8, 0.9, "Ruler 2" );

        DeleteAllRulers();
        \endcode
    */

    void DeleteAllRulers()
    {
        MeasureMgr.DelAllRulers();
    }

    /*!
        Create a new Probe and add it to the Measure Tool
        \code{.cpp}
        string pid1 = AddGeom( "POD", "" );

        SetParmVal( pid1, "Y_Rel_Location", "XForm", 2.0 );

        string probe_id = AddProbe( pid1, 0, 0.5, 0.8, "Probe 1" );

        SetParmVal( FindParm( probe_id, "Len", "Measure" ), 3.0 );
        \endcode
        \param [in] geomid Parent Geom ID
        \param [in] surfindx Main surface index from the parent Geom
        \param [in] u Surface u (0 - 1) coordinate
        \param [in] w Surface w (0 - 1) coordinate
        \param [in] name Probe name
        \return Probe ID
    */

    string AddProbe(const string &geomid, int surfindx, double u, double w, const string &name)
    {
        return MeasureMgr.CreateAndAddProbe(geomid, surfindx, u, w, name);
    }

    /*!
        Get an array of all Probe IDs from the Measure Tool
        \code{.cpp}
        string pid1 = AddGeom( "POD", "" );

        SetParmVal( pid1, "Y_Rel_Location", "XForm", 2.0 );

        string probe_id = AddProbe( pid1, 0, 0.5, 0.8, "Probe 1" );

        array< string > @probe_array = GetAllProbes();

        Print( "One Probe: ", false );

        Print( probe_array[0] );
        \endcode
        \return [in] Array of Probe IDs
    */

    vector<string> GetAllProbes()
    {
        return MeasureMgr.GetAllProbes();
    }

    /*!
        Delete a specific Probe from the Measure Tool
        \code{.cpp}
        string pid1 = AddGeom( "POD", "" );

        SetParmVal( pid1, "Y_Rel_Location", "XForm", 2.0 );

        string probe_id_1 = AddProbe( pid1, 0, 0.5, 0.8, "Probe 1" );
        string probe_id_2 = AddProbe( pid1, 0, 0.2, 0.3, "Probe 2" );

        DelProbe( probe_id_1 );

        array< string > @probe_array = GetAllProbes();

        if ( probe_array.size() != 1 ) { Print( "Error: DelProbe" ); }
        \endcode
        \param [in] id Probe ID
    */

    void DelProbe(const string &id)
    {
        MeasureMgr.DelProbe(id);
    }

    /*!
        Delete all Probes from the Measure Tool
        \code{.cpp}
        string pid1 = AddGeom( "POD", "" );

        SetParmVal( pid1, "Y_Rel_Location", "XForm", 2.0 );

        string probe_id_1 = AddProbe( pid1, 0, 0.5, 0.8, "Probe 1" );
        string probe_id_2 = AddProbe( pid1, 0, 0.2, 0.3, "Probe 2" );

        DeleteAllProbes();

        array< string > @probe_array = GetAllProbes();

        if ( probe_array.size() != 0 ) { Print( "Error: DeleteAllProbes" ); }
        \endcode
    */

    void DeleteAllProbes()
    {
        MeasureMgr.DelAllProbes();
    }

    /*!
        Get the version of the OpenVSP instance currently running
        \code{.cpp}
        Print( "The current OpenVSP version is: ", false );

        Print( GetVSPVersion() );
        \endcode
        \return OpenVSP version string (i.e. "OpenVSP 3.17.1")
    */

    string GetVSPVersion()
    {
        return VSPVERSION4;
    }

    /*!
        Get the path to the OpenVSP executable. OpenVSP will assume that the VSPAERO, VSPSLICER, and VSPVIEWER are in the same directory unless
        instructed otherwise.
        \code{.cpp}
        Print( "The current VSP executable path is: ", false );

        Print( GetVSPExePath() );
        \endcode
        \sa SetVSPAEROPath, CheckForVSPAERO, GetVSPAEROPath
        \return Path to the OpenVSP executable
    */

    string GetVSPExePath()
    {
        Vehicle *veh = VehicleMgr.GetVehicle();
        if (veh)
        {
            return veh->GetExePath();
        }
        return string();
    }

    /*!
        Set the path to the VSPAERO executables (Solver, Viewer, and Slicer). By default, OpenVSP will assume that the VSPAERO executables are in the
        same directory as the VSP executable. However, this may need to be changed when using certain API languages like MATLAB and Python. For example,
        Python may treat the location of the Python executable as the VSP executable path, so either the VSPAERO executable needs to be moved to the same
        directory or this function can be called to tell Python where to look for VSPAERO.
        \code{.cpp}
        if ( !CheckForVSPAERO( GetVSPExePath() ) )
        {
            string vspaero_path = "C:/Users/example_user/Documents/OpenVSP_3.4.5";
            SetVSPAEROPath( vspaero_path );
        }
        \endcode
        \sa GetVSPExePath, CheckForVSPAERO, GetVSPAEROPath
        \param [in] path Absolute path to directory containing VSPAERO executable
        \return Flag that indicates whether or not the path was set correctly
    */

    bool SetVSPAEROPath(const std::string &path)
    {
        Vehicle *veh = VehicleMgr.GetVehicle();
        if (veh)
        {
            return veh->SetVSPAEROPath(path);
        }
        return false;
    }

    /*!
        Get the path that OpenVSP will use to look for all VSPAERO executables (Solver, Slicer, and Viewer) when attempting to execute
        VSPAERO. If the VSPAERO executables are not in this location, they must either be copied there or the VSPAERO path must be set
        using SetVSPAEROPath.
        \code{.cpp}
        if ( !CheckForVSPAERO( GetVSPAEROPath() ) )
        {
            Print( "VSPAERO is not where OpenVSP thinks it is. I should move the VSPAERO executable or call SetVSPAEROPath." );
        }
        \endcode
        \sa GetVSPExePath, CheckForVSPAERO, SetVSPAEROPath
        \return Path OpenVSP will look for VSPAERO
    */

    std::string GetVSPAEROPath()
    {
        Vehicle *veh = VehicleMgr.GetVehicle();
        if (veh)
        {
            return veh->GetVSPAEROPath();
        }
        return string();
    }

    /*!
        Check if all VSPAERO executables (Solver, Viewer, and Slicer) are in a given directory. Note that this function will return false
        if only one or two VSPAERO executables are found. An error message will indicate the executables that are missing. This may be
        acceptable, as only the Solver is needed in all cases. The Viewer and Slicer may not be needed.
        \code{.cpp}
        string vspaero_path = "C:/Users/example_user/Documents/OpenVSP_3.4.5";

        if ( CheckForVSPAERO( vspaero_path ) )
        {
            SetVSPAEROPath( vspaero_path );
        }
        \endcode
        \sa GetVSPExePath, GetVSPAEROPath, SetVSPAEROPath
        \param [in] path Absolute path to check for VSPAERO executables
        \return Flag that indicates if all VSPAERO executables are found or not
    */

    bool CheckForVSPAERO(const std::string &path)
    {
        Vehicle *veh = VehicleMgr.GetVehicle();
        if (veh)
        {
            return veh->CheckForVSPAERO(path);
        }
        return false;
    }

} // vsp namespace