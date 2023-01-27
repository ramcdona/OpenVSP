//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// API.h: interface for the Vehicle Class and Vehicle Mgr Singleton.
// J.R Gloudemans
//
//////////////////////////////////////////////////////////////////////

#include "APIErrorMgr.h"

using namespace vsp;

//===================================================================//
//======================== Error Object =============================//
//===================================================================//

/*!
    ErrorObj is defined by an error code enum and associated error string.
*/

/*! Error object default constructor*/
ErrorObj::ErrorObj()
{
    NoError();
}

/*! Error object constructor from errorcode and string*/
ErrorObj::ErrorObj(ERROR_CODE err_code, const string &err_str)
{
    m_ErrorCode = err_code;
    m_ErrorString = err_str;
}

/*! Error object constructor from another error object*/
ErrorObj::ErrorObj(const ErrorObj &from)
{
    m_ErrorCode = from.m_ErrorCode;
    m_ErrorString = from.m_ErrorString;
}

//===================================================================//
//======================== Error Mgr ================================//
//===================================================================//

/*! Error manager singleton constructor*/
ErrorMgrSingleton::ErrorMgrSingleton()
{
    m_ErrorLastCallFlag = false;
    m_PrintErrors = true;
    MessageBase::Register(string("ErrorMgr"));
}

/*! Error manager singleton destructor*/

ErrorMgrSingleton::~ErrorMgrSingleton()
{
    while (!m_ErrorStack.empty())
        m_ErrorStack.pop();
    MessageMgr::getInstance().UnRegister(this);
}

/*!No Error For Last Call */
void ErrorMgrSingleton::NoError()
{
    m_ErrorLastCallFlag = false;
}

/*!
    Check if there was an error on the last call to the API
    \code{.cpp}
    //==== Force API to silence error messages ====//
    SilenceErrors();

    //==== Bogus Call To Create API Error ====//
    Print( string( "---> Test Error Handling" ) );

    SetParmVal( "BogusParmID", 23.0 );

    if ( !GetErrorLastCallFlag() )                        { Print( "---> Error: API GetErrorLastCallFlag " ); }

    //==== Tell API to print error messages ====//
    PrintOnErrors();
    \endcode
    \return False if no error, true otherwise
*/

bool ErrorMgrSingleton::GetErrorLastCallFlag()
{
    return m_ErrorLastCallFlag;
}

/*!
    Count the total number of errors on the stack
    \code{.cpp}
    //==== Force API to silence error messages ====//
    SilenceErrors();

    Print( "Creating an API error" );
    SetParmVal( "ABCDEFG", "Test_Name", "Test_Group", 123.4 );

    //==== Check For API Errors ====//
    while ( GetNumTotalErrors() > 0 )
    {
        ErrorObj err = PopLastError();
        Print( err.GetErrorString() );
    }

    //==== Tell API to print error messages ====//
    PrintOnErrors();
    \endcode
    \return Number of errors
*/

int ErrorMgrSingleton::GetNumTotalErrors()
{
    return m_ErrorStack.size();
}

/*!
    Pop (remove) and return the most recent error from the stack. Note, errors are printed on occurrence by default.
    \code{.cpp}
    //==== Force API to silence error messages ====//
    SilenceErrors();

    Print( "Creating an API error" );
    SetParmVal( "ABCDEFG", "Test_Name", "Test_Group", 123.4 );

    //==== Check For API Errors ====//
    while ( GetNumTotalErrors() > 0 )
    {
        ErrorObj err = PopLastError();
        Print( err.GetErrorString() );
    }

    //==== Tell API to print error messages ====//
    PrintOnErrors();
    \endcode
    \return Error object
*/

ErrorObj ErrorMgrSingleton::PopLastError()
{
    ErrorObj ret_err;

    if (m_ErrorStack.size() == 0) // Nothing To Undo
    {
        return ret_err;
    }

    ret_err = m_ErrorStack.top();
    m_ErrorStack.pop();

    return ret_err;
}

/*!
    Return the most recent error from the stack (does NOT pop error off the stack)
    \code{.cpp}
    //==== Force API to silence error messages ====//
    SilenceErrors();

    Print( "Creating an API error" );
    SetParmVal( "ABCDEFG", "Test_Name", "Test_Group", 123.4 );

    //==== Check For API Errors ====//
    ErrorObj err = GetLastError();

    Print( err.GetErrorString() );

    //==== Tell API to print error messages ====//
    PrintOnErrors();
    \endcode
    \sa SilenceErrors, PrintOnErrors;
    \return Error object
*/

ErrorObj ErrorMgrSingleton::GetLastError()
{
    ErrorObj ret_err;

    if (m_ErrorStack.size() == 0) // Nothing To Undo
    {
        return ret_err;
    }

    ret_err = m_ErrorStack.top();

    return ret_err;
}

/*! Add Error To Stack And Set Last Call Flag */
void ErrorMgrSingleton::AddError(ERROR_CODE code, const string &desc)
{
    if (code == VSP_OK)
    {
        m_ErrorLastCallFlag = false;
        return;
    }

    if (m_PrintErrors)
    {
        printf("Error Code: %d, Desc: %s\n", (ERROR_CODE)code, desc.c_str());
    }

    m_ErrorLastCallFlag = true;
    m_ErrorStack.push(ErrorObj(code, desc));
}

/*! Check For Error and Print to Stream if Found */
bool ErrorMgrSingleton::PopErrorAndPrint(FILE *stream)
{
    if (!m_ErrorLastCallFlag || m_ErrorStack.size() == 0)
    {
        return false;
    }

    ErrorObj err = m_ErrorStack.top();
    m_ErrorStack.pop();

    fprintf(stream, "Error Code: %d, Desc: %s\n", err.m_ErrorCode, err.m_ErrorString.c_str());
    return true;
}

/*! Message Callbacks */
void ErrorMgrSingleton::MessageCallback(const MessageBase *from, const MessageData &data)
{
    if (data.m_String == string("Error"))
    {
        AddError((ERROR_CODE)data.m_IntVec[0], data.m_StringVec[0]);
    }
}
