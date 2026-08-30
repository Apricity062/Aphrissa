#include <gui/containers/minutefigure.hpp>
#include <touchgfx/Unicode.hpp>

minutefigure::minutefigure()
{

}

void minutefigure::initialize()
{
    minutefigureBase::initialize();
}

void minutefigure::setValue(uint8_t value)
{
    Unicode::snprintf(valueBuffer, BUF_SIZE, "%02u", (unsigned)value);
    minutefigure1.setWildcard(valueBuffer);
    minutefigure1.resizeToCurrentText(); /* 动态文本必须 resize 才显示 */
    minutefigure1.invalidate();
}
