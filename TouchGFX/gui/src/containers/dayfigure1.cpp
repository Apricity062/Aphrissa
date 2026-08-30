#include <gui/containers/dayfigure1.hpp>
#include <touchgfx/Unicode.hpp>

dayfigure1::dayfigure1()
{

}

void dayfigure1::initialize()
{
    dayfigure1Base::initialize();
}

void dayfigure1::setValue(uint8_t value)
{
    Unicode::snprintf(valueBuffer, BUF_SIZE, "%02u", (unsigned)value);
    dayfigure.setWildcard(valueBuffer);
    dayfigure.resizeToCurrentText(); /* 动态文本必须 resize 才显示 */
    dayfigure.invalidate();
}
