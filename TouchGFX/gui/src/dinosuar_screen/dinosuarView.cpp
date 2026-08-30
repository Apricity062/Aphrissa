#include <gui/dinosuar_screen/dinosuarView.hpp>
#include <images/BitmapDatabase.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/hal/Types.hpp>

#ifndef SIMULATOR
extern "C" {
#include "motion_service.h"
}
#endif

/* ── 游戏常量 ── */
#define GROUND_Y 161                    /* 地面线顶部 y */
#define GROUND_PLAYER_Y (GROUND_Y - 48) /* 主角(44x48)站立时顶部 y = 113 */

#define GRAVITY 0.004f       /* 正常重力 px/ms² */
#define GRAVITY_HOLD 0.0012f /* 按住屏幕时重力减小 → 飞更久跳更高 */
#define JUMP_VEL 0.65f       /* 起跳初速度 px/ms (向上) */
#define RELEASE_CUT 0.4f     /* 松开后向上速度衰减, 提前结束跳跃 */
/* 手感参考: 快速点按≈35px小跳, 长按到底≈96px大跳(高障碍58px可过) */
#define MAX_DT_MS 50 /* 帧间隔上限, 防卡顿跳帧 */

#define SPAWN_MIN_MS 700   /* 障碍最短间隔 */
#define SPAWN_RANGE_MS 700 /* 随机范围: 700~1400ms(基础速度下) */
#define BASE_SPEED 300     /* 初始速度 px/s */

static const BitmapId OBSTACLE_BMPS[3] = {
    BITMAP_BIGOBSTACLES_ID,            /* 0: 高仙人掌 32x58 */
    BITMAP_MIDDLEOBSTACLES_ID,         /* 1: 中仙人掌 24x38 */
    BITMAP_LITTLEANDFLYINGOBSTACLES_ID /* 2: 小障碍 28x28 */
};
static const int16_t OBSTACLE_W[3] = {32, 24, 28};
static const int16_t OBSTACLE_H[3] = {58, 38, 28};

/* 简单 LCG 伪随机(避免依赖 rand) */
static uint32_t s_rng = 1;
static inline uint32_t nextRand(void) {
  s_rng = s_rng * 1664525u + 1013904223u;
  return s_rng;
}

dinosuarView::dinosuarView() {}

void dinosuarView::setupScreen() {
  dinosuarViewBase::setupScreen();

  obstacleWidgets[0] = &obstacle1;
  obstacleWidgets[1] = &obstacle2;
  obstacleWidgets[2] = &obstacle3;

  /* 滚动地面: 隐藏静态地面, 用 TiledImage 实现位移动画 */
  dinoground.setVisible(false);
  dinoground.invalidate();
  m_groundScroll.setBitmap(Bitmap(BITMAP_DINOGROUND_ID));
  m_groundScroll.setPosition(0, GROUND_Y, 240, 16);
  m_groundScroll.setOffset(0, 0);
  add(m_groundScroll);

#ifndef SIMULATOR
  s_rng = MotionService_GetTime();
  scoreBuffer[0] = 0;
  /* 分数: 白字(深色背景) + 固定显示区域 */
  scoreText.setPosition(67, 13, 120, 30);
  scoreText.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
  scoreText.setWildcard(scoreBuffer);
  MotionService_SetKeepAwake(0);
#endif
  m_state = READY;
  resetToReady();
}

void dinosuarView::tearDownScreen() {
#ifndef SIMULATOR
  MotionService_SetKeepAwake(0); /* 离开游戏恢复自动熄屏 */
#endif
  dinosuarViewBase::tearDownScreen();
}

/* ── 回到待开始状态: 主角站立, 分数归零, 障碍清空 ── */
void dinosuarView::resetToReady() {
  m_score = 0;
  m_scoreMs = 0;
  m_touching = false;
  m_playerVy = 0.0f;
  m_speedPxPerS = BASE_SPEED;

  playerImg.setY(GROUND_PLAYER_Y);
  playerImg.setBitmap(Bitmap(BITMAP_MIUNORUN1_ID));
  playerImg.invalidate();

  for (int i = 0; i < 3; i++) {
    m_obs[i].active = false;
    obstacleWidgets[i]->setVisible(false);
    obstacleWidgets[i]->invalidate();
  }
  m_groundOffset = 0;
  m_groundScroll.setOffset(0, 0);
  updateScore();
}

