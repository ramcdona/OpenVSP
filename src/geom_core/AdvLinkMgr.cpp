//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
//// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// AdvLinkMgr.h: Parm Adv Link Mgr Singleton.
//
//////////////////////////////////////////////////////////////////////

#include "AdvLinkMgr.h"
#include "ParmMgr.h"
#include "StringUtil.h"
#include "StlHelper.h"

/*! Constructor */
AdvLinkMgrSingleton::AdvLinkMgrSingleton()
{
    m_ActiveLink = NULL;
    m_EditLinkIndex = 0;
}

/*! Init*/
void AdvLinkMgrSingleton::Init()
{
}

/*! Wipe all links and reset state.*/
void AdvLinkMgrSingleton::Wype()
{
    //==== Delete All Links ====//
    for (int i = 0; i < (int)m_LinkVec.size(); i++)
    {
        delete m_LinkVec[i];
    }
    m_LinkVec.clear();
    m_ActiveLink = NULL;
    m_EditLinkIndex = 0;
}

/*! Renew*/
void AdvLinkMgrSingleton::Renew()
{
    Wype();
    Init();
}

/*! Check For Duplicate Link Name */
bool AdvLinkMgrSingleton::DuplicateLinkName(const string &name)
{
    for (int i = 0; i < (int)m_LinkVec.size(); i++)
    {
        if (m_LinkVec[i]->GetName() == name)
            return true;
    }
    return false;
}

/*! Add A Link */
AdvLink *AdvLinkMgrSingleton::AddLink(const string &name)
{
    //==== Create Unique Name ====//
    string base_name = name;
    if (base_name.size() == 0)
    {
        base_name = "Unnamed_Link";
    }
    string link_name = base_name;

    int cnt = 1;
    while (DuplicateLinkName(link_name))
    {
        link_name = base_name + "_" + StringUtil::int_to_string(cnt, "%d");
        cnt++;
    }

    AdvLink *alink = new AdvLink();
    alink->SetName(link_name);
    m_LinkVec.push_back(alink);
    m_EditLinkIndex = (int)m_LinkVec.size() - 1;

    return alink;
}

/*! Delete a Link*/
void AdvLinkMgrSingleton::DelLink(AdvLink *link_ptr)
{
    if (!link_ptr)
    {
        return;
    }

    if (m_ActiveLink == link_ptr)
    {
        m_ActiveLink = NULL;
    }
    m_EditLinkIndex = -1;

    vector_remove_val(m_LinkVec, link_ptr);
    delete link_ptr;
}

/*! Delete all links*/
void AdvLinkMgrSingleton::DelAllLinks()
{
    m_EditLinkIndex = -1;
    m_ActiveLink = NULL;

    for (int i = 0; i < (int)m_LinkVec.size(); i++)
    {
        delete m_LinkVec[i];
    }
    m_LinkVec.clear();
}

/*! Check links*/
void AdvLinkMgrSingleton::CheckLinks()
{
    //==== Check If Any Parms Have Added/Removed From Last Check ====//
    static int check_links_stamp = 0;
    if (ParmMgr.GetNumParmChanges() == check_links_stamp)
    {
        return;
    }

    check_links_stamp = ParmMgr.GetNumParmChanges();

    for (int i = 0; i < (int)m_LinkVec.size(); i++)
    {
        m_LinkVec[i]->ValidateParms();
    }
}

/*! Get Link*/
AdvLink *AdvLinkMgrSingleton::GetLink(int index)
{
    if (index >= 0 && index < (int)m_LinkVec.size())
    {
        return m_LinkVec[index];
    }
    return NULL;
}

/*!
    Add an Advanced Link input Parm
    \code{.cpp}
    // Add Pod Geom
    string pid = AddGeom( "POD" );

    string tess_u_id = FindParm( pid, "Tess_U", "Shape" );

    AddInput( tess_u_id, "ExampleVariable" );
    \endcode
    \param [in] parm_id Parm ID
    \param [in] var_name Advanced Link variable name
*/

void AdvLinkMgrSingleton::AddInput(const string &parm_id, const string &var_name)
{
    AddInputOutput(parm_id, var_name, true);
}

/*!
    Add an Advanced Link output Parm
    \code{.cpp}
    // Add Pod Geom
    string pid = AddGeom( "POD" );

    string tess_u_id = FindParm( pid, "Tess_U", "Shape" );

    AddOutput( tess_u_id, "ExampleVariable" );
    \endcode
    \param [in] parm_id Parm ID
    \param [in] var_name Advanced Link variable name
*/

void AdvLinkMgrSingleton::AddOutput(const string &parm_id, const string &var_name)
{
    AddInputOutput(parm_id, var_name, false);
}

