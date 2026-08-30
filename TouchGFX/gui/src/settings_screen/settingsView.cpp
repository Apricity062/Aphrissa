#include "touchgfx/Bitmap.hpp"
#include "touchgfx/TypedText.hpp"
#include "touchgfx/events/GestureEvent.hpp"
#include <gui/settings_screen/settingsView.hpp>
#include <images/BitmapDatabase.hpp>      // 位图 ID 定义
#include <texts/TextKeysAndLanguages.hpp> // 文本 ID 定义

#ifndef SIMULATOR
extern "C" {
#include "time_service.h"   /* TimeService_DaysInMonth: 纯日期计算 */
}
#endif

settingsView::settingsView()
    : timeSwitchCallback(this, &settingsView::timeSwitchClicked),
      monthWheelCallback(this, &settingsView::onMonthYearWheelChanged),
      yearWheelCallback(this, &settingsView::onMonthYearWheelChanged),
      settingsButtonCallback(this, &settingsView::buttonClicked)
{
}

void settingsView::setupScreen() {
  settingsViewBase::setupScreen();

  /* 覆盖生成层绑定: timesettingbutton/lightsettingbutton 走本层 buttonClicked,
     以便打开窗口时正确初始化滚轮 (initTimeWindow) 等逻辑 */
  timesettingbutton.setAction(settingsButtonCallback);
  lightsettingbutton.setAction(settingsButtonCallback);

  /* timeswitch (12/24切换) 生成层未绑定, 这里手动绑定 */
  timeswitch.setAction(timeSwitchCallback);

  /* 月份/年份滚轮: 绑定联动回调 (天数随年月变化) */
  month.setAnimateToCallback(monthWheelCallback);
  year.setAnimateToCallback(yearWheelCallback);

  /* MVP: 从 presenter 拉取亮度状态 (Model 缓存), 初始化本页 UI */
  uint8_t mode = presenter->getBrightnessMode();
  uint8_t value = presenter->getBrightnessValue();
  bool autoMode = (mode != 0); /* 1=自动 */

  /* 初始化切换按钮显示: 自动->ON图(forceState false), 手动->OFF图(forceState true) */
  lightchanger.forceState(!autoMode);
  lightchanger.invalidate();
  handilighter.setValue(value);
  handilighter.invalidate();

  if (autoMode) {
    autosetlight.setTypedText(touchgfx::TypedText(T_AUTOLIGHT));
    handilighter.setVisible(false); /* 自动=隐藏滑条 */
  } else {
    autosetlight.setTypedText(touchgfx::TypedText(T_HANDILIGHT));
    handilighter.setVisible(true); /* 手动=显示滑条 */
  }
  autosetlight.invalidate();
}

void settingsView::tearDownScreen() { settingsViewBase::tearDownScreen(); }

void settingsView::handleGestureEvent(const touchgfx::GestureEvent &evt) {
  if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL &&
      !(lightwindow.isVisible() || timewindow.isVisible())) {
    // 速度阈值：只有快速、刻意的水平滑动才触发切屏
    // 调节 MIN_SWIPE_VELOCITY 的值：越大越不容易误触，越小越灵敏
    static const int16_t MIN_SWIPE_VELOCITY = 20;
    if (evt.getVelocity() > MIN_SWIPE_VELOCITY) // 向右快速滑 → 切屏
    {
      application()
          .gotoscreen1ScreenSlideTransitionWest(); // 跳转到screen1_screen
    }
    // 速度不够 → 忽略
  }
  settingsViewBase::handleGestureEvent(evt);
  if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL &&
      (lightwindow.isVisible() ||
       timewindow.isVisible())) { // 当窗口存在时，你就可以通过左划来关闭窗口啦
    static const int16_t MIN_SWIPE_VELOCITY = 20;
    if (evt.getVelocity() > MIN_SWIPE_VELOCITY) // 向右快速滑-> 关闭窗口
    {
      bool timeWasOpen = timewindow.isVisible();
      if (timeWasOpen) {
        applyTimeWindow(); /* 关闭时间窗口前: 应用滚轮选中的时间 (MVP) */
      }
      lightwindow.setVisible(false);
      lightwindow.invalidate();
      timewindow.setVisible(false);
      timewindow.invalidate();
      lightsettingbutton.setVisible(true);
      timesettingbutton.setVisible(true);
    }
  }
}

