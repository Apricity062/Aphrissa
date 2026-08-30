#ifndef SETTINGSPRESENTER_HPP
#define SETTINGSPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class settingsView;

class settingsPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    settingsPresenter(settingsView& v);

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

    /* ---- View 请求 (MVP: View 只调这些, 不直接碰裸机) ---- */
    void brightnessModeToggled(bool autoMode);  /* 自动/手动切换 */
    void brightnessValueChanged(uint8_t value); /* 滑条值变化 */
    uint8_t getBrightnessMode() const;          /* View 进界面时拉初始模式 */
    uint8_t getBrightnessValue() const;         /* View 进界面时拉滑条值 */

    /* ---- 时间设置 (MVP: View 经 presenter 请求 Model) ---- */
    void dateTimeChanged(uint16_t year, uint8_t month, uint8_t day,
                         uint8_t hour, uint8_t minute); /* 滚轮确定: 写入 RTC */
    void hourFormatToggled(uint8_t format);             /* 12/24 切换 */
    uint8_t getHourFormat() const;                      /* View 进界面时拉时制 */

    /* 当前时间查询 (View 初始化滚轮用, 数据来自 Model 缓存) */
    uint8_t  getCurrentHour() const;
    uint8_t  getCurrentMinute() const;
    uint16_t getCurrentYear() const;
    uint8_t  getCurrentMonth() const;
    uint8_t  getCurrentDay() const;

    /* ---- Model -> View 通知 (ModelListener 接口实现) ---- */
    virtual void updateBrightness(uint8_t mode, uint8_t value);

    //virtual void updateCount(int count);

    virtual ~settingsPresenter() {}

private:
    settingsPresenter();

    settingsView& view;
};

#endif // SETTINGSPRESENTER_HPP
