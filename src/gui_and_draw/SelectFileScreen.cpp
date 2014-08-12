//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// labelScreen.cpp: implementation of the labelScreen class.
//
//////////////////////////////////////////////////////////////////////

#define QPoint QQPoint
#include <FL/Fl_Preferences.H>
#include "selectFileFlScreen.h"
#include "SelectFileScreen.h"
#include "ScreenMgr.h"
#include "StringUtil.h"
#undef QPoint
#include <QFileDialog>
#include <vector>
#include <string>
using std::string;
using std::vector;

#include <stdio.h>  /* defines FILENAME_MAX */
#ifdef WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

class SelectFileScreenPrivate {
    Q_DISABLE_COPY(SelectFileScreenPrivate)
public:
    Q_DECLARE_PUBLIC(SelectFileScreen)
    SelectFileScreen* const q_ptr;
    SelectFileUI* selectFileUI;
    bool AcceptFlag;
    string FilterString;
    string DirString;
    string FileName;
    string FullPathName;
    string Title;
    vector< string > FavDirVec;

    SelectFileScreenPrivate(SelectFileScreen*);
    void screenCB( Fl_Widget* w );
};

static void staticScreenCB( Fl_Widget *w, void* data )
{
    ( ( SelectFileScreenPrivate* )data )->screenCB( w );
}

SelectFileScreen::SelectFileScreen() :
    d_ptr(new SelectFileScreenPrivate(this))
{
    Q_D(SelectFileScreen);
    char cCurrentPath[FILENAME_MAX];
    GetCurrentDir( cCurrentPath, sizeof( cCurrentPath ) );

    d->DirString = string( cCurrentPath );

    char forwardSlash = '\\';
    StringUtil::change_from_to( d->DirString, forwardSlash, '/' );

    int dirSize = ( int )d->DirString.size();
    if ( d->DirString[dirSize - 1] != '/' )
    {
        d->DirString.append( "/" );
    }

    d->FilterString = string( "*.*" );

    d->selectFileUI = new SelectFileUI();
    d->selectFileUI->UIWindow->position( 30, 30 );
    d->selectFileUI->fileBrowser->type( FL_SELECT_BROWSER );
    d->selectFileUI->fileBrowser->callback( staticScreenCB, d );
    d->selectFileUI->dirInput->callback( staticScreenCB, d );
    d->selectFileUI->dirInput->when( FL_WHEN_CHANGED );

    d->selectFileUI->fileInput->callback( staticScreenCB, d );
    d->selectFileUI->acceptButton->callback( staticScreenCB, d );
    d->selectFileUI->cancelButton->callback( staticScreenCB, d );

    d->selectFileUI->favsMenuButton->callback( staticScreenCB, d );

    LoadFavsMenu();
}

void SelectFileScreen::LoadFavsMenu()
{
    Q_D(SelectFileScreen);
    d->FavDirVec.clear();
    d->selectFileUI->favsMenuButton->clear();
    d->selectFileUI->favsMenuButton->add( "Add to Favorites" );
    d->selectFileUI->favsMenuButton->add( "Delete All Favorites", 0, NULL, ( void* )0, FL_MENU_DIVIDER );

    //==== Preferences ====//
    Fl_Preferences prefs( Fl_Preferences::USER, "NASA", "VSP" );

    //==== Load All Favorites From Preferences ====//
    char tag[256], str[256];
    bool keep_looking = true;
    while ( keep_looking )
    {
        keep_looking = false;
        sprintf( tag, "fav%d", ( int )d->FavDirVec.size() );
        if ( prefs.get( tag, str, "", 256 ) )
        {
            keep_looking = true;

            d->FavDirVec.push_back( string( str ) );
            string menu_label( str );
            menu_label.insert( 0, "/" );
            d->selectFileUI->favsMenuButton->add( menu_label.c_str() );
        }
    }
}

void SelectFileScreen::show()
{
    Q_D(SelectFileScreen);
    if ( d->selectFileUI )
    {
        d->selectFileUI->UIWindow->show();
    }
}

