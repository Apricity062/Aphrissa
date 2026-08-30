#include <gui/containers/yearfigure1.hpp>
#include <touchgfx/Unicode.hpp>

yearfigure1::yearfigure1()
{

}

void yearfigure1::initialize()
{
    yearfigure1Base::initialize();
}

void yearfigure1::setValue(uint16_t year)
{
    Unicode::snprintf(valueBuffer, BUF_SIZE, "%02u", (unsigned)year); /* 只显示后两位 */
    yearfigure.setWildcard(valueBuffer);
    yearfigure.resizeToCurrentText(); /* 动态文本必须 resize 才显示 */
    yearfigure.invalidate();
}
