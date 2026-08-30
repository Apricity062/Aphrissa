#include <gui/alarm_screen/alarmView.hpp>
#include <touchgfx/Unicode.hpp>

/* ========================================================================
 * 构造 / 生命周期
 * ======================================================================== */

alarmView::alarmView()
    : m_editIndex(-1), m_hourFormat(24),
      settingsButtonCallback(this, &alarmView::buttonClicked),
      scrollList1SelectedCallback(this, &alarmView::scrollList1SelectedHandler)
{
}

void alarmView::setupScreen() {
  alarmViewBase::setupScreen();

  /* ScrollList 首帧强制填充: 直接对格子池每个格子调 updateItem
     (itemChanged 可能不触发 updateItemCallback, 手动填充保证显示) */
  for (int i = 0; i < scrollList1ListItems.getNumberOfDrawables(); ++i) {
    scrollList1ListItems[i].setListener(this, i);
    scrollList1UpdateItem(scrollList1ListItems[i], i);  /* 手动填充: 时间+开关图 */
  }

  /* ScrollList 选中回调: 滚动停止选中某项 → 进编辑 */
  scrollList1.setItemSelectedCallback(scrollList1SelectedCallback);

  /* 重新绑定 confirm(开)/cancel(关)/12-24切换按钮到本层回调 */
  alarmconfirm.setAction(settingsButtonCallback);
  alarmcancel.setAction(settingsButtonCallback);
  timeswitch.setAction(settingsButtonCallback);

  m_hourFormat = presenter->getHourFormat(); /* 12/24 时制: 从 Model 拉偏好 */
}

void alarmView::tearDownScreen() { alarmViewBase::tearDownScreen(); }

/* ========================================================================
 * 滚轮格子更新 (与 settings 的 hour/minute/amandpm 一致)
 * ======================================================================== */

void alarmView::hourUpdateItem(hourfigure1 &item, int16_t itemIndex) {
  if (m_hourFormat == 12) {
    item.setValue((uint8_t)(itemIndex + 1)); /* 1-12 */
  } else {
    item.setValue((uint8_t)itemIndex); /* 0-23 */
  }
}

void alarmView::minuteUpdateItem(minutefigure &item, int16_t itemIndex) {
  item.setValue((uint8_t)itemIndex); /* 0-59 */
}

void alarmView::amandpmboxUpdateItem(amandpm1 &item, int16_t itemIndex) {
  item.setValue((uint8_t)itemIndex); /* 0=AM, 1=PM */
}

/* ========================================================================
 * ScrollList 格子更新: 填充每个闹钟项显示
 * ======================================================================== */

void alarmView::scrollList1UpdateItem(AlarmItem &item, int16_t itemIndex) {
  if (itemIndex < 0 || itemIndex >= ALARM_COUNT)
    return;

  /* MVP: 数据从 Model 经 presenter 读取, View 不保存闹钟数据 */
  uint8_t h = 0, m = 0, e = 0;
  presenter->getAlarm((uint8_t)itemIndex, &h, &m, &e);

  item.setListener(this, itemIndex);           /* 绑定交互回调 */
  item.setAlarmTime(h, m);
  item.setEnabled(e != 0);                     /* alarmon/alarmoff 图 */
}
/* ========================================================================
 * AlarmItemListener: 点击闹钟项 → 进编辑
 * ======================================================================== */

/* ScrollList 选中回调: 滚动停止选中某项 → 进编辑 */
void alarmView::scrollList1SelectedHandler(int16_t itemIndex) {
  onAlarmItemClicked(itemIndex);
}

void alarmView::onAlarmItemClicked(int index) {
  if (index < 0 || index >= ALARM_COUNT)
    return;
  m_editIndex = index;

  /* MVP: 从 Model 经 presenter 拉当前时间, 初始化滚轮 */
  uint8_t h = 0, m = 0, e = 0;
  presenter->getAlarm((uint8_t)index, &h, &m, &e);
  initEditWheels(h, m);

  alarmwindow.setVisible(true);
  alarmwindow.invalidate();
}

