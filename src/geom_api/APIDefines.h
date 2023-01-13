//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// API.h: interface for the Vehicle Class and Vehicle Mgr Singleton.
// J.R Gloudemans
//
//////////////////////////////////////////////////////////////////////

#if !defined(VSPDEFINES__INCLUDED_)
#define VSPDEFINES__INCLUDED_

namespace vsp
{
  /*! Enum that indicates if positions are relative or absolute. */
  enum ABS_REL_FLAG
  {
    ABS, /*!< Absolute position */
    REL  /*!< Relative position */
  };

  /*! Enum used to identify the format of exported airfoil files. */
  enum AIRFOIL_EXPORT_TYPE
  {
    SELIG_AF_EXPORT, /*!< Selig airfoil file format */
    BEZIER_AF_EXPORT /*!< Bezier airfoil file format */
  };

  /*! Enum for specifying angular units. */
  enum ANG_UNITS
  {
    ANG_RAD, /*!< Radians */
    ANG_DEG  /*!< Degrees */
  };

  /*! Enum that identifies the atmospheric model used in the Parasite Drag tool. */
  enum ATMOS_TYPE
  {
    ATMOS_TYPE_US_STANDARD_1976 = 0, /*!< US Standard Atmosphere 1976 (default) */
    ATMOS_TYPE_HERRINGTON_1966,      /*!< USAF 1966 */
    ATMOS_TYPE_MANUAL_P_R,           /*!< Manual: pressure and density control */
    ATMOS_TYPE_MANUAL_P_T,           /*!< Manual: pressure and temperature control */
    ATMOS_TYPE_MANUAL_R_T,           /*!< Manual: density and temperature control */
    ATMOS_TYPE_MANUAL_RE_L           /*!< Manual: Reynolds number and length control */
  };

  /*! Enum that identifies the parent to child relative translation coordinate system. */
  enum ATTACH_TRANS_TYPE
  {
    ATTACH_TRANS_NONE = 0, /*!< No parent attachment for translations */
    ATTACH_TRANS_COMP,     /*!< Translation relative to parent body axes */
    ATTACH_TRANS_UV        /*!< Translation relative to parent surface coordinate frame */
  };

  /*! Enum that determines parent to child relative rotation axes. */
  enum ATTACH_ROT_TYPE
  {
    ATTACH_ROT_NONE = 0, /*!< No parent attachment for rotations */
    ATTACH_ROT_COMP,     /*!< Rotation relative to parent body axes */
    ATTACH_ROT_UV        /*!< Rotation relative to parent surface coordinate frame */
  };

  /*! Enum for Body of Revolution mode control. */
  enum BOR_MODE
  {
    BOR_FLOWTHROUGH, /*!< Flowthrough mode (default) */
    BOR_UPPER,       /*!< Upper surface mode */
    BOR_LOWER,       /*!< Lower surface mode */
    BOR_NUM_MODES    /*!< Number of Body of Revolution modes */
  };

  /*! Enum used to select between maximum camber or ideal lift coefficient inputs. */
  enum CAMBER_INPUT_FLAG
  {
    MAX_CAMB, /*!< Input maximum camber, calculate ideal lift coefficient */
    DESIGN_CL /*!< Input ideal lift coefficient, calculate maximum camber */
  };

  /*! Enum that identifies the end cap types for a geometry (i.e. wing root and tip). */
  enum CAP_TYPE
  {
    NO_END_CAP,         /*!< No end cap */
    FLAT_END_CAP,       /*!< Flat end cap */
    ROUND_END_CAP,      /*!< Round end cap */
    EDGE_END_CAP,       /*!< Edge end cap */
    SHARP_END_CAP,      /*!< Sharp end cap */
    POINT_END_CAP,      /*!< Point end cap */
    NUM_END_CAP_OPTIONS /*!< Number of end cap options */
  };

  /*! Enum used to identify each CFD mesh control option (also applicable to FEA Mesh). \\sa SetCFDMeshVal(), SetFEAMeshVal() */
  enum CFD_CONTROL_TYPE
  {
    CFD_MIN_EDGE_LEN,              /*!< Minimum mesh edge length */
    CFD_MAX_EDGE_LEN,              /*!< Maximum mesh edge length */
    CFD_MAX_GAP,                   /*!< Maximum mesh edge gap */
    CFD_NUM_CIRCLE_SEGS,           /*!< Number of edge segments to resolve circle */
    CFD_GROWTH_RATIO,              /*!< Maximum allowed edge growth ratio */
    CFD_LIMIT_GROWTH_FLAG,         /*!< Rigorous 3D growth limiting flag */
    CFD_INTERSECT_SUBSURFACE_FLAG, /*!< Flag to intersect sub-surfaces */
    CFD_HALF_MESH_FLAG,            /*!< Flag to generate a half mesh */
    CFD_FAR_FIELD_FLAG,            /*!< Flag to generate a far field mesh */
    CFD_FAR_MAX_EDGE_LEN,          /*!< Maximum far field mesh edge length */
    CFD_FAR_MAX_GAP,               /*!< Maximum far field mesh edge gap */
    CFD_FAR_NUM_CIRCLE_SEGS,       /*!< Number of far field edge segments to resolve circle */
    CFD_FAR_SIZE_ABS_FLAG,         /*!< Relative or absolute size flag */
    CFD_FAR_LENGTH,                /*!< Far field length */
    CFD_FAR_WIDTH,                 /*!< Far field width */
    CFD_FAR_HEIGHT,                /*!< Far field height */
    CFD_FAR_X_SCALE,               /*!<  Far field X scale */
    CFD_FAR_Y_SCALE,               /*!<  Far field Y scale */
    CFD_FAR_Z_SCALE,               /*!<  Far field Z scale */
    CFD_FAR_LOC_MAN_FLAG,          /*!< Far field location flag: centered or manual */
    CFD_FAR_LOC_X,                 /*!< Far field X location */
    CFD_FAR_LOC_Y,                 /*!< Far field Y location */
    CFD_FAR_LOC_Z,                 /*!< Far field Z location */
    CFD_SRF_XYZ_FLAG,              /*!< Flag to include X,Y,Z intersection curves in export files */
  };

  /*! Enum used to describe various CFD Mesh export file options. \\sa SetComputationFileName(), ComputeCFDMesh() */
  enum CFD_MESH_EXPORT_TYPE
  {
    CFD_STL_FILE_NAME,     /*!< STL export type */
    CFD_POLY_FILE_NAME,    /*!< POLY export type */
    CFD_TRI_FILE_NAME,     /*!< TRI export type */
    CFD_OBJ_FILE_NAME,     /*!< OBJ export type */
    CFD_DAT_FILE_NAME,     /*!< DAT export type */
    CFD_KEY_FILE_NAME,     /*!< KEY export type */
    CFD_GMSH_FILE_NAME,    /*!< GMSH export type */
    CFD_SRF_FILE_NAME,     /*!< SRF export type */
    CFD_TKEY_FILE_NAME,    /*!< TKEY export type */
    CFD_FACET_FILE_NAME,   /*!< FACET export type */
    CFD_CURV_FILE_NAME,    /*!< CURV export type */
    CFD_PLOT3D_FILE_NAME,  /*!< PLOT3D export type */
    CFD_VSPGEOM_FILE_NAME, /*!< VSPGEOM export type */
    CFD_NUM_FILE_NAMES,    /*!< Number of CFD Mesh export file types */
  };

