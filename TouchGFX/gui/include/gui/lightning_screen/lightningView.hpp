#ifndef LIGHTNINGVIEW_HPP
#define LIGHTNINGVIEW_HPP

#include <gui_generated/lightning_screen/lightningViewBase.hpp>
#include <gui/lightning_screen/lightningPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/widgets/AbstractButton.hpp>

class lightningView : public lightningViewBase
{
public:
    lightningView();
    virtual ~lightningView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleGestureEvent(const touchgfx::GestureEvent &evt);

  protected:

  private:
    /* toggleButton1 (手电筒开关) 生成层未绑定, 这里手动绑定 */
    touchgfx::Callback<lightningView, const touchgfx::AbstractButton&> toggleCallback;
    void toggleClicked(const touchgfx::AbstractButton& src);
};

#endif // LIGHTNINGVIEW_HPP