void settingsView::buttonClicked(
    const touchgfx::AbstractButton &src) { // 按钮点击后会触发的回调函数
  if (&src == &lightsettingbutton) {
    // 显示亮度调节窗口, 同时根据目前的参数进行一些初始化
    lightwindow.setVisible(true);
    lightwindow.invalidate();
    lightsettingbutton.setVisible(false);
    timesettingbutton.setVisible(false);
    /* 打开窗口时, 把切换按钮和滑条同步到当前模式 (状态来自 presenter) */
    bool autoMode = (presenter->getBrightnessMode() != 0);
    lightchanger.forceState(!autoMode);
    lightchanger.invalidate();
    handilighter.setValue(presenter->getBrightnessValue());
    handilighter.setVisible(!autoMode); /* 自动=隐藏滑条, 手动=显示滑条 */
    handilighter.invalidate();
  }
  if (&src == &timesettingbutton) {
    // 显示时间设置窗口
    timewindow.setVisible(true);
    timewindow.invalidate();
    lightsettingbutton.setVisible(false);
    timesettingbutton.setVisible(false);
    /* 打开窗口时: 用当前时间初始化 6 个滚轮 + 时制 */
    initTimeWindow();
  }
  if (&src == &timeswitch) {
    /* 12/24 切换 */
    applyHourFormat((m_hourFormat == 24) ? 12 : 24);
  }
}

/* 生成层 buttonCallbackHandler 在 lightchanger 被点击时调用此虚函数:
   MVP: 通知 presenter 切换自动/手动, UI 更新留在本层 */
void settingsView::swithcHandiLighter() {
  /* ToggleButton 点击后内部已自动交换两张图; getState() 反推模式 */
  bool autoMode = !lightchanger.getState();
  presenter->brightnessModeToggled(autoMode);

  if (autoMode) {
    autosetlight.setTypedText(touchgfx::TypedText(T_AUTOLIGHT));
    autosetlight.invalidate();
    handilighter.setVisible(false);
    handilighter.invalidate();
  } else {
    autosetlight.setTypedText(touchgfx::TypedText(T_HANDILIGHT));
    autosetlight.invalidate();
    handilighter.setVisible(true);
    handilighter.invalidate();
    /* 切到手动: 立即应用当前滑条值 */
    presenter->brightnessValueChanged(handilighter.getValue());
  }
}

/* 生成层 sliderValueChangedCallbackHandler 在滑条值变化时调用此虚函数:
   MVP: 通知 presenter 亮度值变化 (业务写入在 Model) */
void settingsView::handisetlight(int value) {
  presenter->brightnessValueChanged((uint8_t)value);
}

/* Model -> presenter -> view: 亮度状态推送 (本页正在显示时更新 UI) */
void settingsView::setBrightnessState(uint8_t mode, uint8_t value) {
  bool autoMode = (mode != 0);
  /* 同步控件 (若窗口打开) */
  lightchanger.forceState(!autoMode);
  lightchanger.invalidate();
  handilighter.setValue(value);
  handilighter.invalidate();
  autosetlight.setTypedText(touchgfx::TypedText(
      autoMode ? T_AUTOLIGHT : T_HANDILIGHT));
  autosetlight.invalidate();
  handilighter.setVisible(!autoMode);
  handilighter.invalidate();
}

/* ========================================================================
 * 时间设置滚轮 (MVP: 显示用 updateItem, 写入经 presenter)
 * ======================================================================== */