/*! Used internally for add input and add output.*/
void AdvLinkMgrSingleton::AddInputOutput(const string &parm_id, const string &var_name, bool input_flag)
{
    AdvLink *edit_link = GetLink(GetEditLinkIndex());
    if (!edit_link)
        return;

    //==== Find Parm Ptr ===//
    Parm *parm_ptr = ParmMgr.FindParm(parm_id);
    if (!parm_ptr)
        return;

    VarDef pd;
    pd.m_ParmID = parm_id;
    pd.m_VarName = var_name;

    edit_link->AddVar(pd, input_flag);
}

/*!
    Set an Advanced Link variable to the specified value
    \code{.cpp}
    // Add Pod Geom
    string pid = AddGeom( "POD" );

    string tess_u_id = FindParm( pid, "Tess_U", "Shape" );

    AddInput( tess_u_id, "ExampleVariable" );

    SetVar( "ExampleVariable", 20 );
    \endcode
    \param [in] var_name Advanced Link variable name
    \param [in] val Value for the variable
*/

void AdvLinkMgrSingleton::SetVar(const string &var_name, double val)
{
    if (!m_ActiveLink)
        return;

    m_ActiveLink->SetVar(var_name, val);
}

/*!
    Get the value of the specified Advanced Link variable
    \code{.cpp}
    // Add Pod Geom
    string pid = AddGeom( "POD" );

    string tess_u_id = FindParm( pid, "Tess_U", "Shape" );

    AddInput( tess_u_id, "ExampleVariable" );

    SetVar( "ExampleVariable", 20 );

    Print( "ExampleVariable: ", false );

    Print( GetVar( "ExampleVariable" ) );
    \endcode
    \param [in] var_name Advanced Link variable name
    \return Value for the variable
*/

double AdvLinkMgrSingleton::GetVar(const string &var_name)
{
    if (!m_ActiveLink)
        return 0.0;

    return m_ActiveLink->GetVar(var_name);
}

/*! Is the given pid an input param?*/

bool AdvLinkMgrSingleton::IsInputParm(const string &pid)
{
    for (int i = 0; i < (int)m_LinkVec.size(); i++)
    {
        vector<VarDef> def_vec = m_LinkVec[i]->GetInputVars();
        for (int j = 0; j < (int)def_vec.size(); j++)
        {
            if (pid == def_vec[j].m_ParmID)
            {
                if (ParmMgr.FindParm(pid))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

/*! Is the given pid an output param?*/

bool AdvLinkMgrSingleton::IsOutputParm(const string &pid)
{
    for (int i = 0; i < (int)m_LinkVec.size(); i++)
    {
        vector<VarDef> def_vec = m_LinkVec[i]->GetOutputVars();
        for (int j = 0; j < (int)def_vec.size(); j++)
        {
            if (pid == def_vec[j].m_ParmID)
            {
                if (ParmMgr.FindParm(pid))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

//==== Parm Changed ====//
/*! Update links*/
void AdvLinkMgrSingleton::UpdateLinks(const string &pid)
{
    //==== Find Parm Ptr ===//
    Parm *parm_ptr = ParmMgr.FindParm(pid);
    if (!parm_ptr)
    {
        return;
    }

    //==== Check All Links And Update If Needed ====//
    for (int i = 0; i < (int)m_LinkVec.size(); i++)
    {
        m_LinkVec[i]->UpdateLink(pid);
    }
}

/*! Force Update of All Links */
void AdvLinkMgrSingleton::ForceUpdate()
{
    for (int i = 0; i < (int)m_LinkVec.size(); i++)
    {
        m_LinkVec[i]->ForceUpdate();
    }
}

/*! encode xml*/

xmlNodePtr AdvLinkMgrSingleton::EncodeXml(xmlNodePtr &node)
{
    xmlNodePtr linkmgr_node = xmlNewChild(node, NULL, BAD_CAST "AdvLinkMgr", NULL);

    for (int i = 0; i < (int)m_LinkVec.size(); i++)
    {
        if (m_LinkVec[i])
        {
            m_LinkVec[i]->EncodeXml(linkmgr_node);
        }
    }

    return linkmgr_node;
}

/*! Decode xml*/
xmlNodePtr AdvLinkMgrSingleton::DecodeXml(xmlNodePtr &node)
{
    xmlNodePtr linkmgr_node = XmlUtil::GetNode(node, "AdvLinkMgr", 0);
    if (linkmgr_node)
    {
        int num = XmlUtil::GetNumNames(linkmgr_node, "AdvLink");
        for (int i = 0; i < num; i++)
        {
            xmlNodePtr link_node = XmlUtil::GetNode(linkmgr_node, "AdvLink", i);
            if (link_node)
            {
                AdvLink *link = AddLink("");
                link->DecodeXml(link_node);
                link->BuildScript();
            }
        }
    }

    return linkmgr_node;
}
