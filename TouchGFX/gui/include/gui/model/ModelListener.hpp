#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>//前向声明，避免循环依赖

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model *m) { model = m; }

    virtual void updateTime(uint8_t hour, uint8_t minute, uint8_t second) {};

    virtual void updateDate(uint8_t year, uint8_t month, uint8_t day, uint8_t weekday) {};

    virtual void updateHeartRate(int32_t hr,int32_t spo2) {};//更新心率

    /* 秒表: Model::tick 推送 (ms=累计毫秒, running=是否运行中) */
    virtual void updateStopwatch(uint32_t ms, bool running) {};

    /* 亮度: Model::tick 同步 (mode=1自动/0手动, value=手动亮度0-100) */
    virtual void updateBrightness(uint8_t mode, uint8_t value) {};

    /* 闹钟: 响铃状态变化 (ringing=1 开始响, 0 停止; index=响铃的闹钟索引) */
    virtual void updateAlarmRing(uint8_t ringing, uint8_t index) {};
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
