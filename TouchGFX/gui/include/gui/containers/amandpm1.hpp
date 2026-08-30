#ifndef AMANDPM1_HPP
#define AMANDPM1_HPP

#include <gui_generated/containers/amandpm1Base.hpp>

class amandpm1 : public amandpm1Base
{
public:
    amandpm1();
    virtual ~amandpm1() {}

    virtual void initialize();

    /* 设置滚轮格子显示的 AM/PM 文本: 0=AM, 1=PM */
    void setValue(uint8_t isPM);

protected:
    static const uint16_t BUF_SIZE = 3; /* "AM"/"PM" + '\0' */
    touchgfx::Unicode::UnicodeChar valueBuffer[BUF_SIZE];
};

#endif // AMANDPM1_HPP
