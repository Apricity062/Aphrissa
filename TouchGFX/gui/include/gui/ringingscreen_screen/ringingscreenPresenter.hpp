#ifndef RINGINGSCREENPRESENTER_HPP
#define RINGINGSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class ringingscreenView;

class ringingscreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    ringingscreenPresenter(ringingscreenView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~ringingscreenPresenter() {}

    /* ---- 闹钟 (MVP: View 调用, 纯转发到 Model) ---- */
    uint8_t getRingingIndex() { return model->getAlarmRingingIndex(); }
    uint8_t getAlarm(uint8_t index, uint8_t *hour, uint8_t *minute, uint8_t *enabled) { return model->getAlarm(index, hour, minute, enabled); }
    void    dismissAlarm()    { model->dismissAlarm(); }

    /* 响铃结束 (Model::tick 推送 ringing=0): 退出响铃界面 */
    virtual void updateAlarmRing(uint8_t ringing, uint8_t index);

private:
    ringingscreenPresenter();

    ringingscreenView& view;
};

#endif // RINGINGSCREENPRESENTER_HPP
