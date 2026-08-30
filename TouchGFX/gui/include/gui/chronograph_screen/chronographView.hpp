#ifndef CHRONOGRAPHVIEW_HPP
#define CHRONOGRAPHVIEW_HPP

#include <gui_generated/chronograph_screen/chronographViewBase.hpp>
#include <gui/chronograph_screen/chronographPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/events/GestureEvent.hpp>
#include <touchgfx/widgets/AbstractButton.hpp>

class chronographView : public chronographViewBase
{
public:
    chronographView();
    virtual ~chronographView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();                                 /* 每帧刷新秒表显示 */
    virtual void handleGestureEvent(const touchgfx::GestureEvent& evt); /* 滑动返回菜单 */

    /* Model -> presenter -> view 数据推送 (MVP) */
    void setStopwatchTime(uint32_t ms, bool running); /* 更新秒表显示 + 复位按钮可见性 */

protected:
    void updateTimeText(uint32_t ms);      /* MM.SS.mmm 写入 timetextBuffer */
    void updateResetVisibility(bool running, uint32_t ms); /* 暂停且非零 → 显示复位按钮 */

private:
    touchgfx::Callback<chronographView, const touchgfx::AbstractButton&> buttonCallback;
    void buttonClicked(const touchgfx::AbstractButton& src);
};

#endif // CHRONOGRAPHVIEW_HPP
