//
// This file is released under the terms of the NASA Open Source Agreement (NOSA)
// version 1.3 as detailed in the LICENSE file which accompanies this software.
//
//
//////////////////////////////////////////////////////////////////////

#include "ManageBackgroundScreen.h"
#include "ScreenMgr.h"

#include "MainVSPScreen.h"
#include "MainGLWindow.h"

#include "GraphicEngine.h"
#include "Display.h"
#include "Viewport.h"
#include "Background.h"
#include "TextureRepo.h"
#include "GraphicSingletons.h"
#include "Common.h"

#include "UiSignalBlocker.h"
#include "ui_BackgroundScreen.h"
#include "VspScreenQt_p.h"
#include <QFileDialog>

class ManageBackgroundScreenPrivate : public QDialog, public VspScreenQtPrivate
{
    Q_OBJECT
    Q_DECLARE_PUBLIC( ManageBackgroundScreen )
    Q_PRIVATE_SLOT( self(), void SetUpdateFlag() )

    QWidget * widget() Q_DECL_OVERRIDE { return this; }
    bool Update() Q_DECL_OVERRIDE;
    Ui::BackgroundScreen Ui;
    QFileDialog * fileDialog;

    ManageBackgroundScreenPrivate( ManageBackgroundScreen * q );
    VSPGraphic::Background * Background();

    Q_SLOT void on_redSlider_valueChanged( double val )
    {
        if (Background()) Background()->setRed( val / 255.0 );
    }
    Q_SLOT void on_greenSlider_valueChanged( double val )
    {
        if (Background()) Background()->setGreen( val / 255.0 );
    }
    Q_SLOT void on_blueSlider_valueChanged( double val )
    {
        if (Background()) Background()->setBlue( val / 255.0 );
    }
    Q_SLOT void on_wScaleSlider_valueChanged( double val )
    {
        if (Background()) Background()->scaleW( val );
    }
    Q_SLOT void on_wScaleInput_valueChanged( double val )
    {
        if (Background()) Background()->scaleW( val );
        Ui.wScaleSlider->setRange( val - 0.5, val + 0.5 );
    }
    Q_SLOT void on_hScaleSlider_valueChanged( double val )
    {
        if (Background()) Background()->scaleH( val );
    }
    Q_SLOT void on_hScaleInput_valueChanged( double val )
    {
        if (Background()) Background()->scaleH( val );
        Ui.hScaleSlider->setRange( val - 0.5, val + 0.5 );
    }
    Q_SLOT void on_colorBackButton_clicked()
    {
        if (Background()) {
            Background()->removeImage();
            Background()->setBackgroundMode( VSPGraphic::Common::VSP_BACKGROUND_COLOR );
        }
    }
    Q_SLOT void on_xOffsetSlider_valueChanged( double val )
    {
        if (Background()) Background()->offsetX( val );
    }
    Q_SLOT void on_xOffsetInput_valueChanged( double val )
    {
        if (Background()) Background()->offsetX( val );
        Ui.xOffsetSlider->setRange( val - 0.5, val + 0.5 );
    }
    Q_SLOT void on_yOffsetSlider_valueChanged( double val )
    {
        if (Background()) Background()->offsetY( val );
    }
    Q_SLOT void on_yOffsetInput_valueChanged( double val )
    {
        if (Background()) Background()->offsetY( val );
        Ui.yOffsetSlider->setRange( val - 0.5, val + 0.5 );
    }
    Q_SLOT void on_resetDefaultsButton_clicked()
    {
        if (Background()) Background()->reset();
    }
    Q_SLOT void on_preserveAspectButton_toggled( bool val )
    {
        if (Background()) Background()->preserveAR( val );
    }
    Q_SLOT void on_jpegBackButton_clicked();
    Q_SLOT void texture_selected();
};
VSP_DEFINE_PRIVATE( ManageBackgroundScreen )

