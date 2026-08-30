#ifndef INDEXSCREENVIEW_HPP
#define INDEXSCREENVIEW_HPP

#include <gui_generated/indexscreen_screen/indexscreenViewBase.hpp>
#include <gui/indexscreen_screen/indexscreenPresenter.hpp>

class indexscreenView : public indexscreenViewBase
{
public:
    indexscreenView();
    virtual ~indexscreenView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();
    virtual void handleClickEvent(const touchgfx::ClickEvent &evt);
    virtual void handleGestureEvent(const touchgfx::GestureEvent &evt); // preseter调用这个函数来执行跳转动画
  protected:
    /* 状态机: IDLE → FLASHING(全乱码闪) → DECODING(逐字解码) → DONE */
    enum State { IDLE, FLASHING, DECODING, DONE };
    State state;
    uint16_t flashTicks;
    int16_t resolvedCount;   /* 已解码的字符数 */
    int16_t sentenceLen;     /* 句子总长 */
    uint8_t sentenceIdx;     /* 选中的句子索引 0-3 */
    const touchgfx::Unicode::UnicodeChar* sentenceText;  /* 句子内容 */

    void buttonClicked(const touchgfx::AbstractButtonContainer& src);
    touchgfx::Callback<indexscreenView, const touchgfx::AbstractButtonContainer&> buttonClickedCallback;

    void pickRandomSentence();      /* 随机选一句 */
    void refreshTextBuffer();       /* 按当前解码进度填充缓冲 */
    touchgfx::Unicode::UnicodeChar randomSymbol();  /* 随机全角符号 */
    uint32_t nextRandom();          /* LCG 伪随机 */
    uint32_t m_seed;                /* 随机种子 */
    uint32_t m_clickCount;          /* 点击次数(混入种子, 保证每次点击不同) */
};

#endif // INDEXSCREENVIEW_HPP