/* 生成层在滚轮需要重绘某个格子时调用: 用 itemIndex 填显示值 */
void settingsView::hourUpdateItem(hourfigure1& item, int16_t itemIndex)
{
  /* 24h: 0-23; 12h: 滚轮 0-11 -> 显示 1-12 */
  if (m_hourFormat == 12) {
    item.setValue((uint8_t)(itemIndex + 1));
  } else {
    item.setValue((uint8_t)itemIndex);
  }
}

void settingsView::minuteUpdateItem(minutefigure& item, int16_t itemIndex)
{
  item.setValue((uint8_t)itemIndex); /* 0-59 */
}

void settingsView::yearUpdateItem(yearfigure1& item, int16_t itemIndex)
{
  item.setValue((uint16_t)itemIndex); /* 滚轮 0-99 = 年份后两位 */
}

void settingsView::amandpmboxUpdateItem(amandpm1& item, int16_t itemIndex)
{
  item.setValue((uint8_t)itemIndex); /* 0=AM, 1=PM */
}

void settingsView::monthUpdateItem(monthfigure1& item, int16_t itemIndex)
{
  item.setValue((uint8_t)(itemIndex + 1)); /* 0-11 -> 1-12 */
}

void settingsView::dayUpdateItem(dayfigure1& item, int16_t itemIndex)
{
  item.setValue((uint8_t)(itemIndex + 1)); /* 0-30 -> 1-31 */
}

/* 打开时间窗口: 用当前 RTC 时间初始化 6 个滚轮 */
void settingsView::initTimeWindow()
{
  /* 时制从 presenter 拉取 (MVP) */
  m_hourFormat = presenter->getHourFormat();

  /* 当前时间经 presenter 拉取 (数据来自 Model 缓存) */
  uint8_t h24 = presenter->getCurrentHour();
  uint8_t mi = presenter->getCurrentMinute();
  uint16_t y = presenter->getCurrentYear();
  uint8_t mo = presenter->getCurrentMonth();
  uint8_t d = presenter->getCurrentDay();

  /* 小时滚轮: 24h 直接放 0-23; 12h 转成 0-11 (1-12) */
  if (m_hourFormat == 12) {
    hour.animateToItem((h24 % 12) == 0 ? 11 : (int16_t)((h24 % 12) - 1), 0);
    amandpmbox.animateToItem((h24 >= 12) ? 1 : 0, 0); /* PM/AM */
  } else {
    hour.animateToItem((int16_t)h24, 0);
    amandpmbox.setVisible(false);
  }

  minute.animateToItem((int16_t)mi, 0);
  year.animateToItem((int16_t)(y - 2000), 0);
  month.animateToItem((int16_t)(mo - 1), 0);
  day.animateToItem((int16_t)(d - 1), 0);

  hour.invalidate();
  minute.invalidate();
  year.invalidate();
  amandpmbox.invalidate();
  month.invalidate();
  day.invalidate();

  refreshDayWheel(); /* 按当前年月设置天数滚轮格数 */
}

/* 月份/年份滚轮选中回调: 天数随年月变化, 重算 day 格数 */
void settingsView::onMonthYearWheelChanged(int16_t /*itemIndex*/)
{
  refreshDayWheel();
}

/* 根据当前年月重算 day 滚轮格数 (28/29/30/31) 并校正越界 */
void settingsView::refreshDayWheel()
{
  int16_t yIdx = year.getSelectedItem();
  int16_t mIdx = month.getSelectedItem();
  uint16_t yearVal = (uint16_t)(2000 + yIdx);   /* 滚轮 0-99 -> 2000-2099 */
  uint8_t  monthVal = (uint8_t)(mIdx + 1);      /* 滚轮 0-11 -> 1-12 */

  uint8_t maxDay;
#ifndef SIMULATOR
  maxDay = TimeService_DaysInMonth(yearVal, monthVal);
#else
  maxDay = 31;
#endif

  /* 当前选中天数 (滚轮 0-30 -> 1-31) 越界则校正到最大天数 */
  int16_t sel = day.getSelectedItem();
  if (sel >= (int16_t)maxDay) {
    day.animateToItem((int16_t)(maxDay - 1), 0);
  }

  day.setNumberOfItems(maxDay);
  day.invalidate();
}