  /*! Enum that indicates the CFD Mesh source type. \\sa AddCFDSource() */
  enum CFD_MESH_SOURCE_TYPE
  {
    POINT_SOURCE,     /*!< Point source */
    LINE_SOURCE,      /*!< Line source */
    BOX_SOURCE,       /*!< Box source */
    ULINE_SOURCE,     /*!< Constant U Line source */
    WLINE_SOURCE,     /*!< Constant W Line source */
    NUM_SOURCE_TYPES, /*!< Number of CFD Mesh source types */
  };

  /*! Enum that identifies the Parasite Drag Tool laminar friction coefficient equation choice. */
  enum CF_LAM_EQN
  {
    CF_LAM_BLASIUS = 0,    /*!< Blasius laminar Cf equation */
    CF_LAM_BLASIUS_W_HEAT, /*!< Blasius laminar Cf equation with heat (NOT IMPLEMENTED) */
  };

  /*! Enum that identifies the Parasite Drag Tool turbulent friction coefficient equation choice. */
  enum CF_TURB_EQN
  {
    CF_TURB_EXPLICIT_FIT_SPALDING = 0,                 /*!< Explicit Fit of Spalding turbulent Cf equation */
    CF_TURB_EXPLICIT_FIT_SPALDING_CHI,                 /*!< Explicit Fit of Spalding and Chi turbulent Cf equation */
    CF_TURB_EXPLICIT_FIT_SCHOENHERR,                   /*!< Explicit Fit of Schoenherr turbulent Cf equation */
    DO_NOT_USE_CF_TURB_IMPLICIT_KARMAN,                /*!< Implicit Karman turbulent Cf equation (DO NOT USE) */
    CF_TURB_IMPLICIT_SCHOENHERR,                       /*!< Implicit Schoenherr turbulent Cf equation */
    CF_TURB_IMPLICIT_KARMAN_SCHOENHERR,                /*!< Implicit Karman-Schoenherr turbulent Cf equation */
    CF_TURB_POWER_LAW_BLASIUS,                         /*!< Power Law Blasius turbulent Cf equation */
    CF_TURB_POWER_LAW_PRANDTL_LOW_RE,                  /*!<Power Law Prandtl Low Re turbulent Cf equation */
    CF_TURB_POWER_LAW_PRANDTL_MEDIUM_RE,               /*!< Power Law Prandtl Medium Re turbulent Cf equation */
    CF_TURB_POWER_LAW_PRANDTL_HIGH_RE,                 /*!< Power Law Prandtl High Re turbulent Cf equation */
    CF_TURB_SCHLICHTING_COMPRESSIBLE,                  /*!< Schlichting Compressible turbulent Cf equation */
    DO_NOT_USE_CF_TURB_SCHLICHTING_INCOMPRESSIBLE,     /*!< Schlichting Incompressible turbulent Cf equation (DO NOT USE) */
    DO_NOT_USE_CF_TURB_SCHLICHTING_PRANDTL,            /*!< Schlichting-Prandtl turbulent Cf equation (DO NOT USE) */
    DO_NOT_USE_CF_TURB_SCHULTZ_GRUNOW_HIGH_RE,         /*!< Schultz-Grunow High Re turbulent Cf equation (DO NOT USE) */
    CF_TURB_SCHULTZ_GRUNOW_SCHOENHERR,                 /*!< Schultz-Grunow Estimate of Schoenherr turbulent Cf equation. */
    DO_NOT_USE_CF_TURB_WHITE_CHRISTOPH_COMPRESSIBLE,   /*!< White-Christoph Compressible turbulent Cf equation (DO NOT USE) */
    CF_TURB_ROUGHNESS_SCHLICHTING_AVG,                 /*!< Roughness Schlichting Avg turbulent Cf equation. */
    DO_NOT_USE_CF_TURB_ROUGHNESS_SCHLICHTING_LOCAL,    /*!< Roughness Schlichting Local turbulent Cf equation (DO NOT USE) */
    DO_NOT_USE_CF_TURB_ROUGHNESS_WHITE,                /*!< Roughness White turbulent Cf equation (DO NOT USE) */
    CF_TURB_ROUGHNESS_SCHLICHTING_AVG_FLOW_CORRECTION, /*!< Roughness Schlichting Avg Compressible turbulent Cf equation. */
    CF_TURB_HEATTRANSFER_WHITE_CHRISTOPH               /*!< Heat Transfer White-Christoph turbulent Cf equation. */
  };

  /*! Enum for Chevron curve modification types. */
  enum CHEVRON_TYPE
  {
    CHEVRON_NONE,     /*!< No chevron. */
    CHEVRON_PARTIAL,  /*!< One or more chevrons of limited extent. */
    CHEVRON_FULL,     /*!< Full period of chevrons. */
    CHEVRON_NUM_TYPES /*!< Number of chevron types. */
  };

  /*! Enum for Chevron W parameter modes. */
  enum CHEVRON_W01_MODES
  {
    CHEVRON_W01_SE,       /*!< Specify chevron W start and end. */
    CHEVRON_W01_CW,       /*!< Specify chevron W center and width. */
    CHEVRON_W01_NUM_MODES /*!< Number of chevron W parameter mode types. */
  };

  /*! Enum for Snap To collision error types. */
  enum COLLISION_ERRORS
  {
    COLLISION_OK,                    /*!< No Error. */
    COLLISION_INTERSECT_NO_SOLUTION, /*!< Touching, no solution */
    COLLISION_CLEAR_NO_SOLUTION,     /*!< Not touching, no solution */
  };

  /*! Enum used to identify various export file types. */
  enum COMPUTATION_FILE_TYPE
  {
    NO_FILE_TYPE = 0,                   /*!< No export file type */
    COMP_GEOM_TXT_TYPE = 1,             /*!< Comp Geom TXT file type */
    COMP_GEOM_CSV_TYPE = 2,             /*!< Comp Geom CSV file type */
    DRAG_BUILD_TSV_TYPE_DEPRECATED = 4, /*! Deprecated*/
    SLICE_TXT_TYPE = 8,                 /*!< Planar Slice TXT file type */
    MASS_PROP_TXT_TYPE = 16,            /*!< Mass Properties TXT file type */
    DEGEN_GEOM_CSV_TYPE = 32,           /*!< Degen Geom CSV file type */
    DEGEN_GEOM_M_TYPE = 64,             /*!< Degen Geom M file type */
    CFD_STL_TYPE = 128,                 /*!< CFD Mesh STL file type */
    CFD_POLY_TYPE = 256,                /*!<CFD Mesh POLY file type */
    CFD_TRI_TYPE = 512,                 /*!< CFD Mesh TRI file type */
    CFD_OBJ_TYPE = 1024,                /*!< CFD Mesh OBJ file type */
    CFD_DAT_TYPE = 2048,                /*!< CFD Mesh DAT file type */
    CFD_KEY_TYPE = 4096,                /*!< CFD Mesh KAY file type */
    CFD_GMSH_TYPE = 8192,               /*!< CFD Mesh GMSH file type */
    CFD_SRF_TYPE = 16384,               /*!< CFD Mesh SRF file type */
    CFD_TKEY_TYPE = 32768,              /*!< CFD Mesh TKEY file type */
    PROJ_AREA_CSV_TYPE = 65536,         /*!< Projected Area CSV file type */
    WAVE_DRAG_TXT_TYPE = 131072,        /*!< Wave Drag TXT file type */
    VSPAERO_PANEL_TRI_TYPE = 262144,    /*!< VSPAERO Panel Method TRI file type */
    DRAG_BUILD_CSV_TYPE = 524288,       /*!< Parasite Drag CSV file type */
    CFD_FACET_TYPE = 1048576,           /*!< CFD Mesh FACET file type */
    CFD_CURV_TYPE = 2097152,            /*!< CFD Mesh CURV file type */
    CFD_PLOT3D_TYPE = 4194304,          /*!< CFD Mesh PLOT3D file type */
    CFD_VSPGEOM_TYPE = 8388608,         /*!< CFD Mesh VSPGEOM file type */
    VSPAERO_VSPGEOM_TYPE = 16777216,    /*!< VSPAERO VSPGEOM file type */
  };

