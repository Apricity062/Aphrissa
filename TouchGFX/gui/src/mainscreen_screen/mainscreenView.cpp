// 这个文件存放着按钮被点击时的回调函数等，和touchgfx联系起来，玻利瓦尔肤层
// 同时被view.各种函数在presenter中调用
#include <gui/mainscreen_screen/mainscreenView.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/Color.hpp>

mainscreenView::mainscreenView()
{

}

void mainscreenView::setupScreen()
{
    mainscreenViewBase::setupScreen();

    /* 星期文本居中: TypedText 的对齐在 Designer 里设置,
       这里用 resizeToCurrentTextWithAlignment() 按对齐方式定位 */
    weekday.setPosition(45, 57, 150, 24);
}

void mainscreenView::tearDownScreen()
{
    mainscreenViewBase::tearDownScreen();
}

void mainscreenView::handleGestureEvent(const touchgfx::GestureEvent& evt)
{
    // TouchGFX 手势: 检测到快速水平滑动时触发
    if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL) {
      // 速度阈值：只有快速、刻意的水平滑动才触发切屏
      // 调节 MIN_SWIPE_VELOCITY 的值：越大越不容易误触，越小越灵敏
      static const int16_t MIN_SWIPE_VELOCITY = 15;
      if (evt.getVelocity() < -MIN_SWIPE_VELOCITY) // 向左快速滑 → 切屏
      {
        application().gotoscreen1ScreenSlideTransitionEast(); // 跳转到screen1_screen
      }
      // 速度不够 → 忽略
    }
    mainscreenViewBase::handleGestureEvent(evt);
}


void mainscreenView::changescreen() //
{
  application().gotoscreen1ScreenSlideTransitionEast(); // 跳转到screen1_screen
}

void mainscreenView::setTime(uint8_t hour, uint8_t minute, uint8_t second) // 设置时间显示
{
    if (m_hourFormat == 12) {
        /* 12小时制标准:
           AM = 午夜12:00 ~ 中午11:59  (24h 0~11)
           PM = 中午12:00 ~ 午夜11:59  (24h 12~23)
           午夜 12:00 AM, 中午 12:00 PM, 即 24h 的 0 和 12 都显示 "12" */
        bool isAM = (hour < 12);
        /* 24h -> 12h 显示值: 0->12, 1->1, ..., 12->12, 13->1, ... */
        uint8_t h12 = (hour % 12);
        if (h12 == 0) h12 = 12;
        digitalClock1.setDisplayMode(touchgfx::DigitalClock::DISPLAY_12_HOUR);
        /* 框架内部 setTime24Hour((h12%12) + (am?0:12)):
           传 h12 即可, 框架 %12 会把 12 归 0 再按 AM/PM 加回, 结果一致 */
        digitalClock1.setTime12Hour(h12, minute, second, isAM);
    } else {
        digitalClock1.setDisplayMode(touchgfx::DigitalClock::DISPLAY_24_HOUR);
        digitalClock1.setTime24Hour(hour, minute, second);
    }
}

void mainscreenView::setHourFormat(uint8_t format) // 12/24 显示偏好
{
    m_hourFormat = (format == 12) ? 12 : 24;
    digitalClock1.setDisplayMode(m_hourFormat == 12
        ? touchgfx::DigitalClock::DISPLAY_12_HOUR
        : touchgfx::DigitalClock::DISPLAY_24_HOUR);
}

void mainscreenView::setDate(uint8_t year, uint8_t month, uint8_t day, uint8_t wday) // 设置日期显示
{
  // Unicode::snprintf 返回缓冲区首地址(不是结束位置)，不能直接用返回值推进
  // 年=0x5E74, 月=0x6708, 日=0x65E5
  Unicode::UnicodeChar *buf = dateBuffer;
  Unicode::UnicodeChar *wb=weekdayBuffer;

  Unicode::snprintf(buf, DATE_SIZE, "%04d", 2000 + year);
  buf += Unicode::strlen(buf);
  *buf++ = 0x5E74; // 年

  Unicode::snprintf(buf, DATE_SIZE - (buf - dateBuffer), "%02d",month);
  buf += Unicode::strlen(buf);
  *buf++ = 0x6708; // 月

  Unicode::snprintf(buf, DATE_SIZE - (buf - dateBuffer), "%02d", day);
  buf += Unicode::strlen(buf);
  *buf++ = 0x65E5; // 日

  *buf = 0; // 终止符

  date.resizeToCurrentText();
  date.invalidate();
  /*下方开始写星期*/
  static const uint16_t weekChars[8] = {
      0,      0x4E00, 0x4E8C, 0x4E09,
      0x56DB, 0x4E94, 0x516D, 0x65E5}; // 由1-7分别对应一二三四五六日

  *wb++ = 0x661F;
  *wb++ = 0x671F;
  if (wday >= 1 && wday <= 7) {
    *wb++=weekChars[wday];
  }
  *wb = 0;
  weekday.setWildcard(weekdayBuffer);
  /* 按 TypedText 的对齐方式 resize (Designer 里设居中对齐) */
  weekday.resizeToCurrentTextWithAlignment();
  weekday.invalidate();
}
