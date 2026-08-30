#include <gui/chronograph_screen/chronographView.hpp>
#include <touchgfx/Unicode.hpp>

chronographView::chronographView()
    : buttonCallback(this, &chronographView::buttonClicked)
{
}

void chronographView::setupScreen()
{
    chronographViewBase::setupScreen();

    /* 基类没给按钮接动作, 在这里手动绑定 */
    startandstop.setAction(buttonCallback);
    reset.setAction(buttonCallback);

    /* 初始: 复位按钮隐藏, 秒表显示 00.00.000 (由 Model 首帧推送校正) */
    reset.setVisible(false);
    reset.invalidate();
    updateTimeText(0);
}

void chronographView::tearDownScreen()
{
    /* 离开秒表界面: 恢复自动熄屏 (经 presenter 请求 Model 处理) */
    presenter->leaveScreen();
    chronographViewBase::tearDownScreen();
}

void chronographView::handleTickEvent()
{
    chronographViewBase::handleTickEvent();
    /* 秒表显示由 Model::tick -> updateStopwatch 推送, 这里不再主动读裸机 */
}

void chronographView::handleGestureEvent(const touchgfx::GestureEvent& evt)
{
    if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL)
    {
        static const int16_t MIN_SWIPE_VELOCITY = 20;
        if (evt.getVelocity() > MIN_SWIPE_VELOCITY) /* 向右快速滑 → 返回菜单 */
        {
            application().gotoscreen1ScreenSlideTransitionEast();
        }
    }
    //chronographViewBase::handleGestureEvent(evt);
}

/* ── 按钮统一回调 (MVP: 只通知 presenter, 不碰裸机) ── */
void chronographView::buttonClicked(const touchgfx::AbstractButton& src)
{
    if (&src == &startandstop) {
        /* ToggleButton 点击后已切换图片; 业务(启停)交给 presenter/model */
        presenter->startStopToggled();
    } else if (&src == &reset) {
        presenter->resetClicked();
        startandstop.forceState(false); /* 复位后回退到"播放"态 (纯UI) */
    }
}

/* ── Model -> presenter -> view: 推送秒表数据 ── */
void chronographView::setStopwatchTime(uint32_t ms, bool running)
{
    updateTimeText(ms);
    updateResetVisibility(running, ms);
}

/* ── MM.SS.mmm → timetextBuffer (只用现有字体就有的数字和点) ── */
void chronographView::updateTimeText(uint32_t ms)
{
    uint32_t mm  = ms / 60000u;
    uint32_t ss  = (ms / 1000u) % 60u;
    uint32_t mmm = ms % 1000u;
    Unicode::snprintf(timetextBuffer, TIMETEXT_SIZE, "%02u.%02u.%03u", mm, ss, mmm);
    timetext.invalidate();
}

/* ── 只有"暂停后且非零"才显示复位按钮 ── */
void chronographView::updateResetVisibility(bool running, uint32_t ms)
{
    bool show = (!running) && (ms > 0);
    reset.setVisible(show);
    reset.invalidate();
}