  /*! Enum used to identify delimiter type. */
  enum DELIM_TYPE
  {
    DELIM_COMMA,    /*!< Comma delimiter */
    DELIM_USCORE,   /*!< Underscore delimiter */
    DELIM_SPACE,    /*!< Space delimiter */
    DELIM_NONE,     /*!< No delimiter */
    DELIM_NUM_TYPES /*!< Number of delimiter types */
  };

  /*! Enum for 2D or 3D DXF export options. */
  enum DIMENSION_SET
  {
    SET_3D, /*!< 3D DXF export (default) */
    SET_2D, /*!< 2D DXF export */
  };

  /*! Enum used to identify axis of rotation or translation. */
  enum DIR_INDEX
  {
    X_DIR = 0,  /*!< X direction */
    Y_DIR = 1,  /*!< Y direction */
    Z_DIR = 2,  /*!< Z direction */
    ALL_DIR = 3 /*!< All directions */
  };

  /*! Enum for selecting the GUI display type for Geoms. */
  enum DISPLAY_TYPE
  {
    DISPLAY_BEZIER,      /*!< Display the normal Bezier surface (default) */
    DISPLAY_DEGEN_SURF,  /*!< Display as surface Degen Geom */
    DISPLAY_DEGEN_PLATE, /*!< Display as plate Degen Geom */
    DISPLAY_DEGEN_CAMBER /*!< Display as camber Degen Geom */
  };

  /*! Enum for selecting the GUI draw type for Geoms. */
  enum DRAW_TYPE
  {
    GEOM_DRAW_WIRE,    /*!< Draw the wireframe mesh (see through) */
    GEOM_DRAW_HIDDEN,  /*!< Draw the hidden mesh */
    GEOM_DRAW_SHADE,   /*!< Draw the shaded mesh */
    GEOM_DRAW_TEXTURE, /*!< Draw the textured mesh */
    GEOM_DRAW_NONE     /*!< Do not draw anything */
  };

  /*! Enum for OpenVSP API error codes. */
  enum ERROR_CODE
  {
    VSP_OK,                           /*!< No error */
    VSP_INVALID_PTR,                  /*!< Invalid pointer error */
    VSP_INVALID_TYPE,                 /*!< Invalid type error */
    VSP_CANT_FIND_TYPE,               /*!< Can't find type error */
    VSP_CANT_FIND_PARM,               /*!< Can't find parm error */
    VSP_CANT_FIND_NAME,               /*!< Can't find name error */
    VSP_INVALID_GEOM_ID,              /*!< Invalid Geom ID error */
    VSP_FILE_DOES_NOT_EXIST,          /*!< File does not exist error */
    VSP_FILE_WRITE_FAILURE,           /*!< File write failure error */
    VSP_FILE_READ_FAILURE,            /*!< File read failure error */
    VSP_WRONG_XSEC_TYPE,              /*!< Wrong XSec type error */
    VSP_WRONG_FILE_TYPE,              /*!< Wrong file type error */
    VSP_INDEX_OUT_RANGE,              /*!< Index out of range error */
    VSP_INVALID_XSEC_ID,              /*!< Invalid XSec ID error */
    VSP_INVALID_ID,                   /*!< Invalid ID error */
    VSP_CANT_SET_NOT_EQ_PARM,         /*!< Can't set NotEqParm error */
    VSP_AMBIGUOUS_SUBSURF,            /*!< Ambiguous flow-through sub-surface error */
    VSP_INVALID_VARPRESET_SETNAME,    /*!< Invalid Variable Preset set name error */
    VSP_INVALID_VARPRESET_GROUPNAME,  /*!< Invalid Variable Preset group name error */
    VSP_CONFORMAL_PARENT_UNSUPPORTED, /*!< Unsupported Conformal Geom parent error */
    VSP_UNEXPECTED_RESET_REMAP_ID,    /*!< Unexpected reset remap ID error */
    VSP_INVALID_INPUT_VAL,            /*!< Invalid input value error */
    VSP_INVALID_CF_EQN,               /*!< Invalid friction coefficient equation error */
    VSP_INVALID_DRIVERS,              /*!< Invalid drivers for driver group */
    VSP_ADV_LINK_BUILD_FAIL           /*!< Advanced link build failure */
  };

  /*! Enum used to indicate Parasite Drag Tool excressence type. */
  enum EXCRES_TYPE
  {
    EXCRESCENCE_COUNT = 0,    /*!< Drag counts excressence type */
    EXCRESCENCE_CD,           /*!< Drag coefficient excressence type */
    EXCRESCENCE_PERCENT_GEOM, /*!< Percent of parent Geom drag coefficient excressence type */
    EXCRESCENCE_MARGIN,       /*!< Percent margin excressence type */
    EXCRESCENCE_DRAGAREA,     /*!< Drag area (D/q) excressence type */
  };

  enum EXPORT_TYPE
  {
    EXPORT_FELISA,         /*!< FELISA export type (NOT IMPLEMENTED) */
    EXPORT_XSEC,           /*!< XSec (*.hrm) export type */
    EXPORT_STL,            /*!< Stereolith (*.stl) export type */
    EXPORT_AWAVE,          /*!< AWAVE export type (NOT IMPLEMENTED) */
    EXPORT_NASCART,        /*!< NASCART (*.dat) export type */
    EXPORT_POVRAY,         /*!< POVRAY (*.pov) export type */
    EXPORT_CART3D,         /*!< Cart3D (*.tri) export type */
    EXPORT_VSPGEOM,        /*!< VSPGeom (*.vspgeom) export type */
    EXPORT_VORXSEC,        /*!< VORXSEC  export type (NOT IMPLEMENTED) */
    EXPORT_XSECGEOM,       /*!< XSECGEOM export type (NOT IMPLEMENTED) */
    EXPORT_GMSH,           /*!< Gmsh (*.msh) export type */
    EXPORT_X3D,            /*!< X3D (*.x3d) export type */
    EXPORT_STEP,           /*!< STEP (*.stp) export type */
    EXPORT_PLOT3D,         /*!< PLOT3D (*.p3d) export type */
    EXPORT_IGES,           /*!< IGES (*.igs) export type */
    EXPORT_BEM,            /*!< Blade Element (*.bem) export type */
    EXPORT_DXF,            /*!< AutoCAD (*.dxf) export type */
    EXPORT_FACET,          /*!< Xpatch (*.facet) export type */
    EXPORT_SVG,            /*!< SVG (*.svg) export type */
    EXPORT_PMARC,          /*!< PMARC 12 (*.pmin) export type */
    EXPORT_OBJ,            /*!< OBJ (*.obj) export type */
    EXPORT_SELIG_AIRFOIL,  /*!< Airfoil points (*.dat) export type */
    EXPORT_BEZIER_AIRFOIL, /*!< Airfoil curves (*.bz) export type */
    EXPORT_IGES_STRUCTURE, /*!< IGES structure (*.igs) export type */
    EXPORT_STEP_STRUCTURE  /*!< STEP structure (*.stp) export type */
  };

