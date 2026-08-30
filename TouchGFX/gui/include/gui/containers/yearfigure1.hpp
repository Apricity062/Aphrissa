#ifndef YEARFIGURE1_HPP
#define YEARFIGURE1_HPP

#include <gui_generated/containers/yearfigure1Base.hpp>

class yearfigure1 : public yearfigure1Base
{
public:
    yearfigure1();
    virtual ~yearfigure1() {}

    virtual void initialize();

    /* 设置滚轮格子显示的年份 (2000-2099, 4位) */
    void setValue(uint16_t year);

protected:
    static const uint16_t BUF_SIZE = 3; /* "2000" + '\0' *///只需要后面的年份
    touchgfx::Unicode::UnicodeChar valueBuffer[BUF_SIZE];
};

#endif // YEARFIGURE1_HPP
