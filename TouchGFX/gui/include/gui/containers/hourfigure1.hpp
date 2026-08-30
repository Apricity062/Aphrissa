#ifndef HOURFIGURE1_HPP
#define HOURFIGURE1_HPP

#include <gui_generated/containers/hourfigure1Base.hpp>

class hourfigure1 : public hourfigure1Base
{
public:
    hourfigure1();
    virtual ~hourfigure1() {}

    virtual void initialize();

    /* 设置滚轮格子显示的两位数 (0-23 等, 自动补零) */
    void setValue(uint8_t value);

protected:
    static const uint16_t BUF_SIZE = 3; /* "00" + '\0' */
    touchgfx::Unicode::UnicodeChar valueBuffer[BUF_SIZE];
};

#endif // HOURFIGURE1_HPP