/* ========================================================================
 * 编辑窗口
 * ======================================================================== */

/* 打开编辑窗口时初始化滚轮到指定时间 */
void alarmView::initEditWheels(uint8_t hIn, uint8_t mIn) {
  if (m_hourFormat == 12) {
    hour.animateToItem((hIn % 12) == 0 ? 11 : (int16_t)((hIn % 12) - 1), 0);
    amandpmbox.animateToItem((hIn >= 12) ? 1 : 0, 0);
    amandpmbox.setVisible(true);
  } else {
    hour.animateToItem((int16_t)hIn, 0);
    amandpmbox.setVisible(false);
  }
  minute.animateToItem((int16_t)mIn, 0);
  hour.invalidate();
  minute.invalidate();
  amandpmbox.invalidate();
}

/* 保存: 读滚轮值 + 开关状态 → 存入闹钟数据 → 刷新格子 → 关闭窗口 */
void alarmView::saveEditWindow(bool enabled) {
  int16_t h = hour.getSelectedItem();
  int16_t m = minute.getSelectedItem();

  /* 12h -> 24h 转换 */
  uint8_t h24;
  if (m_hourFormat == 12) {
    uint8_t h12 = (uint8_t)(h + 1);
    uint8_t isPM = (uint8_t)amandpmbox.getSelectedItem();
    h24 = (h12 % 12);
    if (isPM)
      h24 += 12;
  } else {
    h24 = (uint8_t)h;
  }

  if (m_editIndex >= 0 && m_editIndex < ALARM_COUNT) {
    /* MVP: 数据写回 Model (经 presenter), 内部完成 BKP 存储 + Alarm A 重调度 */
    presenter->setAlarm((uint8_t)m_editIndex, h24, (uint8_t)m,
                        enabled ? 1u : 0u);
    scrollList1.itemChanged(m_editIndex);   /* 刷新该格子的时间+开关图 */
  }

  alarmwindow.setVisible(false);
  alarmwindow.invalidate();
  m_editIndex = -1;
}

/* ========================================================================
 * 按钮回调: confirm=开, cancel=关, timeswitch=12/24
 * ======================================================================== */

void alarmView::buttonClicked(const touchgfx::AbstractButton &src) {
  if (&src == &alarmconfirm) {
    saveEditWindow(true);
  } else if (&src == &alarmcancel) {
    saveEditWindow(false);
  } else if (&src == &timeswitch) {
    toggleHourFormat();     /* 12/24 切换 */
  }
}

/* 12/24 切换: 重配小时滚轮格数 + AM/PM 可见性 */
void alarmView::toggleHourFormat() {
  if (m_hourFormat == 24) {
    m_hourFormat = 12;
    hour.setNumberOfItems(12);
    amandpmbox.setVisible(true);
  } else {
    m_hourFormat = 24;
    hour.setNumberOfItems(24);
    amandpmbox.setVisible(false);
  }
  hour.invalidate();
  amandpmbox.invalidate();
}

void alarmView::handleGestureEvent(const touchgfx::GestureEvent &evt) {
  if(!alarmwindow.isVisible()){//当闹钟编辑界面没有开的时候，可以切回主屏幕
    if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL) {
      static const int16_t MIN_SWIPE_VELOCITY = 20;
      if (evt.getVelocity() > MIN_SWIPE_VELOCITY) /* 向右快速滑 → 切屏 */
      {
        application().gotoscreen1ScreenSlideTransitionEast();
      }
    }
    /* ⚠️ 必须调用基类，确保垂直滑动事件继续向下传到 ScrollList */
    alarmViewBase::handleGestureEvent(evt);
  }
  else {//当闹钟界面开启的时候，左划可以把它关掉。
    if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL) {
      static const int16_t MIN_SWIPE_VELOCITY = 20;
      if (evt.getVelocity() > MIN_SWIPE_VELOCITY) {
        alarmwindow.setVisible(false);
        alarmwindow.invalidate();
        }
      }
  }
}