/* ── 开始/重新开始游戏 ── */
void dinosuarView::startGame() {
  resetToReady();
  m_state = PLAYING;
#ifndef SIMULATOR
  uint32_t now = MotionService_GetTime();
  m_lastTick = now;
  m_lastSpawnTick = now + 600; /* 给玩家一点反应时间再出第一个障碍 */
  m_spawnInterval = (SPAWN_MIN_MS + (nextRand() % SPAWN_RANGE_MS)) *
                    BASE_SPEED / m_speedPxPerS; /* 首个随机间距 */
  m_animTick = now;
  MotionService_SetKeepAwake(1); /* 游戏运行中保持亮屏 */
#endif
}

void dinosuarView::handleTickEvent() {
  dinosuarViewBase::handleTickEvent();
  if (m_state != PLAYING) {
    return;
  }

#ifndef SIMULATOR
  uint32_t now = MotionService_GetTime();
  uint32_t dt = now - m_lastTick;
  if (dt > MAX_DT_MS)
    dt = MAX_DT_MS;
  m_lastTick = now;

  /* 计分: 累计运行毫秒, 每 100ms 得 1 分 */
  m_scoreMs += dt;
  m_score = (int32_t)(m_scoreMs / 100);
  updateScore();

  /* 速度随分数提升 */
  m_speedPxPerS = (uint16_t)(BASE_SPEED + m_score / 5);

  updatePlayer(now, dt);
  moveObstacles(dt);

  /* 地面滚动: 与障碍同向同速 */
  int16_t step = (int16_t)(m_speedPxPerS * dt / 1000);
  m_groundOffset += step;
  m_groundScroll.setOffset(m_groundOffset, 0);

  /* 障碍随机间距生成(随速度缩短, 保持像素间距恒定) */
  if (now - m_lastSpawnTick >= m_spawnInterval) {
    m_lastSpawnTick = now;
    m_spawnInterval = (SPAWN_MIN_MS + (nextRand() % SPAWN_RANGE_MS)) *
                      BASE_SPEED / m_speedPxPerS;
    spawnObstacle();
  }

  /* 游戏区域重绘(顶部..地面): 消除移动拖影, 比整屏更轻 */
  touchgfx::Rect gameArea(0, 0, 240, GROUND_Y + 16);
  invalidateRect(gameArea);

  if (checkCollision()) {
    gameOver();
    invalidateRect(gameArea); /* 死亡帧也刷新 */
    return;
  }
#endif
}

/* ── 点击: READY/GAME_OVER → 开始; PLAYING → 起跳; 按住跳更高, 松开截断 ── */
void dinosuarView::handleClickEvent(const touchgfx::ClickEvent &evt) {
  dinosuarViewBase::handleClickEvent(evt); /* 子控件不消费, 事件仍会走到这里 */

#ifndef SIMULATOR
  if (evt.getType() == touchgfx::ClickEvent::PRESSED) {
    if (m_state == READY || m_state == GAME_OVER) {
      startGame();
    } else { /* PLAYING */
      if (playerImg.getY() >= GROUND_PLAYER_Y) {
        m_playerVy = -JUMP_VEL; /* 起跳(向上为负) */
      }
      m_touching = true;
    }
  } else if (evt.getType() == touchgfx::ClickEvent::RELEASED) {
    if (m_state == PLAYING) {
      m_touching = false;
      if (m_playerVy < 0.0f) {
        m_playerVy *= RELEASE_CUT; /* 松开 → 提前结束上升 */
      }
    }
  }
#endif
}

/* ── 右滑返回菜单 ── */
void dinosuarView::handleGestureEvent(const touchgfx::GestureEvent &evt) {
  if (evt.getType() == touchgfx::GestureEvent::SWIPE_HORIZONTAL) {
    static const int16_t MIN_SWIPE_VELOCITY = 20;
    if (evt.getVelocity() > MIN_SWIPE_VELOCITY) {
      application().gotoscreen1ScreenSlideTransitionWest();
      return;
    }
  }
  //dinosuarViewBase::handleGestureEvent(evt);
}

