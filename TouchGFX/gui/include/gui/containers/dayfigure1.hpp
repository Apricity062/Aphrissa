#ifndef DAYFIGURE1_HPP
#define DAYFIGURE1_HPP

#include <gui_generated/containers/dayfigure1Base.hpp>

class dayfigure1 : public dayfigure1Base
{
public:
    dayfigure1();
    virtual ~dayfigure1() {}

    virtual void initialize();

    /* 设置滚轮格子显示的日期 (1-31, 补零) */
    void setValue(uint8_t value);

protected:
    static const uint16_t BUF_SIZE = 3; /* "00" + '\0' */
    touchgfx::Unicode::UnicodeChar valueBuffer[BUF_SIZE];
};

#endif // DAYFIGURE1_HPP
