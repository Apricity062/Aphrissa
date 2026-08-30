#ifndef MENUITEMCONTAINER_HPP
#define MENUITEMCONTAINER_HPP

#include <gui_generated/containers/menuItemContainerBase.hpp>
#include <cstdint>

class menuItemContainer : public menuItemContainerBase
{
public:
    menuItemContainer();
    virtual ~menuItemContainer() {}
    void updateItem(int16_t itemIndex);

    virtual void initialize();
protected:
};

#endif // MENUITEMCONTAINER_HPP
