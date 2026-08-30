#include <gui/lightning_screen/lightningView.hpp>

lightningView::lightningView()
    : toggleCallback(this, &lightningView::toggleClicked)
{
}

void lightningView::setupScreen()
{
    lightningViewBase::setupScreen();
    /* toggleButton1 (手电筒开关) 生成层未绑定, 这里手动绑定 */
    toggleButton1.setAction(toggleCallback);
}

void lightningView::tearDownScreen()
{
  lightningViewBase::tearDownScreen();
  
}

void lightningView::handleGestureEvent(const touchgfx::GestureEvent &evt) {
  if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL) {
    static const int16_t MIN_SWIPE_VELOCITY = 20;
    if (evt.getVelocity() > MIN_SWIPE_VELOCITY) /* 向右快速滑 → 返回菜单 */
    {
      application().gotoscreen1ScreenSlideTransitionWest();
    }
  }
  // chronographViewBase::handleGestureEvent(evt);
}

/* toggleButton1 点击: ToggleButton 已自动切图 (黑底<->白底), 通知 presenter (MVP) */
void lightningView::toggleClicked(const touchgfx::AbstractButton& src)
{
    /* getState()=true → 白底(开) → 手电筒开; false → 黑底(关) → 手电筒关 */
    presenter->flashlightToggled(toggleButton1.getState());
}