/* 12/24 切换: 重配小时滚轮格数 + AM/PM 滚轮可见性,
   并保持"当前滚轮设置的时间"在切换后数值一致 (如 3:38 PM <-> 15:38) */
void settingsView::applyHourFormat(uint8_t format)
{
  /* 切换前: 把当前滚轮的小时读数转成 24h 基准 (避免切换后数值漂移) */
  int16_t curIdx = hour.getSelectedItem();
  uint8_t h24;
  if (m_hourFormat == 12) {
    uint8_t h12 = (uint8_t)(curIdx + 1);            /* 0-11 -> 1-12 */
    uint8_t isPM = (uint8_t)amandpmbox.getSelectedItem();
    h24 = (h12 % 12);
    if (isPM) h24 += 12;
  } else {
    h24 = (uint8_t)curIdx;                           /* 已是 24h */
  }

  m_hourFormat = format;
  presenter->hourFormatToggled(format); /* 存偏好 (MVP) */

  if (format == 12) {
    hour.setNumberOfItems(12);      /* 1-12 */
    amandpmbox.setVisible(true);
    /* 24h -> 12h: 显示值 1-12, 0/12 都显示 12; AM/PM 由 24h 值决定 */
    uint8_t h12 = (h24 % 12);
    if (h12 == 0) h12 = 12;
    hour.animateToItem((int16_t)(h12 - 1), 0);      /* 滚轮 0-11 -> 1-12 */
    amandpmbox.animateToItem((h24 >= 12) ? 1 : 0, 0); /* PM/AM */
  } else {
    hour.setNumberOfItems(24);      /* 0-23 */
    amandpmbox.setVisible(false);
    /* 12h -> 24h: 直接定位到换算出的 24h 值 */
    hour.animateToItem((int16_t)h24, 0);
  }
  hour.invalidate();
  amandpmbox.invalidate();
}

/* 读取 6 个滚轮选中值 -> 写入 RTC (MVP: 经 presenter -> model) */
void settingsView::applyTimeWindow()
{
  int16_t h = hour.getSelectedItem();
  int16_t mi = minute.getSelectedItem();
  int16_t y = year.getSelectedItem();
  int16_t mo = month.getSelectedItem();
  int16_t d = day.getSelectedItem();

  /* 最终防线: 天数不能超过该月最大天数 (防止 2月31号 之类的非法日期) */
  uint8_t maxDay = 31;
#ifndef SIMULATOR
  maxDay = TimeService_DaysInMonth((uint16_t)(2000 + y), (uint8_t)(mo + 1));
#endif
  if (d >= (int16_t)maxDay)
    d = (int16_t)(maxDay - 1);

  /* 12h -> 24h 转换 */
  uint8_t h24;
  if (m_hourFormat == 12) {
    uint8_t h12 = (uint8_t)(h + 1);            /* 0-11 -> 1-12 */
    uint8_t isPM = (uint8_t)amandpmbox.getSelectedItem();
    h24 = (h12 % 12);
    if (isPM) h24 += 12;
  } else {
    h24 = (uint8_t)h;
  }

  presenter->dateTimeChanged(
      (uint16_t)(2000 + y), (uint8_t)(mo + 1), (uint8_t)(d + 1),
      h24, (uint8_t)mi);
}

/* timeswitch (12/24切换) 点击回调 */
void settingsView::timeSwitchClicked(const touchgfx::AbstractButton& src)
{
  applyHourFormat((m_hourFormat == 24) ? 12 : 24);
}
