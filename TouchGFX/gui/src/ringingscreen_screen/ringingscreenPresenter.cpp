#include <gui/ringingscreen_screen/ringingscreenView.hpp>
#include <gui/ringingscreen_screen/ringingscreenPresenter.hpp>

ringingscreenPresenter::ringingscreenPresenter(ringingscreenView& v)
    : view(v)
{

}

void ringingscreenPresenter::activate()
{

}

void ringingscreenPresenter::deactivate()
{

}

/* 响铃结束 (手动关闭或超时自动停): 退出响铃界面, 回主菜单 */
void ringingscreenPresenter::updateAlarmRing(uint8_t ringing, uint8_t index)
{
    (void)index;
    if (!ringing) {
        view.exitAlarmScreen();
    }
}
