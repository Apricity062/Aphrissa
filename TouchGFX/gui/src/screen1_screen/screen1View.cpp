#include "gui_generated/containers/menuItemContainerBase.hpp"
#include <gui/screen1_screen/screen1View.hpp>
#include <BitmapDatabase.hpp>

screen1View::screen1View():menuItemSelectedCallback(this, &screen1View::menuItemSelectedHandler)
{

}

void screen1View::setupScreen()
{
  screen1ViewBase::setupScreen();
  menubackground.setTouchable(false);
  // 初始化 ScrollList
  menuScrollList.setItemSelectedCallback(menuItemSelectedCallback);
  menuScrollList.setSnapping(true); /* 滑动结束后对齐到整格, 避免停在半格 */
  for (int i = 0; i < menuScrollListListItems.getNumberOfDrawables(); ++i)
    {
    menuScrollList.itemChanged(i); // 初始化菜单项
  }
}

void screen1View::tearDownScreen()
{
    screen1ViewBase::tearDownScreen();
}

void screen1View::menuScrollListUpdateItem(menuItemContainer& item, int16_t itemIndex)
{
    // 更新菜单项的内容
    item.updateItem(itemIndex);
}

void screen1View::menuItemSelectedHandler(int16_t itemSelected)
{
  switch (itemSelected)
    {
        case 0:
            application().gotolightningScreenSlideTransitionEast();//跳转到手电筒界面
            break;
        case 1:
            application().gotoalarmScreenSlideTransitionEast();//跳转到闹钟
            break;
        case 2:
            application().gotoheartrateScreenSlideTransitionEast(); // 跳转到心率界面
            break;
        case 3:
            application().gotosettingsScreenSlideTransitionEast(); // 跳转到设置界面
            break;
        case 4:
            application().gotochronographScreenSlideTransitionEast(); // 跳转到秒表界面
            break;
        case 5:
            application().gotoindexscreenScreenSlideTransitionEast();  // 跳转到indexscreen
            break;
        case 6:
            application().gotodinosuarScreenSlideTransitionEast(); // 跳转到缪因快跑小游戏
            break;
    }
}

void screen1View::handleGestureEvent(const touchgfx::GestureEvent& evt)
{
    if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL)
    {
        // 速度阈值：只有快速、刻意的水平滑动才触发切屏
        // 调节 MIN_SWIPE_VELOCITY 的值：越大越不容易误触，越小越灵敏
        static const int16_t MIN_SWIPE_VELOCITY = 20;
        if (evt.getVelocity() > MIN_SWIPE_VELOCITY) // 向右快速滑 → 切屏
        {
            changescreen();
        }
        // 速度不够 → 忽略，当作垂直滚动的水平抖动
    }
    // ⚠️ 必须调用基类，确保垂直滑动事件继续向下传到 ScrollList
    screen1ViewBase::handleGestureEvent(evt);
}

void screen1View::changescreen() // 执行跳转动画
{
  application().gotomainscreenScreenSlideTransitionWest(); // 跳转到mainscreen_screen
}
