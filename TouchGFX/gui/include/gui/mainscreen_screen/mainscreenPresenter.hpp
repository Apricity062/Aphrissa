//这个文件存放着中转站：presenter，负责接收model的变化，并通知view更新UI
#ifndef MAINSCREENPRESENTER_HPP
#define MAINSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>
#include <cstdint>

using namespace touchgfx;

class mainscreenView;

class mainscreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    mainscreenPresenter(mainscreenView& v);

    /**
     * The activate function is called automatically when this screen is
     * "switched in" (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();
    //void handleSwipGesture(int deltaX);//处理滑动手势，判断滑动手势该做什么
    void updateTime(uint8_t hour, uint8_t minute, uint8_t second); // 实现ModelListener的updateTime方法，当model的时间发生变化时，presenter会收到通知，并调用view的updateTime方法更新UI
    void updateDate(uint8_t year, uint8_t month, uint8_t day, uint8_t weekday); // 实现ModelListener的updateDate方法，当model的日期发生变化时，presenter会收到通知，并调用view的updateDate方法更新UI
        /**
         * The deactivate function is called automatically when this screen is
         * "switched out" (ie. made inactive). Teardown functionality can be
         * placed here.
         */
    virtual void deactivate();

    virtual ~mainscreenPresenter() {}

private : mainscreenPresenter();

    mainscreenView &view;
    int totalDeltaX = 0; // 累计滑动距离  
};

#endif // MAINSCREENPRESENTER_HPP
