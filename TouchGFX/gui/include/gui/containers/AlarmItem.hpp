#ifndef ALARMITEM_HPP
#define ALARMITEM_HPP

#include <gui_generated/containers/AlarmItemBase.hpp>

/* 通知接口: alarmView 实现 (目前仅编辑后刷新用, 可保留或精简) */
class AlarmItemListener
{
public:
    virtual ~AlarmItemListener() {}
    virtual void onAlarmItemClicked(int index) = 0;   /* 选中项 → 进编辑 */
};

class AlarmItem : public AlarmItemBase
{
public:
    AlarmItem();
    virtual ~AlarmItem() {}

    virtual void initialize();

    /* 设置本闹钟项显示的时间 (24h 制时/分) */
    void setAlarmTime(uint8_t hour, uint8_t minute);

    /* 读取本闹钟项当前时间 (24h 制) */
    uint8_t getHour() const { return m_hour; }
    uint8_t getMinute() const { return m_minute; }

    /* 设置开关状态: enabled=true → 显示 alarmon(开), false → alarmoff(关) */
    void setEnabled(bool enabled);

    /* 查询当前开关状态 (true=开) */
    bool isEnabled() const { return alarmon.isVisible(); }

    /* 绑定外层监听器 + 本项索引 */
    void setListener(AlarmItemListener* listener, int index);

private:
    AlarmItemListener* m_listener;
    int m_index;
    uint8_t m_hour;
    uint8_t m_minute;
};

#endif // ALARMITEM_HPP