  enum FEA_CROSS_SECT_TYPE
  {
    FEA_XSEC_GENERAL = 0, /*!< General XSec type */
    FEA_XSEC_CIRC,        /*!< Circle XSec type */
    FEA_XSEC_PIPE,        /*!< Pipe XSec type */
    FEA_XSEC_I,           /*!< I XSec type */
    FEA_XSEC_RECT,        /*!< Rectangle XSec type */
    FEA_XSEC_BOX          /*!< Box XSec type */
  };

  /*! Enum for the various FEA Mesh export types. */
  enum FEA_EXPORT_TYPE
  {
    FEA_MASS_FILE_NAME,     /*!< FEA Mesh mass export type */
    FEA_NASTRAN_FILE_NAME,  /*!< FEA Mesh NASTRAN export type */
    FEA_NKEY_FILE_NAME,     /*!< FEA Mesh NKey export type */
    FEA_CALCULIX_FILE_NAME, /*!< FEA Mesh Calculix export type */
    FEA_STL_FILE_NAME,      /*!< FEA Mesh STL export type */
    FEA_GMSH_FILE_NAME,     /*!< FEA Mesh GMSH export type */
    FEA_SRF_FILE_NAME,      /*!< FEA Mesh SRF export type */
    FEA_CURV_FILE_NAME,     /*!< FEA Mesh CURV export type */
    FEA_PLOT3D_FILE_NAME,   /*!< FEA Mesh PLOT3D export type */
    FEA_IGES_FILE_NAME,     /*!< FEA Mesh trimmed IGES export type */
    FEA_STEP_FILE_NAME,     /*!< FEA Mesh trimmed STEP export type */
    FEA_NUM_FILE_NAMES      /*!< Number of FEA Mesh export type. */
  };

  /*! Enum for FEA material types. */
  enum FEA_MATERIAL_TYPE
  {
    FEA_ISOTROPIC,    /*!< Isotropic material */
    FEA_ENG_ORTHO,    /*!< Orthotropic material in engineering parameters */
    FEA_NUM_MAT_TYPES /*!< Number of FEA material types */
  };

  /*! Enum for FEA material orientation types. */
  enum FEA_ORIENTATION_TYPE
  {
    FEA_ORIENT_GLOBAL_X, /*!< FEA Global X material orientation */
    FEA_ORIENT_GLOBAL_Y, /*!< FEA Global Y material orientation */
    FEA_ORIENT_GLOBAL_Z, /*!< FEA Global Z material orientation */
    FEA_ORIENT_COMP_X,   /*!< FEA Comp X material orientation */
    FEA_ORIENT_COMP_Y,   /*!< FEA Comp Y material orientation */
    FEA_ORIENT_COMP_Z,   /*!< FEA Comp Z material orientation */
    FEA_ORIENT_PART_U,   /*!< FEA Part U material orientation */
    FEA_ORIENT_PART_V,   /*!< FEA Part V material orientation */
    FEA_ORIENT_OML_U,    /*!< FEA OML U material orientation */
    FEA_ORIENT_OML_V,    /*!< FEA OML V material orientation */
    FEA_ORIENT_OML_R,    /*!< FEA OML R material orientation */
    FEA_ORIENT_OML_S,    /*!< FEA OML S material orientation */
    FEA_ORIENT_OML_T,    /*!< FEA OML T material orientation */
    FEA_NUM_ORIENT_TYPES /*!< Number of FEA material orientation types */
  };

  /*! Enum for FEA Part element types. */
  enum FEA_PART_ELEMENT_TYPE
  {
    FEA_SHELL = 0,        /*!< Shell (tris) FEA element type */
    FEA_BEAM,             /*!< Beam FEA element type */
    FEA_SHELL_AND_BEAM,   /*!< Both Shell and Beam FEA element types */
    FEA_NO_ELEMENTS,      /*!< FEA part with no elements */
    FEA_NUM_ELEMENT_TYPES /*!< Number of FEA element type choices */
  };

  /*! Enum used to identify the available FEA Part types. */
  enum FEA_PART_TYPE
  {
    FEA_SLICE = 0,   /*!< Slice FEA Part type */
    FEA_RIB,         /*!< Rib FEA Part type */
    FEA_SPAR,        /*!< Spar FEA Part type */
    FEA_FIX_POINT,   /*!< Fixed Point FEA Part type */
    FEA_DOME,        /*!< Dome FEA Part type */
    FEA_RIB_ARRAY,   /*!< Rib array FEA Part type */
    FEA_SLICE_ARRAY, /*!< Slice array FEA Part type */
    FEA_SKIN,        /*!< Trim FEA Part type */
    FEA_TRIM,        /*!< Skin FEA Part type */
    FEA_NUM_TYPES    /*!< Number of FEA Part types */
  };

  /*! Enum for FEA Slice orientation. */
  enum FEA_SLICE_TYPE
  {
    XY_BODY = 0, /*!< Slice is parallel to parent Geom body XY plane */
    YZ_BODY,     /*!< Slice is parallel to parent Geom body YZ plane */
    XZ_BODY,     /*!< Slice is parallel to parent Geom body XZ plane */
    XY_ABS,      /*!< Slice is parallel to absolute XY plane */
    YZ_ABS,      /*!< Slice is parallel to absolute YZ plane */
    XZ_ABS,      /*!< Slice is parallel to absolute XZ plane */
    SPINE_NORMAL /*!< Slice is perpendicular to thespine of the parent Geom */
  };

  /*! Enum used to identify the FEA Mesh unit system. */
  enum FEA_UNIT_TYPE
  {
    SI_UNIT = 0, /*!< SI unit system m, kg*/
    CGS_UNIT,    /*!< CGS unit system cm, g*/
    MPA_UNIT,    /*!< MPA unit system mm, tonne*/
    BFT_UNIT,    /*!< BFT unit system ft, slug*/
    BIN_UNIT     /*!< BIN unit system in, lbf*sec^2/in*/
  };

  /*! Enum that defines the type of edge to set the initial position of FEA Ribs and FEA Rib Arrays to. */
  enum FEA_RIB_NORMAL
  {
    NO_NORMAL,  /*!< FEA Rib or Rib Array has no set perpendicular edge */
    LE_NORMAL,  /*!< FEA Rib or Rib Array is set perpendicular to the leading edge */
    TE_NORMAL,  /*!< FEA Rib or Rib Array is set perpendicular to the trailing edge */
    SPAR_NORMAL /*!< FEA Rib or Rib Array is set perpendicular to an FEA Spar */
  };

