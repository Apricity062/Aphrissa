#ifndef CHRONOGRAPHPRESENTER_HPP
#define CHRONOGRAPHPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class chronographView;

class chronographPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    chronographPresenter(chronographView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    /* ---- View 请求 (MVP: View 只调这些, 不直接碰裸机) ---- */
    void startStopToggled();   /* 开始/暂停切换 */
    void resetClicked();       /* 复位 */
    void leaveScreen();        /* 离开界面: 恢复自动熄屏 */

    /* ---- Model -> View 通知 (ModelListener 接口实现) ---- */
    virtual void updateStopwatch(uint32_t ms, bool running);

    //virtual void updateCount(int count); /* ModelListener 纯虚, 空实现 */

    virtual ~chronographPresenter() {}

private:
    chronographPresenter();

    chronographView& view;
};

#endif // CHRONOGRAPHPRESENTER_HPP
