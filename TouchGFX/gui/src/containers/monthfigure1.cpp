#include <gui/containers/monthfigure1.hpp>
#include <touchgfx/Unicode.hpp>

monthfigure1::monthfigure1()
{

}

void monthfigure1::initialize()
{
    monthfigure1Base::initialize();
}

void monthfigure1::setValue(uint8_t value)
{
    Unicode::snprintf(valueBuffer, BUF_SIZE, "%02u", (unsigned)value);
    monthfigure.setWildcard(valueBuffer);
    monthfigure.resizeToCurrentText(); /* 动态文本必须 resize 才显示 */
    monthfigure.invalidate();
}
