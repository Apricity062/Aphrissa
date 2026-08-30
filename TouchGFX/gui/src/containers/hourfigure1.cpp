#include <gui/containers/hourfigure1.hpp>
#include <touchgfx/Unicode.hpp>

hourfigure1::hourfigure1()
{

}

void hourfigure1::initialize()
{
    hourfigure1Base::initialize();
}

void hourfigure1::setValue(uint8_t value)
{
    Unicode::snprintf(valueBuffer, BUF_SIZE, "%02u", (unsigned)value);
    hourfigure.setWildcard(valueBuffer);
    hourfigure.resizeToCurrentText(); /* 动态文本必须 resize 才显示 */
    hourfigure.invalidate();
}
