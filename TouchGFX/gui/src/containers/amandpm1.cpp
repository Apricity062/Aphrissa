#include <gui/containers/amandpm1.hpp>
#include <touchgfx/Unicode.hpp>

amandpm1::amandpm1()
{

}

void amandpm1::initialize()
{
    amandpm1Base::initialize();
}

void amandpm1::setValue(uint8_t isPM)
{
    if (isPM) {
        valueBuffer[0] = 'P';
        valueBuffer[1] = 'M';
    } else {
        valueBuffer[0] = 'A';
        valueBuffer[1] = 'M';
    }
    valueBuffer[2] = 0;
    amandpm.setWildcard(valueBuffer);
    amandpm.resizeToCurrentText(); /* 动态文本必须 resize 才显示 */
    amandpm.invalidate();
}
