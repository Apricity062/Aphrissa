#include <gui/containers/AlarmItem.hpp>
#include <touchgfx/Unicode.hpp>

AlarmItem::AlarmItem()
    : m_listener(0),
      m_index(0),
      m_hour(0),
      m_minute(0)
{
}

void AlarmItem::initialize()
{
    AlarmItemBase::initialize();
}

/* 显示时间, 例如 "07:30" */
void AlarmItem::setAlarmTime(uint8_t hour, uint8_t minute)
{
    m_hour = hour;
    m_minute = minute;
    Unicode::snprintf(alarmtimeBuffer, ALARMTIME_SIZE, "%02u:%02u",
                      (unsigned)hour, (unsigned)minute);
    alarmtime.setWildcard(alarmtimeBuffer);
    alarmtime.resizeToCurrentText(); /* 动态文本必须 resize 才显示 */
    alarmtime.invalidate();
}

/* 开关状态: enabled=true → 显示 alarmon(开), false → 显示 alarmoff(关) */
void AlarmItem::setEnabled(bool enabled)
{
    alarmon.setVisible(enabled);
    alarmoff.setVisible(!enabled);
    alarmon.invalidate();
    alarmoff.invalidate();
}

void AlarmItem::setListener(AlarmItemListener* listener, int index)
{
    m_listener = listener;
    m_index = index;
}