string SelectFileScreen::FileChooser( const char* title, const char* filter )
{
    Q_D(SelectFileScreen);
    string file_name;
    d->AcceptFlag = false;

    d->FileName = string();

    char filter_str[256];
    sprintf( filter_str, "   (%s)", filter );

    d->Title = string( title );
    d->Title.append( filter_str );
    d->selectFileUI->titleBox->label( d->Title.c_str() );

    d->FilterString = filter;

    d->selectFileUI->fileInput->value( d->FileName.c_str() );
    d->selectFileUI->fileBrowser->filter( d->FilterString.c_str() );
    d->selectFileUI->fileBrowser->load( d->DirString.c_str() );
    d->selectFileUI->dirInput->value( d->DirString.c_str() );
    show();

    while( d->selectFileUI->UIWindow->shown() )
    {
        Fl::wait();
    }

    if ( d->AcceptFlag )
    {
        d->FullPathName = d->DirString;
        d->FullPathName.append( d->FileName );
        file_name = d->FullPathName;
    }

    return file_name;
}

string SelectFileScreen::FileChooser( const char* title, const char* filter, const char* dir )
{
    Q_D(SelectFileScreen);
    d->DirString = dir;
    return FileChooser( title, filter );
}

SelectFileScreen::~SelectFileScreen()
{}

SelectFileScreenPrivate::SelectFileScreenPrivate(SelectFileScreen * parent) :
    q_ptr(parent)
{}

void SelectFileScreenPrivate::screenCB( Fl_Widget* w )
{
    Q_Q(SelectFileScreen);
    if ( w == selectFileUI->fileBrowser )
    {
        int sid = selectFileUI->fileBrowser->value();
        selectFileUI->fileBrowser->select( sid );

        // Check for NULL Char pointer
        const char * text = selectFileUI->fileBrowser->text( sid );
        if ( text == NULL )
        {
            return;
        }

        string selText = string( text );

        if ( selText == "../" )
        {
            if ( StringUtil::count_char_matches( DirString, '/' ) > 1 )
            {
                int dirLen = DirString.size();
                DirString.erase( dirLen - 1, 1 );

                int slashLoc = DirString.find_last_of( '/' );

                if ( slashLoc + 1 <= ( dirLen - 1 ) )
                {
                    DirString.erase( slashLoc + 1, dirLen - slashLoc );
                }

                selectFileUI->fileBrowser->load( DirString.c_str() );
                selectFileUI->dirInput->value( DirString.c_str() );

            }
        }
        else if ( StringUtil::count_char_matches( selText, '/' ) >= 1 )
        {
            DirString.append( selText );
            selectFileUI->fileBrowser->load( DirString.c_str() );
            selectFileUI->dirInput->value( DirString.c_str() );
        }
        else
        {
            FileName = selText;
            selectFileUI->fileInput->value( FileName.c_str() );
        }
    }
    else if ( w == selectFileUI->dirInput )
    {
        DirString = string( selectFileUI->dirInput->value() );
        char forwardSlash = '\\';
        StringUtil::change_from_to( DirString, forwardSlash, '/' );
        int dirSize = DirString.size();
        if ( DirString[dirSize - 1] != '/' )
        {
            DirString.append( "/" );
        }
        selectFileUI->fileBrowser->load( DirString.c_str() );

    }
    else if ( w == selectFileUI->fileInput )
    {
        FileName = selectFileUI->fileInput->value();
    }
    else if ( w == selectFileUI->acceptButton )
    {
        AcceptFlag = true;
        selectFileUI->UIWindow->hide();
    }
    else if ( w == selectFileUI->cancelButton )
    {
        AcceptFlag = false;
        selectFileUI->UIWindow->hide();
    }
    else if ( w == selectFileUI->favsMenuButton )
    {
        int val = selectFileUI->favsMenuButton->value();

        if ( val == 0 )             // Add To Favorites
        {
            Fl_Preferences prefs( Fl_Preferences::USER, "NASA", "VSP" );
            char favstr[256];
            sprintf( favstr, "fav%d", static_cast<int>( FavDirVec.size() ) );
            prefs.set( favstr, DirString.c_str() );
            prefs.flush();
            q->LoadFavsMenu();
        }
        else if ( val == 1 )
        {
            FavDirVec.clear();
            Fl_Preferences prefs( Fl_Preferences::USER, "NASA", "VSP" );
            for ( int i = 0 ; i < ( int )prefs.entries() ; i++ )
            {
                prefs.deleteEntry( prefs.entry( i ) );
            }
            prefs.flush();
            q->LoadFavsMenu();
        }
        else
        {
            //==== Select Favorite Dir ====//
            int ind = val - 2;
            if ( ind >= 0 && ind < ( int )FavDirVec.size() )
            {
                DirString = FavDirVec[ind];
//              DirString.delete_range( 0, 0 );
//              DirString.remove_leading('/');
                selectFileUI->fileBrowser->load( DirString.c_str() );
                selectFileUI->dirInput->value( DirString.c_str() );
            }
        }
    }
}
