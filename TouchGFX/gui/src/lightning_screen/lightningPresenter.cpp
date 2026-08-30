#include <gui/lightning_screen/lightningView.hpp>
#include <gui/lightning_screen/lightningPresenter.hpp>

lightningPresenter::lightningPresenter(lightningView& v)
    : view(v)
{

}

void lightningPresenter::activate()
{
    
}

void lightningPresenter::deactivate()
{
    model->flashlightOff();
}

/* ---- View 请求 -> Model (MVP 中转) ---- */

void lightningPresenter::flashlightToggled(bool on)
{
    if (on) {
        model->flashlightOn();
    } else {
        model->flashlightOff();
    }
}

