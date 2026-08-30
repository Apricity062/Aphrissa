#ifndef DINOSUARVIEW_HPP
#define DINOSUARVIEW_HPP

#include <gui_generated/dinosuar_screen/dinosuarViewBase.hpp>
#include <gui/dinosuar_screen/dinosuarPresenter.hpp>
#include <touchgfx/Bitmap.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/events/GestureEvent.hpp>
#include <touchgfx/widgets/TiledImage.hpp>

class dinosuarView : public dinosuarViewBase
{
public:
    dinosuarView();
    virtual ~dinosuarView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    virtual void handleTickEvent();   /* 游戏主循环 */
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt);    /* 点击跳/开始/重开 */
    virtual void handleGestureEvent(const touchgfx::GestureEvent& evt);/* 右滑返回 */

protected:
    void resetToReady();
    void startGame();
    void updateScore();
    void updatePlayer(uint32_t now, uint32_t dt);
    void moveObstacles(uint32_t dt);
    void setObstacleImage(int i);
    void spawnObstacle();
    bool checkCollision();
    void gameOver();

private:
    enum GameState { READY, PLAYING, GAME_OVER };
    GameState m_state;

    touchgfx::Unicode::UnicodeChar scoreBuffer[8];
    touchgfx::Image* obstacleWidgets[3];

    uint32_t m_lastTick;      /* 上一帧 hw_ms */
    uint32_t m_lastSpawnTick; /* 上次生成障碍时刻 */
    uint32_t m_animTick;      /* 跑步动画下一帧切换时刻 */
    uint8_t  m_animFrame;     /* 跑步动画帧 0/1 */
    bool     m_touching;      /* 是否按住屏幕 */
    float    m_playerVy;      /* 主角垂直速度 (px/ms, 向下为正) */
    int32_t  m_score;
    uint32_t m_scoreMs;       /* 累计运行毫秒(得分来源) */
    uint16_t m_speedPxPerS;   /* 障碍移动速度 */
    uint32_t m_spawnInterval; /* 本次障碍生成间隔(随机) */

    touchgfx::TiledImage m_groundScroll; /* 滚动地面 */
    int16_t  m_groundOffset;             /* 地面滚动偏移 */

    struct Obstacle {
        bool    active;
        uint8_t type;      /* 0=big 1=middle 2=little */
        int16_t x, y;      /* 左上角 */
        int16_t w, h;
    } m_obs[3];
};

#endif // DINOSUARVIEW_HPP
