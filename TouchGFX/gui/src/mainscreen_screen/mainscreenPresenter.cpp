// 这个文件存放着中转站：presenter
#include <gui/mainscreen_screen/mainscreenPresenter.hpp>
#include <gui/mainscreen_screen/mainscreenView.hpp>


mainscreenPresenter::mainscreenPresenter(mainscreenView &v) : view(v) {}

void mainscreenPresenter::activate() // presenter被使用的时候
{
  //view.showimage(model->getCount());
  view.setHourFormat(model->getHourFormat()); /* 同步时制偏好 (12/24) */
  view.setTime(model->getCurrentHour(), model->getCurrentMinute(),
               model->getCurrentSecond());
  view.setDate(model->getCurrentYear(), model->getCurrentMonth(),
               model->getCurrentDay(), model->getCurrentWeekday());
}

void mainscreenPresenter::deactivate() {}

/*void mainscreenPresenter::handleSwipGesture(int deltaX) {
  static const int threshold = 60; // 设置滑动阈值
  totalDeltaX += deltaX;           // 累计滑动距离
  if (totalDeltaX < -threshold) {
    // 累计向左滑动超过阈值，执行跳转
    totalDeltaX = 0;
    view.changescreen();
  } else if (totalDeltaX > threshold) { // 防止反方向误触
    totalDeltaX = 0;
  }
}*/

void mainscreenPresenter::updateTime(uint8_t hour, uint8_t minute,
                                     uint8_t second) {
  // 当model的时间发生变化时，presenter会收到来自modelListener的通知，并调用view的updateTime方法更新UI
  view.setTime(hour, minute, second);
}

void mainscreenPresenter::updateDate(uint8_t year, uint8_t month, uint8_t day,
                                     uint8_t weekday) {
  // 当model的日期发生变化时，presenter会收到来自modelListener的通知，并调用view的updateDate方法更新UI
  view.setDate(year, month, day, weekday);
}