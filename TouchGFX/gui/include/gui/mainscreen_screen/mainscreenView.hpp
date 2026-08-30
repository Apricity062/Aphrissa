// 这个文件存放着按钮被点击时的回调函数等，和touchgfx联系起来，玻利瓦尔肤层
#ifndef MAINSCREENVIEW_HPP
#define MAINSCREENVIEW_HPP

#include <gui_generated/mainscreen_screen/mainscreenViewBase.hpp>
#include <gui/mainscreen_screen/mainscreenPresenter.hpp>

class mainscreenView : public mainscreenViewBase
{
public:
  mainscreenView();
  virtual ~mainscreenView() {}
  virtual void setupScreen();
  virtual void tearDownScreen();
  virtual void changescreen(); // preseter调用这个函数来执行跳转动画
  virtual void handleGestureEvent(const touchgfx::GestureEvent &evt); // 处理右滑切屏手势
  void setTime(uint8_t hour, uint8_t minute, uint8_t second);   // 设置时间显示
  void setDate(uint8_t year, uint8_t month, uint8_t day, uint8_t weekday); // 设置日期显示
  void setHourFormat(uint8_t format); // 12/24 显示偏好 (来自 Model)

protected:
private:
    const int maximages = 2;
    uint8_t m_hourFormat = 24; /* 当前显示时制 */
};

#endif // MAINSCREENVIEW_HPP