  /*! Enum for Parasite Drag Tool form factor equations for body-type components. */
  enum FF_B_EQN
  {
    FF_B_MANUAL = 0,                 /*!< Manual FF equation */
    FF_B_SCHEMENSKY_FUSE,            /*!< Schemensky Fuselage FF equation */
    FF_B_SCHEMENSKY_NACELLE,         /*!< Schemensky Nacelle FF equation */
    FF_B_HOERNER_STREAMBODY,         /*!< Hoerner Streamlined Body FF equation */
    FF_B_TORENBEEK,                  /*!< Torenbeek FF equation */
    FF_B_SHEVELL,                    /*!< Shevell FF equation */
    FF_B_COVERT,                     /*!< Covert FF equation */
    FF_B_JENKINSON_FUSE,             /*!< Jenkinson Fuselage FF equation */
    FF_B_JENKINSON_WING_NACELLE,     /*!< Jenkinson Wing Nacelle FF equation */
    FF_B_JENKINSON_AFT_FUSE_NACELLE, /*!< Jenkinson Aft Fuselage Nacelle FF equation */
  };

  /*! Enum for Parasite Drag Tool form factor equations for wing-type components. */
  enum FF_W_EQN
  {
    FF_W_MANUAL = 0,                  /*!< Manual FF equation */
    FF_W_EDET_CONV,                   /*!< EDET Conventional Airfoil FF equation */
    FF_W_EDET_ADV,                    /*!< EDET Advanced Airfoil FF equation */
    FF_W_HOERNER,                     /*!< Hoerner FF equation */
    FF_W_COVERT,                      /*!< Covert FF equation */
    FF_W_SHEVELL,                     /*!< Shevell FF equation */
    FF_W_KROO,                        /*!< Kroo FF equation */
    FF_W_TORENBEEK,                   /*!< Torenbeek FF equation */
    FF_W_DATCOM,                      /*!< DATCOM FF equation */
    FF_W_SCHEMENSKY_6_SERIES_AF,      /*!< Schemensky 6 Series Airfoil FF equation */
    FF_W_SCHEMENSKY_4_SERIES_AF,      /*!< Schemensky 4 Series Airfoil FF equation */
    FF_W_JENKINSON_WING,              /*!< Jenkinson Wing FF equation */
    FF_W_JENKINSON_TAIL,              /*!< Jenkinson Tail FF equation */
    FF_W_SCHEMENSKY_SUPERCRITICAL_AF, /*!< Schemensky Supercritical Airfoil FF equation */
  };

  /*! Enum for Parasite Drag Tool freestream unit system. */
  enum FREESTREAM_PD_UNITS
  {
    PD_UNITS_IMPERIAL = 0, /*!< Imperial unit system */
    PD_UNITS_METRIC        /*!< Metric unit system */
  };

  /*! Enum for OpenVSP Human component gender types. */
  enum GENDER
  {
    MALE,  /*!< Male Human component */
    FEMALE /*!< Female Human component */
  };

  /*! Initial shape enums for XS_EDIT_CURVE type XSecs. */
  enum INIT_EDIT_XSEC_TYPE
  {
    EDIT_XSEC_CIRCLE,   /*!< Circle initialized as cubic Bezier type */
    EDIT_XSEC_ELLIPSE,  /*!< Ellipse initialized as PCHIP type */
    EDIT_XSEC_RECTANGLE /*!< Rectangle initialized as linear type */
  };

  /*! Enum for OpenVSP import types. */
  enum IMPORT_TYPE
  {
    IMPORT_STL,        /*!< Stereolith (*.stl) import */
    IMPORT_NASCART,    /*!< NASCART (*.dat) import */
    IMPORT_CART3D_TRI, /*!< Cart3D (*.try) import */
    IMPORT_XSEC_MESH,  /*!< XSec as Tri Mesh (*.hrm) import */
    IMPORT_PTS,        /*!< Point Cloud (*.pts) import */
    IMPORT_V2,         /*!< OpenVSP v2 (*.vsp) import */
    IMPORT_BEM,        /*!< Blade Element (*.bem) import */
    IMPORT_XSEC_WIRE,  /*!< XSec as Wireframe (*.hrm) import */
    IMPORT_P3D_WIRE    /*!< Plot3D as Wireframe (*.p3d) import */
  };

  /*! Enum for Surface Intersection export file types. */
  enum INTERSECT_EXPORT_TYPE
  {
    INTERSECT_SRF_FILE_NAME,    /*!< SRF intersection file type */
    INTERSECT_CURV_FILE_NAME,   /*!< CURV intersection file type */
    INTERSECT_PLOT3D_FILE_NAME, /*!< PLOT3D intersection file type */
    INTERSECT_IGES_FILE_NAME,   /*!< IGES intersection file type */
    INTERSECT_STEP_FILE_NAME,   /*!< STEP intersection file type */
    INTERSECT_NUM_FILE_NAMES    /*!< Number of surface intersection file types */
  };

  /*! Enum that describes units for length. */
  enum LEN_UNITS
  {
    LEN_MM,      /*!< Millimeter  */
    LEN_CM,      /*!< Centimeter */
    LEN_M,       /*!< Meter */
    LEN_IN,      /*!< Inch */
    LEN_FT,      /*!< Feet */
    LEN_YD,      /*!< Yard */
    LEN_UNITLESS /*!< Unitless */
  };

  /*! Enum that describes units for mass. */
  enum MASS_UNIT
  {
    MASS_UNIT_G = 0, /*!< Gram */
    MASS_UNIT_KG,    /*!< Kilogram */
    MASS_UNIT_TONNE, /*!< Tonne */
    MASS_UNIT_LBM,   /*!< Pound-mass */
    MASS_UNIT_SLUG,  /*!< Slug */
    MASS_LBFSEC2IN   /*!< Pound-force-second squared per inch  */
  };

  /*! Enum for OpenVSP's various Parm class types. */
  enum PARM_TYPE
  {
    PARM_DOUBLE_TYPE,      /*!< Double Parm type (Parm) */
    PARM_INT_TYPE,         /*!< Integer Parm type (IntParm) */
    PARM_BOOL_TYPE,        /*!< Bool Parm type (BoolParm) */
    PARM_FRACTION_TYPE,    /*!< Fraction Parm type (FractionParm) */
    PARM_LIMITED_INT_TYPE, /*!< Limited integer Parm type (LimIntParm) */
    PARM_NOTEQ_TYPE,       /*!< Not equal Parm type (NotEqParm) */
    PARM_POWER_INT_TYPE    /*!< No clue TODO */
  };

  /*! Enum used to identify patch types for WireGeoms. */
  enum PATCH_TYPE
  {
    PATCH_NONE,     /*!< No patch */
    PATCH_POINT,    /*!< Point patch type */
    PATCH_LINE,     /*!< Line patch type */
    PATCH_COPY,     /*!< Copy patch type */
    PATCH_HALFWAY,  /*!< Halfway patch type */
    PATCH_NUM_TYPES /*!< Number of patch types */
  };

  /*! Enum for parametric curve types. */
  enum PCURV_TYPE
  {
    LINEAR,        /*!< Linear curve type */
    PCHIP,         /*!< Piecewise Cubic Hermite Interpolating Polynomial curve type */
    CEDIT,         /*!< Cubic Bezier curve type */
    APPROX_CEDIT,  /*!< Approximate curve as Cubic Bezier */
    NUM_PCURV_TYPE /*!< Number of curve types */
  };

