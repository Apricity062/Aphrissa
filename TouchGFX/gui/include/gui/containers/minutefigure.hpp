#ifndef MINUTEFIGURE_HPP
#define MINUTEFIGURE_HPP

#include <gui_generated/containers/minutefigureBase.hpp>

class minutefigure : public minutefigureBase
{
public:
    minutefigure();
    virtual ~minutefigure() {}

    virtual void initialize();

    /* 设置滚轮格子显示的两位数 (0-59 等, 自动补零) */
    void setValue(uint8_t value);

protected:
    static const uint16_t BUF_SIZE = 3; /* "00" + '\0' */
    touchgfx::Unicode::UnicodeChar valueBuffer[BUF_SIZE];
};

#endif // MINUTEFIGURE_HPP
