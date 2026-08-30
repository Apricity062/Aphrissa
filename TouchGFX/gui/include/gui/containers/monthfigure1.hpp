#ifndef MONTHFIGURE1_HPP
#define MONTHFIGURE1_HPP

#include <gui_generated/containers/monthfigure1Base.hpp>

class monthfigure1 : public monthfigure1Base
{
public:
    monthfigure1();
    virtual ~monthfigure1() {}

    virtual void initialize();

    /* 设置滚轮格子显示的月份 (1-12, 补零) */
    void setValue(uint8_t value);

protected:
    static const uint16_t BUF_SIZE = 3; /* "00" + '\0' */
    touchgfx::Unicode::UnicodeChar valueBuffer[BUF_SIZE];
};

#endif // MONTHFIGURE1_HPP
