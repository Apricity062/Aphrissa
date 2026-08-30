#ifndef LIGHTNINGPRESENTER_HPP
#define LIGHTNINGPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class lightningView;

class lightningPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    lightningPresenter(lightningView& v);

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

    /* ---- View 请求 (MVP) ---- */
    void flashlightToggled(bool on); /* 手电筒开/关 */

    virtual ~lightningPresenter() {}

private:
    lightningPresenter();

    lightningView& view;
};

#endif // LIGHTNINGPRESENTER_HPP