  /*! Enum that describes units for pressure. */
  enum PRES_UNITS
  {
    PRES_UNIT_PSF = 0, /*!< Pounds per square foot */
    PRES_UNIT_PSI,     /*!< Pounds per square inch */
    PRES_UNIT_BA,      /*!< Barye */
    PRES_UNIT_PA,      /*!< Pascal */
    PRES_UNIT_KPA,     /*!< Kilopascal */
    PRES_UNIT_MPA,     /*!< Megapascal */
    PRES_UNIT_INCHHG,  /*!< Inch of mercury */
    PRES_UNIT_MMHG,    /*!< Millimeter of mercury */
    PRES_UNIT_MMH20,   /*!< Millimeter of water */
    PRES_UNIT_MB,      /*!< Millibar */
    PRES_UNIT_ATM      /*!< Atmosphere */
  };

  /*! Enum for Projected Area boundary option type. */
  enum PROJ_BNDY_TYPE
  {
    NO_BOUNDARY,          /*!< No boundary */
    SET_BOUNDARY,         /*!< Set boundary */
    GEOM_BOUNDARY,        /*!< Geom boundary */
    NUM_PROJ_BNDY_OPTIONS /*!< Number of projected area boundary options */
  };

  /*! Enum for Projected Area direction type. */
  enum PROJ_DIR_TYPE
  {
    X_PROJ,              /*!< Project in X axis direction */
    Y_PROJ,              /*!< Project in Y axis direction */
    Z_PROJ,              /*!< Project in Z axis direction */
    GEOM_PROJ,           /*!< Project toward a Geom */
    VEC_PROJ,            /*!< Project along a 3D vector */
    NUM_PROJ_DIR_OPTIONS /*!< Number of Projected Area direction types */
  };

  /*! Enum for Projected Area target type. */
  enum PROJ_TGT_TYPE
  {
    SET_TARGET,          /*!< Set target type */
    GEOM_TARGET,         /*!< Geom target type */
    NUM_PROJ_TGT_OPTIONS /*!< Number of Projected Area target types */
  };

  /*! Enum used to specify the mode of a propeller Geom. */
  enum PROP_MODE
  {
    PROP_BLADES, /*!< Propeller Geom is defined by individual propeller blades */
    PROP_BOTH,   /*!< Propeller Geom is defined by blades and a disk together */
    PROP_DISK    /*!< Propeller Geom is defined by a flat circular disk */
  };

  /*! Enum for the various propeller blade curve parameterization options. */
  enum PROP_PCURVE
  {
    PROP_CHORD,      /*!< Chord parameterization */
    PROP_TWIST,      /*!< Twist parameterization */
    PROP_RAKE,       /*!< Rake parameterization */
    PROP_SKEW,       /*!< Skew parameterization */
    PROP_SWEEP,      /*!< Sweep parameterization */
    PROP_THICK,      /*!< Thickness parameterization */
    PROP_CLI,        /*!< Induced lift coefficient parameterization */
    PROP_AXIAL,      /*!< Axial parameterization */
    PROP_TANGENTIAL, /*!< Tangential parameterization */
    NUM_PROP_PCURVE  /*!< Number of propeller blade curve parameterization options */
  };

  /*! Enum used to indicate manual or component reference type. Aerodynamic reference area and length*/
  enum REF_WING_TYPE
  {
    MANUAL_REF = 0, /*! manually specify the reference areas and lengths*/
    COMPONENT_REF,  /*! use a particular wing to calculate the reference area and lengths*/
    NUM_REF_TYPES   /*!< Number of wing reference types */
  };

  /*! Enum representing the possible data types returned from the ResultsMgr. */
  enum RES_DATA_TYPE
  {
    INVALID_TYPE = -1,      /*!< Invalid results data type */
    INT_DATA = 0,           /*!< Integer result data type */
    DOUBLE_DATA = 1,        /*!< Double result data type */
    STRING_DATA = 2,        /*!< String result data type */
    VEC3D_DATA = 3,         /*!< Vec3d result data type */
    DOUBLE_MATRIX_DATA = 4, /*!< Double matrix result data type */
  };

  /*! Enum representing the possible Geom types returned from the ResultsMgr. */
  enum RES_GEOM_TYPE
  {
    MESH_INDEXED_TRI,         /*!< Indexed triangulated mesh Geom type */
    MESH_SLICE_TRI,           /*!< Sliced Triangulated mesh Geom type */
    GEOM_XSECS,               /*!< GeomXSec Geom type */
    MESH_INDEX_AND_SLICE_TRI, /*!< Both indexed and sliced triangulated mesh Geom type */
  };

  /*! Enum that describes units for density. */
  enum RHO_UNITS
  {
    RHO_UNIT_SLUG_FT3 = 0, /*!< Slug per cubic foot */
    RHO_UNIT_G_CM3,        /*!< Gram per cubic centimeter */
    RHO_UNIT_KG_M3,        /*!< Kilogram per cubic meter */
    RHO_UNIT_TONNE_MM3,    /*!< Tonne per cubic millimeter */
    RHO_UNIT_LBF_FT3,      /*!< Pound-force per cubic foot */
    RHO_UNIT_LBFSEC2_IN4   /*!< Pound-force-second squared per inch to the fourth */
  };                       // Rho Units ENUM

  /*! Enum for specifying named set types. */
  enum SET_TYPE
  {
    SET_NONE = -1,     /*!< None set */
    SET_ALL = 0,       /*!< All set */
    SET_SHOWN = 1,     /*!< Shown set */
    SET_NOT_SHOWN = 2, /*!< Not Shown set */
    SET_FIRST_USER = 3 /*!< 1st user named set */
  };

  /*! Enum that identifies the trimmed STEP export representation type. */
  enum STEP_REPRESENTATION
  {
    STEP_SHELL, /*!< Manifold shell surface STEP file representation */
    STEP_BREP   /*!< Manifold solid BREP STEP file representation */
  };

  /*! Enum used to identify Parasite Drag Tool sub-surface treatment. */
  enum SUBSURF_INCLUDE
  {
    SS_INC_TREAT_AS_PARENT,    /*!< Treat the sub-surface the same as the parent */
    SS_INC_SEPARATE_TREATMENT, /*!< Treat the sub-surface separately from the parent */
    SS_INC_ZERO_DRAG,          /*!< No drag contribution for the sub-surface */
  };

  /*! Enum for indicating which part of the parent surface a sub-surfacce is dedfine. */
  enum SUBSURF_INOUT
  {
    INSIDE,  /*!< The interior of the sub-surface is its surface */
    OUTSIDE, /*!< The exterior of the sub-surface is its surface */
    NONE     /*!< No part of the parent surface belongs to the sub-surface */
  };

  /*! Enum that identifies which surface coordinate is constant for a line sub-surface. */
  enum SUBSURF_LINE_TYPE
  {
    CONST_U, /*!< Constant U sub-surface */
    CONST_W  /*!< Constant W sub-surface */
  };

  /*! Enum for the various sub-surface types. */
  enum SUBSURF_TYPE
  {
    SS_LINE,        /*!< Line sub-surface type */
    SS_RECTANGLE,   /*!< Rectangle sub-surface type */
    SS_ELLIPSE,     /*!< Ellipse sub-surface type */
    SS_CONTROL,     /*!< Control sub-surface type */
    SS_LINE_ARRAY,  /*!< Line array sub-surface type */
    SS_FINITE_LINE, /*!< Finite line sub-surface type */
    SS_NUM_TYPES    /*!< Number of sub-surface types */
  };