ManageBackgroundScreenPrivate::ManageBackgroundScreenPrivate( ManageBackgroundScreen * q) :
    VspScreenQtPrivate( q ),
    fileDialog( 0 )
{
    Ui.setupUi( this );
    UiSignalBlocker blocker( this );

    Ui.wScaleSlider->setRange( 0.5, 1.5 );
    Ui.hScaleSlider->setRange( 0.5, 1.5 );
    Ui.xOffsetSlider->setRange( -0.5, 0.5 );
    Ui.yOffsetSlider->setRange( -0.5, 0.5 );

    auto sliders = QList< ValueSlider * >() << Ui.redSlider << Ui.greenSlider << Ui.blueSlider;
    foreach ( ValueSlider * slider, sliders ) {
        slider->setDecimals( 0 );
        slider->setButtonSymbols( QAbstractSpinBox::NoButtons );
    }
    Ui.redSlider->setColorization( Qt::red );
    Ui.greenSlider->setColorization( Qt::green );
    Ui.blueSlider->setColorization( Qt::blue );

    const char * stylesheet = "border: 1px solid black; color: darkBlue; "
                              "font: bold 12px; ";
    Ui.colorBackButton->setStyleSheet( stylesheet );
    Ui.jpegBackButton->setStyleSheet( stylesheet );

    BlockSignalsInUpdates();
    ConnectUpdateFlag();
}

ManageBackgroundScreen::ManageBackgroundScreen( ScreenMgr * mgr ) :
    VspScreenQt( *new ManageBackgroundScreenPrivate( this ), mgr )
{
}

ManageBackgroundScreen::~ManageBackgroundScreen() {}

bool ManageBackgroundScreenPrivate::Update()
{
    MainVSPScreen * main =
        dynamic_cast<MainVSPScreen*>( GetScreenMgr()->GetScreen( ScreenMgr::VSP_MAIN_SCREEN ) );
    if( !main )
    {
        return false;
    }

    VSPGUI::VspGlWindow * glwin = main->GetGLWindow();

    VSPGraphic::Viewport * viewport = glwin->getGraphicEngine()->getDisplay()->getViewport();
    if( !viewport )
    {
        return false;
    }

    VSPGraphic::Common::VSPenum mode = viewport->getBackground()->getBackgroundMode();
    switch( mode )
    {
    case VSPGraphic::Common::VSP_BACKGROUND_COLOR:
        Ui.colorBackButton->setChecked( true );
        break;

    case VSPGraphic::Common::VSP_BACKGROUND_IMAGE:
        Ui.jpegBackButton->setChecked( true );
        break;

    default:
        break;
    }

    VSPGraphic::Background * background = viewport->getBackground();

    Ui.preserveAspectButton->setChecked( background->getARFlag() );

    Ui.redSlider->setValue( background->getRed() * 255 );
    Ui.greenSlider->setValue( background->getGreen() * 255 );
    Ui.blueSlider->setValue( background->getBlue() * 255 );

    Ui.wScaleInput->setValue( background->getScaleW() );
    Ui.wScaleSlider->setValue( background->getScaleW() );
    Ui.hScaleInput->setValue( background->getScaleH() );
    Ui.hScaleSlider->setValue( background->getScaleH() );

    Ui.xOffsetInput->setValue( background->getOffsetX() );
    Ui.xOffsetSlider->setValue( background->getOffsetX() );
    Ui.yOffsetInput->setValue( background->getOffsetY() );
    Ui.yOffsetSlider->setValue( background->getOffsetY() );

    return true;
}

VSPGraphic::Background * ManageBackgroundScreenPrivate::Background()
{
    MainVSPScreen * main =
        dynamic_cast<MainVSPScreen*>( GetScreen( ScreenMgr::VSP_MAIN_SCREEN ) );
    if( !main ) return 0;

    VSPGUI::VspGlWindow * glwin = main->GetGLWindow();

    VSPGraphic::Viewport * viewport = glwin->getGraphicEngine()->getDisplay()->getViewport();
    if( !viewport ) return 0;

    return viewport->getBackground();
}

void ManageBackgroundScreenPrivate::on_jpegBackButton_clicked()
{
    if ( !Background() ) return;
    if ( !fileDialog ) {
        fileDialog = new QFileDialog( this, "Background Texture", QString(), "TGA, JPG Files (*.{tga,jpg})" );
        fileDialog->setFileMode( QFileDialog::ExistingFile );
        fileDialog->setAcceptMode( QFileDialog::AcceptOpen );
        connect( fileDialog, SIGNAL( accepted() ), SLOT( texture_selected() ) );
    }
    fileDialog->open();
}

void ManageBackgroundScreenPrivate::texture_selected()
{
    QString tf = fileDialog->selectedFiles().first();
    Background()->removeImage();
    Background()->attachImage( VSPGraphic::GlobalTextureRepo()->get2DTexture( tf.toLocal8Bit().constData() ) );
    Background()->setBackgroundMode( VSPGraphic::Common::VSP_BACKGROUND_IMAGE );
}

#include "ManageBackgroundScreen.moc"
