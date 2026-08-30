#ifndef ALARMVIEW_HPP
#define ALARMVIEW_HPP

#include <gui_generated/alarm_screen/alarmViewBase.hpp>
#include <gui/alarm_screen/alarmPresenter.hpp>
#include <gui/containers/AlarmItem.hpp>

#define ALARM_COUNT 8   /* 闹钟数量, 与 AlarmService 保持一致 */

class alarmView : public alarmViewBase, public AlarmItemListener
{
public:
    alarmView();
    virtual ~alarmView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /* 滚轮格子更新 (生成层 updateItemCallbackHandler 调用) */
    virtual void hourUpdateItem(hourfigure1& item, int16_t itemIndex);
    virtual void minuteUpdateItem(minutefigure& item, int16_t itemIndex);
    virtual void amandpmboxUpdateItem(amandpm1& item, int16_t itemIndex);

    /* ScrollList 格子更新 (生成层调用: 填充每个闹钟项显示) */
    virtual void scrollList1UpdateItem(AlarmItem& item, int16_t itemIndex);

    /* ScrollList 选中回调: 滚动停止选中某项 → 进编辑 */
    void scrollList1SelectedHandler(int16_t itemIndex);

    /* AlarmItemListener: 点击闹钟项 → 进编辑 */
    virtual void onAlarmItemClicked(int index);    /* 选中项 → 编辑 */

    virtual void handleGestureEvent(const touchgfx::GestureEvent &evt);

    /* 编辑窗口: 保存 (confirm=开, cancel=关) */
    void saveEditWindow(bool enabled);

    /* 按钮回调 (confirm=开 / cancel=关 / 12-24切换) */
    void buttonClicked(const touchgfx::AbstractButton& src);

protected:
    int m_editIndex;               /* 当前编辑的闹钟索引 (-1=无) */
    uint8_t m_hourFormat;          /* 12/24 时制 */

    /* 编辑弹窗: 打开时初始化滚轮 */
    void initEditWheels(uint8_t hour, uint8_t minute);

    /* 12/24 切换 */
    void toggleHourFormat();

private:
    /* 保存/取消/12-24切换按钮回调 (生成层只隐藏窗口, 本层做数据处理) */
    touchgfx::Callback<alarmView, const touchgfx::AbstractButton&> settingsButtonCallback;
    /* ScrollList 选中回调 (滚动停止选中某项 → 进编辑) */
    touchgfx::Callback<alarmView, int16_t> scrollList1SelectedCallback;
};

#endif // ALARMVIEW_HPP
