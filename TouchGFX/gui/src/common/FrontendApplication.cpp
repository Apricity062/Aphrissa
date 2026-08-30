#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/ringingscreen_screen/ringingscreenView.hpp>
#include <gui/ringingscreen_screen/ringingscreenPresenter.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/transitions/NoTransition.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{
}

void FrontendApplication::handleTickEvent()
{
    model.tick();
    FrontendApplicationBase::handleTickEvent();

    /* 响铃开始 (上升沿) → 自动跳转到响铃界面, 任何屏幕都生效 */
    uint8_t ringing = model.isAlarmRinging();
    if (ringing && !lastRinging) {
        gotoringscreenScreenNoTransition();
    }
    lastRinging = ringing;
}

void FrontendApplication::gotoringscreenScreenNoTransition()
{
    ringTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoringscreenScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &ringTransitionCallback;
}

void FrontendApplication::gotoringscreenScreenNoTransitionImpl()
{
    touchgfx::makeTransition<ringingscreenView, ringingscreenPresenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}
