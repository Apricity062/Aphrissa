#ifndef HEARTRATEPRESENTER_HPP
#define HEARTRATEPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class heartrateView;

class heartratePresenter : public touchgfx::Presenter, public ModelListener
{
public:
    heartratePresenter(heartrateView& v);

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

    //virtual void updateCount(int count); /* ModelListener 纯虚, 空实现 */

    virtual ~heartratePresenter() {}

    virtual void updateHeartRate(int32_t hr,int32_t spo2); // 更新心率

private:
    heartratePresenter();

    heartrateView& view;
};

#endif // HEARTRATEPRESENTER_HPP
