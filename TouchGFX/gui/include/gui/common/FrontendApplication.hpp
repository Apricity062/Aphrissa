#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    virtual void handleTickEvent();

    /* 响铃界面跳转 (Designer 未为 ringingscreen 生成 goto 方法,
       因无 interaction 指向它; 此处手写, 由响铃状态驱动) */
    void gotoringscreenScreenNoTransition();

private:
    void gotoringscreenScreenNoTransitionImpl();

    touchgfx::Callback<FrontendApplication> ringTransitionCallback;
    uint8_t lastRinging = 0;   /* 上一帧响铃状态, 用于检测上升沿 */
};

#endif // FRONTENDAPPLICATION_HPP
