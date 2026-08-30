// 这个文件存放着model，负责数据的处理和存储，并通知presenter数据的变化，受害者腐殖质
#include <cstdint>
#ifndef MODEL_HPP
#define MODEL_HPP

class ModelListener;

class Model
{
public:
    Model();
    // 绑定presenter，让model可以通知数据变化
    void bind(ModelListener *listener) { modelListener = listener; }
    // 计数自增
    
    void increment();

    //定时回调
    void tick();

    int getCount() const { return count; }
    uint8_t getCurrentHour() const { return currentHour; }
    uint8_t getCurrentMinute() const { return currentMinute; }
    uint8_t getCurrentSecond() const { return currentSecond; }
    uint8_t getCurrentYear() const { return currentYear; }
    uint8_t getCurrentMonth() const { return currentMonth; }
    uint8_t getCurrentDay() const { return currentDay; }
    uint8_t getCurrentWeekday() const { return currentWeekday; }//这6个是时间的公开接口，别的啥东西可以通过这个方便地获取时间

    bool isScreenOn() const; // 抬手亮屏状态查询

    void stopwatchStart(); // view请求，开始
    void stopwatchStop();
    void stopwatchReset();
    void releaseKeepAwake(); // 离开秒表界面/复位: 恢复自动熄屏
    uint32_t getStopwatchMs() const { return currentStopwatchMs; }      // 秒表当前值
    bool     isStopwatchRunning() const { return stopwatchRunning; }    // 秒表是否运行

    /* ---- 亮度: 模式 + 手动值 (MVP: View 只经 presenter 调用) ---- */
    void setBrightnessMode(uint8_t mode);        // 调 Brightness_SetMode
    void setBrightnessValue(uint8_t value);      // 调 Brightness_SetManualValue
    uint8_t getBrightnessMode() const { return brightnessMode; }   // 进界面时拉初始状态
    uint8_t getBrightnessValue() const { return brightnessValue; } // 进界面时拉滑条值

    /* ---- 手电筒 (lightning 屏幕) ---- */
    void flashlightOn();   /* 记录当前亮度 -> 拉满 */
    void flashlightOff();  /* 恢复进入手电筒前的亮度 */

    /*时间设置*/
    void setDateTime(uint16_t y,uint8_t mo,uint8_t d,uint8_t h,uint8_t mi);//调用TimeService_SetDatetime
    void setHourFormat(uint8_t fmt);//存偏好
    uint8_t getHourFormat()const;//进入界面拉取

    /* ---- 闹钟 (MVP: View 经 presenter 调用这里) ---- */
    uint8_t getAlarm(uint8_t index, uint8_t *hour, uint8_t *minute, uint8_t *enabled); // 读单个闹钟
    void    setAlarm(uint8_t index, uint8_t hour, uint8_t minute, uint8_t enabled);    // 写单个闹钟
    void    dismissAlarm();                                                              // 关闭响铃
    uint8_t isAlarmRinging() const { return alarmRinging; }   // 是否响铃中
    uint8_t getAlarmRingingIndex() const { return alarmRingingIndex; } // 响铃的闹钟索引

protected:
  ModelListener *modelListener;
private:
  int count = 0;
  const int maximages = 2;
  bool screenOn = true;   /* 抬手亮屏状态，目标板在 tick() 中同步 */
  uint8_t currentHour = 0;
  uint8_t currentMinute = 0;
  uint8_t currentSecond = 0;
  uint16_t tickCounter = 0;
  uint8_t currentYear = 6;
  uint8_t currentMonth = 6;
  uint8_t currentDay = 2;
  uint8_t currentWeekday = 5;

  /* ---- 秒表状态缓存 (Model::tick 轮询裸机服务后缓存, 供 View 拉取) ---- */
  uint32_t currentStopwatchMs = 0;
  bool     stopwatchRunning = false;
  uint32_t lastStopwatchMs = 0xFFFFFFFF; /* 初值不同确保首帧即推送 */
  bool     lastStopwatchRunning = false;

  /* ---- 亮度状态缓存 ---- */
  uint8_t brightnessMode = 1;    /* 默认自动 */
  uint8_t brightnessValue = 60;  /* 默认手动 60% */

  /* ---- 手电筒: 进入手电筒前的亮度+模式, 关闭时恢复
          模式值: 1=自动(BRIGHT_MODE_AUTO), 0=手动(BRIGHT_MODE_MANUAL) ---- */
  uint8_t savedBrightness = 60;
  uint8_t savedBrightnessMode = 1; /* 默认自动 */

  /* ---- 闹钟状态缓存 (Model::tick 轮询裸机服务后缓存, 供 View 拉取) ---- */
  uint8_t alarmRinging = 0;        /* 1=响铃中 */
  uint8_t alarmRingingIndex = 0;   /* 响铃的闹钟索引 */
};

#endif // MODEL_HPP
