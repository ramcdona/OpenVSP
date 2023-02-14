//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// ScriptMgr.h: interface to AngelScript
// J.R Gloudemans
//
// Note: For consistency when adding Doxygen comments, follow the Qt style
// identified here: http://www.doxygen.nl/manual/docblocks.html#specialblock
//
// FIXME: asDocInfo group can't contain any underscores!
//
//////////////////////////////////////////////////////////////////////

#include "ScriptMgr.h"

#include "Parm.h"
#include "Matrix4d.h"
#include "VSP_Geom_API.h"
#include "CustomGeom.h"
#include "AdvLinkMgr.h"
#include "StringUtil.h"
#include "FileUtil.h"

// Make sure int32_t is defined.
#ifdef _MSC_VER
#if _MSC_VER >= 1600
#include <cstdint>
#else
typedef __int32 int32_t;
#endif
#else
#include <cstdint>
#endif

using namespace vsp;

//==== Implement a simple message callback function ====//
void MessageCallback(const asSMessageInfo *msg, void *param)
{
    char str[1024];
    const char *type = "ERR ";
    if (msg->type == asMSGTYPE_WARNING)
    {
        type = "WARN";
    }
    else if (msg->type == asMSGTYPE_INFORMATION)
    {
        type = "INFO";
    }
    sprintf(str, "%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);
    ScriptMgr.AddToMessages(str);
    printf("%s", str);
}

//==================================================================================================//
//========================================= ScriptMgr      =========================================//
//==================================================================================================//

//==== Constructor ====//
ScriptMgrSingleton::ScriptMgrSingleton()
{
    m_SaveInt = 0;
    m_ScriptEngine = NULL;
    m_ScriptMessages = "";
}

//==== Set Up Script Engine, Script Error Callbacks ====//
void ScriptMgrSingleton::Init()
{
    //==== Only Init Once ====//
    static bool init_flag = false;
    if (init_flag)
        return;
    init_flag = true;

    //==== Create the Script Engine ====//
    m_ScriptEngine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
    asIScriptEngine *se = m_ScriptEngine;

    //==== Set the message callback to receive information on errors in human readable form.  ====//
    int r = se->SetMessageCallback(vspFUNCTION(MessageCallback), 0, vspCALL_CDECL);
    assert(r >= 0);

    //==== Register Addons ====//
    RegisterStdString(m_ScriptEngine);

    string comment_str = R"(
  //!  AngelScript ScriptExtension for representing the C++ std::string
  /*! <a href="https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_stdlib_string.html">Angelscript string Documentation </a>
  */)";

    se->AddSkipComment("string", comment_str.c_str());

    RegisterScriptArray(m_ScriptEngine, true);

    comment_str = R"(
  //!  AngelScript ScriptExtension for representing the C++ std::vector
  /*! <a href="https://www.angelcode.com/angelscript/sdk/docs/manual/doc_datatypes_arrays.html">Angelscript array Documentation </a>
  */)";

    se->AddSkipComment("array", comment_str.c_str());

    //    RegisterScriptDateTime(m_ScriptEngine);

    //     comment_str = R"(
    //   //!  AngelScript ScriptExtension for obtain the system date and time
    //   /*! <a href="https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_stdlib_datetime.html">Angelscript datetime Documentation </a>
    //   */)";

    //     se->AddSkipComment("datetime", comment_str.c_str());

    RegisterScriptFile(m_ScriptEngine);

    comment_str = R"(
  //!  AngelScript ScriptExtension for representing the C++ std::FILE
  /*! <a href="https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_stdlib_file.html">Angelscript file Documentation </a>
  */)";

    se->AddSkipComment("file", comment_str.c_str());

    //     RegisterScriptFileSystem(m_ScriptEngine);

    //     comment_str = R"(
    //   //!  AngelScript ScriptExtension for working with the filesystem
    //   /*! <a href="https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_stdlib_filesystem.html">Angelscript filesystem Documentation </a>
    //   */)";

    //     se->AddSkipComment("filesystem", comment_str.c_str());

    RegisterStdStringUtils(m_ScriptEngine);

    comment_str = R"(
  //!  AngelScript ScriptExtension for representing the C++ std::string
  /*! <a href="https://www.angelcode.com/angelscript/sdk/docs/manual/doc_datatypes_arrays.html">Angelscript string Documentation </a>
  */)";

    se->AddSkipComment("string_util", comment_str.c_str()); // FIXME

    RegisterScriptMath(m_ScriptEngine);

    comment_str = R"(
  //!  AngelScript ScriptExtension for representing the C++ std::math collection of functions
  /*! <a href="https://www.angelcode.com/angelscript/sdk/docs/manual/doc_addon_math.html">Angelscript array Documentation </a>
  */)";

    se->AddSkipComment("math", comment_str.c_str()); // FIXME

    RegisterScriptAny(m_ScriptEngine);

    comment_str = R"(
  //!  AngelScript ScriptExtension for representing generic container that can hold any value
  /*! <a href="https://www.angelcode.com/angelscript/sdk/docs/manual/doc_addon_any.html">Angelscript any Documentation </a>
  */)";

    se->AddSkipComment("any", comment_str.c_str());

    //==== Cache Some Common Types ====//
    m_IntArrayType = se->GetTypeInfoById(se->GetTypeIdByDecl("array<int>"));
    assert(m_IntArrayType);
    m_DoubleArrayType = se->GetTypeInfoById(se->GetTypeIdByDecl("array<double>"));
    assert(m_DoubleArrayType);
    m_DoubleMatArrayType = se->GetTypeInfoById(se->GetTypeIdByDecl("array<array<double>@>"));
    assert(m_DoubleMatArrayType);
    m_StringArrayType = se->GetTypeInfoById(se->GetTypeIdByDecl("array<string>"));
    assert(m_StringArrayType);

    //==== Register VSP Enums ====//
    RegisterEnums(m_ScriptEngine);

    //==== Register VSP Objects ====//
    RegisterVec3d(m_ScriptEngine);
    m_Vec3dArrayType = se->GetTypeInfoById(se->GetTypeIdByDecl("array<vec3d>"));
    assert(m_Vec3dArrayType);

    RegisterMatrix4d(m_ScriptEngine);
    RegisterCustomGeomMgr(m_ScriptEngine);
    RegisterAdvLinkMgr(m_ScriptEngine);
    RegisterAPIErrorObj(m_ScriptEngine);
    RegisterAPI(m_ScriptEngine);
    RegisterUtility(m_ScriptEngine);
}

void ScriptMgrSingleton::RunTestScripts()
{
    ////===== Run Test Scripts ====//
    // ScriptMgr.ReadScript( "TestScript", "../../TestScript.as"  );
    // ScriptMgr.ReadScriptFromFile( "TestScript", "../../../TestScript.as"  );
    // ScriptMgr.ExecuteScript( "TestScript", "void main()" );
    // ScriptMgr.ExecuteScript( "TestScript", "void TestAPIScript()" );
}

//==== Read And Execute Script File  ====//
int ScriptMgrSingleton::ReadExecuteScriptFile(const string &file_name, const string &function_name)
{
    string module_name = ReadScriptFromFile("ReadExecute", file_name);

    return ExecuteScript(module_name.c_str(), function_name.c_str(), false, 0.0, false);
}

vector<string> ScriptMgrSingleton::ReadScriptsFromDir(const string &dir_name, const string &suffix)
{
    vector<string> mod_name_vec;

    vector<string> file_vec = ScanFolder(dir_name.c_str());

    for (int i = 0; i < (int)file_vec.size(); i++)
    {
        int s_num = suffix.size();
        if (file_vec[i].size() > s_num)
        {
            if (file_vec[i].compare(file_vec[i].size() - s_num, s_num, suffix.c_str()) == 0)
            {
                string sub = file_vec[i].substr(0, file_vec[i].size() - s_num);
                string file_name = dir_name;
                file_name.append(file_vec[i]);
                string module_name = ScriptMgr.ReadScriptFromFile(sub, file_name);

                if (module_name.size())
                    mod_name_vec.push_back(module_name);
            }
        }
    }

    return mod_name_vec;
}

//==== Start A New Module And Read Script ====//
string ScriptMgrSingleton::ReadScriptFromFile(const string &module_name, const string &file_name)
{
    string content = ExtractContent(file_name);

    if (content.size() < 2)
    {
        return string();
    }

    return ReadScriptFromMemory(module_name, content);
}

//==== Start A New Module And Read Script ====//
string ScriptMgrSingleton::ReadScriptFromMemory(const string &module_name, const string &script_content)
{
    int r;
    string updated_module_name = module_name;
    map<string, string>::iterator iter;

    //==== Check If Module Name Already Exists ====//
    iter = m_ModuleContentMap.find(updated_module_name);
    if (iter != m_ModuleContentMap.end())
    {
        //==== Check If Content is Same ====//
        if (iter->second == script_content)
            return iter->first;

        //==== Need To Change Module Name ====//
        static int dup_cnt = 0;
        updated_module_name.append(StringUtil::int_to_string(dup_cnt, "%d"));
        dup_cnt++;
    }

    //==== Make Sure Not Duplicate Of Any Other Module ====//
    for (iter = m_ModuleContentMap.begin(); iter != m_ModuleContentMap.end(); iter++)
    {
        if (iter->second == script_content)
            return iter->first;
    }

    //==== Start A New Module ====//
    r = m_ScriptBuilder.StartNewModule(m_ScriptEngine, updated_module_name.c_str());
    if (r < 0)
        return string();

    r = m_ScriptBuilder.AddSectionFromMemory(updated_module_name.c_str(), script_content.c_str(), script_content.size());
    if (r < 0)
        return string();

    r = m_ScriptBuilder.BuildModule();
    if (r < 0)
        return string();

    //==== Add To Map ====//
    m_ModuleContentMap[updated_module_name] = script_content;

    return updated_module_name;
}

//==== Extract Content From File Into String ====//
string ScriptMgrSingleton::ExtractContent(const string &file_name)
{
    string file_content;
    FILE *fp = fopen(file_name.c_str(), "r");
    if (fp)
    {
        char buff[512];
        while (fgets(buff, 512, fp))
        {
            file_content.append(buff);
        }
        file_content.append("\0");
        fclose(fp);
    }
    return file_content;
}

//==== Find Script And Remove ====//
bool ScriptMgrSingleton::RemoveScript(const string &module_name)
{
    //==== Find Module ====//
    map<string, string>::iterator iter;
    iter = m_ModuleContentMap.find(module_name);
    if (iter == m_ModuleContentMap.end())
    {
        return false; // Could not find module name;
    }

    m_ModuleContentMap.erase(iter);

    int ret = m_ScriptEngine->DiscardModule(module_name.c_str());

    if (ret < 0)
        return false;
    return true;
}

//==== Execute Function in Module ====//
int ScriptMgrSingleton::ExecuteScript(const char *module_name, const char *function_name, bool arg_flag, double arg, bool by_decl)
{
    // Find the function that is to be called.
    asIScriptModule *mod = m_ScriptEngine->GetModule(module_name);

    if (!mod)
    {
        printf("Error ExecuteScript GetModule %s\n", module_name);
        return 1;
    }

    asIScriptFunction *func = NULL;
    if (by_decl)
    {
        func = mod->GetFunctionByDecl(function_name);
    }
    else
    {
        func = mod->GetFunctionByName(function_name);
    }

    if (func == 0)
    {
        return 1;
    }

    // Create our context, prepare it, and then execute
    asIScriptContext *ctx = m_ScriptEngine->CreateContext();
    ctx->Prepare(func);
    if (arg_flag)
    {
        ctx->SetArgDouble(0, arg);
    }
    int r = ctx->Execute();
    if (r != asEXECUTION_FINISHED)
    {
        // The execution didn't complete as expected. Determine what happened.
        if (r == asEXECUTION_EXCEPTION)
        {
            // An exception occurred, let the script writer know what happened so it can be corrected.
            printf("An exception '%s' occurred \n", ctx->GetExceptionString());
        }
        return 1;
    }

    asDWORD ret = ctx->GetReturnDWord();
    int32_t rval = ret;

    return rval;
}

//==== Return Script Content Given Module Name ====//
string ScriptMgrSingleton::FindModuleContent(const string &module_name)
{
    map<string, string>::iterator iter;
    iter = m_ModuleContentMap.find(module_name);

    string file_string;
    if (iter != m_ModuleContentMap.end())
    {
        file_string = iter->second;
    }
    return file_string;
}

//==== Write Script Content To File ====//
int ScriptMgrSingleton::SaveScriptContentToFile(const string &module_name, const string &file_name)
{
    map<string, string>::iterator iter;
    iter = m_ModuleContentMap.find(module_name);

    if (iter == m_ModuleContentMap.end())
        return -1;

    FILE *fp = fopen(file_name.c_str(), "w");
    if (!fp)
        return -2;

    if (iter->second.size() == 0)
        return -3;

    fprintf(fp, "%s", iter->second.c_str());
    fclose(fp);

    return 0;
}

//==== Find Includes And Replace With Included Code ====//
string ScriptMgrSingleton::ReplaceIncludes(const string &script_contents, const string &inc_file_path)
{
    vector<string::size_type> start_pos_vec;
    vector<string::size_type> end_pos_vec;
    vector<string> file_content_vec;

    string::size_type find_pos = 0;

    string ret_content;

    while (1)
    {
        //==== Find Include ====//
        find_pos = script_contents.find("#include", find_pos);
        if (find_pos == std::string::npos)
            break;

        string::size_type first_quote = script_contents.find('"', find_pos + 8);
        if (first_quote == std::string::npos)
            break;

        string::size_type second_quote = script_contents.find('"', first_quote + 1);
        if (second_quote == std::string::npos)
            break;

        start_pos_vec.push_back(find_pos);
        end_pos_vec.push_back(second_quote);

        string inc_file_name = script_contents.substr(first_quote + 1, second_quote - first_quote - 1);

        string full_path = inc_file_path + inc_file_name;
        string content = ExtractContent(full_path);
        file_content_vec.push_back(content);

        find_pos = second_quote + 1;
    }

    //==== No Includes ====//
    if (file_content_vec.size() == 0)
        return script_contents;

    string::size_type curr_pos = 0;
    for (int i = 0; i < (int)file_content_vec.size(); i++)
    {
        string::size_type s = start_pos_vec[i];
        string::size_type e = end_pos_vec[i];
        ret_content.append(script_contents.substr(curr_pos, s - curr_pos));

        ret_content.append("// Begin Include Replacement\n");
        ret_content.append("//");
        ret_content.append(script_contents, s, e - s + 1);
        ret_content.append("\n");
        if (file_content_vec[i].size() > 0)
        {
            ret_content.append(file_content_vec[i]);
        }
        ret_content.append("// End Include Replacement\n");

        curr_pos = end_pos_vec[i] + 1;
    }

    ret_content.append(script_contents.substr(curr_pos, script_contents.size() - curr_pos));

    // FILE * fp = fopen( "TestWrite.txt", "w" );
    // if ( fp )
    //{
    //     fprintf( fp, "%s", ret_content.c_str() );
    //     fclose( fp );
    // };

    return ret_content;
}