  /*! Enum that represents various symmetry types. */
  enum SYM_FLAG
  {
    SYM_XY = (1 << 0),    /*!< Symmetry about the XY plane */
    SYM_XZ = (1 << 1),    /*!< Symmetry about the XZ plane */
    SYM_YZ = (1 << 2),    /*!< Symmetry about the YZ plane */
    SYM_ROT_X = (1 << 3), /*!< Rotational symmetry about the X axis */
    SYM_ROT_Y = (1 << 4), /*!< Rotational symmetry about the Y axis */
    SYM_ROT_Z = (1 << 5), /*!< Rotational symmetry about the Z axis */
    SYM_PLANAR_TYPES = 3, /*!< Number of planar symmetry types */
    SYM_NUM_TYPES = 6     /*!< Number of symmetry types */
  };

  /*! Symmetry enum for Rounded Rectangle and Edit Curve type XSecs. */
  enum SYM_XSEC_TYP
  {
    SYM_NONE, /*!< No symmetry */
    SYM_RL,   /*!< Right-left symmetry: right is mirrored to the left */
    SYM_TB,   /*!< Top-bottom symmetry: top is mirrored to the bottom */
    SYM_ALL   /*!< All symmetry with top right as primary */
  };

  /*! Enum that describes units for temperature. */
  enum TEMP_UNITS
  {
    TEMP_UNIT_K = 0, /*!< Kelvin */
    TEMP_UNIT_C,     /*!< Celsius */
    TEMP_UNIT_F,     /*!< Fahrenheit  */
    TEMP_UNIT_R,     /*!< Rankine  */
  };

  /*! Enum that describes units for velocity. */
  enum VEL_UNITS
  {
    V_UNIT_FT_S = 0, /*!< Feet per second */
    V_UNIT_M_S,      /*!< Meter per second */
    V_UNIT_MPH,      /*!< Mile per hour */
    V_UNIT_KM_HR,    /*!< Kilometer per hour */
    V_UNIT_KEAS,     /*!< Knots equivalent airspeed */
    V_UNIT_KTAS,     /*!< Knots true airspeed */
    V_UNIT_MACH,     /*!< Mach */
  };

  /*! Enum for 2D drawing types (DXF & SVG). */
  enum VIEW_NUM
  {
    VIEW_1,    /*!< One 2D view */
    VIEW_2HOR, /*!< Two horizontal 2D views */
    VIEW_2VER, /*!< Two vertical 2D views */
    VIEW_4,    /*!< Four 2D views */
  };

  /*! Enum for describing 2D view rotations (DXF & SVG). */
  enum VIEW_ROT
  {
    ROT_0,   /*!< No rotation */
    ROT_90,  /*!< 90 degree rotation */
    ROT_180, /*!< 180 degree rotation */
    ROT_270, /*!< 270 degree rotation */
  };

  /*! Enum for describing 2D view types (DXF & SVG). */
  enum VIEW_TYPE
  {
    VIEW_LEFT,     /*!< Left 2D view type */
    VIEW_RIGHT,    /*!< Right 2D view type */
    VIEW_TOP,      /*!< Top 2D view type */
    VIEW_BOTTOM,   /*!< Bottom 2D view type */
    VIEW_FRONT,    /*!< Front 2D view type */
    VIEW_REAR,     /*!< Rear 2D view type */
    VIEW_NONE,     /*!< No 2D view type */
    VIEW_NUM_TYPES /*!< TODO */
  };

  /*! Enum that definies the VSPAERO analysis method. */
  enum VSPAERO_ANALYSIS_METHOD
  {
    VORTEX_LATTICE, /*!< VSPAERO vortex lattice method */
    PANEL           /*!< VSPAERO panel method */
  };

  /*! Enums for VSPAERO unsteady noise calculation types. */
  enum VSPAERO_NOISE_TYPE
  {
    NOISE_FLYBY,     /*!< Set up fly by noise analysis in VSPAERO for PSU-WOPWOP */
    NOISE_FOOTPRINT, /*!< Set up footprint noise analysis in VSPAERO for PSU-WOPWOP */
    NOISE_STEADY,    /*!< Set up steady state noise analysis in VSPAERO for PSU-WOPWOP */
  };

  /*! Enums for VSPAERO unsteady noise units. */
  enum VSPAERO_NOISE_UNIT
  {
    NOISE_SI,     /*!< Assume geometry and VSPAERO inputs in SI (m N kg s) for PSU-WOPWOP  */
    NOISE_ENGLISH /*!< Assume geometry and VSPAERO inputs in english (ft lbf slug s) units, will convert to SI (m N kg s) for PSU-WOPWOP */
  };

  /*! Enum for the types of preconditioner used in VSPAERO. */
  enum VSPAERO_PRECONDITION
  {
    PRECON_MATRIX = 0, /*!< Matrix preconditioner */
    PRECON_JACOBI,     /*!< Jacobi preconditioner */
    PRECON_SSOR,       /*!< Symmetric successive over-relaxation preconditioner */
  };

  /*! Enum for the types of VSPAERO stability analyses. */
  enum VSPAERO_STABILITY_TYPE
  {
    STABILITY_OFF,        /*!< No stability analysis (off) */
    STABILITY_DEFAULT,    /*!< Normal steady stability analysis */
    STABILITY_P_ANALYSIS, /*!< P stability analysis */
    STABILITY_Q_ANALYSIS, /*!< Q stability analysis */
    STABILITY_R_ANALYSIS, /*!< R stability analysis */
    STABILITY_UNSTEADY,   /*!< TODO: Implement with later VSPAERO version*/
    STABILITY_HEAVE,      /*!< TODO: Implement with later VSPAERO version*/
    STABILITY_IMPULSE     /*!< TODO: Implement with later VSPAERO version*/
  };

  /*! Enum for the VSPAERO stall modeling options (Cl Max VSPAERO input). */
  enum VSPAERO_CLMAX_TYPE
  {
    CLMAX_OFF,    /*!< Stall modeling off (Cl Max = 0) */
    CLMAX_2D,     /*!< 2D Cl Max stall modeling with user defined value */
    CLMAX_CARLSON /*!< Carlson's Pressure Correlation stal model (Cl Max = 999) */
  };

  /*! Enum that is used to describe surfaces in CFD Mesh. */
  enum VSP_SURF_CFD_TYPE
  {
    CFD_NORMAL,      /*!< Normal CFD Mesh surface */
    CFD_NEGATIVE,    /*!< Negative volume CFD Mesh surface */
    CFD_TRANSPARENT, /*!< Transparent CFD Mesh surface */
    CFD_STRUCTURE,   /*!< FEA structure CFD Mesh surface */
    CFD_STIFFENER,   /*!< FEA stiffener CFD Mesh surface */
    CFD_NUM_TYPES,   /*!< Number of CFD Mesh surface types */
  };

  /*! Enum for the different surface types in OpenVSP. */
  enum VSP_SURF_TYPE
  {
    NORMAL_SURF,    /*!< Normal VSP surface */
    WING_SURF,      /*!< Wing VSP surface */
    DISK_SURF,      /*!< Disk VSP surface */
    PROP_SURF,      /*!< Propeller VSP surface */
    NUM_SURF_TYPES, /*!< Number of VSP surface types */
  };

