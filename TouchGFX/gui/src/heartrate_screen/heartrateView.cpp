#include <gui/heartrate_screen/heartrateView.hpp>
#include <stdio.h>

#ifndef SIMULATOR
extern "C" {
#include "motion_service.h"
#include "max30102_service.h"
}
#endif

heartrateView::heartrateView()
{

}

void heartrateView::setupScreen()
{
    heartrateViewBase::setupScreen();
#ifndef SIMULATOR
    MotionService_SetKeepAwake(1); /* 测心率时保持亮屏 */
    Max30102_Start();              /* 进心率屏才开启传感器 */
#endif
    heartrateBuffer[0] = 0;
    heartrate.setWildcard(heartrateBuffer);
    heartrate.setPosition(39, 32, 140, 34); /* 基类只setXY没设尺寸(0×0), 必须给宽高否则不渲染 */
    bloodoxygenBuffer[0] = 0;
    bloodoxygen.setWildcard(bloodoxygenBuffer);
    bloodoxygen.setPosition(39, 64, 140, 34);
}

void heartrateView::tearDownScreen()
{
#ifndef SIMULATOR
    MotionService_SetKeepAwake(0); /* 离开心率屏恢复自动熄屏 */
    Max30102_Stop();               /* 离开心率屏关闭传感器(省电) */
#endif
    heartrateViewBase::tearDownScreen();
}

void heartrateView::handleGestureEvent(const touchgfx::GestureEvent &evt) {
  if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL) {
    static const int16_t MIN_SWIPE_VELOCITY = 20;
    if (evt.getVelocity() > MIN_SWIPE_VELOCITY) {
      application().gotoscreen1ScreenSlideTransitionWest();
      return;
    }
  }
  //heartrateViewBase::handleGestureEvent(evt);
}

void heartrateView::updateHeartRate(int32_t hr,int32_t spo2) {
    printf("[VIEW] hr=%d spo2=%d\n", (int)hr, (int)spo2); /* 联调 */
    if(hr<0){
        Unicode::snprintf(heartrateBuffer, 8, "--");
    }else{
        Unicode::snprintf(heartrateBuffer, 8, "%d", hr);
    }
    heartrate.setWildcard(heartrateBuffer);
    heartrate.invalidate();
    if(spo2<0){
        Unicode::snprintf(bloodoxygenBuffer, 8, "--");
    }else{
        Unicode::snprintf(bloodoxygenBuffer, 8, "%d", spo2);
    }
    bloodoxygen.setWildcard(bloodoxygenBuffer);
    bloodoxygen.invalidate();
}