//==== Register Enums ====//
void ScriptMgrSingleton::RegisterEnums(asIScriptEngine *se)
{
    // The format for enum comments:
    //! Brief description that appears in overview.
    /*! A more detailed description. */

    //
    string enumname;
    int r;
// Syntactic Sugar for registering enums. Not sure if the trick to get multiline macros hindered compile performance.
// TODO change name so you can still do it manually?
#define RegisterEnum()                          \
    do                                          \
    {                                           \
        r = se->RegisterEnum(enumname.c_str()); \
        assert(r >= 0);                         \
    } while (0)

// Syntactic Sugar for registering enum values. Not sure if the trick to get multiline macros hindered compile performance.
// TODO change name so you can still do it manually?
#define RegisterEnumValue(y)                                \
    do                                                      \
    {                                                       \
        r = se->RegisterEnumValue(enumname.c_str(), #y, y); \
        assert(r >= 0);                                     \
    } while (0)

    asDocInfo doc_struct;
    string group = "";
    doc_struct.group = group.c_str();

    string group_description = "";

    se->AddGroup(group.c_str(), group.c_str(), group_description.c_str());

    enumname = "ABS_REL_FLAG";
    RegisterEnum();
    RegisterEnumValue(ABS);
    RegisterEnumValue(REL);

    enumname = "AIRFOIL_EXPORT_TYPE";
    RegisterEnum();
    RegisterEnumValue(SELIG_AF_EXPORT);
    RegisterEnumValue(BEZIER_AF_EXPORT);

    enumname = "ANG_UNITS";
    RegisterEnum();
    RegisterEnumValue(ANG_RAD);
    RegisterEnumValue(ANG_DEG);

    enumname = "ATMOS_TYPE";
    RegisterEnum();
    RegisterEnumValue(ATMOS_TYPE_US_STANDARD_1976);
    RegisterEnumValue(ATMOS_TYPE_HERRINGTON_1966);
    RegisterEnumValue(ATMOS_TYPE_MANUAL_P_R);
    RegisterEnumValue(ATMOS_TYPE_MANUAL_P_T);
    RegisterEnumValue(ATMOS_TYPE_MANUAL_R_T);
    RegisterEnumValue(ATMOS_TYPE_MANUAL_RE_L);

    enumname = "ATTACH_TRANS_TYPE";
    RegisterEnum();
    RegisterEnumValue(ATTACH_TRANS_NONE);
    RegisterEnumValue(ATTACH_TRANS_COMP);
    RegisterEnumValue(ATTACH_TRANS_UV);

    enumname = "ATTACH_ROT_TYPE";
    RegisterEnum();
    RegisterEnumValue(ATTACH_ROT_NONE);
    RegisterEnumValue(ATTACH_ROT_COMP);
    RegisterEnumValue(ATTACH_ROT_UV);

    enumname = "BOR_MODE";
    RegisterEnum();
    RegisterEnumValue(BOR_FLOWTHROUGH);
    RegisterEnumValue(BOR_UPPER);
    RegisterEnumValue(BOR_LOWER);
    RegisterEnumValue(BOR_NUM_MODES);

    enumname = "CAMBER_INPUT_FLAG";
    RegisterEnum();
    RegisterEnumValue(MAX_CAMB);
    RegisterEnumValue(DESIGN_CL);

    enumname = "CAP_TYPE";
    RegisterEnum();
    RegisterEnumValue(NO_END_CAP);
    RegisterEnumValue(FLAT_END_CAP);
    RegisterEnumValue(ROUND_END_CAP);
    RegisterEnumValue(EDGE_END_CAP);
    RegisterEnumValue(SHARP_END_CAP);
    RegisterEnumValue(NUM_END_CAP_OPTIONS);

    enumname = "CFD_CONTROL_TYPE";
    RegisterEnum();
    RegisterEnumValue(CFD_MIN_EDGE_LEN);
    RegisterEnumValue(CFD_MAX_EDGE_LEN);
    RegisterEnumValue(CFD_MAX_GAP);
    RegisterEnumValue(CFD_NUM_CIRCLE_SEGS);
    RegisterEnumValue(CFD_GROWTH_RATIO);
    RegisterEnumValue(CFD_LIMIT_GROWTH_FLAG);
    RegisterEnumValue(CFD_INTERSECT_SUBSURFACE_FLAG);
    RegisterEnumValue(CFD_HALF_MESH_FLAG);
    RegisterEnumValue(CFD_FAR_FIELD_FLAG);
    RegisterEnumValue(CFD_FAR_MAX_EDGE_LEN);
    RegisterEnumValue(CFD_FAR_MAX_GAP);
    RegisterEnumValue(CFD_FAR_NUM_CIRCLE_SEGS);
    RegisterEnumValue(CFD_FAR_SIZE_ABS_FLAG);
    RegisterEnumValue(CFD_FAR_LENGTH);
    RegisterEnumValue(CFD_FAR_WIDTH);
    RegisterEnumValue(CFD_FAR_HEIGHT);
    RegisterEnumValue(CFD_FAR_X_SCALE);
    RegisterEnumValue(CFD_FAR_Y_SCALE);
    RegisterEnumValue(CFD_FAR_Z_SCALE);
    RegisterEnumValue(CFD_FAR_LOC_MAN_FLAG);
    RegisterEnumValue(CFD_FAR_LOC_X);
    RegisterEnumValue(CFD_FAR_LOC_Y);
    RegisterEnumValue(CFD_FAR_LOC_Z);
    RegisterEnumValue(CFD_SRF_XYZ_FLAG);

    enumname = "CFD_MESH_EXPORT_TYPE";
    RegisterEnum();
    RegisterEnumValue(CFD_STL_FILE_NAME);
    RegisterEnumValue(CFD_POLY_FILE_NAME);
    RegisterEnumValue(CFD_TRI_FILE_NAME);
    RegisterEnumValue(CFD_OBJ_FILE_NAME);
    RegisterEnumValue(CFD_DAT_FILE_NAME);
    RegisterEnumValue(CFD_KEY_FILE_NAME);
    RegisterEnumValue(CFD_GMSH_FILE_NAME);
    RegisterEnumValue(CFD_TKEY_FILE_NAME);
    RegisterEnumValue(CFD_FACET_FILE_NAME);
    RegisterEnumValue(CFD_VSPGEOM_FILE_NAME);
    RegisterEnumValue(CFD_NUM_FILE_NAMES);
    RegisterEnumValue(CFD_SRF_FILE_NAME);
    RegisterEnumValue(CFD_CURV_FILE_NAME);
    RegisterEnumValue(CFD_PLOT3D_FILE_NAME);

    enumname = "CFD_MESH_SOURCE_TYPE";
    RegisterEnum();
    RegisterEnumValue(POINT_SOURCE);
    RegisterEnumValue(LINE_SOURCE);
    RegisterEnumValue(BOX_SOURCE);
    RegisterEnumValue(ULINE_SOURCE);
    RegisterEnumValue(WLINE_SOURCE);
    RegisterEnumValue(NUM_SOURCE_TYPES);

    enumname = "CF_LAM_EQN";
    RegisterEnum();
    RegisterEnumValue(CF_LAM_BLASIUS);
    RegisterEnumValue(CF_LAM_BLASIUS_W_HEAT); // TODO: Remove or implement

    enumname = "CF_TURB_EQN";
    RegisterEnum();
    RegisterEnumValue(CF_TURB_EXPLICIT_FIT_SPALDING);
    RegisterEnumValue(CF_TURB_EXPLICIT_FIT_SPALDING_CHI);
    RegisterEnumValue(CF_TURB_EXPLICIT_FIT_SCHOENHERR);
    RegisterEnumValue(DO_NOT_USE_CF_TURB_IMPLICIT_KARMAN);
    RegisterEnumValue(CF_TURB_IMPLICIT_SCHOENHERR);
    RegisterEnumValue(CF_TURB_IMPLICIT_KARMAN_SCHOENHERR);
    RegisterEnumValue(CF_TURB_POWER_LAW_BLASIUS);
    RegisterEnumValue(CF_TURB_POWER_LAW_PRANDTL_LOW_RE);
    RegisterEnumValue(CF_TURB_POWER_LAW_PRANDTL_MEDIUM_RE);
    RegisterEnumValue(CF_TURB_POWER_LAW_PRANDTL_HIGH_RE);
    RegisterEnumValue(CF_TURB_SCHLICHTING_COMPRESSIBLE);
    RegisterEnumValue(DO_NOT_USE_CF_TURB_SCHLICHTING_INCOMPRESSIBLE);
    RegisterEnumValue(DO_NOT_USE_CF_TURB_SCHLICHTING_PRANDTL);
    RegisterEnumValue(DO_NOT_USE_CF_TURB_SCHULTZ_GRUNOW_HIGH_RE);
    RegisterEnumValue(CF_TURB_SCHULTZ_GRUNOW_SCHOENHERR);
    RegisterEnumValue(DO_NOT_USE_CF_TURB_WHITE_CHRISTOPH_COMPRESSIBLE);
    RegisterEnumValue(CF_TURB_ROUGHNESS_SCHLICHTING_AVG);
    RegisterEnumValue(DO_NOT_USE_CF_TURB_ROUGHNESS_SCHLICHTING_LOCAL);
    RegisterEnumValue(DO_NOT_USE_CF_TURB_ROUGHNESS_WHITE);
    RegisterEnumValue(CF_TURB_ROUGHNESS_SCHLICHTING_AVG_FLOW_CORRECTION);
    RegisterEnumValue(CF_TURB_HEATTRANSFER_WHITE_CHRISTOPH);

    enumname = "CHEVRON_TYPE";
    RegisterEnum();
    RegisterEnumValue(CHEVRON_NONE);
    RegisterEnumValue(CHEVRON_PARTIAL);
    RegisterEnumValue(CHEVRON_FULL);
    RegisterEnumValue(CHEVRON_NUM_TYPES);

    enumname = "CHEVRON_W01_MODES";
    RegisterEnum();
    RegisterEnumValue(CHEVRON_W01_SE);
    RegisterEnumValue(CHEVRON_W01_CW);
    RegisterEnumValue(CHEVRON_W01_NUM_MODES);

    enumname = "COLLISION_ERRORS";
    RegisterEnum();
    RegisterEnumValue(COLLISION_OK);
    RegisterEnumValue(COLLISION_INTERSECT_NO_SOLUTION);
    RegisterEnumValue(COLLISION_CLEAR_NO_SOLUTION);

    enumname = "COMPUTATION_FILE_TYPE";
    RegisterEnum();
    RegisterEnumValue(NO_FILE_TYPE);
    RegisterEnumValue(COMP_GEOM_TXT_TYPE);
    RegisterEnumValue(COMP_GEOM_CSV_TYPE);
    RegisterEnumValue(SLICE_TXT_TYPE);
    RegisterEnumValue(MASS_PROP_TXT_TYPE);
    RegisterEnumValue(DEGEN_GEOM_CSV_TYPE);
    RegisterEnumValue(DEGEN_GEOM_M_TYPE);
    RegisterEnumValue(CFD_STL_TYPE);
    RegisterEnumValue(CFD_POLY_TYPE);
    RegisterEnumValue(CFD_TRI_TYPE);
    RegisterEnumValue(CFD_OBJ_TYPE);
    RegisterEnumValue(CFD_DAT_TYPE);
    RegisterEnumValue(CFD_KEY_TYPE);
    RegisterEnumValue(CFD_GMSH_TYPE);
    RegisterEnumValue(CFD_TKEY_TYPE);
    RegisterEnumValue(PROJ_AREA_CSV_TYPE);
    RegisterEnumValue(WAVE_DRAG_TXT_TYPE);
    RegisterEnumValue(VSPAERO_PANEL_TRI_TYPE);
    RegisterEnumValue(DRAG_BUILD_CSV_TYPE);
    RegisterEnumValue(CFD_FACET_TYPE);
    RegisterEnumValue(CFD_VSPGEOM_TYPE);
    RegisterEnumValue(VSPAERO_VSPGEOM_TYPE);
    RegisterEnumValue(CFD_SRF_TYPE);
    RegisterEnumValue(CFD_CURV_TYPE);
    RegisterEnumValue(CFD_PLOT3D_TYPE);

    enumname = "DELIM_TYPE";
    RegisterEnum();
    RegisterEnumValue(DELIM_COMMA);
    RegisterEnumValue(DELIM_USCORE);
    RegisterEnumValue(DELIM_SPACE);
    RegisterEnumValue(DELIM_NONE);
    RegisterEnumValue(DELIM_NUM_TYPES);

    enumname = "DIMENSION_SET";
    RegisterEnum();
    RegisterEnumValue(SET_3D);
    RegisterEnumValue(SET_2D);

    enumname = "DIR_INDEX";
    RegisterEnum();
    RegisterEnumValue(X_DIR);
    RegisterEnumValue(Y_DIR);
    RegisterEnumValue(Z_DIR);
    RegisterEnumValue(ALL_DIR);

    enumname = "DISPLAY_TYPE";
    RegisterEnum();
    RegisterEnumValue(DISPLAY_BEZIER);
    RegisterEnumValue(DISPLAY_DEGEN_SURF);
    RegisterEnumValue(DISPLAY_DEGEN_PLATE);
    RegisterEnumValue(DISPLAY_DEGEN_CAMBER);

    enumname = "DRAW_TYPE";
    RegisterEnum();
    RegisterEnumValue(GEOM_DRAW_WIRE);
    RegisterEnumValue(GEOM_DRAW_HIDDEN);
    RegisterEnumValue(GEOM_DRAW_SHADE);
    RegisterEnumValue(GEOM_DRAW_TEXTURE);
    RegisterEnumValue(GEOM_DRAW_NONE);

    enumname = "ERROR_CODE";
    RegisterEnum();
    RegisterEnumValue(VSP_OK);
    RegisterEnumValue(VSP_INVALID_PTR);
    RegisterEnumValue(VSP_INVALID_TYPE);
    RegisterEnumValue(VSP_CANT_FIND_TYPE);
    RegisterEnumValue(VSP_CANT_FIND_PARM);
    RegisterEnumValue(VSP_CANT_FIND_NAME);
    RegisterEnumValue(VSP_INVALID_GEOM_ID);
    RegisterEnumValue(VSP_FILE_DOES_NOT_EXIST);
    RegisterEnumValue(VSP_FILE_WRITE_FAILURE);
    RegisterEnumValue(VSP_FILE_READ_FAILURE);
    RegisterEnumValue(VSP_WRONG_XSEC_TYPE);
    RegisterEnumValue(VSP_WRONG_FILE_TYPE);
    RegisterEnumValue(VSP_INDEX_OUT_RANGE);
    RegisterEnumValue(VSP_INVALID_XSEC_ID);
    RegisterEnumValue(VSP_INVALID_ID);
    RegisterEnumValue(VSP_CANT_SET_NOT_EQ_PARM);
    RegisterEnumValue(VSP_AMBIGUOUS_SUBSURF);
    RegisterEnumValue(VSP_INVALID_VARPRESET_SETNAME);
    RegisterEnumValue(VSP_INVALID_VARPRESET_GROUPNAME);
    RegisterEnumValue(VSP_CONFORMAL_PARENT_UNSUPPORTED);
    RegisterEnumValue(VSP_UNEXPECTED_RESET_REMAP_ID);
    RegisterEnumValue(VSP_INVALID_INPUT_VAL);
    RegisterEnumValue(VSP_INVALID_CF_EQN);
    RegisterEnumValue(VSP_INVALID_DRIVERS);
    RegisterEnumValue(VSP_ADV_LINK_BUILD_FAIL);

    // RegisterEnumValue("ERROR_CODE", "VSP_DEPRECATED", vsp::VSP_DEPRECATED);
    //
    enumname = "EXCRES_TYPE";
    RegisterEnum();
    RegisterEnumValue(EXCRESCENCE_COUNT);
    RegisterEnumValue(EXCRESCENCE_CD);
    RegisterEnumValue(EXCRESCENCE_PERCENT_GEOM);
    RegisterEnumValue(EXCRESCENCE_MARGIN);
    RegisterEnumValue(EXCRESCENCE_DRAGAREA);

    enumname = "EXPORT_TYPE";
    RegisterEnum();
    RegisterEnumValue(EXPORT_FELISA); // TODO: Remove or implement
    RegisterEnumValue(EXPORT_XSEC);
    RegisterEnumValue(EXPORT_STL);
    RegisterEnumValue(EXPORT_AWAVE); // TODO: Remove or implement
    RegisterEnumValue(EXPORT_NASCART);
    RegisterEnumValue(EXPORT_POVRAY);
    RegisterEnumValue(EXPORT_CART3D);
    RegisterEnumValue(EXPORT_VSPGEOM);
    RegisterEnumValue(EXPORT_VORXSEC);  // TODO: Remove or implement
    RegisterEnumValue(EXPORT_XSECGEOM); // TODO: Remove or implement
    RegisterEnumValue(EXPORT_GMSH);
    RegisterEnumValue(EXPORT_X3D);
    RegisterEnumValue(EXPORT_STEP);
    RegisterEnumValue(EXPORT_PLOT3D);
    RegisterEnumValue(EXPORT_IGES);
    RegisterEnumValue(EXPORT_BEM);
    RegisterEnumValue(EXPORT_DXF);
    RegisterEnumValue(EXPORT_FACET);
    RegisterEnumValue(EXPORT_SVG);
    RegisterEnumValue(EXPORT_PMARC);
    RegisterEnumValue(EXPORT_OBJ);
    RegisterEnumValue(EXPORT_SELIG_AIRFOIL);
    RegisterEnumValue(EXPORT_BEZIER_AIRFOIL);
    RegisterEnumValue(EXPORT_IGES_STRUCTURE);
    RegisterEnumValue(EXPORT_STEP_STRUCTURE);

    enumname = "FEA_CROSS_SECT_TYPE";
    RegisterEnum();
    RegisterEnumValue(FEA_XSEC_GENERAL);
    RegisterEnumValue(FEA_XSEC_CIRC);
    RegisterEnumValue(FEA_XSEC_PIPE);
    RegisterEnumValue(FEA_XSEC_I);
    RegisterEnumValue(FEA_XSEC_RECT);
    RegisterEnumValue(FEA_XSEC_BOX);

    enumname = "FEA_EXPORT_TYPE";
    RegisterEnum();
    RegisterEnumValue(FEA_MASS_FILE_NAME);
    RegisterEnumValue(FEA_NASTRAN_FILE_NAME);
    RegisterEnumValue(FEA_NKEY_FILE_NAME);
    RegisterEnumValue(FEA_CALCULIX_FILE_NAME);
    RegisterEnumValue(FEA_STL_FILE_NAME);
    RegisterEnumValue(FEA_GMSH_FILE_NAME);
    RegisterEnumValue(FEA_SRF_FILE_NAME);
    RegisterEnumValue(FEA_CURV_FILE_NAME);
    RegisterEnumValue(FEA_PLOT3D_FILE_NAME);
    RegisterEnumValue(FEA_IGES_FILE_NAME);
    RegisterEnumValue(FEA_STEP_FILE_NAME);
    RegisterEnumValue(FEA_NUM_FILE_NAMES);

    enumname = "FEA_MATERIAL_TYPE";
    RegisterEnum();
    RegisterEnumValue(FEA_ISOTROPIC);
    RegisterEnumValue(FEA_ENG_ORTHO);
    RegisterEnumValue(FEA_NUM_MAT_TYPES);

    enumname = "FEA_ORIENTATION_TYPE";
    RegisterEnum();
    RegisterEnumValue(FEA_ORIENT_GLOBAL_X);
    RegisterEnumValue(FEA_ORIENT_GLOBAL_Y);
    RegisterEnumValue(FEA_ORIENT_GLOBAL_Z);
    RegisterEnumValue(FEA_ORIENT_COMP_X);
    RegisterEnumValue(FEA_ORIENT_COMP_Y);
    RegisterEnumValue(FEA_ORIENT_COMP_Z);
    RegisterEnumValue(FEA_ORIENT_PART_U);
    RegisterEnumValue(FEA_ORIENT_PART_V);
    RegisterEnumValue(FEA_ORIENT_OML_U);
    RegisterEnumValue(FEA_ORIENT_OML_V);
    RegisterEnumValue(FEA_ORIENT_OML_R);
    RegisterEnumValue(FEA_ORIENT_OML_S);
    RegisterEnumValue(FEA_ORIENT_OML_T);
    RegisterEnumValue(FEA_NUM_ORIENT_TYPES);

    enumname = "FEA_PART_ELEMENT_TYPE";
    RegisterEnum();
    RegisterEnumValue(FEA_SHELL);
    RegisterEnumValue(FEA_BEAM);
    RegisterEnumValue(FEA_SHELL_AND_BEAM);
    RegisterEnumValue(FEA_NO_ELEMENTS);
    RegisterEnumValue(FEA_NUM_ELEMENT_TYPES);

    enumname = "FEA_PART_TYPE";
    RegisterEnum();
    RegisterEnumValue(FEA_SLICE);
    RegisterEnumValue(FEA_RIB);
    RegisterEnumValue(FEA_SPAR);
    RegisterEnumValue(FEA_FIX_POINT);
    RegisterEnumValue(FEA_DOME);
    RegisterEnumValue(FEA_RIB_ARRAY);
    RegisterEnumValue(FEA_SLICE_ARRAY);
    RegisterEnumValue(FEA_TRIM);
    RegisterEnumValue(FEA_SKIN);
    RegisterEnumValue(FEA_NUM_TYPES);

    enumname = "FEA_SLICE_TYPE";
    RegisterEnum();
    RegisterEnumValue(XY_BODY);
    RegisterEnumValue(YZ_BODY);
    RegisterEnumValue(XZ_BODY);
    RegisterEnumValue(XY_ABS);
    RegisterEnumValue(YZ_ABS);
    RegisterEnumValue(XZ_ABS);
    RegisterEnumValue(SPINE_NORMAL);

    enumname = "FEA_UNIT_TYPE";
    RegisterEnum();
    RegisterEnumValue(SI_UNIT);
    RegisterEnumValue(CGS_UNIT);
    RegisterEnumValue(MPA_UNIT);
    RegisterEnumValue(BFT_UNIT);
    RegisterEnumValue(BIN_UNIT);

    enumname = "FEA_RIB_NORMAL";
    RegisterEnum();
    RegisterEnumValue(NO_NORMAL);
    RegisterEnumValue(LE_NORMAL);
    RegisterEnumValue(TE_NORMAL);
    RegisterEnumValue(SPAR_NORMAL);

    enumname = "FF_B_EQN";
    RegisterEnum();
    RegisterEnumValue(FF_B_MANUAL);
    RegisterEnumValue(FF_B_SCHEMENSKY_FUSE);
    RegisterEnumValue(FF_B_SCHEMENSKY_NACELLE);
    RegisterEnumValue(FF_B_HOERNER_STREAMBODY);
    RegisterEnumValue(FF_B_TORENBEEK);
    RegisterEnumValue(FF_B_SHEVELL);
    RegisterEnumValue(FF_B_COVERT);
    RegisterEnumValue(FF_B_JENKINSON_FUSE);
    RegisterEnumValue(FF_B_JENKINSON_WING_NACELLE);
    RegisterEnumValue(FF_B_JENKINSON_AFT_FUSE_NACELLE);

    enumname = "FF_W_EQN";
    RegisterEnum();
    RegisterEnumValue(FF_W_MANUAL);
    RegisterEnumValue(FF_W_EDET_CONV);
    RegisterEnumValue(FF_W_EDET_ADV);
    RegisterEnumValue(FF_W_HOERNER);
    RegisterEnumValue(FF_W_COVERT);
    RegisterEnumValue(FF_W_SHEVELL);
    RegisterEnumValue(FF_W_KROO);
    RegisterEnumValue(FF_W_TORENBEEK);
    RegisterEnumValue(FF_W_DATCOM);
    RegisterEnumValue(FF_W_SCHEMENSKY_6_SERIES_AF);
    RegisterEnumValue(FF_W_SCHEMENSKY_4_SERIES_AF);
    RegisterEnumValue(FF_W_JENKINSON_WING);
    RegisterEnumValue(FF_W_JENKINSON_TAIL);
    RegisterEnumValue(FF_W_SCHEMENSKY_SUPERCRITICAL_AF);

    enumname = "FREESTREAM_PD_UNITS";
    RegisterEnum();
    RegisterEnumValue(PD_UNITS_IMPERIAL);
    RegisterEnumValue(PD_UNITS_METRIC);

    enumname = "GDEV";
    RegisterEnum();
    RegisterEnumValue(GDEV_TAB);
    RegisterEnumValue(GDEV_SCROLL_TAB);
    RegisterEnumValue(GDEV_GROUP);
    RegisterEnumValue(GDEV_PARM_BUTTON);
    RegisterEnumValue(GDEV_INPUT);
    RegisterEnumValue(GDEV_OUTPUT);
    RegisterEnumValue(GDEV_SLIDER);
    RegisterEnumValue(GDEV_SLIDER_ADJ_RANGE);
    RegisterEnumValue(GDEV_CHECK_BUTTON);
    RegisterEnumValue(GDEV_RADIO_BUTTON);
    RegisterEnumValue(GDEV_TOGGLE_BUTTON);
    RegisterEnumValue(GDEV_TOGGLE_RADIO_GROUP); // TODO: Implement or remove
    RegisterEnumValue(GDEV_TRIGGER_BUTTON);
    RegisterEnumValue(GDEV_COUNTER);
    RegisterEnumValue(GDEV_CHOICE);
    RegisterEnumValue(GDEV_ADD_CHOICE_ITEM);
    RegisterEnumValue(GDEV_SLIDER_INPUT);
    RegisterEnumValue(GDEV_SLIDER_ADJ_RANGE_INPUT);
    RegisterEnumValue(GDEV_SLIDER_ADJ_RANGE_TWO_INPUT); // TODO: Implement or remove
    RegisterEnumValue(GDEV_FRACT_PARM_SLIDER);
    RegisterEnumValue(GDEV_STRING_INPUT);
    RegisterEnumValue(GDEV_INDEX_SELECTOR);
    RegisterEnumValue(GDEV_COLOR_PICKER);
    RegisterEnumValue(GDEV_YGAP);
    RegisterEnumValue(GDEV_DIVIDER_BOX);
    RegisterEnumValue(GDEV_BEGIN_SAME_LINE);
    RegisterEnumValue(GDEV_END_SAME_LINE);
    RegisterEnumValue(GDEV_FORCE_WIDTH);
    RegisterEnumValue(GDEV_SET_FORMAT);

    enumname = "GENDER";
    RegisterEnum();
    RegisterEnumValue(MALE);
    RegisterEnumValue(FEMALE);

    enumname = "INIT_EDIT_XSEC_TYPE";
    RegisterEnum();
    RegisterEnumValue(EDIT_XSEC_CIRCLE);
    RegisterEnumValue(EDIT_XSEC_ELLIPSE);
    RegisterEnumValue(EDIT_XSEC_RECTANGLE);

    enumname = "IMPORT_TYPE";
    RegisterEnum();
    RegisterEnumValue(IMPORT_STL);
    RegisterEnumValue(IMPORT_NASCART);
    RegisterEnumValue(IMPORT_CART3D_TRI);
    RegisterEnumValue(IMPORT_XSEC_MESH);
    RegisterEnumValue(IMPORT_PTS);
    RegisterEnumValue(IMPORT_V2);
    RegisterEnumValue(IMPORT_BEM);
    RegisterEnumValue(IMPORT_XSEC_WIRE);
    RegisterEnumValue(IMPORT_P3D_WIRE);

    enumname = "INTERSECT_EXPORT_TYPE";
    RegisterEnum();
    RegisterEnumValue(INTERSECT_SRF_FILE_NAME);
    RegisterEnumValue(INTERSECT_CURV_FILE_NAME);
    RegisterEnumValue(INTERSECT_PLOT3D_FILE_NAME);
    RegisterEnumValue(INTERSECT_IGES_FILE_NAME);
    RegisterEnumValue(INTERSECT_STEP_FILE_NAME);
    RegisterEnumValue(INTERSECT_NUM_FILE_NAMES);

    enumname = "LEN_UNITS";
    RegisterEnum();
    RegisterEnumValue(LEN_MM);
    RegisterEnumValue(LEN_CM);
    RegisterEnumValue(LEN_M);
    RegisterEnumValue(LEN_IN);
    RegisterEnumValue(LEN_FT);
    RegisterEnumValue(LEN_YD);
    RegisterEnumValue(LEN_UNITLESS);

    enumname = "MASS_UNIT";
    RegisterEnum();
    RegisterEnumValue(MASS_UNIT_G);
    RegisterEnumValue(MASS_UNIT_KG);
    RegisterEnumValue(MASS_UNIT_TONNE);
    RegisterEnumValue(MASS_UNIT_LBM);
    RegisterEnumValue(MASS_UNIT_SLUG);
    RegisterEnumValue(MASS_LBFSEC2IN);

    enumname = "PARM_TYPE";
    RegisterEnum();
    RegisterEnumValue(PARM_DOUBLE_TYPE);
    RegisterEnumValue(PARM_INT_TYPE);
    RegisterEnumValue(PARM_BOOL_TYPE);
    RegisterEnumValue(PARM_FRACTION_TYPE);
    RegisterEnumValue(PARM_LIMITED_INT_TYPE);
    RegisterEnumValue(PARM_NOTEQ_TYPE);

    enumname = "PATCH_TYPE";
    RegisterEnum();
    RegisterEnumValue(PATCH_NONE);
    RegisterEnumValue(PATCH_POINT);
    RegisterEnumValue(PATCH_LINE);
    RegisterEnumValue(PATCH_COPY);
    RegisterEnumValue(PATCH_HALFWAY);
    RegisterEnumValue(PATCH_NUM_TYPES);

    enumname = "PCURV_TYPE";
    RegisterEnum();
    RegisterEnumValue(LINEAR);
    RegisterEnumValue(PCHIP);
    RegisterEnumValue(CEDIT);
    RegisterEnumValue(APPROX_CEDIT);
    RegisterEnumValue(NUM_PCURV_TYPE);

    enumname = "PRES_UNITS";
    RegisterEnum();
    RegisterEnumValue(PRES_UNIT_PSF);
    RegisterEnumValue(PRES_UNIT_PSI);
    RegisterEnumValue(PRES_UNIT_BA);
    RegisterEnumValue(PRES_UNIT_PA);
    RegisterEnumValue(PRES_UNIT_KPA);
    RegisterEnumValue(PRES_UNIT_MPA);
    RegisterEnumValue(PRES_UNIT_INCHHG);
    RegisterEnumValue(PRES_UNIT_MMHG);
    RegisterEnumValue(PRES_UNIT_MMH20);
    RegisterEnumValue(PRES_UNIT_MB);
    RegisterEnumValue(PRES_UNIT_ATM);

    enumname = "PROJ_BNDY_TYPE";
    RegisterEnum();
    RegisterEnumValue(NO_BOUNDARY);
    RegisterEnumValue(SET_BOUNDARY);
    RegisterEnumValue(GEOM_BOUNDARY);
    RegisterEnumValue(NUM_PROJ_BNDY_OPTIONS);

    enumname = "PROJ_DIR_TYPE";
    RegisterEnum();
    RegisterEnumValue(X_PROJ);
    RegisterEnumValue(Y_PROJ);
    RegisterEnumValue(Z_PROJ);
    RegisterEnumValue(GEOM_PROJ);
    RegisterEnumValue(VEC_PROJ);
    RegisterEnumValue(NUM_PROJ_DIR_OPTIONS);

    enumname = "PROJ_TGT_TYPE";
    RegisterEnum();
    RegisterEnumValue(SET_TARGET);
    RegisterEnumValue(GEOM_TARGET);
    RegisterEnumValue(NUM_PROJ_TGT_OPTIONS);

    enumname = "PROP_MODE";
    RegisterEnum();
    RegisterEnumValue(PROP_BLADES);
    RegisterEnumValue(PROP_BOTH);
    RegisterEnumValue(PROP_DISK);

    enumname = "PROP_PCURVE";
    RegisterEnum();
    RegisterEnumValue(PROP_CHORD);
    RegisterEnumValue(PROP_TWIST);
    RegisterEnumValue(PROP_RAKE);
    RegisterEnumValue(PROP_SKEW);
    RegisterEnumValue(PROP_SWEEP);
    RegisterEnumValue(PROP_THICK);
    RegisterEnumValue(PROP_CLI);
    RegisterEnumValue(PROP_AXIAL);
    RegisterEnumValue(PROP_TANGENTIAL);
    RegisterEnumValue(NUM_PROP_PCURVE);

    enumname = "REF_WING_TYPE";
    RegisterEnum();
    RegisterEnumValue(MANUAL_REF);
    RegisterEnumValue(COMPONENT_REF);
    RegisterEnumValue(NUM_REF_TYPES);

    enumname = "RES_DATA_TYPE";
    RegisterEnum();
    RegisterEnumValue(INVALID_TYPE);
    RegisterEnumValue(INT_DATA);
    RegisterEnumValue(DOUBLE_DATA);
    RegisterEnumValue(STRING_DATA);
    RegisterEnumValue(VEC3D_DATA);
    RegisterEnumValue(DOUBLE_MATRIX_DATA);

    enumname = "RES_GEOM_TYPE";
    RegisterEnum();
    RegisterEnumValue(MESH_INDEXED_TRI);
    RegisterEnumValue(MESH_SLICE_TRI);
    RegisterEnumValue(GEOM_XSECS);
    RegisterEnumValue(MESH_INDEX_AND_SLICE_TRI);

    enumname = "RHO_UNITS";
    RegisterEnum();
    RegisterEnumValue(RHO_UNIT_SLUG_FT3);
    RegisterEnumValue(RHO_UNIT_G_CM3);
    RegisterEnumValue(RHO_UNIT_KG_M3);
    RegisterEnumValue(RHO_UNIT_TONNE_MM3);
    RegisterEnumValue(RHO_UNIT_LBF_FT3);
    RegisterEnumValue(RHO_UNIT_LBFSEC2_IN4);

    enumname = "SET_TYPE";
    RegisterEnum();
    RegisterEnumValue(SET_NONE);
    RegisterEnumValue(SET_ALL);
    RegisterEnumValue(SET_SHOWN);
    RegisterEnumValue(SET_NOT_SHOWN);
    RegisterEnumValue(SET_FIRST_USER);

    enumname = "STEP_REPRESENTATION";
    RegisterEnum();
    RegisterEnumValue(STEP_SHELL);
    RegisterEnumValue(STEP_BREP);

    enumname = "SUBSURF_INCLUDE";
    RegisterEnum();
    RegisterEnumValue(SS_INC_TREAT_AS_PARENT);
    RegisterEnumValue(SS_INC_SEPARATE_TREATMENT);
    RegisterEnumValue(SS_INC_ZERO_DRAG);

    enumname = "SUBSURF_INOUT";
    RegisterEnum();
    RegisterEnumValue(INSIDE);
    RegisterEnumValue(OUTSIDE);
    RegisterEnumValue(NONE);

    enumname = "SUBSURF_LINE_TYPE";
    RegisterEnum();
    RegisterEnumValue(CONST_U);
    RegisterEnumValue(CONST_W);

    enumname = "SUBSURF_TYPE";
    RegisterEnum();
    RegisterEnumValue(SS_LINE);
    RegisterEnumValue(SS_RECTANGLE);
    RegisterEnumValue(SS_ELLIPSE);
    RegisterEnumValue(SS_CONTROL);
    RegisterEnumValue(SS_LINE_ARRAY);
    RegisterEnumValue(SS_FINITE_LINE);
    RegisterEnumValue(SS_NUM_TYPES);

    enumname = "SYM_FLAG";
    RegisterEnum();
    RegisterEnumValue(SYM_XY);
    RegisterEnumValue(SYM_XZ);
    RegisterEnumValue(SYM_YZ);
    RegisterEnumValue(SYM_ROT_X);
    RegisterEnumValue(SYM_ROT_Y);
    RegisterEnumValue(SYM_ROT_Z);
    RegisterEnumValue(SYM_PLANAR_TYPES);
    RegisterEnumValue(SYM_NUM_TYPES);

    enumname = "SYM_XSEC_TYPE";
    RegisterEnum();
    RegisterEnumValue(SYM_NONE);
    RegisterEnumValue(SYM_RL);
    RegisterEnumValue(SYM_TB);
    RegisterEnumValue(SYM_ALL);

    enumname = "TEMP_UNITS";
    RegisterEnum();
    RegisterEnumValue(TEMP_UNIT_K);
    RegisterEnumValue(TEMP_UNIT_C);
    RegisterEnumValue(TEMP_UNIT_F);
    RegisterEnumValue(TEMP_UNIT_R);

    enumname = "VEL_UNITS";
    RegisterEnum();
    RegisterEnumValue(V_UNIT_FT_S);
    RegisterEnumValue(V_UNIT_M_S);
    RegisterEnumValue(V_UNIT_MPH);
    RegisterEnumValue(V_UNIT_KM_HR);
    RegisterEnumValue(V_UNIT_KEAS);
    RegisterEnumValue(V_UNIT_KTAS);
    RegisterEnumValue(V_UNIT_MACH);

    enumname = "VIEW_NUM";
    RegisterEnum();
    RegisterEnumValue(VIEW_1);
    RegisterEnumValue(VIEW_2HOR);
    RegisterEnumValue(VIEW_2VER);
    RegisterEnumValue(VIEW_4);

    enumname = "VIEW_ROT";
    RegisterEnum();
    RegisterEnumValue(ROT_0);
    RegisterEnumValue(ROT_90);
    RegisterEnumValue(ROT_180);
    RegisterEnumValue(ROT_270);

    enumname = "VIEW_TYPE";
    RegisterEnum();
    RegisterEnumValue(VIEW_LEFT);
    RegisterEnumValue(VIEW_RIGHT);
    RegisterEnumValue(VIEW_TOP);
    RegisterEnumValue(VIEW_BOTTOM);
    RegisterEnumValue(VIEW_FRONT);
    RegisterEnumValue(VIEW_REAR);
    RegisterEnumValue(VIEW_NONE);

    enumname = "VSPAERO_ANALYSIS_METHOD";
    RegisterEnum();
    RegisterEnumValue(VORTEX_LATTICE);
    RegisterEnumValue(PANEL);

    enumname = "VSPAERO_NOISE_TYPE";
    RegisterEnum();
    RegisterEnumValue(NOISE_FLYBY);
    RegisterEnumValue(NOISE_FOOTPRINT);
    RegisterEnumValue(NOISE_STEADY);

    enumname = "VSPAERO_NOISE_UNIT";
    RegisterEnum();
    RegisterEnumValue(NOISE_SI);
    RegisterEnumValue(NOISE_ENGLISH);

    enumname = "VSPAERO_PRECONDITION";
    RegisterEnum();
    RegisterEnumValue(PRECON_MATRIX);
    RegisterEnumValue(PRECON_JACOBI);
    RegisterEnumValue(PRECON_SSOR);

    enumname = "VSPAERO_STABILITY_TYPE";
    RegisterEnum();
    RegisterEnumValue(STABILITY_OFF);
    RegisterEnumValue(STABILITY_DEFAULT);
    RegisterEnumValue(STABILITY_P_ANALYSIS);
    RegisterEnumValue(STABILITY_Q_ANALYSIS);
    RegisterEnumValue(STABILITY_R_ANALYSIS);
    RegisterEnumValue(STABILITY_HEAVE);
    RegisterEnumValue(STABILITY_IMPULSE);

    // RegisterEnumValue("VSPAERO_STABILITY_TYPE", "STABILITY_PITCH", STABILITY_PITCH);
    //
    // RegisterEnumValue("VSPAERO_STABILITY_TYPE", "STABILITY_NUM_TYPES", STABILITY_NUM_TYPES);
    //

    enumname = "VSPAERO_CLMAX_TYPE";
    RegisterEnum();
    RegisterEnumValue(CLMAX_OFF);
    RegisterEnumValue(CLMAX_2D);
    RegisterEnumValue(CLMAX_CARLSON);

    enumname = "VSP_SURF_CFD_TYPE";
    RegisterEnum();
    RegisterEnumValue(CFD_NORMAL);
    RegisterEnumValue(CFD_NEGATIVE);
    RegisterEnumValue(CFD_TRANSPARENT);
    RegisterEnumValue(CFD_STRUCTURE);
    RegisterEnumValue(CFD_STIFFENER);
    RegisterEnumValue(CFD_NUM_TYPES);

    enumname = "VSP_SURF_TYPE";
    RegisterEnum();
    RegisterEnumValue(NORMAL_SURF);
    RegisterEnumValue(WING_SURF);
    RegisterEnumValue(DISK_SURF);
    RegisterEnumValue(PROP_SURF);
    RegisterEnumValue(NUM_SURF_TYPES);

    enumname = "WING_BLEND";
    RegisterEnum();
    RegisterEnumValue(BLEND_FREE);
    RegisterEnumValue(BLEND_ANGLES);
    RegisterEnumValue(BLEND_MATCH_IN_LE_TRAP);
    RegisterEnumValue(BLEND_MATCH_IN_TE_TRAP);
    RegisterEnumValue(BLEND_MATCH_OUT_LE_TRAP);
    RegisterEnumValue(BLEND_MATCH_OUT_TE_TRAP);
    RegisterEnumValue(BLEND_MATCH_IN_ANGLES);
    RegisterEnumValue(BLEND_MATCH_LE_ANGLES);
    RegisterEnumValue(BLEND_NUM_TYPES);

    enumname = "WING_DRIVERS";
    RegisterEnum();
    RegisterEnumValue(AR_WSECT_DRIVER);
    RegisterEnumValue(SPAN_WSECT_DRIVER);
    RegisterEnumValue(AREA_WSECT_DRIVER);
    RegisterEnumValue(TAPER_WSECT_DRIVER);
    RegisterEnumValue(AVEC_WSECT_DRIVER);
    RegisterEnumValue(ROOTC_WSECT_DRIVER);
    RegisterEnumValue(TIPC_WSECT_DRIVER);
    RegisterEnumValue(SECSWEEP_WSECT_DRIVER);
    RegisterEnumValue(NUM_WSECT_DRIVER);
    RegisterEnumValue(SWEEP_WSECT_DRIVER);
    RegisterEnumValue(SWEEPLOC_WSECT_DRIVER);
    RegisterEnumValue(SECSWEEPLOC_WSECT_DRIVER);

    enumname = "XDDM_QUANTITY_TYPE";
    RegisterEnum();
    RegisterEnumValue(XDDM_VAR);
    RegisterEnumValue(XDDM_CONST);

    enumname = "XSEC_CLOSE_TYPE";
    RegisterEnum();
    RegisterEnumValue(CLOSE_NONE);
    RegisterEnumValue(CLOSE_SKEWLOW);
    RegisterEnumValue(CLOSE_SKEWUP);
    RegisterEnumValue(CLOSE_SKEWBOTH);
    RegisterEnumValue(CLOSE_EXTRAP);
    RegisterEnumValue(CLOSE_NUM_TYPES);

    enumname = "XSEC_CRV_TYPE";
    RegisterEnum();
    RegisterEnumValue(XS_UNDEFINED);
    RegisterEnumValue(XS_POINT);
    RegisterEnumValue(XS_CIRCLE);
    RegisterEnumValue(XS_ELLIPSE);
    RegisterEnumValue(XS_SUPER_ELLIPSE);
    RegisterEnumValue(XS_ROUNDED_RECTANGLE);
    RegisterEnumValue(XS_GENERAL_FUSE);
    RegisterEnumValue(XS_FILE_FUSE);
    RegisterEnumValue(XS_FOUR_SERIES);
    RegisterEnumValue(XS_SIX_SERIES);
    RegisterEnumValue(XS_BICONVEX);
    RegisterEnumValue(XS_WEDGE);
    RegisterEnumValue(XS_EDIT_CURVE);
    RegisterEnumValue(XS_FILE_AIRFOIL);
    RegisterEnumValue(XS_CST_AIRFOIL);
    RegisterEnumValue(XS_VKT_AIRFOIL);
    RegisterEnumValue(XS_FOUR_DIGIT_MOD);
    RegisterEnumValue(XS_FIVE_DIGIT);
    RegisterEnumValue(XS_FIVE_DIGIT_MOD);
    RegisterEnumValue(XS_ONE_SIX_SERIES);
    RegisterEnumValue(XS_NUM_TYPES);

    enumname = "XSEC_DRIVERS";
    RegisterEnum(); // TODO: improve these comments
    RegisterEnumValue(WIDTH_XSEC_DRIVER);
    RegisterEnumValue(AREA_XSEC_DRIVER);
    RegisterEnumValue(HEIGHT_XSEC_DRIVER);
    RegisterEnumValue(HWRATIO_XSEC_DRIVER);
    RegisterEnumValue(NUM_XSEC_DRIVER);
    RegisterEnumValue(CIRCLE_NUM_XSEC_DRIVER);

    enumname = "XSEC_SIDES_TYPE";
    RegisterEnum(); // TODO: improve these comments
    RegisterEnumValue(XSEC_BOTH_SIDES);
    RegisterEnumValue(XSEC_LEFT_SIDE);
    RegisterEnumValue(XSEC_RIGHT_SIDE);

    enumname = "XSEC_TRIM_TYPE";
    RegisterEnum();
    RegisterEnumValue(TRIM_NONE);
    RegisterEnumValue(TRIM_X);
    RegisterEnumValue(TRIM_THICK);
    RegisterEnumValue(TRIM_NUM_TYPES);

    enumname = "XSEC_TYPE";
    RegisterEnum();
    RegisterEnumValue(XSEC_FUSE);
    RegisterEnumValue(XSEC_STACK);
    RegisterEnumValue(XSEC_WING);
    RegisterEnumValue(XSEC_CUSTOM);
    RegisterEnumValue(XSEC_PROP);
    RegisterEnumValue(XSEC_NUM_TYPES);

    enumname = "XSEC_WIDTH_SHIFT";
    RegisterEnum(); // TODO: improve these comments
    RegisterEnumValue(XS_SHIFT_LE);
    RegisterEnumValue(XS_SHIFT_MID);
    RegisterEnumValue(XS_SHIFT_TE);

//
#undef RegisterEnumValue
}

//==== Vec3d Constructors ====//
static void Vec3dDefaultConstructor(vec3d *self)
{
    new (self) vec3d();
}
static void Vec3dCopyConstructor(const vec3d &other, vec3d *self)
{
    new (self) vec3d(other);
}
static void Vec3dInitConstructor(double x, double y, double z, vec3d *self)
{
    new (self) vec3d(x, y, z);
}

//==== Register Vec3d Object ====//
void ScriptMgrSingleton::RegisterVec3d(asIScriptEngine *se)
{
    asDocInfo doc_struct;
    string group = "";
    string group_description = "";

    se->AddGroup(group.c_str(), "Vec3D Functions", group_description.c_str());

    //==== Register vec3d Object =====//
    int r = se->RegisterObjectType("vec3d", sizeof(vec3d), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA);
    assert(r >= 0);

    //==== Register the vec3d Constructors  ====//
    // Not shown in header
    r = se->RegisterObjectBehaviour("vec3d", asBEHAVE_CONSTRUCT, "void f()", vspFUNCTION(Vec3dDefaultConstructor), vspCALL_CDECL_OBJLAST);
    assert(r >= 0);

    // Not shown in header
    r = se->RegisterObjectBehaviour("vec3d", asBEHAVE_CONSTRUCT, "void f(double, double, double)", vspFUNCTION(Vec3dInitConstructor), vspCALL_CDECL_OBJLAST);
    assert(r >= 0);

    // Not shown in header
    r = se->RegisterObjectBehaviour("vec3d", asBEHAVE_CONSTRUCT, "void f(const vec3d &in)", vspFUNCTION(Vec3dCopyConstructor), vspCALL_CDECL_OBJLAST);
    assert(r >= 0);

    //==== Register the vec3d Methods  ====//

    // Syntactical sugar for doing condensed method registration
#define RegisterObjectMethodvec3d(outsig, operstuff)                                                  \
    do                                                                                                \
    {                                                                                                 \
        r = se->RegisterObjectMethod("vec3d", outsig, vspMETHOD(vec3d, operstuff), vspCALL_THISCALL); \
        assert(r >= 0);                                                                               \
    } while (0)

    r = se->RegisterObjectMethod("vec3d", "double& opIndex(int) const", vspMETHODPR(vec3d, operator[], (int), double &), vspCALL_THISCALL);
    assert(r >= 0);

    RegisterObjectMethodvec3d("double x() const", x);
    RegisterObjectMethodvec3d("double y() const", y);
    RegisterObjectMethodvec3d("double z() const", z);
    RegisterObjectMethodvec3d("vec3d& set_xyz(double x, double y, double z)", set_xyz);
    RegisterObjectMethodvec3d("vec3d& set_x(double x)", set_x);
    RegisterObjectMethodvec3d("vec3d& set_y(double y)", set_y);
    RegisterObjectMethodvec3d("vec3d& set_z(double z)", set_z);
    RegisterObjectMethodvec3d("void rotate_x(double cos_alpha, double sin_alpha)", rotate_x);
    RegisterObjectMethodvec3d("void rotate_y(double cos_alpha, double sin_alpha)", rotate_y);
    RegisterObjectMethodvec3d("void rotate_z(double cos_alpha, double sin_alpha)", rotate_z);
    RegisterObjectMethodvec3d("void scale_x(double scale)", scale_x);
    RegisterObjectMethodvec3d("void scale_y(double scale)", scale_y);
    RegisterObjectMethodvec3d("void scale_z(double scale)", scale_z);
    RegisterObjectMethodvec3d("void offset_x(double offset)", offset_x);
    RegisterObjectMethodvec3d("void offset_y(double offset)", offset_y);
    RegisterObjectMethodvec3d("void offset_z(double offset)", offset_z);
    RegisterObjectMethodvec3d("void rotate_z_zero_x(double cos_alpha, double sin_alpha)", rotate_z_zero_x);
    RegisterObjectMethodvec3d("void rotate_z_zero_y(double cos_alpha, double sin_alpha)", rotate_z_zero_y);
    RegisterObjectMethodvec3d("vec3d reflect_xy()", reflect_xy);
    RegisterObjectMethodvec3d("vec3d reflect_xz()", reflect_xz);
    RegisterObjectMethodvec3d("vec3d reflect_yz()", reflect_yz);

    r = se->RegisterObjectMethod("vec3d", "vec3d opAdd(const vec3d &in) const", vspFUNCTIONPR(operator+, (const vec3d &, const vec3d &), vec3d), vspCALL_CDECL_OBJFIRST);
    assert(r >= 0);

    r = se->RegisterObjectMethod("vec3d", "vec3d opSub(const vec3d &in) const", vspFUNCTIONPR(operator-, (const vec3d &, const vec3d &), vec3d), vspCALL_CDECL_OBJFIRST);
    assert(r >= 0);

    r = se->RegisterObjectMethod("vec3d", "vec3d opMul(double b) const", vspFUNCTIONPR(operator*, (const vec3d &a, double b), vec3d), vspCALL_CDECL_OBJFIRST);
    assert(r >= 0);

    r = se->RegisterObjectMethod("vec3d", "vec3d opMul_r(const vec3d &in) const", vspFUNCTIONPR(operator*, (const vec3d &, const vec3d &), vec3d), vspCALL_CDECL_OBJFIRST);
    assert(r >= 0);

    r = se->RegisterObjectMethod("vec3d", "vec3d opDiv(double b) const", vspFUNCTIONPR(operator/, (const vec3d &, double b), vec3d), vspCALL_CDECL_OBJFIRST);
    assert(r >= 0);

    RegisterObjectMethodvec3d("double mag() const", mag);
    RegisterObjectMethodvec3d("void normalize()", normalize);

    r = se->RegisterGlobalFunction("double dist(const vec3d& in a, const vec3d& in b)", vspFUNCTIONPR(dist, (const vec3d &, const vec3d &), double), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("double dist_squared(const vec3d& in a, const vec3d& in b)", vspFUNCTIONPR(dist_squared, (const vec3d &, const vec3d &), double), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("double dot(const vec3d& in a, const vec3d& in b)", vspFUNCTIONPR(dot, (const vec3d &, const vec3d &), double), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("vec3d cross(const vec3d& in a, const vec3d& in b)", vspFUNCTIONPR(cross, (const vec3d &, const vec3d &), vec3d), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("double angle(const vec3d& in a, const vec3d& in b)", vspFUNCTIONPR(angle, (const vec3d &, const vec3d &), double), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("double signed_angle(const vec3d& in a, const vec3d& in b, const vec3d& in ref )", vspFUNCTIONPR(signed_angle, (const vec3d &a, const vec3d &b, const vec3d &ref), double), vspCALL_CDECL);
    assert(r >= 0);

    // TODO: verify description
    r = se->RegisterGlobalFunction("double cos_angle(const vec3d& in a, const vec3d& in b )", vspFUNCTIONPR(cos_angle, (const vec3d &a, const vec3d &b), double), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("vec3d RotateArbAxis(const vec3d& in p, double theta, const vec3d& in axis )", vspFUNCTIONPR(RotateArbAxis, (const vec3d &p, double theta, const vec3d &axis), vec3d), vspCALL_CDECL);
    assert(r >= 0);
#undef RegisterObjectMethodvec3d
}

//==== Matrix4d Constructors ====//
static void Matrix4dDefaultConstructor(Matrix4d *self)
{
    new (self) Matrix4d();
}

//==== Register Matrix4d Object ====//
void ScriptMgrSingleton::RegisterMatrix4d(asIScriptEngine *se)
{
    asDocInfo doc_struct;
    string group_description = "";
    string group = "";
    se->AddGroup(group.c_str(), "Matrix4d Functions", group_description.c_str());

    //==== Register Matrix4d Object =====//

    int r = se->RegisterObjectType("Matrix4d", sizeof(Matrix4d), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA);
    assert(r >= 0);

    //===== Register the Matrix4d constructor =====//
    r = se->RegisterObjectBehaviour("Matrix4d", asBEHAVE_CONSTRUCT, "void f()", vspFUNCTION(Matrix4dDefaultConstructor), vspCALL_CDECL_OBJLAST);
    assert(r >= 0); // TODO?

    //===== Register the Matrix4d methods =====//

    // Syntactic sugar for registering matrix4d stuff
#define RegisterObjectMethodMatrix4d(outsig, operstuff)                                                     \
    do                                                                                                      \
    {                                                                                                       \
        r = se->RegisterObjectMethod("Matrix4d", outsig, vspMETHOD(Matrix4d, operstuff), vspCALL_THISCALL); \
        assert(r >= 0);                                                                                     \
    } while (0)

    RegisterObjectMethodMatrix4d("void loadIdentity()", loadIdentity);
    RegisterObjectMethodMatrix4d("void translatef( const double & in x, const double & in y, const double & in z)", translatef);
    RegisterObjectMethodMatrix4d("void rotateX( const double & in ang )", rotateX);
    RegisterObjectMethodMatrix4d("void rotateY( const double & in ang )", rotateY);
    RegisterObjectMethodMatrix4d("void rotateZ( const double & in ang )", rotateZ);
    RegisterObjectMethodMatrix4d("void rotate( const double & in ang, const vec3d & in axis )", rotate);
    RegisterObjectMethodMatrix4d("void scale( const double & in scale )", scale);
    RegisterObjectMethodMatrix4d("vec3d xform( const vec3d & in v )", xform);
    RegisterObjectMethodMatrix4d("vec3d getAngles()", getAngles);
    RegisterObjectMethodMatrix4d("void loadXZRef()", loadXZRef);
    RegisterObjectMethodMatrix4d("void loadXYRef()", loadXYRef);
    RegisterObjectMethodMatrix4d("void loadYZRef()", loadYZRef);
    RegisterObjectMethodMatrix4d("void affineInverse()", affineInverse);
    RegisterObjectMethodMatrix4d("void buildXForm( const vec3d & in pos, const vec3d & in rot, const vec3d & in cent_rot )", buildXForm);

// TODO: Expose additional functions to the API (i.e. matMult)
#undef RegisterOjectMethodMatrix4d
}

//==== Register Custom Geom Mgr Object ====//
void ScriptMgrSingleton::RegisterCustomGeomMgr(asIScriptEngine *se)
{
    asDocInfo doc_struct;
    string group_description = "";
    string group = "";
    se->AddGroup(group.c_str(), "Custom Geometry Functions", group_description.c_str());

    int r;
#define RegGFCustomGeomMgrSingleton(outsig, operstuff)                                                                                   \
    do                                                                                                                                   \
    {                                                                                                                                    \
        r = se->RegisterGlobalFunction(outsig, vspMETHOD(CustomGeomMgrSingleton, operstuff), vspCALL_THISCALL_ASGLOBAL, &CustomGeomMgr); \
        assert(r);                                                                                                                       \
    } while (0)

    RegGFCustomGeomMgrSingleton("string AddParm( int type, const string & in name, const string & in group )", AddParm);
    RegGFCustomGeomMgrSingleton("string GetCurrCustomGeom()", GetCurrCustomGeom);
    RegGFCustomGeomMgrSingleton("string GetCustomParm( int index )", GetCustomParm);
    RegGFCustomGeomMgrSingleton("int AddGui( int type, const string & in label = string(), const string & in parm_name = string(), const string & in group_name = string(), double range = 10.0 )", AddGui);
    RegGFCustomGeomMgrSingleton("void UpdateGui( int gui_id, const string & in parm_id )", AddUpdateGui);
    RegGFCustomGeomMgrSingleton("string AddXSecSurf()", AddXSecSurf);
    RegGFCustomGeomMgrSingleton("void RemoveXSecSurf(const string & in xsec_id)", RemoveXSecSurf);
    RegGFCustomGeomMgrSingleton("void ClearXSecSurfs()", ClearXSecSurfs);
    RegGFCustomGeomMgrSingleton("void SkinXSecSurf( bool closed_flag = false )", SkinXSecSurf);
    RegGFCustomGeomMgrSingleton("void CloneSurf(int index, Matrix4d & in mat)", CloneSurf);
    RegGFCustomGeomMgrSingleton("void TransformSurf(int index, Matrix4d & in mat)", TransformSurf);
    RegGFCustomGeomMgrSingleton("void SetVspSurfType( int type, int surf_index = -1 )", SetVspSurfType);
    RegGFCustomGeomMgrSingleton("void SetVspSurfCfdType( int type, int surf_index = -1 )", SetVspSurfCfdType);
    RegGFCustomGeomMgrSingleton("void SetCustomXSecLoc( const string & in xsec_id, const vec3d & in loc )", SetCustomXSecLoc);
    RegGFCustomGeomMgrSingleton("vec3d GetCustomXSecLoc( const string & in xsec_id )", GetCustomXSecLoc);
    RegGFCustomGeomMgrSingleton("void SetCustomXSecRot( const string & in xsec_id, const vec3d & in rot )", SetCustomXSecRot);
    RegGFCustomGeomMgrSingleton("vec3d GetCustomXSecRot( const string & in xsec_id )", GetCustomXSecRot);
    RegGFCustomGeomMgrSingleton("bool CheckClearTriggerEvent( int gui_id )", CheckClearTriggerEvent);

    r = se->RegisterGlobalFunction(
        "void SetupCustomDefaultSource( int type, int surf_index, double l1, double r1, double u1, double w1, double l2 = 0, double r2 = 0, double u2 = 0, double w2 = 0 )",
        vspMETHOD(CustomGeomMgrSingleton, SetupCustomDefaultSource), vspCALL_THISCALL_ASGLOBAL, &CustomGeomMgr);
    assert(r >= 0);

    RegGFCustomGeomMgrSingleton("void ClearAllCustomDefaultSources()", ClearAllCustomDefaultSources);
    RegGFCustomGeomMgrSingleton("void SetCustomCenter( double x, double y, double z )", SetCustomCenter);
    RegGFCustomGeomMgrSingleton("string AppendXSec( const string & in xsec_surf_id, int type )", AppendCustomXSec);
    // WARNING: Both versions of the AppendCustomXSec must be available to avoid breaking existing CustomGeom scripts
    RegGFCustomGeomMgrSingleton("string AppendCustomXSec( const string & in xsec_surf_id, int type )", AppendCustomXSec);
    RegGFCustomGeomMgrSingleton("void CutCustomXSec( const string & in xsec_surf_id, int index )", CutCustomXSec);
    RegGFCustomGeomMgrSingleton("void CopyCustomXSec( const string & in xsec_surf_id, int index )", CopyCustomXSec);
    RegGFCustomGeomMgrSingleton("void PasteCustomXSec( const string & in xsec_surf_id, int index )", PasteCustomXSec);
    RegGFCustomGeomMgrSingleton("string InsertCustomXSec( const string & in xsec_surf_id, int type, int index )", InsertCustomXSec);
#undef RegGFCustomGeomMgrSingleton
}

//==== Register Adv Link Mgr Object ====//
void ScriptMgrSingleton::RegisterAdvLinkMgr(asIScriptEngine *se)
{
    int r;
    asDocInfo doc_struct;
    string group_description = "";
    string group = "";
    se->AddGroup(group.c_str(), "Advanced Link Functions", group_description.c_str());

    r = se->RegisterGlobalFunction("void AddInput( const string & in parm_id, const string & in var_name )",
                                   vspMETHOD(AdvLinkMgrSingleton, AddInput), vspCALL_THISCALL_ASGLOBAL, &AdvLinkMgr);
    assert(r);

    r = se->RegisterGlobalFunction("void AddOutput( const string & in parm_id, const string & in var_name )",
                                   vspMETHOD(AdvLinkMgrSingleton, AddOutput), vspCALL_THISCALL_ASGLOBAL, &AdvLinkMgr);
    assert(r);

    r = se->RegisterGlobalFunction("void SetVar( const string & in var_name, double val )", vspMETHOD(AdvLinkMgrSingleton, SetVar), vspCALL_THISCALL_ASGLOBAL, &AdvLinkMgr);
    assert(r);

    r = se->RegisterGlobalFunction("double GetVar( const string & in var_name )", vspMETHOD(AdvLinkMgrSingleton, GetVar), vspCALL_THISCALL_ASGLOBAL, &AdvLinkMgr);
    assert(r);
}

//==== Register API E Functions ====//
void ScriptMgrSingleton::RegisterAPIErrorObj(asIScriptEngine *se)
{
    asDocInfo doc_struct;

    //==== Register ErrorObj Object =====//
    int r = se->RegisterObjectType("ErrorObj", sizeof(vsp::ErrorObj), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CDA);
    assert(r >= 0);

    r = se->RegisterObjectMethod("ErrorObj", "ERROR_CODE GetErrorCode()", vspMETHOD(vsp::ErrorObj, GetErrorCode), vspCALL_THISCALL);
    assert(r >= 0);

    r = se->RegisterObjectMethod("ErrorObj", "string GetErrorString()", vspMETHOD(vsp::ErrorObj, GetErrorString), vspCALL_THISCALL);
    assert(r >= 0);
}

//==== Register VSP API Functions ====//
void ScriptMgrSingleton::RegisterAPI(asIScriptEngine *se)
{
    int r;

// Syntactic sugar for global functions
#define RegisterGlobalFunctionSimple(outsig, operstuff)                                \
    do                                                                                 \
    {                                                                                  \
        r = se->RegisterGlobalFunction(outsig, vspFUNCTION(operstuff), vspCALL_CDECL); \
        assert(r >= 0);                                                                \
    } while (0)

    // Syntactic sugar for methods
#define RegisterGlobalMethodScriptMgr(outsig, operstuff)                                                                         \
    do                                                                                                                           \
    {                                                                                                                            \
        r = se->RegisterGlobalFunction(outsig, vspMETHOD(ScriptMgrSingleton, operstuff), vspCALL_THISCALL_ASGLOBAL, &ScriptMgr); \
        assert(r >= 0);                                                                                                          \
    } while (0)

#define RegisterGlobalMethodError(outsig, operstuff)                                                                           \
    do                                                                                                                         \
    {                                                                                                                          \
        r = se->RegisterGlobalFunction(outsig, vspMETHOD(ErrorMgrSingleton, operstuff), vspCALL_THISCALL_ASGLOBAL, &ErrorMgr); \
        assert(r >= 0);                                                                                                        \
    } while (0)

    asDocInfo doc_struct;
    string group_description = "";
    string group = "";
    se->AddGroup(group.c_str(), "API Error Functions", group_description.c_str());

    RegisterGlobalMethodError("bool GetErrorLastCallFlag()", GetErrorLastCallFlag);
    RegisterGlobalMethodError("int GetNumTotalErrors()", GetNumTotalErrors);
    RegisterGlobalMethodError("ErrorObj PopLastError()", PopLastError);
    RegisterGlobalMethodError("ErrorObj GetLastError()", GetLastError);
    RegisterGlobalMethodError("void SilenceErrors()", SilenceErrors);
    RegisterGlobalMethodError("void PrintOnErrors()", PrintOnErrors);

    //==== Visualization Functions ====//
    group = "Visualization";
    se->AddGroup(group.c_str(), "Visualization Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void ScreenGrab( const string & in file_name, int w, int h, bool transparentBG, bool autocrop = false )", ScreenGrab);
    RegisterGlobalFunctionSimple("void SetViewAxis( bool vaxis )", SetViewAxis);
    RegisterGlobalFunctionSimple("void SetShowBorders( bool brdr )", SetShowBorders);
    RegisterGlobalFunctionSimple("void SetGeomDrawType( const string & in geom_id, int type )", SetGeomDrawType);
    RegisterGlobalFunctionSimple("void SetGeomDisplayType( const string & in geom_id, int type )", SetGeomDisplayType);
    RegisterGlobalFunctionSimple("void SetBackground( double r, double g, double b )", SetBackground);

    //==== Vehicle Functions ====//
    group = "Vehicle";

    se->AddGroup(group.c_str(), "Vehicle Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void Update( bool update_managers = true)", Update);
    RegisterGlobalFunctionSimple("void VSPExit( int error_code )", VSPExit);
    RegisterGlobalFunctionSimple("void ClearVSPModel()", ClearVSPModel);
    RegisterGlobalFunctionSimple("string GetVSPFileName()", GetVSPFileName);

    //==== File I/O Functions ====//
    group = "FileIO";
    se->AddGroup(group.c_str(), "File Input and Output Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void ReadVSPFile( const string & in file_name )", ReadVSPFile);
    RegisterGlobalFunctionSimple("void WriteVSPFile( const string & in file_name, int set )", WriteVSPFile);
    RegisterGlobalFunctionSimple("void SetVSP3FileName( const string & in file_name )", SetVSP3FileName);
    RegisterGlobalFunctionSimple("void InsertVSPFile( const string & in file_name, const string & in parent )", InsertVSPFile);
    RegisterGlobalFunctionSimple("string ExportFile( const string & in file_name, int thick_set, int file_type, int thin_set = -1 )", ExportFile);
    RegisterGlobalFunctionSimple("string ImportFile( const string & in file_name, int file_type, const string & in parent )", ImportFile);
    RegisterGlobalFunctionSimple("void SetBEMPropID( const string & in prop_id )", SetBEMPropID);

    //==== Design File Functions ====//
    group = "DesignFile";

    se->AddGroup(group.c_str(), "Design File Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void ReadApplyDESFile( const string & in file_name )", ReadApplyDESFile);
    RegisterGlobalFunctionSimple("void WriteDESFile( const string & in file_name )", WriteDESFile);
    RegisterGlobalFunctionSimple("void ReadApplyXDDMFile( const string & in file_name )", ReadApplyXDDMFile);
    RegisterGlobalFunctionSimple("void WriteXDDMFile( const string & in file_name )", WriteXDDMFile);
    RegisterGlobalFunctionSimple("int GetNumDesignVars()", GetNumDesignVars);
    RegisterGlobalFunctionSimple("void AddDesignVar( const string & in parm_id, int type )", AddDesignVar);
    RegisterGlobalFunctionSimple("void DeleteAllDesignVars()", DeleteAllDesignVars);
    RegisterGlobalFunctionSimple("string GetDesignVar( int index )", GetDesignVar);
    RegisterGlobalFunctionSimple("int GetDesignVarType( int index )", GetDesignVarType);

    //==== Computations ====//
    group = "Computations";

    se->AddGroup(group.c_str(), "General Computation Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("string ComputeMassProps( int set, int num_slices )", ComputeMassProps);
    RegisterGlobalFunctionSimple("string ComputeCompGeom( int set, bool half_mesh, int file_export_types )", ComputeCompGeom);
    RegisterGlobalFunctionSimple("string ComputePlaneSlice( int set, int num_slices, const vec3d & in norm, bool auto_bnd, double start_bnd = 0, double end_bnd = 0 )", ComputePlaneSlice);
    RegisterGlobalFunctionSimple("void ComputeDegenGeom( int set, int file_type )", ComputeDegenGeom);

    //==== CFD Mesh ====//
    group = "CFDMesh";

    se->AddGroup(group.c_str(), "CFD Mesh Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void SetComputationFileName( int file_type, const string & in file_name )", SetComputationFileName);
    // TODO: FIXME for FEA Mesh
    RegisterGlobalFunctionSimple("void ComputeCFDMesh( int set, int file_type )", ComputeCFDMesh);
    RegisterGlobalFunctionSimple("void SetCFDMeshVal( int type, double val )", SetCFDMeshVal);
    RegisterGlobalFunctionSimple("void SetCFDWakeFlag( const string & in geom_id, bool flag )", SetCFDWakeFlag);
    RegisterGlobalFunctionSimple("void DeleteAllCFDSources()", DeleteAllCFDSources);
    RegisterGlobalFunctionSimple("void AddDefaultSources()", AddDefaultSources);
    RegisterGlobalFunctionSimple("void AddCFDSource( int type, const string & in geom_id, int surf_index, double l1, double r1, double u1, double w1, double l2 = 0, double r2 = 0, double u2 = 0, double w2 = 0 )", AddCFDSource);

    //==== Analysis Functions ====//
    group = "Analysis";

    se->AddGroup(group.c_str(), "Analysis Manager Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("int GetNumAnalysis( )", GetNumAnalysis);
    RegisterGlobalMethodScriptMgr("array<string>@ ListAnalysis()", ListAnalysis);
    RegisterGlobalMethodScriptMgr("array<string>@ GetAnalysisInputNames(const string & in analysis )", GetAnalysisInputNames);
    RegisterGlobalFunctionSimple("string ExecAnalysis( const string & in analysis )", ExecAnalysis);
    RegisterGlobalFunctionSimple("int GetNumAnalysisInputData( const string & in analysis, const string & in name )", GetNumAnalysisInputData);
    RegisterGlobalFunctionSimple("int GetAnalysisInputType( const string & in analysis, const string & in name )", GetAnalysisInputType);
    RegisterGlobalMethodScriptMgr("array<int>@ GetIntAnalysisInput( const string & in analysis, const string & in name, int index = 0 )", GetIntAnalysisInput);
    RegisterGlobalMethodScriptMgr("array<double>@ GetDoubleAnalysisInput( const string & in analysis, const string & in name, int index = 0 )", GetDoubleAnalysisInput);
    RegisterGlobalMethodScriptMgr("array<string>@ GetStringAnalysisInput( const string & in analysis, const string & in name, int index = 0 )", GetStringAnalysisInput);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetVec3dAnalysisInput( const string & in analysis, const string & in name, int index = 0 )", GetVec3dAnalysisInput);
    RegisterGlobalFunctionSimple("void SetAnalysisInputDefaults( const string & in analysis )", SetAnalysisInputDefaults);
    RegisterGlobalMethodScriptMgr("void SetIntAnalysisInput( const string & in analysis, const string & in name, array<int>@ indata_arr, int index = 0 )", SetIntAnalysisInput);
    RegisterGlobalMethodScriptMgr("void SetDoubleAnalysisInput( const string & in analysis, const string & in name, array<double>@ indata_arr, int index = 0 )", SetDoubleAnalysisInput);
    RegisterGlobalMethodScriptMgr("void SetStringAnalysisInput( const string & in analysis, const string & in name, array<string>@ indata_arr, int index = 0 )", SetStringAnalysisInput);
    RegisterGlobalMethodScriptMgr("void SetVec3dAnalysisInput( const string & in analysis, const string & in name, array<vec3d>@ indata_arr, int index = 0 )", SetVec3dAnalysisInput);

    //==== Results Functions ====//
    group = "Results";

    se->AddGroup(group.c_str(), "Results Manager Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("int GetNumResults( const string & in name )", GetNumResults);
    RegisterGlobalFunctionSimple("string GetResultsName( const string & in results_id )", GetResultsName);
    RegisterGlobalFunctionSimple("string FindResultsID( const string & in name, int index = 0 )", FindResultsID);
    RegisterGlobalFunctionSimple("string FindLatestResultsID( const string & in name )", FindLatestResultsID);
    RegisterGlobalFunctionSimple("int GetNumData( const string & in results_id, const string & in data_name )", GetNumData);
    RegisterGlobalFunctionSimple("int GetResultsType( const string & in results_id, const string & in data_name )", GetResultsType);
    RegisterGlobalMethodScriptMgr("array<string>@ GetAllResultsNames()", GetAllResultsNames);
    RegisterGlobalMethodScriptMgr("array<string>@ GetAllDataNames(const string & in results_id )", GetAllDataNames);
    RegisterGlobalMethodScriptMgr("array<int>@ GetIntResults( const string & in id, const string & in name, int index = 0 )", GetIntResults);
    RegisterGlobalMethodScriptMgr("array<double>@ GetDoubleResults( const string & in id, const string & in name, int index = 0 )", GetDoubleResults);
    RegisterGlobalMethodScriptMgr("array<string>@ GetStringResults( const string & in id, const string & in name, int index = 0 )", GetStringResults);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetVec3dResults( const string & in id, const string & in name, int index = 0 )", GetVec3dResults);
    RegisterGlobalFunctionSimple("string CreateGeomResults( const string & in geom_id, const string & in name )", CreateGeomResults);
    RegisterGlobalFunctionSimple("void DeleteAllResults()", DeleteAllResults);
    RegisterGlobalFunctionSimple("void DeleteResult( const string & in id )", DeleteResult);
    RegisterGlobalFunctionSimple("void WriteResultsCSVFile( const string & in id, const string & in file_name )", WriteResultsCSVFile);
    RegisterGlobalFunctionSimple("void PrintResults( const string & in id )", PrintResults);

    r = se->RegisterGlobalMethod("void WriteTestResults()", vspMethod(ResultsMgrSingleton, WriteTestResults), vspCALL_THISCALL_ASGLOBAL, &ResultsMgr);
    assert(r >= 0);

    //==== Geom Functions ====//
    group = "Geom";

    se->AddGroup(group.c_str(), "Geom Functions", group_description.c_str());

    RegisterGlobalMethodScriptMgr("array<string>@ GetGeomTypes()", GetGeomTypes);
    RegisterGlobalFunctionSimple("string AddGeom( const string & in type, const string & in parent = string() )", AddGeom);
    RegisterGlobalFunctionSimple("void UpdateGeom(const string & in geom_id)", UpdateGeom);
    RegisterGlobalFunctionSimple("void DeleteGeom(const string & in geom_id)", DeleteGeom);
    RegisterGlobalMethodScriptMgr("void DeleteGeomVec( array<string>@ del_arr )", DeleteGeomVec);
    RegisterGlobalFunctionSimple("void CutGeomToClipboard(const string & in geom_id)", CutGeomToClipboard);
    RegisterGlobalFunctionSimple("void CopyGeomToClipboard(const string & in geom_id)", CopyGeomToClipboard);
    RegisterGlobalMethodScriptMgr("array<string>@ PasteGeomClipboard( const string & in parent_id = \"\" )", PasteGeomClipboard);
    RegisterGlobalMethodScriptMgr("array<string>@ FindGeoms()", FindGeoms);
    RegisterGlobalMethodScriptMgr("array<string>@ FindGeomsWithName(const string & in name)", FindGeomsWithName);
    RegisterGlobalFunctionSimple("string FindGeom(const string & in name, int index)", FindGeom);
    RegisterGlobalFunctionSimple("void SetGeomName( const string & in geom_id, const string & in name )", SetGeomName);
    RegisterGlobalFunctionSimple("string GetGeomName( const string & in geom_id )", GetGeomName);
    RegisterGlobalMethodScriptMgr("array<string>@ GetGeomParmIDs(const string & in geom_id )", GetGeomParmIDs);
    RegisterGlobalFunctionSimple("int GetGeomVSPSurfCfdType( const string & in geom_id, int main_surf_ind = 0 )", GetGeomVSPSurfCfdType);
    RegisterGlobalFunctionSimple("int GetGeomVSPSurfType( const string & in geom_id, int main_surf_ind = 0 )", GetGeomVSPSurfType);
    RegisterGlobalFunctionSimple("string GetGeomTypeName(const string & in geom_id )", GetGeomTypeName);
    RegisterGlobalFunctionSimple("int GetNumMainSurfs( const string & in geom_id )", GetNumMainSurfs);
    RegisterGlobalFunctionSimple("int GetTotalNumSurfs( const string & in geom_id )", GetTotalNumSurfs);
    RegisterGlobalFunctionSimple("vec3d GetGeomBBoxMax( const string & in geom_id, int main_surf_ind = 0, bool ref_frame_is_absolute = true )", GetGeomBBoxMax);
    RegisterGlobalFunctionSimple("vec3d GetGeomBBoxMin( const string & in geom_id, int main_surf_ind = 0, bool ref_frame_is_absolute = true )", GetGeomBBoxMin);
    RegisterGlobalFunctionSimple("string GetGeomParent( const string & in geom_id )", GetGeomParent);
    RegisterGlobalMethodScriptMgr("array<string>@ GetGeomChildren( const string & in geom_id )", GetGeomChildren);
    RegisterGlobalFunctionSimple("void SetDriverGroup( const string & in geom_id, int section_index, int driver_0, int driver_1 = -1, int driver_2 = -1)", SetDriverGroup);

    //==== SubSurface Functions ====//
    group = "SubSurface";

    se->AddGroup(group.c_str(), "Sub-Surface Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("string AddSubSurf( const string & in geom_id, int type, int surfindex = 0 )", AddSubSurf);

    r = se->RegisterGlobalFunction("void DeleteSubSurf( const string & in geom_id, const string & in sub_id )", vspFUNCTIONPR(vsp::DeleteSubSurf, (const string &, const string &), void), vspCALL_CDECL);
    assert(r >= 0);
    // TODO: Why are there two DeleteSubSurf if Geom ID isn't needed?

    r = se->RegisterGlobalFunction("void DeleteSubSurf( const string & in sub_id )", vspFUNCTIONPR(vsp::DeleteSubSurf, (const string &), void), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("string GetSubSurf( const string & in geom_id, int index )", vspFUNCTIONPR(vsp::GetSubSurf, (const string &, int), string), vspCALL_CDECL);
    assert(r >= 0);

    RegisterGlobalMethodScriptMgr("array<string>@ GetSubSurf( const string & in geom_id, const string & in name )", GetSubSurf);

    r = se->RegisterGlobalFunction("void SetSubSurfName( const string & in geom_id, const string & in sub_id, const string & in name )", vspFUNCTIONPR(vsp::SetSubSurfName, (const string &, const string &, const string &), void), vspCALL_CDECL);
    assert(r >= 0);
    // TODO: Why are there two SetSubSurfName if Geom ID isn't needed?

    r = se->RegisterGlobalFunction("void SetSubSurfName( const string & in sub_id, const string & in name )", vspFUNCTIONPR(vsp::SetSubSurfName, (const string &, const string &), void), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("string GetSubSurfName( const string & in geom_id, const string & in sub_id )", vspFUNCTIONPR(vsp::GetSubSurfName, (const string &, const string &), string), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("string GetSubSurfName( const string & in sub_id )", vspFUNCTIONPR(vsp::GetSubSurfName, (const string &), string), vspCALL_CDECL);
    assert(r >= 0);
    // TODO: Why are there two GetSubSurfName if Geom ID isn't needed?

    RegisterGlobalFunctionSimple("int GetSubSurfIndex( const string & in sub_id )", GetSubSurfIndex);
    RegisterGlobalMethodScriptMgr("array<string>@ GetSubSurfIDVec( const string & in geom_id )", GetSubSurfIDVec);
    RegisterGlobalMethodScriptMgr("array<string>@ GetAllSubSurfIDs()", GetAllSubSurfIDs);
    RegisterGlobalFunctionSimple("int GetNumSubSurf( const string & in geom_id )", GetNumSubSurf);
    RegisterGlobalFunctionSimple("int GetSubSurfType( const string & in sub_id )", GetSubSurfType);
    RegisterGlobalMethodScriptMgr("array<string>@ GetSubSurfParmIDs(const string & in sub_id )", GetSubSurfParmIDs);

    //==== VSPAERO CS Group Functions ====//
    group = "CSGroup";

    se->AddGroup(group.c_str(), "VSPAERO Control Surface Group Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void AutoGroupVSPAEROControlSurfaces()", AutoGroupVSPAEROControlSurfaces);
    RegisterGlobalFunctionSimple("int GetNumControlSurfaceGroups()", GetNumControlSurfaceGroups);
    RegisterGlobalFunctionSimple("int CreateVSPAEROControlSurfaceGroup()", CreateVSPAEROControlSurfaceGroup);
    RegisterGlobalFunctionSimple("void AddAllToVSPAEROControlSurfaceGroup( int CSGroupIndex )", AddAllToVSPAEROControlSurfaceGroup);
    RegisterGlobalFunctionSimple("void RemoveAllFromVSPAEROControlSurfaceGroup( int CSGroupIndex )", RemoveAllFromVSPAEROControlSurfaceGroup);
    RegisterGlobalMethodScriptMgr("array<string>@ GetActiveCSNameVec( int CSGroupIndex )", GetActiveCSNameVec);
    RegisterGlobalMethodScriptMgr("array<string>@ GetCompleteCSNameVec( )", GetCompleteCSNameVec);
    RegisterGlobalMethodScriptMgr("array<string>@ GetAvailableCSNameVec( int CSGroupIndex )", GetAvailableCSNameVec);
    RegisterGlobalFunctionSimple("void SetVSPAEROControlGroupName( const string & in name, int CSGroupIndex )", SetVSPAEROControlGroupName);
    RegisterGlobalFunctionSimple("string GetVSPAEROControlGroupName( int CSGroupIndex )", GetVSPAEROControlGroupName);
    RegisterGlobalMethodScriptMgr("void AddSelectedToCSGroup( array<int>@ selected, int CSGroupIndex )", AddSelectedToCSGroup);
    RegisterGlobalMethodScriptMgr("void RemoveSelectedFromCSGroup( array<int>@ selected, int CSGroupIndex )", RemoveSelectedFromCSGroup);
    // FIXME: RemoveSelectedFromCSGroup not working

    //==== VSPAERO Functions ====//
    group = "VSPAERO";

    se->AddGroup(group.c_str(), "VSPAERO Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("string GetVSPAERORefWingID()", GetVSPAERORefWingID);
    RegisterGlobalFunctionSimple("string SetVSPAERORefWingID( const string & in geom_id )", SetVSPAERORefWingID);

    //==== VSPAERO Disk and Prop Functions ====//
    group = "VSPAERODiskAndProp";

    se->AddGroup(group.c_str(), "VSPAERO Actuator Disk and Propeller Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("string FindActuatorDisk( int disk_index )", FindActuatorDisk);
    RegisterGlobalFunctionSimple("int GetNumActuatorDisks()", GetNumActuatorDisks);
    RegisterGlobalFunctionSimple("string FindUnsteadyGroup( int group_index )", FindUnsteadyGroup);
    RegisterGlobalFunctionSimple("string GetUnsteadyGroupName( int group_index )", GetUnsteadyGroupName);
    RegisterGlobalMethodScriptMgr("array<string>@ GetUnsteadyGroupCompIDs( int group_index )", GetUnsteadyGroupCompIDs);
    RegisterGlobalMethodScriptMgr("array<int>@ GetUnsteadyGroupSurfIndexes( int group_index )", GetUnsteadyGroupSurfIndexes);
    RegisterGlobalFunctionSimple("int GetNumUnsteadyGroups()", GetNumUnsteadyGroups);
    RegisterGlobalFunctionSimple("int GetNumUnsteadyRotorGroups()", GetNumUnsteadyRotorGroups);

    //==== XSecSurf Functions ====//
    group = "XSecSurf";

    se->AddGroup(group.c_str(), "XSecSurf Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("int GetNumXSecSurfs( const string & in geom_id )", GetNumXSecSurfs);
    RegisterGlobalFunctionSimple("string GetXSecSurf( const string & in geom_id, int index )", GetXSecSurf);
    RegisterGlobalFunctionSimple("int GetNumXSec( const string & in xsec_surf_id )", GetNumXSec);
    RegisterGlobalFunctionSimple("string GetXSec( const string & in xsec_surf_id, int xsec_index )", GetXSec);
    RegisterGlobalFunctionSimple("void ChangeXSecShape( const string & in xsec_surf_id, int xsec_index, int type )", ChangeXSecShape);
    RegisterGlobalFunctionSimple("void SetXSecSurfGlobalXForm( const string & in xsec_surf_id, const Matrix4d & in mat )", SetXSecSurfGlobalXForm);
    RegisterGlobalFunctionSimple("Matrix4d GetXSecSurfGlobalXForm( const string & in xsec_surf_id )", GetXSecSurfGlobalXForm);

    //==== XSec Functions ====//
    group = "XSec";

    se->AddGroup(group.c_str(), "XSec and Airfoil Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void CutXSec( const string & in geom_id, int index )", CutXSec);
    RegisterGlobalFunctionSimple("void CopyXSec( const string & in geom_id, int index )", CopyXSec);
    RegisterGlobalFunctionSimple("void PasteXSec( const string & in geom_id, int index )", PasteXSec);
    RegisterGlobalFunctionSimple("void InsertXSec( const string & in geom_id, int index, int type )", InsertXSec);
    RegisterGlobalFunctionSimple("int GetXSecShape( const string& in xsec_id )", GetXSecShape);
    RegisterGlobalFunctionSimple("double GetXSecWidth( const string& in xsec_id )", GetXSecWidth);
    RegisterGlobalFunctionSimple("double GetXSecHeight( const string& in xsec_id )", GetXSecHeight);
    RegisterGlobalFunctionSimple("void SetXSecWidth( const string& in xsec_id, double w )", SetXSecWidth);
    RegisterGlobalFunctionSimple("void SetXSecHeight( const string& in xsec_id, double h )", SetXSecHeight);
    RegisterGlobalFunctionSimple("void SetXSecWidthHeight( const string& in xsec_id, double w, double h )", SetXSecWidthHeight);
    RegisterGlobalMethodScriptMgr("array<string>@ GetXSecParmIDs(const string & in xsec_id )", GetXSecParmIDs);
    RegisterGlobalFunctionSimple("string GetXSecParm( const string& in xsec_id, const string& in name )", GetXSecParm);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ ReadFileXSec(const string& in xsec_id, const string& in file_name )", ReadFileXSec);
    RegisterGlobalMethodScriptMgr("void SetXSecPnts( const string& in xsec_id, array<vec3d>@ pnt_arr )", SetXSecPnts);
    RegisterGlobalFunctionSimple("vec3d ComputeXSecPnt( const string& in xsec_id, double fract )", ComputeXSecPnt);
    RegisterGlobalFunctionSimple("vec3d ComputeXSecTan( const string& in xsec_id, double fract )", ComputeXSecTan);
    RegisterGlobalFunctionSimple("void ResetXSecSkinParms( const string& in xsec_id )", ResetXSecSkinParms);
    RegisterGlobalFunctionSimple("void SetXSecContinuity( const string& in xsec_id, int cx )", SetXSecContinuity);
    RegisterGlobalFunctionSimple("void SetXSecTanAngles( const string& in xsec_id, int side, double top, double right = -1.0e12, double bottom = -1.0e12, double left = -1.0e12 )", SetXSecTanAngles);
    RegisterGlobalFunctionSimple("void SetXSecTanSlews( const string& in xsec_id, int side, double top, double right = -1.0e12, double bottom = -1.0e12, double left = -1.0e12 )", SetXSecTanSlews);
    RegisterGlobalFunctionSimple("void SetXSecTanStrengths( const string& in xsec_id, int side, double top, double right = -1.0e12, double bottom = -1.0e12, double left = -1.0e12 )", SetXSecTanStrengths);
    RegisterGlobalFunctionSimple("void SetXSecCurvatures( const string& in xsec_id, int side, double top, double right = -1.0e12, double bottom = -1.0e12, double left = -1.0e12 )", SetXSecCurvatures);
    RegisterGlobalFunctionSimple("void ReadFileAirfoil( const string& in xsec_id, const string& in file_name )", ReadFileAirfoil);
    RegisterGlobalMethodScriptMgr("void SetAirfoilUpperPnts( const string& in xsec_id, array<vec3d>@ up_pnt_vec )", SetAirfoilUpperPnts);
    RegisterGlobalMethodScriptMgr("void SetAirfoilLowerPnts( const string& in xsec_id, array<vec3d>@ low_pnt_vec )", SetAirfoilLowerPnts);
    RegisterGlobalMethodScriptMgr("void SetAirfoilPnts( const string& in xsec_id, array<vec3d>@ up_pnt_vec, array<vec3d>@ low_pnt_vec )", SetAirfoilPnts);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetHersheyBarLiftDist( const int& in npts, const double& in alpha, const double& in Vinf, const double& in span, bool full_span_flag = false )", GetHersheyBarLiftDist);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetHersheyBarDragDist( const int& in npts, const double& in alpha, const double& in Vinf, const double& in span, bool full_span_flag = false )", GetHersheyBarDragDist);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetVKTAirfoilPnts( const int& in npts, const double& in alpha, const double& in epsilon, const double& in kappa, const double& in tau )", GetVKTAirfoilPnts);
    RegisterGlobalMethodScriptMgr("array<double>@ GetVKTAirfoilCpDist( const double& in alpha, const double& in epsilon, const double& in kappa, const double& in tau, array<vec3d>@ xydata )", GetVKTAirfoilCpDist);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetEllipsoidSurfPnts( const vec3d& in center, const vec3d& in abc_rad, int u_npts = 20, int w_npts = 20 )", GetEllipsoidSurfPnts);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetFeatureLinePnts( const string& in geom_id )", GetFeatureLinePnts);
    RegisterGlobalMethodScriptMgr("array<double>@ GetEllipsoidCpDist( array<vec3d>@ surf_pnt_arr, const vec3d& in abc_rad, const vec3d& in V_inf )", GetEllipsoidCpDist);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetAirfoilUpperPnts(const string& in xsec_id )", GetAirfoilUpperPnts),);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetAirfoilLowerPnts(const string& in xsec_id )", GetAirfoilLowerPnts);
    RegisterGlobalMethodScriptMgr("array<double>@ GetUpperCSTCoefs( const string & in xsec_id )", GetUpperCSTCoefs);
    RegisterGlobalMethodScriptMgr("array<double>@ GetLowerCSTCoefs( const string & in xsec_id )", GetLowerCSTCoefs);
    RegisterGlobalFunctionSimple("int GetUpperCSTDegree( const string& in xsec_id )", GetUpperCSTDegree);
    RegisterGlobalFunctionSimple("int GetLowerCSTDegree( const string& in xsec_id )", GetLowerCSTDegree);
    RegisterGlobalMethodScriptMgr("void SetUpperCST( const string& in xsec_id, int deg, array<double>@ coeff_arr )", SetUpperCST);
    RegisterGlobalMethodScriptMgr("void SetLowerCST( const string& in xsec_id, int deg, array<double>@ coeff_arr )", SetLowerCST);
    RegisterGlobalFunctionSimple("void PromoteCSTUpper( const string& in xsec_id )", PromoteCSTUpper);
    RegisterGlobalFunctionSimple("void PromoteCSTLower( const string& in xsec_id )", PromoteCSTLower);
    RegisterGlobalFunctionSimple("void DemoteCSTUpper( const string& in xsec_id )", DemoteCSTUpper);
    RegisterGlobalFunctionSimple("void DemoteCSTLower( const string& in xsec_id )", DemoteCSTLower);
    RegisterGlobalFunctionSimple("void FitAfCST( const string& in xsec_surf_id, int xsec_index, int deg )", FitAfCST);
    RegisterGlobalFunctionSimple("void WriteBezierAirfoil( const string& in file_name, const string& in geom_id, const double& in foilsurf_u )", WriteBezierAirfoil);
    RegisterGlobalFunctionSimple("void WriteSeligAirfoil( const string& in file_name, const string& in geom_id, const double& in foilsurf_u )", WriteSeligAirfoil);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetAirfoilCoordinates( const string& in geom_id, const double& in foilsurf_u )", GetAirfoilCoordinates);

    //==== Edit Curve XSec Functions ====//
    group = "EditCurveXSec";

    se->AddGroup(group.c_str(), "Edit Curve XSec Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void EditXSecInitShape( const string& in xsec_id )", EditXSecInitShape);
    RegisterGlobalFunctionSimple("void EditXSecConvertTo( const string& in xsec_id, const int& in newtype )", EditXSecConvertTo);
    RegisterGlobalMethodScriptMgr("array<double>@ GetEditXSecUVec( const string& in xsec_id )", GetEditXSecUVec);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetEditXSecCtrlVec( const string& in xsec_id, const bool non_dimensional = true )", GetEditXSecCtrlVec);
    RegisterGlobalMethodScriptMgr("void SetEditXSecPnts( const string& in xsec_id, array<double>@ u_vec, array<vec3d>@ control_pts, array<double>@ r_vec )", SetEditXSecPnts);
    RegisterGlobalFunctionSimple("void EditXSecDelPnt( const string& in xsec_id, const int& in indx )", EditXSecDelPnt);
    RegisterGlobalFunctionSimple("int EditXSecSplit01( const string& in xsec_id, const double& in u )", EditXSecSplit01);
    RegisterGlobalFunctionSimple("void MoveEditXSecPnt( const string& in xsec_id, const int& in indx, const vec3d& in new_pnt )", MoveEditXSecPnt);
    RegisterGlobalFunctionSimple("void ConvertXSecToEdit( const string& in geom_id, const int& in indx = 0 )", ConvertXSecToEdit);
    RegisterGlobalMethodScriptMgr("array<bool>@ GetEditXSecFixedUVec( const string& in xsec_id )", GetEditXSecFixedUVec);
    RegisterGlobalMethodScriptMgr("void SetEditXSecFixedUVec( const string& in xsec_id, array<bool>@ fixed_u_vec )", SetEditXSecFixedUVec);
    RegisterGlobalFunctionSimple("void ReparameterizeEditXSec( const string& in xsec_id )", ReparameterizeEditXSec);

    //==== BOR Functions ====//
    group = "BOR";

    se->AddGroup(group.c_str(), "BOR Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void ChangeBORXSecShape( const string & in geom_id, int type )", ChangeBORXSecShape);
    RegisterGlobalFunctionSimple("int GetBORXSecShape( const string & in geom_id )", GetBORXSecShape);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ ReadBORFileXSec(const string& in bor_id, const string& in file_name )", ReadBORFileXSec);
    RegisterGlobalMethodScriptMgr("void SetBORXSecPnts( const string& in bor_id, array<vec3d>@ pnt_arr )", SetBORXSecPnts);
    RegisterGlobalFunctionSimple("vec3d ComputeBORXSecPnt( const string& in bor_id, double fract )", ComputeBORXSecPnt);
    RegisterGlobalFunctionSimple("vec3d ComputeBORXSecTan( const string& in bor_id, double fract )", ComputeBORXSecTan);
    RegisterGlobalFunctionSimple("void ReadBORFileAirfoil( const string& in bor_id, const string& in file_name )", ReadBORFileAirfoil);
    RegisterGlobalMethodScriptMgr("void SetBORAirfoilUpperPnts( const string& in bor_id, array<vec3d>@ up_pnt_vec )", SetBORAirfoilUpperPnts);
    RegisterGlobalMethodScriptMgr("void SetBORAirfoilLowerPnts( const string& in bor_id, array<vec3d>@ low_pnt_vec )", SetBORAirfoilLowerPnts);
    RegisterGlobalMethodScriptMgr("void SetBORAirfoilPnts( const string& in bor_id, array<vec3d>@ up_pnt_vec, array<vec3d>@ low_pnt_vec )", SetBORAirfoilPnts);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetBORAirfoilUpperPnts(const string& in bor_id )", GetBORAirfoilUpperPnts);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ GetBORAirfoilLowerPnts(const string& in bor_id )", GetBORAirfoilLowerPnts);
    RegisterGlobalMethodScriptMgr("array<double>@ GetBORUpperCSTCoefs( const string & in bor_id )", GetBORUpperCSTCoefs);
    RegisterGlobalMethodScriptMgr("array<double>@ GetBORLowerCSTCoefs( const string & in bor_id )", GetBORLowerCSTCoefs);
    RegisterGlobalFunctionSimple("int GetBORUpperCSTDegree( const string& in bor_id )", GetBORUpperCSTDegree);
    RegisterGlobalFunctionSimple("int GetBORLowerCSTDegree( const string& in bor_id )", GetBORLowerCSTDegree);
    RegisterGlobalMethodScriptMgr("void SetBORUpperCST( const string& in bor_id, int deg, array<double>@ coeff_arr )", SetBORUpperCST);
    RegisterGlobalMethodScriptMgr("void SetBORLowerCST( const string& in bor_id, int deg, array<double>@ coeff_arr )", SetBORLowerCST);
    RegisterGlobalFunctionSimple("void PromoteBORCSTUpper( const string& in bor_id )", PromoteBORCSTUpper);
    RegisterGlobalFunctionSimple("void PromoteBORCSTLower( const string& in bor_id )", PromoteBORCSTLower);
    RegisterGlobalFunctionSimple("void DemoteBORCSTUpper( const string& in bor_id )", DemoteBORCSTUpper);
    RegisterGlobalFunctionSimple("void DemoteBORCSTLower( const string& in bor_id )", DemoteBORCSTLower);
    RegisterGlobalFunctioSimple("void FitBORAfCST( const string& in bor_id, int deg )", FitBORAfCST);

    //==== Sets Functions ====//
    group = "Sets";

    se->AddGroup(group.c_str(), "Functions for Sets", group_description.c_str());

    RegisterGlobalFunctionSimple("int GetNumSets()", GetNumSets);
    RegisterGlobalFunctionSimple("void SetSetName( int index, const string& in name )", SetSetName);
    RegisterGlobalFunctionSimple("string GetSetName( int index )", GetSetName);
    RegisterGlobalMethodScriptMgr("array<string>@ GetGeomSetAtIndex( int index )", GetGeomSetAtIndex);
    RegisterGlobalMethodScriptMgr("array<string>@ GetGeomSet( const string & in name )", GetGeomSet);
    RegisterGlobalFunctionSimple("int GetSetIndex( const string & in name )", GetSetIndex);
    RegisterGlobalFunctionSimple("bool GetSetFlag( const string & in geom_id, int set_index )", GetSetFlag);
    RegisterGlobalFunctionSimple("void SetSetFlag( const string & in geom_id, int set_index, bool flag )", SetSetFlag);
    RegisterGlobalFunctionSimple("void CopyPasteSet(  int copyIndex, int pasteIndex  )", CopyPasteSet);

    //=== Group Modifications ===//
    group = "GroupMod";

    se->AddGroup(group.c_str(), "Group Modification Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void ScaleSet( int set_index, double scale )", ScaleSet);
    RegisterGlobalFunctionSimple("void RotateSet( int set_index, double x_rot_deg, double y_rot_deg, double z_rot_deg )", RotateSet);
    RegisterGlobalFunctionSimple("void TranslateSet( int set_index, const vec3d & in translation_vec )", TranslateSet);
    RegisterGlobalFunctionSimple("void TransformSet( int set_index, const vec3d & in translation_vec, double x_rot_deg, double y_rot_deg, double z_rot_deg, double scale, bool scale_translations_flag )", TransformSet);

    //==== Parm Functions ====//
    group = "Parm";

    se->AddGroup(group.c_str(), "Parm Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("bool ValidParm( const string & in id )", ValidParm);

    RegisterGlobalFunction("double SetParmVal(const string & in parm_id, double val )", vspFUNCTIONPR(vsp::SetParmVal, (const string &, double val), double), vspCALL_CDECL);
    assert(r >= 0);

    RegisterGlobalFunctionSimple("double SetParmValLimits(const string & in parm_id, double val, double lower_limit, double upper_limit )", SetParmValLimits);

    r = se->RegisterGlobalFunction("double SetParmValUpdate(const string & in parm_id, double val )",
                                   vspFUNCTIONPR(vsp::SetParmValUpdate, (const string &, double val), double), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("double SetParmVal(const string & in container_id, const string & in name, const string & in group, double val )",
                                   vspFUNCTIONPR(vsp::SetParmVal, (const string &, const string &, const string &, double val), double), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("double SetParmValUpdate(const string & in container_id, const string & in parm_name, const string & in parm_group_name, double val )",
                                   vspFUNCTIONPR(vsp::SetParmValUpdate, (const string &, const string &, const string &, double val), double), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("double GetParmVal(const string & in parm_id )", vspFUNCTIONPR(vsp::GetParmVal, (const string &), double), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("double GetParmVal(const string & in container_id, const string & in name, const string & in group )",
                                   vspFUNCTIONPR(vsp::GetParmVal, (const string &, const string &, const string &), double), vspCALL_CDECL);
    assert(r >= 0);

    RegisterGlobalFunctionSimple("int GetIntParmVal(const string & in parm_id )", GetIntParmVal);
    RegisterGlobalFunctionSimple("bool GetBoolParmVal(const string & in parm_id )", GetBoolParmVal);
    RegisterGlobalFunctionSimple("void SetParmUpperLimit( const string & in parm_id, double val )", SetParmUpperLimit);
    RegisterGlobalFunctionSimple("double GetParmUpperLimit( const string & in parm_id )", GetParmUpperLimit);
    RegisterGlobalFunctionSimple("void SetParmLowerLimit( const string & in parm_id, double val )", SetParmLowerLimit);
    RegisterGlobalFunctionSimple("double GetParmLowerLimit( const string & in parm_id )", GetParmLowerLimit);
    RegisterGlobalFunctionSimple("int GetParmType( const string & in parm_id )", GetParmType);
    RegisterGlobalFunctionSimple("string GetParmName( const string & in parm_id )", GetParmName);
    RegisterGlobalFunctionSimple("string GetParmGroupName( const string & in parm_id )", GetParmGroupName);
    RegisterGlobalFunctionSimple("string GetParmDisplayGroupName( const string & in parm_id )", GetParmDisplayGroupName);
    RegisterGlobalFunctionSimple("string GetParmContainer( const string & in parm_id )", GetParmContainer);
    RegisterGlobalFunctionSimple("void SetParmDescript( const string & in parm_id, const string & in desc )", SetParmDescript);
    RegisterGlobalFunctionSimple("string FindParm( const string & in parm_container_id, const string & in parm_name, const string & in group_name )", FindParm);
    RegisterGlobalFunctionSimple("string GetParm(const string & in container_id, const string & in name, const string & in group )", GetParm);

    //=== Parm Container Functions ===//
    group = "ParmContainer";

    se->AddGroup(group.c_str(), "Parm Container Functions", group_description.c_str());

    RegisterGlobalMethodScriptMgr("array<string>@ FindContainers()", FindContainers);
    RegisterGlobalMethodScriptMgr("array<string>@ FindContainersWithName( const string & in name )", FindContainersWithName);
    RegisterGlobalFunctionSimple("string FindContainer( const string & in name, int index )", FindContainer);
    RegisterGlobalFunctionSimple("string GetContainerName( const string & in parm_container_id )", GetContainerName);
    RegisterGlobalMethodScriptMgr("array<string>@ FindContainerGroupNames( const string & in parm_container_id )", FindContainerGroupNames);
    RegisterGlobalMethodScriptMgr("array<string>@ FindContainerParmIDs( const string & in parm_container_id )", FindContainerParmIDs);
    RegisterGlobalFunctionSimple("string GetVehicleID()", GetVehicleID);

    //=== Register Snap To Functions ====//
    group = "SnapTo";

    se->AddGroup(group.c_str(), "Snap-To Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("double ComputeMinClearanceDistance( const string & in geom_id, int set )", ComputeMinClearanceDistance);
    // TODO: Validate inc_flag description
    RegisterGlobalFunctionSimple("double SnapParm( const string & in parm_id, double target_min_dist, bool inc_flag, int set )", SnapParm);

    //=== Register Var Preset Functions ====//
    group = "VariablePreset";

    se->AddGroup(group.c_str(), "Variable Preset Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void AddVarPresetGroup( const string & in group_name )", AddVarPresetGroup);
    RegisterGlobalFunctionSimple("void AddVarPresetSetting( const string & in setting_name )", AddVarPresetSetting);

    r = se->RegisterGlobalFunction("void AddVarPresetParm( const string & in parm_ID )", vspFUNCTIONPR(vsp::AddVarPresetParm, (const string &), void), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("void AddVarPresetParm( const string & in parm_ID, const string & in group_name )", vspFUNCTIONPR(vsp::AddVarPresetParm, (const string &, const string &), void), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("void EditVarPresetParm( const string & in parm_ID, double parm_val )", vspFUNCTIONPR(vsp::EditVarPresetParm, (const string &, double), void), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("void EditVarPresetParm( const string & in parm_ID, double parm_val, const string & in group_name, const string & in setting_name )", vspFUNCTIONPR(vsp::EditVarPresetParm, (const string &, double, const string &, const string &), void), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("void DeleteVarPresetParm( const string & in parm_ID )", vspFUNCTIONPR(vsp::DeleteVarPresetParm, (const string &), void), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("void DeleteVarPresetParm( const string & in parm_ID, const string & in group_name )", vspFUNCTIONPR(vsp::DeleteVarPresetParm, (const string &, const string &), void), vspCALL_CDECL);
    assert(r >= 0);

    RegisterGlobalFunctionSimple("void SwitchVarPreset( const string & in group_name, const string & in setting_name )", SwitchVarPreset);
    RegisterGlobalFunctionSimple("bool DeleteVarPresetSet( const string & in group_name, const string & in setting_name )", DeleteVarPresetSet);
    RegisterGlobalFunctionSimple("string GetCurrentGroupName()", GetCurrentGroupName);
    RegisterGlobalFunctionSimple("string GetCurrentSettingName()", GetCurrentSettingName);
    RegisterGlobalMethodScriptMgr("array<string>@ GetVarPresetGroupNames()", GetVarPresetGroupNames);
    RegisterGlobalMethodScriptMgr("array<string>@ GetVarPresetSettingNamesWName( const string & in group_name )", GetVarPresetSettingNamesWName);
    RegisterGlobalMethodScriptMgr("array<string>@ GetVarPresetSettingNamesWIndex( int group_index )", GetVarPresetSettingNamesWIndex);
    RegisterGlobalMethodScriptMgr("array<double>@ GetVarPresetParmVals()", GetVarPresetParmVals);
    RegisterGlobalMethodScriptMgr("array<double>@ GetVarPresetParmValsWNames( const string & in group_name, const string & in setting_name )", GetVarPresetParmValsWNames);
    RegisterGlobalMethodScriptMgr("array<string>@ GetVarPresetParmIDs()", GetVarPresetParmIDs);
    RegisterGlobalMethodScriptMgr("array<string>@ GetVarPresetParmIDsWName( const string & in group_name )", GetVarPresetParmIDsWName);

    //=== Register PCurve Functions ====//
    group = "PCurve";

    se->AddGroup(group.c_str(), "Propeller Blade Curve Functions", group_description.c_str());

    RegisterGlobalMethodScriptMgr("void SetPCurve( const string& in geom_id, const int & in pcurveid, array<double>@ tvec, array<double>@ valvec, const int & in newtype )", SetPCurve);
    RegisterGlobalFunctionSimple("void PCurveConvertTo( const string & in geom_id, const int & in pcurveid, const int & in newtype )", PCurveConvertTo);
    RegisterGlobalFunctionSimple("int PCurveGetType( const string & in geom_id, const int & in pcurveid )", PCurveGetType);
    RegisterGlobalMethodScriptMgr("array<double>@ PCurveGetTVec( const string & in geom_id, const int & in pcurveid )", PCurveGetTVec);
    RegisterGlobalMethodScriptMgr("array<double>@ PCurveGetValVec( const string & in geom_id, const int & in pcurveid )", PCurveGetValVec);
    RegisterGlobalFunctionSimple("void PCurveDeletePt( const string & in geom_id, const int & in pcurveid, const int & in indx )", PCurveDeletePt);
    RegisterGlobalFunctionSimple("int PCurveSplit( const string & in geom_id, const int & in pcurveid, const double & in tsplit )", PCurveSplit);
    RegisterGlobalFunctionSimple("void ApproximateAllPropellerPCurves( const string & in geom_id )", ApproximateAllPropellerPCurves);
    RegisterGlobalFunctionSimple("void ResetPropellerThicknessCurve( const string & in geom_id )", ResetPropellerThicknessCurve);

    //=== Register ParasiteDragTool Functions ====//
    group = "ParasiteDrag";

    se->AddGroup(group.c_str(), "Parasite Drag Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("void AddExcrescence(const string & in excresName, const int & in excresType, const double & in excresVal)", AddExcrescence);
    RegisterGlobalFunctionSimple("void DeleteExcrescence(const int & in excresName)", DeleteExcrescence);

    RegisterGlobalFunctionSimple("void UpdateParasiteDrag()", UpdateParasiteDrag);
    RegisterGlobalFunctionSimple("void WriteAtmosphereCSVFile( const string & in file_name, const int & in atmos_type )", WriteAtmosphereCSVFile);
    RegisterGlobalFunctionSimple("void CalcAtmosphere( const double & in alt, const double & in delta_temp, const int & in atmos_type, double & out temp, double & out pres, double & out pres_ratio, double & out rho_ratio )", CalcAtmosphere);
    RegisterGlobalFunctionSimple("void WriteBodyFFCSVFile( const string & in file_name )", WriteBodyFFCSVFile);
    RegisterGlobalFunctionSimple("void WriteWingFFCSVFile( const string & in file_name )", WriteWingFFCSVFile);
    RegisterGlobalFunctionSimple("void WriteCfEqnCSVFile( const string & in file_name )", WriteCfEqnCSVFile);
    RegisterGlobalFunctionSimple("void WritePartialCfMethodCSVFile( const string & in file_name )", WritePartialCfMethodCSVFile);

    //=== Register Surface Query Functions ===//
    group = "SurfaceQuery";

    se->AddGroup(group.c_str(), "Geom Surface Query Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("vec3d CompPnt01( const string & in geom_id, const int & in surf_indx, const double & in u, const double & in w )", CompPnt01);
    RegisterGlobalFunctionSimple("vec3d CompNorm01( const string & in geom_id, const int & in surf_indx, const double & in u, const double & in w )", CompNorm01);
    RegisterGlobalFunctionSimple("vec3d CompTanU01( const string & in geom_id, const int & in surf_indx, const double & in u, const double & in w )", CompTanU01);
    RegisterGlobalFunctionSimple("vec3d CompTanW01( const string & in geom_id, const int & in surf_indx, const double & in u, const double & in w )", CompTanW01);
    RegisterGlobalFunctionSimple("void CompCurvature01( const string & in geom_id, const int & in surf_indx, const double & in u, const double & in w, double & out k1, double & out k2, double & out ka, double & out kg )", CompCurvature01);
    RegisterGlobalFunctionSimple("double ProjPnt01( const string & in geom_id, const int & in surf_indx, const vec3d & in pt, double & out u, double & out w )", ProjPnt01);
    RegisterGlobalFunctionSimple("double ProjPnt01I( const string & in geom_id, const vec3d & in pt, int & out surf_indx, double & out u, double & out w )", ProjPnt01I);
    RegisterGlobalFunctionSimple("double ProjPnt01Guess( const string & in geom_id, const int & in surf_indx, const vec3d & in pt, const double & in u0, const double & in w0, double & out u, double & out w )", ProjPnt01Guess);
    RegisterGlobalFunctionSimple("double AxisProjPnt01( const string & in geom_id, const int & in surf_indx, const int & in iaxis, const vec3d & in pt, double & out u_out, double & out w_out, vec3d & out p_out )", AxisProjPnt01);
    RegisterGlobalFunctionSimple("double AxisProjPnt01I( const string & in geom_id, const int & in iaxis, const vec3d & in pt, int & out surf_indx_out, double & out u_out, double & out w_out, vec3d & out p_out )", AxisProjPnt01I);
    RegisterGlobalFunctionSimple("double AxisProjPnt01Guess( const string & in geom_id, const int & in surf_indx, const int & in iaxis, const vec3d & in pt, const double & in u0, const double & in w0, double & out u_out, double & out w_out, vec3d & out p_out )", AxisProjPnt01Guess);
    RegisterGlobalFunctionSimple("bool InsideSurf( const string & in geom_id, const int & in surf_indx, const vec3d & in pt )", InsideSurf);
    RegisterGlobalFunctionSimple("double FindRST( const string & in geom_id, const int & in surf_indx, const vec3d & in pt, double & out r, double & out s, double & out t )", FindRST);
    RegisterGlobalFunctionSimple("double FindRSTGuess( const string & in geom_id, const int & in surf_indx, const vec3d & in pt, const double & in r0, const double & in s0, const double & in t0, double & out r, double & out s, double & out t )", FindRSTGuess);
    RegisterGlobalFunctionSimple("vec3d CompPntRST( const string & in geom_id, const int & in surf_indx, const double & in r, const double & in s, const double & in t )", CompPntRST);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ CompVecPntRST(const string & in geom_id, const int & in surf_indx, array<double>@ rs, array<double>@ ss, array<double>@ ts )", CompVecPntRST);
    RegisterGlobalFunctionSimple("vec3d ConvertRSTtoLMN( const string & in geom_id, const int & in surf_indx, const double & in r, const double & in s, const double & in t, double & out l, double & out m, double & out n )", ConvertRSTtoLMN);
    RegisterGlobalFunctionSimple("vec3d ConvertLMNtoRST( const string & in geom_id, const int & in surf_indx, const double & in l, const double & in m, const double & in n, double & out r, double & out s, double & out t )", ConvertLMNtoRST);
    RegisterGlobalMethodScriptMgr("void ConvertRSTtoLMNVec(const string & in geom_id, const int & in surf_indx, array<double>@ rs, array<double>@ ss, array<double>@ ts, array<double>@ ls, array<double>@ ms, array<double>@ ns )", ConvertRSTtoLMNVec);
    RegisterGlobalMethodScriptMgr("void ConvertLMNtoRSTVec(const string & in geom_id, const int & in surf_indx, array<double>@ ls, array<double>@ ms, array<double>@ ns, array<double>@ rs, array<double>@ ss, array<double>@ ts )", ConvertLMNtoRSTVec);
    RegisterGlobalMethodScriptMgr("void GetUWTess01(const string & in geom_id, int & in surf_indx, array<double>@ us, array<double>@ ws )", GetUWTess01);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ CompVecPnt01(const string & in geom_id, const int & in surf_indx, array<double>@ us, array<double>@ ws )", CompVecPnt01);
    RegisterGlobalMethodScriptMgr("array<vec3d>@ CompVecNorm01(const string & in geom_id, const int & in surf_indx, array<double>@ us, array<double>@ws )", CompVecNorm01);
    RegisterGlobalMethodScriptMgr("void CompVecCurvature01(const string & in geom_id, const int & in surf_indx, array<double>@ us, array<double>@ ws, array<double>@ k1s, array<double>@ k2s, array<double>@ kas, array<double>@ kgs)", CompVecCurvature01);
    RegisterGlobalMethodScriptMgr("void ProjVecPnt01(const string & in geom_id, const int & in surf_indx, array<vec3d>@ pts, array<double>@ us, array<double>@ ws, array<double>@ ds )", ProjVecPnt01);
    RegisterGlobalMethodScriptMgr("void ProjVecPnt01Guess(const string & in geom_id, const int & in surf_indx, array<vec3d>@ pts, array<double>@ u0s, array<double>@ w0s, array<double>@ us, array<double>@ ws, array<double>@ ds )", ProjVecPnt01Guess);
    RegisterGlobalMethodScriptMgr("void AxisProjVecPnt01(const string & in geom_id, const int & in surf_indx, const int & in iaxis, array<vec3d>@ pts, array<double>@ us, array<double>@ ws, array<vec3d>@ ps_out, array<double>@ ds )", AxisProjVecPnt01);
    RegisterGlobalMethodScriptMgr("void AxisProjVecPnt01Guess(const string & in geom_id, int & in surf_indx, const int & in iaxis, array<vec3d>@ pts, array<double>@ u0s, array<double>@ w0s, array<double>@ us, array<double>@ ws, array<vec3d>@ ps_out, array<double>@ ds )", AxisProjVecPnt01Guess);
    RegisterGlobalFunctionSimple("array<bool>@  VecInsideSurf( const string & in geom_id, const int & in surf_indx, array<vec3d>@ pts )", VecInsideSurf);
    RegisterGlobalMethodScriptMgr("void FindRSTVec(const string & in geom_id, const int & in surf_indx, array<vec3d>@ pts, array<double>@ rs, array<double>@ ss, array<double>@ ts, array<double>@ ds )", FindRSTVec);
    RegisterGlobalMethodScriptMgr("void FindRSTVecGuess(const string & in geom_id, const int & in surf_indx, array<vec3d>@ pts, array<double>@ r0s, array<double>@ s0s, array<double>@ t0s, array<double>@ rs, array<double>@ ss, array<double>@ ts, array<double>@ ds )", FindRSTVecGuess);

    //=== Register Measure Functions ===//
    group = "Measure";

    se->AddGroup(group.c_str(), "Measure Tool Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("string AddRuler( const string & in startgeomid, int startsurfindx, double startu, double startw, const string & in endgeomid, int endsurfindx, double endu, double endw, const string & in name )", AddRuler);
    RegisterGlobalMethodScriptMgr("array<string>@ GetAllRulers()", GetAllRulers);
    RegisterGlobalFunctionSimple("void DelRuler( const string & in id )", DelRuler);
    RegisterGlobalFunctionSimple("void DeleteAllRulers()", DeleteAllRulers);
    RegisterGlobalFunctionSimple("string AddProbe( const string & in geomid, int surfindx, double u, double w, const string & in name )", AddProbe);
    RegisterGlobalMethodScriptMgr("array<string>@ GetAllProbes()", GetAllProbes);
    RegisterGlobalFunctionSimple("void DelProbe( const string & in id )", DelProbe);
    RegisterGlobalFunctionSimple("void DeleteAllProbes()", DeleteAllProbes);

    //=== Register FeaStructure and FEA Mesh Functions ====//
    group = "FEAMesh";

    se->AddGroup(group.c_str(), "FEA Mesh Functions", group_description.c_str());

    RegisterGlobalFunctionSimple("int AddFeaStruct( const string & in geom_id, bool init_skin = true, int surfindex = 0 )", AddFeaStruct); // TODO: Force init_skin to true always
    RegisterGlobalFunctionSimple("void DeleteFeaStruct( const string & in geom_id, int fea_struct_ind )", DeleteFeaStruct);
    RegisterGlobalFunctionSimple("void SetFeaMeshStructIndex( int struct_index )", SetFeaMeshStructIndex);
    RegisterGlobalFunctionSimple("string GetFeaStructID( const string & in geom_id, int fea_struct_ind )", GetFeaStructID);
    RegisterGlobalFunctionSimple("int GetFeaStructIndex( const string & in struct_id )", GetFeaStructIndex);
    RegisterGlobalFunctionSimple("string GetFeaStructParentGeomID( const string & in struct_id )", GetFeaStructParentGeomID);
    RegisterGlobalFunctionSimple("string GetFeaStructName( const string & in geom_id, int fea_struct_ind )", GetFeaStructName);
    RegisterGlobalFunctionSimple("void SetFeaStructName( const string & in geom_id, int fea_struct_ind, const string & in name )", SetFeaStructName);
    RegisterGlobalMethodScriptMgr("array<string>@ GetFeaStructIDVec()", GetFeaStructIDVec);
    RegisterGlobalFunctionSimple("void SetFeaPartName( const string & in part_id, const string & in name )", SetFeaPartName);
    RegisterGlobalFunctionSimple("void SetFeaMeshVal( const string & in geom_id, int fea_struct_ind, int type, double val )", SetFeaMeshVal);
    RegisterGlobalFunctionSimple("void SetFeaMeshFileName( const string & in geom_id, int fea_struct_ind, int file_type, const string & in file_name )", SetFeaMeshFileName);

    r = se->RegisterGlobalFunction("void ComputeFeaMesh( const string & in geom_id, int fea_struct_ind, int file_type )", vspFUNCTIONPR(vsp::ComputeFeaMesh, (const string &in, int, int), void), vspCALL_CDECL);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("void ComputeFeaMesh( const string & in struct_id, int file_type )", vspFUNCTIONPR(vsp::ComputeFeaMesh, (const string &in, int), void), vspCALL_CDECL);
    assert(r >= 0);

    RegisterGlobalFunctionSimple("string AddFeaPart( const string & in geom_id, int fea_struct_ind, int type )", AddFeaPart);
    RegisterGlobalFunctionSimple("void DeleteFeaPart( const string & in geom_id, int fea_struct_ind, const string & in part_id )", DeleteFeaPart);
    RegisterGlobalFunctionSimple("string GetFeaPartID( const string & in fea_struct_id, int fea_part_index )", GetFeaPartID);
    RegisterGlobalFunctionSimple("string GetFeaPartName( const string & in part_id )", GetFeaPartName);
    RegisterGlobalFunctionSimple("int GetFeaPartType( const string & in part_id )", GetFeaPartType);
    RegisterGlobalFunctionSimple("int GetFeaSubSurfIndex( const string & in ss_id )", GetFeaSubSurfIndex);
    RegisterGlobalFunctionSimple("int NumFeaStructures()", NumFeaStructures);
    RegisterGlobalFunctionSimple("int NumFeaParts( const string & in fea_struct_id )", NumFeaParts);
    RegisterGlobalFunctionSimple("int NumFeaSubSurfs( const string & in fea_struct_id )", NumFeaSubSurfs);
    RegisterGlobalMethodScriptMgr("array<string>@ GetFeaPartIDVec(const string & in fea_struct_id)", GetFeaPartIDVec);
    RegisterGlobalMethodScriptMgr("array<string>@ GetFeaSubSurfIDVec(const string & in fea_struct_id)", GetFeaSubSurfIDVec);
    RegisterGlobalFunctionSimple("void SetFeaPartPerpendicularSparID( const string & in part_id, const string & in perpendicular_spar_id )", SetFeaPartPerpendicularSparID);
    RegisterGlobalFunctionSimple("string GetFeaPartPerpendicularSparID( const string & in part_id )", GetFeaPartPerpendicularSparID);
    RegisterGlobalFunctionSimple("string AddFeaSubSurf( const string & in geom_id, int fea_struct_ind, int type )", AddFeaSubSurf);
    RegisterGlobalFunctionSimple("void DeleteFeaSubSurf( const string & in geom_id, int fea_struct_ind, const string & in ss_id )", DeleteFeaSubSurf);
    RegisterGlobalFunctionSimple("string AddFeaMaterial()", AddFeaMaterial);
    RegisterGlobalFunctionSimple("string AddFeaProperty( int property_type = 0 )", AddFeaProperty);
#undef RegisterGlobalFunctionSimple
#undef RegisterGlobalMethodScriptMgr
}

void ScriptMgrSingleton::RegisterUtility(asIScriptEngine *se)
{
    //==== Register Utility Functions ====//
    int r;

#define RegisterGlobalFunctionSimple(outsig, operstuff)                                \
    do                                                                                 \
    {                                                                                  \
        r = se->RegisterGlobalFunction(outsig, vspFUNCTION(operstuff), vspCALL_CDECL); \
        assert(r >= 0);                                                                \
    } while (0)

    // Syntactic sugar for methods
#define RegisterGlobalMethodScriptMgr(outsig, operstuff)                                                                         \
    do                                                                                                                           \
    {                                                                                                                            \
        r = se->RegisterGlobalFunction(outsig, vspMETHOD(ScriptMgrSingleton, operstuff), vspCALL_THISCALL_ASGLOBAL, &ScriptMgr); \
        assert(r >= 0);                                                                                                          \
    } while (0)

    asDocInfo doc_struct;
    string group_description = "";
    string group = "";
    se->AddGroup(group.c_str(), "General API Utility Functions", group_description.c_str());

    r = se->RegisterGlobalFunction("void Print(const string & in data, bool new_line = true )", vspMETHODPR(ScriptMgrSingleton, Print, (const string &, bool), void), vspCALL_THISCALL_ASGLOBAL, &ScriptMgr);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("void Print(const vec3d & in data, bool new_line = true )", vspMETHODPR(ScriptMgrSingleton, Print, (const vec3d &, bool), void), vspCALL_THISCALL_ASGLOBAL, &ScriptMgr);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("void Print(double data, bool new_line = true )", vspMETHODPR(ScriptMgrSingleton, Print, (double, bool), void), vspCALL_THISCALL_ASGLOBAL, &ScriptMgr);
    assert(r >= 0);

    r = se->RegisterGlobalFunction("void Print(int data, bool new_line = true )", vspMETHODPR(ScriptMgrSingleton, Print, (int, bool), void), vspCALL_THISCALL_ASGLOBAL, &ScriptMgr);
    assert(r >= 0);

    RegisterGlobalMethodScriptMgr("double Min( double x, double y)", Min);
    RegisterGlobalMethodScriptMgr("double Max( double x, double y)", Max);
    RegisterGlobalMethodScriptMgr("double Rad2Deg( double r )", Rad2Deg);
    RegisterGlobalMethodScriptMgr("double Deg2Rad( double d )", Deg2Rad);
    RegisterGlobalFunctionSimple("string GetVSPVersion( )", GetVSPVersion);
    RegisterGlobalFunctionSimple("string GetVSPExePath()", GetVSPExePath);
    RegisterGlobalFunctionSimple("bool SetVSPAEROPath( const string & in path )", SetVSPAEROPath);
    RegisterGlobalFunctionSimple("string GetVSPAEROPath()", GetVSPAEROPath);
    RegisterGlobalFunctionSimple("bool CheckForVSPAERO( const string & in path )", CheckForVSPAERO);
    RegisterGlobalFunctionSimple("void VSPCheckSetup()", VSPCheckSetup);
    RegisterGlobalFunctionSimple("void VSPRenew()", VSPRenew);

    //====  Register Proxy Utility Functions ====//
    group = "ProxyUtitity";

    se->AddGroup(group.c_str(), "API Proxy Utility Functions", group_description.c_str());

    RegisterGlobalFunction("array<vec3d>@ GetProxyVec3dArray()", vspMETHOD(ScriptMgrSingleton, GetProxyVec3dArray);
#undef RegisterGlobalFunctionSimple
#undef RegisterGlobalMethodScriptMgr
}

//===== Utility Functions Vec3d Proxy Array =====//
CScriptArray *ScriptMgrSingleton::GetProxyVec3dArray()
{
    //==== This Will Get Deleted By The Script Engine ====//
    CScriptArray *sarr = CScriptArray::Create(m_Vec3dArrayType, m_ProxyVec3dArray.size());
    for (int i = 0; i < (int)sarr->GetSize(); i++)
    {
        sarr->SetValue(i, &m_ProxyVec3dArray[i]);
    }
    return sarr;
}

//==== Utility Functions String Proxy Array =====//
CScriptArray *ScriptMgrSingleton::GetProxyStringArray()
{
    //==== This Will Get Deleted By The Script Engine ====//
    CScriptArray *sarr = CScriptArray::Create(m_StringArrayType, m_ProxyStringArray.size());
    for (int i = 0; i < (int)sarr->GetSize(); i++)
    {
        sarr->SetValue(i, &m_ProxyStringArray[i]);
    }
    return sarr;
}

//==== Utility Functions String Proxy Array =====//
CScriptArray *ScriptMgrSingleton::GetProxyIntArray()
{
    //==== This Will Get Deleted By The Script Engine ====//
    CScriptArray *sarr = CScriptArray::Create(m_IntArrayType, m_ProxyIntArray.size());
    for (int i = 0; i < (int)sarr->GetSize(); i++)
    {
        sarr->SetValue(i, &m_ProxyIntArray[i]);
    }
    return sarr;
}

//==== Utility Functions String Proxy Array =====//
CScriptArray *ScriptMgrSingleton::GetProxyDoubleArray()
{
    //==== This Will Get Deleted By The Script Engine ====//
    CScriptArray *sarr = CScriptArray::Create(m_DoubleArrayType, m_ProxyDoubleArray.size());
    for (int i = 0; i < (int)sarr->GetSize(); i++)
    {
        sarr->SetValue(i, &m_ProxyDoubleArray[i]);
    }
    return sarr;
}

CScriptArray *ScriptMgrSingleton::GetProxyDoubleMatArray()
{
    CScriptArray *sarr = CScriptArray::Create(m_DoubleMatArrayType, m_ProxyDoubleMatArray.size());
    for (int i = 0; i < (int)sarr->GetSize(); i++)
    {
        CScriptArray *darr = CScriptArray::Create(m_DoubleArrayType, m_ProxyDoubleMatArray[i].size());
        for (int j = 0; j < (int)darr->GetSize(); j++)
        {
            darr->SetValue(j, &m_ProxyDoubleMatArray[i][j]);
        }
        sarr->SetValue(i, &darr);
    }
    return sarr;
}

template <class T>
void ScriptMgrSingleton::FillArray(vector<T> &in, CScriptArray *out)
{
    out->Resize(in.size());
    for (int i = 0; i < (int)in.size(); i++)
    {
        out->SetValue(i, &in[i]);
    }
}

template <class T>
void ScriptMgrSingleton::FillArray(CScriptArray *in, vector<T> &out)
{
    out.resize(in->GetSize());
    for (int i = 0; i < (int)in->GetSize(); i++)
    {
        out[i] = *(T *)(in->At(i));
    }
}

//==== Wrappers For API Functions That Return Vectors ====//
CScriptArray *ScriptMgrSingleton::GetGeomTypes()
{
    m_ProxyStringArray = vsp::GetGeomTypes();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::PasteGeomClipboard(const string &parent)
{
    m_ProxyStringArray = vsp::PasteGeomClipboard(parent);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::FindGeoms()
{
    m_ProxyStringArray = vsp::FindGeoms();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::FindGeomsWithName(const string &name)
{
    m_ProxyStringArray = vsp::FindGeomsWithName(name);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetGeomParmIDs(const string &geom_id)
{
    m_ProxyStringArray = vsp::GetGeomParmIDs(geom_id);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetGeomChildren(const string &geom_id)
{
    m_ProxyStringArray = vsp::GetGeomChildren(geom_id);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetSubSurfIDVec(const string &geom_id)
{
    m_ProxyStringArray = vsp::GetSubSurfIDVec(geom_id);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetAllSubSurfIDs()
{
    m_ProxyStringArray = vsp::GetAllSubSurfIDs();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetSubSurf(const string &geom_id, const string &name)
{
    m_ProxyStringArray = vsp::GetSubSurf(geom_id, name);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetSubSurfParmIDs(const string &sub_id)
{
    m_ProxyStringArray = vsp::GetSubSurfParmIDs(sub_id);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetActiveCSNameVec(int CSGroupIndex)
{
    m_ProxyStringArray = vsp::GetActiveCSNameVec(CSGroupIndex);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetCompleteCSNameVec()
{
    m_ProxyStringArray = vsp::GetCompleteCSNameVec();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetAvailableCSNameVec(int CSGroupIndex)
{
    m_ProxyStringArray = vsp::GetAvailableCSNameVec(CSGroupIndex);
    return GetProxyStringArray();
}

void ScriptMgrSingleton::AddSelectedToCSGroup(CScriptArray *selected, int CSGroupIndex)
{
    vector<int> int_vec;
    FillArray(selected, int_vec);

    vsp::AddSelectedToCSGroup(int_vec, CSGroupIndex);
}

void ScriptMgrSingleton::RemoveSelectedFromCSGroup(CScriptArray *selected, int CSGroupIndex)
{
    vector<int> int_vec;
    FillArray(selected, int_vec);

    vsp::RemoveSelectedFromCSGroup(int_vec, CSGroupIndex);
}

CScriptArray *ScriptMgrSingleton::GetUnsteadyGroupCompIDs(int group_index)
{
    m_ProxyStringArray = vsp::GetUnsteadyGroupCompIDs(group_index);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetUnsteadyGroupSurfIndexes(int group_index)
{
    m_ProxyIntArray = vsp::GetUnsteadyGroupSurfIndexes(group_index);
    return GetProxyIntArray();
}

CScriptArray *ScriptMgrSingleton::GetXSecParmIDs(const string &xsec_id)
{
    m_ProxyStringArray = vsp::GetXSecParmIDs(xsec_id);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::ReadFileXSec(const string &xsec_id, const string &file_name)
{
    m_ProxyVec3dArray = vsp::ReadFileXSec(xsec_id, file_name);
    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetAirfoilUpperPnts(const string &xsec_id)
{
    m_ProxyVec3dArray = vsp::GetAirfoilUpperPnts(xsec_id);
    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetAirfoilLowerPnts(const string &xsec_id)
{
    m_ProxyVec3dArray = vsp::GetAirfoilLowerPnts(xsec_id);
    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::ReadBORFileXSec(const string &bor_id, const string &file_name)
{
    m_ProxyVec3dArray = vsp::ReadBORFileXSec(bor_id, file_name);
    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetBORAirfoilUpperPnts(const string &bor_id)
{
    m_ProxyVec3dArray = vsp::GetBORAirfoilUpperPnts(bor_id);
    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetBORAirfoilLowerPnts(const string &bor_id)
{
    m_ProxyVec3dArray = vsp::GetBORAirfoilLowerPnts(bor_id);
    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetGeomSetAtIndex(int index)
{
    m_ProxyStringArray = vsp::GetGeomSetAtIndex(index);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetGeomSet(const string &name)
{
    m_ProxyStringArray = vsp::GetGeomSet(name);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::ListAnalysis()
{
    m_ProxyStringArray = vsp::ListAnalysis();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetAnalysisInputNames(const string &analysis)
{
    m_ProxyStringArray = vsp::GetAnalysisInputNames(analysis);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetIntAnalysisInput(const string &analysis, const string &name, int index)
{
    m_ProxyIntArray = vsp::GetIntAnalysisInput(analysis, name, index);
    return GetProxyIntArray();
}

CScriptArray *ScriptMgrSingleton::GetDoubleAnalysisInput(const string &analysis, const string &name, int index)
{
    m_ProxyDoubleArray = vsp::GetDoubleAnalysisInput(analysis, name, index);
    return GetProxyDoubleArray();
}

CScriptArray *ScriptMgrSingleton::GetStringAnalysisInput(const string &analysis, const string &name, int index)
{
    m_ProxyStringArray = vsp::GetStringAnalysisInput(analysis, name, index);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetVec3dAnalysisInput(const string &analysis, const string &name, int index)
{
    m_ProxyVec3dArray = vsp::GetVec3dAnalysisInput(analysis, name, index);
    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetAllResultsNames()
{
    m_ProxyStringArray = vsp::GetAllResultsNames();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetAllDataNames(const string &results_id)
{
    m_ProxyStringArray = vsp::GetAllDataNames(results_id);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetIntResults(const string &id, const string &name, int index)
{
    m_ProxyIntArray = vsp::GetIntResults(id, name, index);
    return GetProxyIntArray();
}

CScriptArray *ScriptMgrSingleton::GetDoubleResults(const string &id, const string &name, int index)
{
    m_ProxyDoubleArray = vsp::GetDoubleResults(id, name, index);
    return GetProxyDoubleArray();
}

CScriptArray *ScriptMgrSingleton::GetDoubleMatResults(const string &id, const string &name, int index)
{
    m_ProxyDoubleMatArray = vsp::GetDoubleMatResults(id, name, index);
    return GetProxyDoubleMatArray();
}

CScriptArray *ScriptMgrSingleton::GetStringResults(const string &id, const string &name, int index)
{
    m_ProxyStringArray = vsp::GetStringResults(id, name, index);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetVec3dResults(const string &id, const string &name, int index)
{
    m_ProxyVec3dArray = vsp::GetVec3dResults(id, name, index);
    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::FindContainers()
{
    m_ProxyStringArray = vsp::FindContainers();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::FindContainersWithName(const string &name)
{
    m_ProxyStringArray = vsp::FindContainersWithName(name);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::FindContainerGroupNames(const string &parm_container_id)
{
    m_ProxyStringArray = vsp::FindContainerGroupNames(parm_container_id);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::FindContainerParmIDs(const string &parm_container_id)
{
    m_ProxyStringArray = vsp::FindContainerParmIDs(parm_container_id);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetUpperCSTCoefs(const string &xsec_id)
{
    m_ProxyDoubleArray = vsp::GetUpperCSTCoefs(xsec_id);
    return GetProxyDoubleArray();
}

CScriptArray *ScriptMgrSingleton::GetLowerCSTCoefs(const string &xsec_id)
{
    m_ProxyDoubleArray = vsp::GetLowerCSTCoefs(xsec_id);
    return GetProxyDoubleArray();
}

CScriptArray *ScriptMgrSingleton::GetBORUpperCSTCoefs(const string &bor_id)
{
    m_ProxyDoubleArray = vsp::GetBORUpperCSTCoefs(bor_id);
    return GetProxyDoubleArray();
}

CScriptArray *ScriptMgrSingleton::GetBORLowerCSTCoefs(const string &bor_id)
{
    m_ProxyDoubleArray = vsp::GetBORLowerCSTCoefs(bor_id);
    return GetProxyDoubleArray();
}

void ScriptMgrSingleton::DeleteGeomVec(CScriptArray *del_arr)
{
    vector<string> del_vec;
    FillArray(del_arr, del_vec);

    vsp::DeleteGeomVec(del_vec);
}

void ScriptMgrSingleton::SetXSecPnts(const string &xsec_id, CScriptArray *pnt_arr)
{
    vector<vec3d> pnt_vec;
    FillArray(pnt_arr, pnt_vec);

    vsp::SetXSecPnts(xsec_id, pnt_vec);
}

void ScriptMgrSingleton::SetAirfoilUpperPnts(const string &xsec_id, CScriptArray *up_pnt_arr)
{
    vector<vec3d> up_pnt_vec;
    up_pnt_vec.resize(up_pnt_arr->GetSize());
    for (int i = 0; i < (int)up_pnt_arr->GetSize(); i++)
    {
        up_pnt_vec[i] = *(vec3d *)(up_pnt_arr->At(i));
    }

    vsp::SetAirfoilUpperPnts(xsec_id, up_pnt_vec);
}

void ScriptMgrSingleton::SetAirfoilLowerPnts(const string &xsec_id, CScriptArray *low_pnt_arr)
{
    vector<vec3d> low_pnt_vec;
    low_pnt_vec.resize(low_pnt_arr->GetSize());
    for (int i = 0; i < (int)low_pnt_arr->GetSize(); i++)
    {
        low_pnt_vec[i] = *(vec3d *)(low_pnt_arr->At(i));
    }

    vsp::SetAirfoilLowerPnts(xsec_id, low_pnt_vec);
}

void ScriptMgrSingleton::SetAirfoilPnts(const string &xsec_id, CScriptArray *up_pnt_arr, CScriptArray *low_pnt_arr)
{
    vector<vec3d> up_pnt_vec;
    FillArray(up_pnt_arr, up_pnt_vec);

    vector<vec3d> low_pnt_vec;
    FillArray(low_pnt_arr, low_pnt_vec);

    vsp::SetAirfoilPnts(xsec_id, up_pnt_vec, low_pnt_vec);
}

void ScriptMgrSingleton::SetBORXSecPnts(const string &bor_id, CScriptArray *pnt_arr)
{
    vector<vec3d> pnt_vec;
    FillArray(pnt_arr, pnt_vec);

    vsp::SetBORXSecPnts(bor_id, pnt_vec);
}

void ScriptMgrSingleton::SetBORAirfoilUpperPnts(const string &bor_id, CScriptArray *up_pnt_arr)
{
    vector<vec3d> up_pnt_vec;
    up_pnt_vec.resize(up_pnt_arr->GetSize());
    for (int i = 0; i < (int)up_pnt_arr->GetSize(); i++)
    {
        up_pnt_vec[i] = *(vec3d *)(up_pnt_arr->At(i));
    }

    vsp::SetBORAirfoilUpperPnts(bor_id, up_pnt_vec);
}

void ScriptMgrSingleton::SetBORAirfoilLowerPnts(const string &bor_id, CScriptArray *low_pnt_arr)
{
    vector<vec3d> low_pnt_vec;
    low_pnt_vec.resize(low_pnt_arr->GetSize());
    for (int i = 0; i < (int)low_pnt_arr->GetSize(); i++)
    {
        low_pnt_vec[i] = *(vec3d *)(low_pnt_arr->At(i));
    }

    vsp::SetBORAirfoilLowerPnts(bor_id, low_pnt_vec);
}

void ScriptMgrSingleton::SetBORAirfoilPnts(const string &bor_id, CScriptArray *up_pnt_arr, CScriptArray *low_pnt_arr)
{
    vector<vec3d> up_pnt_vec;
    FillArray(up_pnt_arr, up_pnt_vec);

    vector<vec3d> low_pnt_vec;
    FillArray(low_pnt_arr, low_pnt_vec);

    vsp::SetBORAirfoilPnts(bor_id, up_pnt_vec, low_pnt_vec);
}

CScriptArray *ScriptMgrSingleton::GetAirfoilCoordinates(const std::string &geom_id, const double &foilsurf_u)
{
    m_ProxyVec3dArray = vsp::GetAirfoilCoordinates(geom_id, foilsurf_u);

    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetHersheyBarLiftDist(const int &npts, const double &alpha, const double &Vinf, const double &span, bool full_span_flag)
{
    m_ProxyVec3dArray = vsp::GetHersheyBarLiftDist(npts, alpha, Vinf, span, full_span_flag);

    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetHersheyBarDragDist(const int &npts, const double &alpha, const double &Vinf, const double &span, bool full_span_flag)
{
    m_ProxyVec3dArray = vsp::GetHersheyBarDragDist(npts, alpha, Vinf, span, full_span_flag);

    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetVKTAirfoilPnts(const int &npts, const double &alpha, const double &epsilon, const double &kappa, const double &tau)
{
    m_ProxyVec3dArray = vsp::GetVKTAirfoilPnts(npts, alpha, epsilon, kappa, tau);

    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetVKTAirfoilCpDist(const double &alpha, const double &epsilon, const double &kappa, const double &tau, CScriptArray *xyzdata)
{
    vector<vec3d> xyz_vec;
    FillArray(xyzdata, xyz_vec);

    m_ProxyDoubleArray = vsp::GetVKTAirfoilCpDist(alpha, epsilon, kappa, tau, xyz_vec);

    return GetProxyDoubleArray();
}

CScriptArray *ScriptMgrSingleton::GetEllipsoidSurfPnts(const vec3d &center, const vec3d &abc_rad, int u_npts, int w_npts)
{
    m_ProxyVec3dArray = vsp::GetEllipsoidSurfPnts(center, abc_rad, u_npts, w_npts);

    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetFeatureLinePnts(const string &geom_id)
{
    m_ProxyVec3dArray = vsp::GetFeatureLinePnts(geom_id);

    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::GetEllipsoidCpDist(CScriptArray *surf_pnt_arr, const vec3d &abc_rad, const vec3d &V_inf)
{
    vector<vec3d> surf_pnt_vec;
    FillArray(surf_pnt_arr, surf_pnt_vec);

    m_ProxyDoubleArray = vsp::GetEllipsoidCpDist(surf_pnt_vec, abc_rad, V_inf);

    return GetProxyDoubleArray();
}

void ScriptMgrSingleton::SetUpperCST(const string &xsec_id, int deg, CScriptArray *coefs_arr)
{
    vector<double> coefs_vec;
    FillArray(coefs_arr, coefs_vec);

    vsp::SetUpperCST(xsec_id, deg, coefs_vec);
}

void ScriptMgrSingleton::SetLowerCST(const string &xsec_id, int deg, CScriptArray *coefs_arr)
{
    vector<double> coefs_vec;
    FillArray(coefs_arr, coefs_vec);

    vsp::SetLowerCST(xsec_id, deg, coefs_vec);
}

void ScriptMgrSingleton::SetBORUpperCST(const string &bor_id, int deg, CScriptArray *coefs_arr)
{
    vector<double> coefs_vec;
    FillArray(coefs_arr, coefs_vec);

    vsp::SetBORUpperCST(bor_id, deg, coefs_vec);
}

void ScriptMgrSingleton::SetBORLowerCST(const string &bor_id, int deg, CScriptArray *coefs_arr)
{
    vector<double> coefs_vec;
    FillArray(coefs_arr, coefs_vec);

    vsp::SetBORLowerCST(bor_id, deg, coefs_vec);
}

//==== Edit Curve XSec Functions ====//
CScriptArray *ScriptMgrSingleton::GetEditXSecUVec(const std::string &xsec_id)
{
    m_ProxyDoubleArray = vsp::GetEditXSecUVec(xsec_id);

    return GetProxyDoubleArray();
}

CScriptArray *ScriptMgrSingleton::GetEditXSecCtrlVec(const std::string &xsec_id, const bool non_dimensional)
{
    m_ProxyVec3dArray = vsp::GetEditXSecCtrlVec(xsec_id, non_dimensional);

    return GetProxyVec3dArray();
}

void ScriptMgrSingleton::SetEditXSecPnts(const string &xsec_id, CScriptArray *u_vec, CScriptArray *control_pts, CScriptArray *r_vec)
{
    vector<vec3d> control_pnt_vec;
    FillArray(control_pts, control_pnt_vec);

    vector<double> new_u_vec;
    FillArray(u_vec, new_u_vec);

    vector<double> new_r_vec;
    FillArray(r_vec, new_r_vec);

    vsp::SetEditXSecPnts(xsec_id, new_u_vec, control_pnt_vec, new_r_vec);
}

CScriptArray *ScriptMgrSingleton::GetEditXSecFixedUVec(const std::string &xsec_id)
{
    vector<bool> temp_vec = vsp::GetEditXSecFixedUVec(xsec_id);

    m_ProxyIntArray.clear();
    m_ProxyIntArray.resize(temp_vec.size());
    for (size_t i = 0; i < temp_vec.size(); i++)
    {
        // Cast bool to int
        m_ProxyIntArray[i] = (int)temp_vec[i];
    }

    return GetProxyIntArray();
}

void ScriptMgrSingleton::SetEditXSecFixedUVec(const string &xsec_id, CScriptArray *fixed_u_vec)
{
    vector<bool> new_fixed_u_vec;
    FillArray(fixed_u_vec, new_fixed_u_vec);

    vsp::SetEditXSecFixedUVec(xsec_id, new_fixed_u_vec);
}

//==== Variable Preset Functions ====//
CScriptArray *ScriptMgrSingleton::GetVarPresetGroupNames()
{
    m_ProxyStringArray = vsp::GetVarPresetGroupNames();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetVarPresetSettingNamesWName(string group_name)
{
    m_ProxyStringArray = vsp::GetVarPresetSettingNamesWName(group_name);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetVarPresetSettingNamesWIndex(int group_index)
{
    m_ProxyStringArray = vsp::GetVarPresetSettingNamesWIndex(group_index);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetVarPresetParmVals()
{
    m_ProxyDoubleArray = vsp::GetVarPresetParmVals();
    return GetProxyDoubleArray();
}

CScriptArray *ScriptMgrSingleton::GetVarPresetParmValsWNames(string group_name, string setting_name)
{
    m_ProxyDoubleArray = vsp::GetVarPresetParmValsWNames(group_name, setting_name);
    return GetProxyDoubleArray();
}

CScriptArray *ScriptMgrSingleton::GetVarPresetParmIDs()
{
    m_ProxyStringArray = vsp::GetVarPresetParmIDs();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetVarPresetParmIDsWName(string group_name)
{
    m_ProxyStringArray = vsp::GetVarPresetParmIDsWName(group_name);
    return GetProxyStringArray();
}

void ScriptMgrSingleton::AddVarPresetGroup(const string &group_name)
{
    vsp::AddVarPresetGroup(group_name);
}

void ScriptMgrSingleton::AddVarPresetSetting(const string &setting_name)
{
    vsp::AddVarPresetSetting(setting_name);
}

void ScriptMgrSingleton::AddVarPresetParm(const string &parm_ID)
{
    vsp::AddVarPresetParm(parm_ID);
}

void ScriptMgrSingleton::AddVarPresetParm(const string &parm_ID, string group_name)
{
    vsp::AddVarPresetParm(parm_ID, group_name);
}

void ScriptMgrSingleton::EditVarPresetParm(const string &parm_ID, double parm_val)
{
    vsp::EditVarPresetParm(parm_ID, parm_val);
}

void ScriptMgrSingleton::EditVarPresetParm(const string &parm_ID, double parm_val, string group_name, string setting_name)
{
    vsp::EditVarPresetParm(parm_ID, parm_val, group_name, setting_name);
}

void ScriptMgrSingleton::DeleteVarPresetParm(const string &parm_ID)
{
    vsp::DeleteVarPresetParm(parm_ID);
}

void ScriptMgrSingleton::DeleteVarPresetParm(const string &parm_ID, string group_name)
{
    vsp::DeleteVarPresetParm(parm_ID, group_name);
}

void ScriptMgrSingleton::SwitchVarPreset(string group_name, string setting_name)
{
    vsp::SwitchVarPreset(group_name, setting_name);
}

void ScriptMgrSingleton::DeleteVarPresetSet(string group_name, string setting_name)
{
    vsp::DeleteVarPresetSet(group_name, setting_name);
}

//==== PCurve Functions ====//
void ScriptMgrSingleton::SetPCurve(const string &geom_id, const int &pcurveid, CScriptArray *t_arr, CScriptArray *val_arr, const int &newtype)
{
    vector<double> t_vec;
    FillArray(t_arr, t_vec);

    vector<double> val_vec;
    FillArray(val_arr, val_vec);

    vsp::SetPCurve(geom_id, pcurveid, t_vec, val_vec, newtype);
}

CScriptArray *ScriptMgrSingleton::PCurveGetTVec(const std::string &geom_id, const int &pcurveid)
{
    m_ProxyDoubleArray = vsp::PCurveGetTVec(geom_id, pcurveid);
    return GetProxyDoubleArray();
}

CScriptArray *ScriptMgrSingleton::PCurveGetValVec(const std::string &geom_id, const int &pcurveid)
{
    m_ProxyDoubleArray = vsp::PCurveGetValVec(geom_id, pcurveid);
    return GetProxyDoubleArray();
}

//==== Parasite Drag Tool Functions ====//
void ScriptMgrSingleton::AddExcrescence(const std::string &excresName, int excresType, double excresVal)
{
    vsp::AddExcrescence(excresName, excresType, excresVal);
}

void ScriptMgrSingleton::DeleteExcrescence(int index)
{
    vsp::DeleteExcrescence(index);
}

CScriptArray *ScriptMgrSingleton::GetFeaStructIDVec()
{
    m_ProxyStringArray = vsp::GetFeaStructIDVec();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetFeaSubSurfIDVec(const string &fea_struct_id)
{
    m_ProxyStringArray = vsp::GetFeaSubSurfIDVec(fea_struct_id);
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetFeaPartIDVec(const string &fea_struct_id)
{
    m_ProxyStringArray = vsp::GetFeaPartIDVec(fea_struct_id);
    return GetProxyStringArray();
}

void ScriptMgrSingleton::SetIntAnalysisInput(const string &analysis, const string &name, CScriptArray *indata, int index)
{
    vector<int> indata_vec;
    FillArray(indata, indata_vec);

    vsp::SetIntAnalysisInput(analysis, name, indata_vec, index);
}

void ScriptMgrSingleton::SetDoubleAnalysisInput(const string &analysis, const string &name, CScriptArray *indata, int index)
{
    vector<double> indata_vec;
    FillArray(indata, indata_vec);

    vsp::SetDoubleAnalysisInput(analysis, name, indata_vec, index);
}

void ScriptMgrSingleton::SetStringAnalysisInput(const string &analysis, const string &name, CScriptArray *indata, int index)
{
    vector<string> indata_vec;
    FillArray(indata, indata_vec);

    vsp::SetStringAnalysisInput(analysis, name, indata_vec, index);
}

void ScriptMgrSingleton::SetVec3dAnalysisInput(const string &analysis, const string &name, CScriptArray *indata, int index)
{
    vector<vec3d> indata_vec;
    FillArray(indata, indata_vec);

    vsp::SetVec3dAnalysisInput(analysis, name, indata_vec, index);
}

CScriptArray *ScriptMgrSingleton::CompVecPnt01(const string &geom_id, const int &surf_indx, CScriptArray *us, CScriptArray *ws)
{
    vector<double> in_us;
    FillArray(us, in_us);

    vector<double> in_ws;
    FillArray(ws, in_ws);

    m_ProxyVec3dArray = vsp::CompVecPnt01(geom_id, surf_indx, in_us, in_ws);
    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::CompVecPntRST(const string &geom_id, const int &surf_indx, CScriptArray *rs, CScriptArray *ss, CScriptArray *ts)
{
    vector<double> in_rs;
    FillArray(rs, in_rs);

    vector<double> in_ss;
    FillArray(ss, in_ss);

    vector<double> in_ts;
    FillArray(ts, in_ts);

    m_ProxyVec3dArray = vsp::CompVecPntRST(geom_id, surf_indx, in_rs, in_ss, in_ts);
    return GetProxyVec3dArray();
}

CScriptArray *ScriptMgrSingleton::CompVecNorm01(const string &geom_id, const int &surf_indx, CScriptArray *us, CScriptArray *ws)
{
    vector<double> in_us;
    FillArray(us, in_us);

    vector<double> in_ws;
    FillArray(ws, in_ws);

    m_ProxyVec3dArray = vsp::CompVecNorm01(geom_id, surf_indx, in_us, in_ws);
    return GetProxyVec3dArray();
}

void ScriptMgrSingleton::CompVecCurvature01(const string &geom_id, const int &surf_indx, CScriptArray *us, CScriptArray *ws, CScriptArray *k1s, CScriptArray *k2s, CScriptArray *kas, CScriptArray *kgs)
{
    vector<double> in_us;
    FillArray(us, in_us);

    vector<double> in_ws;
    FillArray(ws, in_ws);

    vector<double> out_k1s;
    vector<double> out_k2s;
    vector<double> out_kas;
    vector<double> out_kgs;

    vsp::CompVecCurvature01(geom_id, surf_indx, in_us, in_ws, out_k1s, out_k2s, out_kas, out_kgs);

    FillArray(out_k1s, k1s);
    FillArray(out_k2s, k2s);
    FillArray(out_kas, kas);
    FillArray(out_kgs, kgs);
}

void ScriptMgrSingleton::ProjVecPnt01(const string &geom_id, const int &surf_indx, CScriptArray *pts, CScriptArray *us, CScriptArray *ws, CScriptArray *ds)
{
    vector<vec3d> in_pts;
    FillArray(pts, in_pts);

    vector<double> out_us;
    vector<double> out_ws;
    vector<double> out_ds;

    vsp::ProjVecPnt01(geom_id, surf_indx, in_pts, out_us, out_ws, out_ds);

    FillArray(out_us, us);
    FillArray(out_ws, ws);
    FillArray(out_ds, ds);
}

void ScriptMgrSingleton::ProjVecPnt01Guess(const string &geom_id, const int &surf_indx, CScriptArray *pts, CScriptArray *u0s, CScriptArray *w0s, CScriptArray *us, CScriptArray *ws, CScriptArray *ds)
{
    vector<vec3d> in_pts;
    FillArray(pts, in_pts);

    vector<double> in_u0s;
    FillArray(u0s, in_u0s);

    vector<double> in_w0s;
    FillArray(w0s, in_w0s);

    vector<double> out_us;
    vector<double> out_ws;
    vector<double> out_ds;

    vsp::ProjVecPnt01Guess(geom_id, surf_indx, in_pts, in_u0s, in_w0s, out_us, out_ws, out_ds);

    FillArray(out_us, us);
    FillArray(out_ws, ws);
    FillArray(out_ds, ds);
}

void ScriptMgrSingleton::AxisProjVecPnt01(const string &geom_id, const int &surf_indx, const int &iaxis, CScriptArray *pts, CScriptArray *us, CScriptArray *ws, CScriptArray *ps_out, CScriptArray *ds)
{
    vector<vec3d> in_pts;
    FillArray(pts, in_pts);

    vector<double> out_us;
    vector<double> out_ws;
    vector<double> out_ds;
    vector<vec3d> out_pts;

    vsp::AxisProjVecPnt01(geom_id, surf_indx, iaxis, in_pts, out_us, out_ws, out_pts, out_ds);

    FillArray(out_us, us);
    FillArray(out_ws, ws);
    FillArray(out_ds, ds);
    FillArray(out_pts, ps_out);
}

void ScriptMgrSingleton::AxisProjVecPnt01Guess(const string &geom_id, const int &surf_indx, const int &iaxis, CScriptArray *pts, CScriptArray *u0s, CScriptArray *w0s, CScriptArray *us, CScriptArray *ws, CScriptArray *ps_out, CScriptArray *ds)
{
    vector<vec3d> in_pts;
    FillArray(pts, in_pts);

    vector<double> in_u0s;
    FillArray(u0s, in_u0s);

    vector<double> in_w0s;
    FillArray(w0s, in_w0s);

    vector<double> out_us;
    vector<double> out_ws;
    vector<double> out_ds;
    vector<vec3d> out_pts;

    vsp::AxisProjVecPnt01Guess(geom_id, surf_indx, iaxis, in_pts, in_u0s, in_w0s, out_us, out_ws, out_pts, out_ds);

    FillArray(out_us, us);
    FillArray(out_ws, ws);
    FillArray(out_ds, ds);
    FillArray(out_pts, ps_out);
}

void ScriptMgrSingleton::FindRSTVec(const string &geom_id, const int &surf_indx, CScriptArray *pts, CScriptArray *rs, CScriptArray *ss, CScriptArray *ts, CScriptArray *ds)
{
    vector<vec3d> in_pts;
    FillArray(pts, in_pts);

    vector<double> out_rs;
    vector<double> out_ss;
    vector<double> out_ts;
    vector<double> out_ds;

    vsp::FindRSTVec(geom_id, surf_indx, in_pts, out_rs, out_ss, out_ts, out_ds);

    FillArray(out_rs, rs);
    FillArray(out_ss, ss);
    FillArray(out_ts, ts);
    FillArray(out_ds, ds);
}

void ScriptMgrSingleton::FindRSTVecGuess(const string &geom_id, const int &surf_indx, CScriptArray *pts, CScriptArray *r0s, CScriptArray *s0s, CScriptArray *t0s, CScriptArray *rs, CScriptArray *ss, CScriptArray *ts, CScriptArray *ds)
{
    vector<vec3d> in_pts;
    FillArray(pts, in_pts);

    vector<double> in_r0s;
    vector<double> in_s0s;
    vector<double> in_t0s;

    FillArray(r0s, in_r0s);
    FillArray(s0s, in_s0s);
    FillArray(t0s, in_t0s);

    vector<double> out_rs;
    vector<double> out_ss;
    vector<double> out_ts;
    vector<double> out_ds;

    vsp::FindRSTVecGuess(geom_id, surf_indx, in_pts, in_r0s, in_s0s, in_t0s, out_rs, out_ss, out_ts, out_ds);

    FillArray(out_rs, rs);
    FillArray(out_ss, ss);
    FillArray(out_ts, ts);
    FillArray(out_ds, ds);
}

void ScriptMgrSingleton::ConvertRSTtoLMNVec(const string &geom_id, const int &surf_indx, CScriptArray *rs, CScriptArray *ss, CScriptArray *ts, CScriptArray *ls, CScriptArray *ms, CScriptArray *ns)
{
    vector<double> in_rs;
    vector<double> in_ss;
    vector<double> in_ts;

    FillArray(rs, in_rs);
    FillArray(ss, in_ss);
    FillArray(ts, in_ts);

    vector<double> out_ls;
    vector<double> out_ms;
    vector<double> out_ns;

    vsp::ConvertRSTtoLMNVec(geom_id, surf_indx, in_rs, in_ss, in_ts, out_ls, out_ms, out_ns);

    FillArray(out_ls, ls);
    FillArray(out_ms, ms);
    FillArray(out_ns, ns);
}

void ScriptMgrSingleton::ConvertLMNtoRSTVec(const string &geom_id, const int &surf_indx, CScriptArray *ls, CScriptArray *ms, CScriptArray *ns, CScriptArray *rs, CScriptArray *ss, CScriptArray *ts)
{
    vector<double> in_ls;
    vector<double> in_ms;
    vector<double> in_ns;

    FillArray(ls, in_ls);
    FillArray(ms, in_ms);
    FillArray(ns, in_ns);

    vector<double> out_rs;
    vector<double> out_ss;
    vector<double> out_ts;

    vsp::ConvertLMNtoRSTVec(geom_id, surf_indx, in_ls, in_ms, in_ns, out_rs, out_ss, out_ts);

    FillArray(out_rs, rs);
    FillArray(out_ss, ss);
    FillArray(out_ts, ts);
}

void ScriptMgrSingleton::GetUWTess01(const string &geom_id, int &surf_indx, CScriptArray *us, CScriptArray *ws)
{
    vector<double> out_us;
    vector<double> out_ws;

    vsp::GetUWTess01(geom_id, surf_indx, out_us, out_ws);

    FillArray(out_us, us);
    FillArray(out_ws, ws);
}

//=== Register Measure Functions ===//
CScriptArray *ScriptMgrSingleton::GetAllRulers()
{
    m_ProxyStringArray = vsp::GetAllRulers();
    return GetProxyStringArray();
}

CScriptArray *ScriptMgrSingleton::GetAllProbes()
{
    m_ProxyStringArray = vsp::GetAllProbes();
    return GetProxyStringArray();
}

//==== Console Print String Data ====//
void ScriptMgrSingleton::Print(const string &data, bool new_line)
{
    printf(" %s ", data.c_str());
    if (new_line)
        printf("\n");
}

//==== Console Print Vec3d Data ====//
void ScriptMgrSingleton::Print(const vec3d &data, bool new_line)
{
    printf(" %f, %f, %f ", data.x(), data.y(), data.z());
    if (new_line)
        printf("\n");
}

//==== Console Print Double Data ====//
void ScriptMgrSingleton::Print(double data, bool new_line)
{
    printf(" %f ", data);
    if (new_line)
        printf("\n");
}

//==== Console Print Int Data ====//
void ScriptMgrSingleton::Print(int data, bool new_line)
{
    printf(" %d ", data);
    if (new_line)
        printf("\n");
}