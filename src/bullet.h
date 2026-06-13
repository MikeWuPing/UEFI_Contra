#ifndef BULLET_H
#define BULLET_H

#include "types.h"
#include "player.h"

#define MAX_PLAYER_BULLETS 8
#define BULLET_SPEED       0x0400   // 4 px/frame in 8.8 fixed

typedef struct {
  UINT8   Active;
  UINT8   Owner;
  INT32   X, Y;
  INT32   XVelocity, YVelocity;
  UINT8   SpriteCode;
  UINT16  Timer;
  UINT8   FacingRight;
} BULLET_SLOT;

extern BULLET_SLOT gPlayerBullets[MAX_PLAYER_BULLETS];

#define MAX_ENEMY_BULLETS 12
#define ENEMY_BULLET_SPEED 0x0180  // 1.5 px/frame

extern BULLET_SLOT gEnemyBullets[MAX_ENEMY_BULLETS];

VOID UpdatePlayerBullets(VOID);
VOID DrawPlayerBullets(VOID);
VOID PlayerFire(UINT8 playerIdx, PLAYER_STATE *p);

VOID EnemyFire(INT32 worldX, INT32 worldY, UINT8 facingRight);
VOID EnemyFireAimed(INT32 worldX, INT32 worldY);  // Aim at player
VOID UpdateEnemyBullets(VOID);
VOID DrawEnemyBullets(VOID);
VOID CheckEnemyBulletPlayerCollision(VOID);

#endif
