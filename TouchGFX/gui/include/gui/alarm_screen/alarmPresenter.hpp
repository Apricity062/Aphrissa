#ifndef ALARMPRESENTER_HPP
#define ALARMPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class alarmView;

class alarmPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    alarmPresenter(alarmView& v);

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

    virtual ~alarmPresenter() {}

    /* ---- 闹钟 (MVP: View 调用, 纯转发到 Model) ---- */
    uint8_t getAlarm(uint8_t index, uint8_t *hour, uint8_t *minute, uint8_t *enabled) { return model->getAlarm(index, hour, minute, enabled); }
    void    setAlarm(uint8_t index, uint8_t hour, uint8_t minute, uint8_t enabled)    { model->setAlarm(index, hour, minute, enabled); }
    void    dismissAlarm()                                                              { model->dismissAlarm(); }
    uint8_t getHourFormat() { return model->getHourFormat(); }   /* 12/24 时制偏好 */

private:
    alarmPresenter();

    alarmView& view;
};

#endif // ALARMPRESENTER_HPP
