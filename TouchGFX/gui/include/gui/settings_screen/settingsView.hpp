#ifndef SETTINGSVIEW_HPP
#define SETTINGSVIEW_HPP

#include <gui_generated/settings_screen/settingsViewBase.hpp>
#include <gui/settings_screen/settingsPresenter.hpp>

class settingsView : public settingsViewBase
{
public:
    settingsView();
    virtual ~settingsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleGestureEvent(const touchgfx::GestureEvent &evt);

    /* 生成层 sliderValueChangedCallbackHandler 在滑条值变化时调用此虚函数,
       在手写层 override: 通知 presenter (MVP) */
    virtual void handisetlight(int value);

    /* 生成层 buttonCallbackHandler 里 lightchanger 分支调用此虚函数,
       在手写层 override: 通知 presenter (MVP) */
    virtual void swithcHandiLighter();
    void buttonClicked(const touchgfx::AbstractButton &src);//按钮点击后会触发的回调函数

    /* Model -> presenter -> view 数据推送 (MVP): 亮度状态同步 */
    void setBrightnessState(uint8_t mode, uint8_t value);

    /* ================= 时间设置滚轮 ================= */
    /* 生成层 updateItemCallbackHandler 调用的 6 个滚轮更新虚函数 */
    virtual void hourUpdateItem(hourfigure1& item, int16_t itemIndex);
    virtual void minuteUpdateItem(minutefigure& item, int16_t itemIndex);
    virtual void yearUpdateItem(yearfigure1& item, int16_t itemIndex);
    virtual void amandpmboxUpdateItem(amandpm1& item, int16_t itemIndex);
    virtual void monthUpdateItem(monthfigure1& item, int16_t itemIndex);
    virtual void dayUpdateItem(dayfigure1& item, int16_t itemIndex);

protected:
    /* 打开时间窗口时: 用当前 RTC 时间初始化 6 个滚轮 */
    void initTimeWindow();

    /* 读取 6 个滚轮当前选中值, 组装后写入 RTC (MVP: 经 presenter) */
    void applyTimeWindow();

    /* 12/24 小时制切换: 重配小时滚轮/AM-PM滚轮 + 存偏好 */
    void applyHourFormat(uint8_t format);

    /* 内部状态: 当前时制 (进窗口时从 presenter 拉取) */
    uint8_t m_hourFormat = 24;

    /* 月份/年份滚轮选中回调: 联动更新天数滚轮格数 */
    void onMonthYearWheelChanged(int16_t itemIndex);
    touchgfx::Callback<settingsView, int16_t> monthWheelCallback;
    touchgfx::Callback<settingsView, int16_t> yearWheelCallback;

    /* 根据当前年月重算 day 滚轮格数 (28/29/30/31) 并校正越界 */
    void refreshDayWheel();

private:
    /* timeswitch (12/24切换) 在生成层没绑定, 这里手动绑定 */
    touchgfx::Callback<settingsView, const touchgfx::AbstractButton&> timeSwitchCallback;
    void timeSwitchClicked(const touchgfx::AbstractButton& src);

    /* 覆盖生成层绑定: timesettingbutton/lightsettingbutton 走本层 buttonClicked
       (生成层 buttonCallback 是 private, 子类无法引用, 用本类回调替代) */
    touchgfx::Callback<settingsView, const touchgfx::AbstractButton&> settingsButtonCallback;
};

#endif // SETTINGSVIEW_HPP