  /*!< TODO */
  enum W_HINT
  {
    W_RIGHT_0, /*!< TODO */
    W_BOTTOM,  /*!< TODO */
    W_LEFT,    /*!< TODO */
    W_TOP,     /*!< TODO */
    W_RIGHT_1, /*!< TODO */
    W_FREE,    /*!< TODO */
  };

  /*! Enum used to identify the type of wing blending between XSecs. */
  enum WING_BLEND
  {
    BLEND_FREE,              /*!< Free blending */
    BLEND_ANGLES,            /*!< Blend based on angles (sweep & dihedral) */
    BLEND_MATCH_IN_LE_TRAP,  /*!< Match inboard leading edge trapezoid */
    BLEND_MATCH_IN_TE_TRAP,  /*!< Match inboard trailing edge trapezoid */
    BLEND_MATCH_OUT_LE_TRAP, /*!< Match outboard leading edge trapezoid */
    BLEND_MATCH_OUT_TE_TRAP, /*!< Match outboard trailing edge trapezoid */
    BLEND_MATCH_IN_ANGLES,   /*!< Match inboard angles */
    BLEND_MATCH_LE_ANGLES,   /*!< Match leading edge angles */
    BLEND_NUM_TYPES          /*!< Number of blending types */
  };

  /*! Enum for controlling wing section planform parameter control and linking. */
  enum WING_DRIVERS
  {
    AR_WSECT_DRIVER,                                      /*!< Aspect ratio driver */
    SPAN_WSECT_DRIVER,                                    /*!< Span driver */
    AREA_WSECT_DRIVER,                                    /*!< Area driver */
    TAPER_WSECT_DRIVER,                                   /*!< Taper driver */
    AVEC_WSECT_DRIVER,                                    /*!< Average chord driver */
    ROOTC_WSECT_DRIVER,                                   /*!< Root chord driver */
    TIPC_WSECT_DRIVER,                                    /*!< Tip chord driver */
    SECSWEEP_WSECT_DRIVER,                                /*!< Section sweep driver */
    NUM_WSECT_DRIVER,                                     /*!< Number of wing section drivers */
    SWEEP_WSECT_DRIVER = SECSWEEP_WSECT_DRIVER + 1,       /*!< Sweepo driver */
    SWEEPLOC_WSECT_DRIVER = SECSWEEP_WSECT_DRIVER + 2,    /*!< Sweep location driver */
    SECSWEEPLOC_WSECT_DRIVER = SECSWEEP_WSECT_DRIVER + 3, /*!< Secondary sweep location driver */
  };

  /*! Enum that identifies the working XDDM type. */
  enum XDDM_QUANTITY_TYPE
  {
    XDDM_VAR,   /*!< Variable XDDM type */
    XDDM_CONST, /*!< Constant XDDM type */
  };

  /*! Enum for modifying XSec through closure types. */
  enum XSEC_CLOSE_TYPE
  {
    CLOSE_NONE,     /*!< No closure */
    CLOSE_SKEWLOW,  /*!< Skew lower closure */
    CLOSE_SKEWUP,   /*!< Skew upper closure */
    CLOSE_SKEWBOTH, /*!< Skew both closure */
    CLOSE_EXTRAP,   /*!< Extrapolate closure */
    CLOSE_NUM_TYPES /*!< Number of XSec closure types */
  };

  /*! Enum that identifies the various OpenVSP XSecCurve types. */
  enum XSEC_CRV_TYPE
  {
    XS_UNDEFINED = -1,    /*!< Undefined XSec */
    XS_POINT,             /*!< Point XSec */
    XS_CIRCLE,            /*!< Circle XSec */
    XS_ELLIPSE,           /*!< Ellipse XSec */
    XS_SUPER_ELLIPSE,     /*!< Super ellipse XSec */
    XS_ROUNDED_RECTANGLE, /*!< Rounded rectangle XSec */
    XS_GENERAL_FUSE,      /*!< General fuselage XSec */
    XS_FILE_FUSE,         /*!< Fuselage file XSec */
    XS_FOUR_SERIES,       /*!< Four series XSec */
    XS_SIX_SERIES,        /*!< Six series XSec */
    XS_BICONVEX,          /*!< Biconvex XSec */
    XS_WEDGE,             /*!< Wedge XSec */
    XS_EDIT_CURVE,        /*!< Generic Edit Curve XSec */
    XS_FILE_AIRFOIL,      /*!< Airfoil file XSec */
    XS_CST_AIRFOIL,       /*!< CST airfoil XSec */
    XS_VKT_AIRFOIL,       /*!< VKT airfoil XSec */
    XS_FOUR_DIGIT_MOD,    /*!< Four digit modified XSec */
    XS_FIVE_DIGIT,        /*!< Five digit XSec */
    XS_FIVE_DIGIT_MOD,    /*!< Five digit modified XSec */
    XS_ONE_SIX_SERIES,    /*!< One six series XSec */
    XS_NUM_TYPES          /*!< Number of XSec types */
  };

  /*! Enum for XSec drivers. TODO improve comments */
  enum XSEC_DRIVERS
  {
    WIDTH_XSEC_DRIVER,         /*!< Width driver First two are used for Circle.  Others are used for general XSecCurves*/
    AREA_XSEC_DRIVER,          /*!< Area driver Area must be second entry.*/
    HEIGHT_XSEC_DRIVER,        /*!< Height driver */
    HWRATIO_XSEC_DRIVER,       /*!< Height/width ratio driver */
    NUM_XSEC_DRIVER,           /*!< Number of XSec drivers */
    CIRCLE_NUM_XSEC_DRIVER = 2 /*!< Number of Circle XSec drivers */
  };

  /*! Enum for XSec side types. TODO improve these comments*/
  enum XSEC_SIDES_TYPE
  {
    XSEC_BOTH_SIDES, /*!< Both sides */
    XSEC_LEFT_SIDE,  /*!< Left side */
    XSEC_RIGHT_SIDE  /*!< Right side */
  };

  /*! Enum used to identify XSec trim type. */
  enum XSEC_TRIM_TYPE
  {
    TRIM_NONE,     /*!< No trimming */
    TRIM_X,        /*!< Trim XSec by X */
    TRIM_THICK,    /*!< Trim XSec by thickness */
    TRIM_NUM_TYPES /*!< Number of trimming types */
  };

  /*! Enum for the various XSec types in OpenVSP. */
  enum XSEC_TYPE
  {
    XSEC_FUSE,     /*!< Fuselage XSec Geom */
    XSEC_STACK,    /*!< Stack XSec Geom */
    XSEC_WING,     /*!< Wing XSec Geom */
    XSEC_CUSTOM,   /*!< Custom XSec Geom */
    XSEC_PROP,     /*!< Propeller XSec Geom */
    XSEC_NUM_TYPES /*!< Number of XSec types */
  };

  /*! Enum for XSec width shift. TODO improve these comments*/
  enum XSEC_WIDTH_SHIFT
  {
    XS_SHIFT_LE = 0,  /*!< Shift leading edge */
    XS_SHIFT_MID = 1, /*!< Shift midpoint */
    XS_SHIFT_TE = 2   /*!< Shift trailing edge */
  };
} // Namespace

#endif // !defined(VSPDEFINES__INCLUDED_)
