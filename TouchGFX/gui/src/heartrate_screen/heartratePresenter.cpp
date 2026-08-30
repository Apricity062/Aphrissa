#include <gui/heartrate_screen/heartrateView.hpp>
#include <gui/heartrate_screen/heartratePresenter.hpp>
#include <stdio.h>

heartratePresenter::heartratePresenter(heartrateView& v)
    : view(v)
{

}

void heartratePresenter::activate()
{

}

void heartratePresenter::deactivate()
{

}



void heartratePresenter::updateHeartRate(int32_t hr, int32_t spo2) {
  view.updateHeartRate(hr, spo2); // 转发给view，view再更新UI
}