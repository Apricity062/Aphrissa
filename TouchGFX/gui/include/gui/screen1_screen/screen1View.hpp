#ifndef SCREEN1VIEW_HPP
#define SCREEN1VIEW_HPP

#include <gui_generated/screen1_screen/screen1ViewBase.hpp>
#include <gui/screen1_screen/screen1Presenter.hpp>

class screen1View : public screen1ViewBase
{
public:
    screen1View();
    virtual ~screen1View() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void menuScrollListUpdateItem(menuItemContainer& item, int16_t itemIndex);//掌管第二屏幕的滑动菜单 // 纯虚函数，子类必须实现//touchgfx::ListLayout的回调函数，当列表项需要更新时调用
    virtual void changescreen(); // preseter调用这个函数来执行跳转动画
    virtual void handleGestureEvent(const touchgfx::GestureEvent &evt);// 处理手势事件

  protected:
    Callback<screen1View, int16_t> menuItemSelectedCallback;
    void menuItemSelectedHandler(int16_t itemSelected); // 当菜单项被选中时调用
};

#endif // SCREEN1VIEW_HPP
