#ifndef HEARTRATEVIEW_HPP
#define HEARTRATEVIEW_HPP

#include <gui_generated/heartrate_screen/heartrateViewBase.hpp>
#include <gui/heartrate_screen/heartratePresenter.hpp>

class heartrateView : public heartrateViewBase
{
public:
    heartrateView();
    virtual ~heartrateView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleGestureEvent(const touchgfx::GestureEvent &evt); // 调用这个函数来执行跳转动画
    virtual void updateHeartRate(int32_t hr, int32_t spo2); // 更新心率

  protected:
    touchgfx::Unicode::UnicodeChar heartrateBuffer[8];
    touchgfx::Unicode::UnicodeChar bloodoxygenBuffer[8];
};

#endif // HEARTRATEVIEW_HPP
