#include <gui/indexscreen_screen/indexscreenView.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/hal/HAL.hpp>

#ifndef SIMULATOR
#include "rng.h"   /* 目标板: 硬件真随机 RNG (hrng) */
#endif

indexscreenView::indexscreenView() :
    buttonClickedCallback(this, &indexscreenView::buttonClicked),
    m_seed(1),
    m_clickCount(0)
{
}

void indexscreenView::setupScreen()
{
    indexscreenViewBase::setupScreen();

    /* 初始: 显示按钮, 隐藏文字区 */
    textscript.setVisible(false);
    textscript.invalidate();
    scriptbutton.setVisible(true);
    scriptbutton.invalidate();
    scriptbutton.setAction(buttonClickedCallback);

    state = IDLE;
    flashTicks = 0;
    resolvedCount = 0;
    sentenceLen = 0;
    sentenceText = 0;
}

void indexscreenView::tearDownScreen()
{
    indexscreenViewBase::tearDownScreen();
}

/* ── 点击按钮: 隐藏按钮, 显示文字区, 开始闪烁 ── */
void indexscreenView::buttonClicked(const touchgfx::AbstractButtonContainer& src)
{
    scriptbutton.setVisible(false);
    scriptbutton.invalidate();
    textscript.setVisible(true);
    state = FLASHING;
    flashTicks = 0;
    m_clickCount++;  /* 点击次数递增 */
#ifndef SIMULATOR
    /* 目标板: 硬件真随机 (RNG 热噪声), 失败则回退伪随机 */
    {
        uint32_t seed;
        if (HAL_RNG_GenerateRandomNumber(&hrng, &seed) == HAL_OK)
        {
            m_seed = seed;
        }
        else
        {
            m_seed = m_clickCount * 2654435761u + 12345u;
        }
    }
#else
    /* 模拟器: 无 RNG 硬件, 用点击计数 + CPU周期伪随机 */
    m_seed = m_clickCount * 2654435761u + touchgfx::HAL::getInstance()->getCPUCycles();
#endif
    pickRandomSentence();  /* 先选好这次要显示的句子 */
}

/* ── 随机选一句, 拿到内容 ── */
void indexscreenView::pickRandomSentence()
{
    sentenceIdx = (uint8_t)((nextRandom() >> 16) % 15u);  /* 用 LCG 高位, 分布更均匀 */
    switch (sentenceIdx)
    {
    case 0: sentenceText = touchgfx::TypedText(T_SCRIPT1).getText(); break;
    case 1: sentenceText = touchgfx::TypedText(T_SCRIPT2).getText(); break;
    case 2: sentenceText = touchgfx::TypedText(T_SCRIPT3).getText(); break;
    case 3: sentenceText = touchgfx::TypedText(T_SCRIPT4).getText(); break;
    case 4: sentenceText = touchgfx::TypedText(T_SCRIPT5).getText(); break;
    case 5: sentenceText = touchgfx::TypedText(T_SCRIPT6).getText(); break;
    case 6: sentenceText = touchgfx::TypedText(T_SCRIPT7).getText(); break;
    case 7: sentenceText = touchgfx::TypedText(T_SCRIPT8).getText(); break;
    case 8: sentenceText = touchgfx::TypedText(T_SCRIPT9).getText(); break;
    case 9: sentenceText = touchgfx::TypedText(T_SCRIPT10).getText(); break;
    case 10: sentenceText = touchgfx::TypedText(T_SCRIPT11).getText(); break;
    case 11: sentenceText = touchgfx::TypedText(T_SCRIPT12).getText(); break;
    case 12: sentenceText = touchgfx::TypedText(T_SCRIPT13).getText(); break;
    case 13: sentenceText = touchgfx::TypedText(T_SCRIPT14).getText(); break;
    default: sentenceText = touchgfx::TypedText(T_SCRIPT15).getText(); break;
    }
    sentenceLen = (int16_t)touchgfx::Unicode::strlen(sentenceText);
    if (sentenceLen > TEXTSCRIPT_SIZE - 1)  /* 防止缓冲溢出 */
    {
        sentenceLen = TEXTSCRIPT_SIZE - 1;
    }
    resolvedCount = 0;
}

