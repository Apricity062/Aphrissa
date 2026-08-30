#include <gui/chronograph_screen/chronographView.hpp>
#include <gui/chronograph_screen/chronographPresenter.hpp>

chronographPresenter::chronographPresenter(chronographView& v)
    : view(v)
{

}

void chronographPresenter::activate()
{

}

void chronographPresenter::deactivate()
{

}

/* ---- View 请求 -> Model (MVP 中转, 不含业务逻辑) ---- */

void chronographPresenter::startStopToggled()
{
    if (model->isStopwatchRunning()) {
        model->stopwatchStop();
    } else {
        model->stopwatchStart();
    }
}

void chronographPresenter::resetClicked()
{
    model->stopwatchReset();
}

void chronographPresenter::leaveScreen()
{
    model->releaseKeepAwake(); /* 离开界面: 恢复自动熄屏 */
}

/* ---- Model -> View 通知 ---- */

void chronographPresenter::updateStopwatch(uint32_t ms, bool running)
{
    view.setStopwatchTime(ms, running);
}