/* ── 主角物理: 重力 + 跳跃 + 按住跳更高 + 跑步动画 ── */
void dinosuarView::updatePlayer(uint32_t now, uint32_t dt) {
  float g = (m_touching && m_playerVy < 0.0f) ? GRAVITY_HOLD : GRAVITY;
  m_playerVy += g * (float)dt;

  int32_t y = playerImg.getY() + (int32_t)(m_playerVy * (float)dt);
  if (y < 0) { /* 防越出屏幕顶部 */
    y = 0;
    m_playerVy = 0.0f;
  }
  if (y >= GROUND_PLAYER_Y) {
    y = GROUND_PLAYER_Y;
    m_playerVy = 0.0f;
  }
  playerImg.setY((int16_t)y);
  playerImg.invalidate();

  /* 图片: 空中 = jump; 地面 = run1/run2 交替 */
  if (y >= GROUND_PLAYER_Y) {
    if ((int32_t)(now - m_animTick) >= 0) {
      m_animTick = now + 120; /* 每 120ms 换一帧 */
      m_animFrame ^= 1;
    }
    playerImg.setBitmap(m_animFrame ? Bitmap(BITMAP_MIUNORUN2_ID)
                                    : Bitmap(BITMAP_MIUNORUN1_ID));
  } else {
    playerImg.setBitmap(Bitmap(BITMAP_MIUNOJUMP_ID));
  }
}

/* ── 障碍左移, 移出屏外回收 ── */
void dinosuarView::moveObstacles(uint32_t dt) {
  int16_t step = (int16_t)(m_speedPxPerS * dt / 1000);
  for (int i = 0; i < 3; i++) {
    if (!m_obs[i].active)
      continue;
    m_obs[i].x -= step;
    if (m_obs[i].x + m_obs[i].w < 0) {
      m_obs[i].active = false; /* 出屏回收 */
    }
    setObstacleImage(i);
  }
}

void dinosuarView::setObstacleImage(int i) {
  touchgfx::Image *w = obstacleWidgets[i];
  if (m_obs[i].active) {
    w->setBitmap(Bitmap(OBSTACLE_BMPS[m_obs[i].type]));
    w->setXY(m_obs[i].x, m_obs[i].y);
    w->setVisible(true);
  } else {
    w->setVisible(false);
  }
  w->invalidate();
}

/* ── 从右边生成一个新障碍 ── */
void dinosuarView::spawnObstacle() {
  int slot = -1;
  for (int i = 0; i < 3; i++) {
    if (!m_obs[i].active) {
      slot = i;
      break;
    }
  }
  if (slot < 0)
    return; /* 三个都活跃, 跳过 */

  uint8_t type = (uint8_t)(nextRand() % 3);
  m_obs[slot].active = true;
  m_obs[slot].type = type;
  m_obs[slot].w = OBSTACLE_W[type];
  m_obs[slot].h = OBSTACLE_H[type];
  m_obs[slot].x = 240;
  m_obs[slot].y = GROUND_Y - m_obs[slot].h; /* 贴地 */
}

/* ── 碰撞判定(碰撞盒略缩, 更公平) ── */
bool dinosuarView::checkCollision() {
  int px = playerImg.getX();
  int py = playerImg.getY();
  for (int i = 0; i < 3; i++) {
    if (!m_obs[i].active)
      continue;
    int ox = m_obs[i].x + 3;
    int oy = m_obs[i].y + 3;
    int ow = m_obs[i].w - 6;
    int oh = m_obs[i].h - 6;
    if (px + 38 > ox && px + 6 < ox + ow && py + 44 > oy && py + 6 < oy + oh) {
      return true;
    }
  }
  return false;
}

void dinosuarView::gameOver() {
  m_state = GAME_OVER;
  playerImg.setBitmap(Bitmap(BITMAP_MIUNODEAD_ID));
  playerImg.invalidate();
#ifndef SIMULATOR
  MotionService_SetKeepAwake(0);
#endif
  /* 分数保留在 scoreText; 再点一下重开 */
}

void dinosuarView::updateScore() {
#ifndef SIMULATOR
  Unicode::snprintf(scoreBuffer, 8, "%05d", (int)m_score);
  scoreText.setWildcard(scoreBuffer);
  scoreText.invalidate();
#endif
}
