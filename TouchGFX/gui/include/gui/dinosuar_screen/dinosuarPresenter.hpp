#ifndef DINOSUARPRESENTER_HPP
#define DINOSUARPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class dinosuarView;

class dinosuarPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    dinosuarPresenter(dinosuarView& v);

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

    virtual ~dinosuarPresenter() {}

private:
    dinosuarPresenter();

    dinosuarView& view;
};

#endif // DINOSUARPRESENTER_HPP
