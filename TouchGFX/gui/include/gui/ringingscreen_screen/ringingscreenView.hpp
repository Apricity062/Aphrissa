#ifndef RINGINGSCREENVIEW_HPP
#define RINGINGSCREENVIEW_HPP

#include <gui_generated/ringingscreen_screen/ringingscreenViewBase.hpp>
#include <gui/ringingscreen_screen/ringingscreenPresenter.hpp>

class ringingscreenView : public ringingscreenViewBase
{
public:
    ringingscreenView();
    virtual ~ringingscreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /* quitalarm 按钮回调: 关闭响铃 */
    void quitAlarmClicked(const touchgfx::AbstractButton& src);

    /* 响铃结束 (手动关闭或超时自动停): 退出响铃界面, 回主界面 */
    void exitAlarmScreen();

protected:
    /* 显示正在响铃的闹钟时间到 timeshowing */
    void updateRingingTime();

private:
    touchgfx::Callback<ringingscreenView, const touchgfx::AbstractButton&> quitAlarmCallback;
};

#endif // RINGINGSCREENVIEW_HPP
