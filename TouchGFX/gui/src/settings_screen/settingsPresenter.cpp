#include <gui/settings_screen/settingsView.hpp>
#include <gui/settings_screen/settingsPresenter.hpp>

settingsPresenter::settingsPresenter(settingsView& v)
    : view(v)
{

}

void settingsPresenter::activate()
{

}

void settingsPresenter::deactivate()
{

}

/* ---- View 请求 -> Model (MVP 中转, 不含业务逻辑) ---- */

void settingsPresenter::brightnessModeToggled(bool autoMode)
{
    model->setBrightnessMode(autoMode ? 1u : 0u);
}

void settingsPresenter::brightnessValueChanged(uint8_t value)
{
    model->setBrightnessValue(value);
}

uint8_t settingsPresenter::getBrightnessMode() const
{
    return model->getBrightnessMode();
}

uint8_t settingsPresenter::getBrightnessValue() const
{
    return model->getBrightnessValue();
}

/* ---- Model -> View 通知 ---- */

void settingsPresenter::updateBrightness(uint8_t mode, uint8_t value)
{
    view.setBrightnessState(mode, value);
}

/*时间相关设置↓*/
void settingsPresenter::dateTimeChanged(uint16_t year, uint8_t month, uint8_t day,
                                        uint8_t hour, uint8_t minute)
{
    model->setDateTime(year, month, day, hour, minute);
}

void settingsPresenter::hourFormatToggled(uint8_t format)
{
    model->setHourFormat(format);
}

uint8_t settingsPresenter::getHourFormat() const
{
    return model->getHourFormat();
}

/* 当前时间查询 (MVP: Model 缓存 -> presenter -> view) */
uint8_t settingsPresenter::getCurrentHour() const    { return model->getCurrentHour(); }
uint8_t settingsPresenter::getCurrentMinute() const  { return model->getCurrentMinute(); }
uint16_t settingsPresenter::getCurrentYear() const   { return (uint16_t)(2000u + model->getCurrentYear()); }
uint8_t settingsPresenter::getCurrentMonth() const   { return model->getCurrentMonth(); }
uint8_t settingsPresenter::getCurrentDay() const     { return model->getCurrentDay(); }