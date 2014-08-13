//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//

// SelectFileScreen.cpp: implementation of the SelectFileScreen class.
//
//////////////////////////////////////////////////////////////////////

#define QPoint QQPoint
#include "SelectFileScreen.h"
#include <FL/Fl_Preferences.H>
#undef QPoint
#include <QFileDialog>
#include <string>
using std::string;

/* Note: QFileDialog saves its favorite list automatically in QSettings.
 * We do it ourselves, nevertheless. */
class SelectFileScreenPrivate {
    Q_DISABLE_COPY(SelectFileScreenPrivate)
public:
    Q_DECLARE_PUBLIC(SelectFileScreen)
    SelectFileScreen* const q_ptr;

    QFileDialog Window;

    SelectFileScreenPrivate(SelectFileScreen*);
    void LoadFavorites();
    void SaveFavorites();
};

SelectFileScreen::SelectFileScreen() :
    d_ptr(new SelectFileScreenPrivate(this))
{
    Q_D(SelectFileScreen);
    d->Window.setDirectory(QDir::currentPath());
    d->Window.setNameFilter("(*.*)");;
    d->LoadFavorites();
}

void SelectFileScreenPrivate::LoadFavorites()
{
    QList<QUrl> favorites;
    Fl_Preferences prefs( Fl_Preferences::USER, "NASA", "VSP" );
    Fl_Preferences favs( prefs, "favorites" );

    char str[64];
    for ( int i = 0; i < favs.entries(); ++i )
    {
        if ( favs.get( favs.entry(i), str, "", 256 ) )
        {
            favorites << QUrl::fromLocalFile(str);
        }
    }
    Window.setSidebarUrls(favorites);
    prefs.flush();
}

void SelectFileScreenPrivate::SaveFavorites()
{
    QList<QUrl> favorites = Window.sidebarUrls();
    Fl_Preferences prefs( Fl_Preferences::USER, "NASA", "VSP" );
    Fl_Preferences favs( prefs, "favorites" );

    favs.deleteAllEntries();
    char favstr[64];
    for (int i = 0; i < favorites.size(); ++ i)
    {
        sprintf( favstr, "fav%d", static_cast<int>( i ) );
        favs.set( favstr, favorites[i].toLocalFile().toLocal8Bit().constData() );
    }
    prefs.flush();
}

string SelectFileScreen::FileChooser( const char* title, const char* filter )
{
    Q_D(SelectFileScreen);
    string file_name;

    QString filter_str = QString("  (%1)").arg(filter);
    QString title_str = QString("%1%2").arg(title).arg(filter_str);

    d->Window.setWindowTitle( title_str );
    d->Window.setNameFilter( filter_str );
    d->Window.setFileMode( QFileDialog::ExistingFile );
    d->Window.setOption( QFileDialog::DontUseNativeDialog );

    d->Window.exec();
    if (d->Window.result() == QDialog::Accepted)
    {
        d->SaveFavorites();
    }

    QStringList const files = d->Window.selectedFiles();
    if (! files.isEmpty() )
    {
        file_name = files.first().toStdString();
    }
    return file_name;
}

string SelectFileScreen::FileChooser( const char* title, const char* filter, const char* dir )
{
    Q_D(SelectFileScreen);
    d->Window.setDirectory(dir);
    return FileChooser( title, filter );
}

SelectFileScreen::~SelectFileScreen()
{}

SelectFileScreenPrivate::SelectFileScreenPrivate(SelectFileScreen * parent) :
    q_ptr(parent)
{}
