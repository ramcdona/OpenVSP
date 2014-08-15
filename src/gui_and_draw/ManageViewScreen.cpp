//
// License terms are missing.
//
//////////////////////////////////////////////////////////////////////

#include "ManageViewScreen.h"
#include "ScreenMgr.h"
#include "MainVSPScreen.h"
#include "MainGLWindow.h"
#include "VspScreenQt_p.h"
#include "ui_ManageViewScreen.h"

class ManageViewScreenPrivate : public QDialog, public VspScreenQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( ManageViewScreen )
    Ui::ManageViewScreen Ui;

    ManageViewScreenPrivate( ManageViewScreen * q );
    VSPGUI::VspGlWindow * glwin();
    QWidget * widget() Q_DECL_OVERRIDE { return this; }

    Q_SLOT void setUpdateFlag()
    {
        SetUpdateFlag();
    }
    Q_SLOT void on_xPosMinus1_clicked()
    {
        if (glwin()) glwin()->pan( -1, 0, true );
    }
    Q_SLOT void on_xPosMinus2_clicked()
    {
        if (glwin()) glwin()->pan( -1, 0, false );
    }
    Q_SLOT void on_xPosPlus1_clicked()
    {
        if (glwin()) glwin()->pan( 1, 0, true );
    }
    Q_SLOT void on_xPosPlus2_clicked()
    {
        if (glwin()) glwin()->pan( 1, 0, false );
    }
    Q_SLOT void on_yPosMinus1_clicked()
    {
        if (glwin()) glwin()->pan( 0, -1, true );
    }
    Q_SLOT void on_yPosMinus2_clicked()
    {
        if (glwin()) glwin()->pan( 0, -1, false );
    }
    Q_SLOT void on_yPosPlus1_clicked()
    {
        if (glwin()) glwin()->pan( 0, 1, true );
    }
    Q_SLOT void on_yPosPlus2_clicked()
    {
        if (glwin()) glwin()->pan( 0, 1, false );
    }
    Q_SLOT void on_zoomMinus1_clicked()
    {
        if (glwin()) glwin()->zoom( 1, true );
    }
    Q_SLOT void on_zoomMinus2_clicked()
    {
        if (glwin()) glwin()->zoom( 1, false );
    }
    Q_SLOT void on_zoomPlus1_clicked()
    {
        if (glwin()) glwin()->zoom( -1, true );
    }
    Q_SLOT void on_zoomPlus2_clicked()
    {
        if (glwin()) glwin()->zoom( -1, false );
    }
};
VSP_DEFINE_PRIVATE( ManageViewScreen )

ManageViewScreenPrivate::ManageViewScreenPrivate( ManageViewScreen * q ) :
    VspScreenQtPrivate( q )
{
    Ui.setupUi( this );
    foreach ( QToolButton * button, findChildren<QToolButton*>() ) {
        connect( button, SIGNAL( clicked() ), SLOT( setUpdateFlag() ) );
    }
}

VSPGUI::VspGlWindow * ManageViewScreenPrivate::glwin()
{
    auto main =
            dynamic_cast<MainVSPScreen*>( GetScreen( ScreenMgr::VSP_MAIN_SCREEN ) );
    if ( !main ) return 0;
    return main->GetGLWindow();
}

ManageViewScreen::ManageViewScreen( ScreenMgr * mgr ) :
    VspScreenQt( *new ManageViewScreenPrivate( this ), mgr )
{
    d_func()->move( 775, 50 );
}

ManageViewScreen::~ManageViewScreen()
{
}

#include "ManageViewScreen.moc"