/* ── 按当前解码进度填充缓冲: 已解析=句子字符, 未解析=随机符号 ── */
void indexscreenView::refreshTextBuffer()
{
    textscript.setTypedText(touchgfx::TypedText(T___SINGLEUSE_SNO8)); /* 通配符文本(乱码区) */
    for (int i = 0; i < sentenceLen; i++)
    {
        if (i < resolvedCount)
        {
            textscriptBuffer[i] = sentenceText[i];
        }
        else
        {
            textscriptBuffer[i] = randomSymbol();
        }
    }
    textscriptBuffer[sentenceLen] = 0;  /* 结尾符 */
    textscript.invalidate();
}

/* ── 每帧: 驱动动画 ── */
void indexscreenView::handleTickEvent()
{
    if (state == FLASHING)
    {
        flashTicks++;
        if ((flashTicks & 1) == 0)
        {
            refreshTextBuffer();  /* 全乱码(还没开始解析) */
        }
        if (flashTicks >= 30)  /* ~1秒纯乱码闪烁 */
        {
            state = DECODING;
            resolvedCount = 0;
            flashTicks = 0;
        }
    }
    else if (state == DECODING)
    {
        flashTicks++;
        if ((flashTicks & 1) == 0)  /* 每 2 tick 多解析一个字符 */
        {
            if (resolvedCount < sentenceLen)
            {
                resolvedCount++;
            }
        }
        refreshTextBuffer();
        if (resolvedCount >= sentenceLen)  /* 全部解析完 */
        {
            state = DONE;
            /* 最终用句子文字资源完整显示 (确保渲染正确) */
            switch (sentenceIdx)
            {
            case 0: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT1)); break;
            case 1: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT2)); break;
            case 2: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT3)); break;
            case 3: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT4)); break;
            case 4: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT5)); break;
            case 5: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT6)); break;
            case 6: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT7)); break;
            case 7: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT8)); break;
            case 8: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT9)); break;
            case 9: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT10)); break;
            case 10: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT11)); break;
            case 11: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT12)); break;
            case 12: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT13)); break;
            case 13: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT14)); break;
            default: textscript.setTypedText(touchgfx::TypedText(T_SCRIPT15)); break;
            }
            textscript.invalidate();
        }
    }
}

/* ── DONE 状态点击 → 回到 IDLE, 重新显示按钮, 可重播 ── */
void indexscreenView::handleClickEvent(const touchgfx::ClickEvent& evt)
{
    if (state == DONE && evt.getType() == touchgfx::ClickEvent::RELEASED)
    {
        scriptbutton.setVisible(true);
        scriptbutton.invalidate();
        textscript.setVisible(false);
        textscript.invalidate();
        state = IDLE;
    }
    indexscreenViewBase::handleClickEvent(evt);
}

/* ── LCG 伪随机 ── */
uint32_t indexscreenView::nextRandom()
{
    m_seed = m_seed * 1103515245u + 12345u;
    return m_seed;
}

/* ── 随机全角符号 (和中文等宽) ── */
touchgfx::Unicode::UnicodeChar indexscreenView::randomSymbol()
{
    static const touchgfx::Unicode::UnicodeChar symbols[] = {
        0xFF20, /* ＠ */ 0xFF03, /* ＃ */ 0xFF04, /* ＄ */ 0xFF05, /* ％ */
        0xFF06, /* ＆ */ 0xFF0A, /* ＊ */ 0xFF0B, /* ＋ */ 0xFF1D, /* ＝ */
        0xFF1F, /* ？ */ 0xFF5E, /* ～ */ 0xFF01, /* ！ */
    };
    return symbols[(nextRandom() >> 16) % (sizeof(symbols) / sizeof(symbols[0]))];
}

void indexscreenView::handleGestureEvent(const touchgfx::GestureEvent& evt) // 执行跳转动画
{
  if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL) {
    // 速度阈值：只有快速、刻意的水平滑动才触发切屏
    // 调节 MIN_SWIPE_VELOCITY 的值：越大越不容易误触，越小越灵敏
    static const int16_t MIN_SWIPE_VELOCITY = 20;
    if (evt.getVelocity() > MIN_SWIPE_VELOCITY) // 向右快速滑 → 切屏
    {
      application().gotoscreen1ScreenSlideTransitionWest(); // 跳转到screen1_screen
    }
    // 速度不够 → 忽略
  }
}