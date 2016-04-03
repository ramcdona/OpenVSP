#if !defined(BLANKSCREEN__INCLUDED_)
#define BLANKSCREEN__INCLUDED_

#include "ScreenBase.h"
#include "GuiDevice.h"

#include <FL/Fl.H>

class BlankScreen : public GeomScreen
{
public:
    BlankScreen( ScreenMgr* mgr );
    virtual ~BlankScreen()                          {}

    virtual void Show();
    virtual bool Update();

    virtual void CallBack( Fl_Widget *w );

protected:

    GroupLayout m_MassLayout;
    ToggleButton m_PointMassButton;
    SliderAdjRangeInput m_MassSlider;
    SliderAdjRangeInput m_IxxSlider;
    SliderAdjRangeInput m_IyySlider;
    SliderAdjRangeInput m_IzzSlider;
    SliderAdjRangeInput m_IxySlider;
    SliderAdjRangeInput m_IxzSlider;
    SliderAdjRangeInput m_IyzSlider;

    SliderAdjRangeInput m_AxisLengthSlider;

};


#endif // !defined(BLANKSCREEN__INCLUDED_)
