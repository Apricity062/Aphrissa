#include <gui/ringingscreen_screen/ringingscreenView.hpp>
#include <touchgfx/Unicode.hpp>

ringingscreenView::ringingscreenView()
    : quitAlarmCallback(this, &ringingscreenView::quitAlarmClicked)
{
}

void ringingscreenView::setupScreen()
{
    ringingscreenViewBase::setupScreen();

    /* 显示正在响铃的闹钟时间 */
    updateRingingTime();

    /* quitalarm 按钮: 生成层未绑定 action, 在此手动绑定 */
    quitalarm.setAction(quitAlarmCallback);
}

void ringingscreenView::tearDownScreen()
{
    ringingscreenViewBase::tearDownScreen();
}

void ringingscreenView::updateRingingTime()
{
    /* MVP: 从 Model 经 presenter 拉取响铃中的闹钟时间 */
    uint8_t idx = presenter->getRingingIndex();
    uint8_t h = 0, m = 0, e = 0;
    presenter->getAlarm(idx, &h, &m, &e);

    Unicode::snprintf(timeshowingBuffer, TIMESHOWING_SIZE, "%02u:%02u",
                      (unsigned)h, (unsigned)m);
    timeshowing.setWildcard(timeshowingBuffer);
    timeshowing.resizeToCurrentText(); /* 动态文本必须 resize 才显示 */

    /* 水平居中: resize 后按文本实际宽度重算 X (屏幕 240 宽) */
    timeshowing.setX((240 - timeshowing.getWidth()) / 2);
    timeshowing.invalidate();
}

void ringingscreenView::quitAlarmClicked(const touchgfx::AbstractButton& src)
{
    (void)src;
    /* 关闭响铃: 停蜂鸣器 + 清标志, 随后 exitAlarmScreen 由响铃结束事件触发 */
    presenter->dismissAlarm();
}

void ringingscreenView::exitAlarmScreen()
{
    application().gotoscreen1ScreenSlideTransitionEast(); /* 回主菜单 */
}
