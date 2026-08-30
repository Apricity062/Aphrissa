#ifndef INDEXSCREENPRESENTER_HPP
#define INDEXSCREENPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class indexscreenView;

class indexscreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    indexscreenPresenter(indexscreenView& v);

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

    /* 实现 ModelListener 纯虚函数 (indexscreen 不需要计数, 空实现) */
    //virtual void updateCount(int count);

    virtual ~indexscreenPresenter() {}

private:
    indexscreenPresenter();

    indexscreenView& view;
};

#endif // INDEXSCREENPRESENTER_HPP
