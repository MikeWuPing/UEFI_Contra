#ifndef ENEMY_H
#define ENEMY_H

#include "types.h"
#include "player.h"

#define MAX_ENEMIES 12

// NES Contra enemy types (matching original game)
#define ENEMY_WEAPON_ITEM      0x00
#define ENEMY_BULLET_ENEMY     0x01
#define ENEMY_PILL_BOX         0x02
#define ENEMY_FLYING_CAPSULE   0x03
#define ENEMY_ROTATING_GUN     0x04
#define ENEMY_SOLDIER          0x05
#define ENEMY_SNIPER           0x06
#define ENEMY_RED_TURRET       0x07
#define ENEMY_BOMB_TURRET      0x10  // Level 1 boss cannon
#define ENEMY_PLATED_DOOR      0x11  // Level 1 boss target
#define ENEMY_EXPLODING_BRIDGE 0x12  // Level 1 bridges

typedef struct {
  UINT8   Active;
  UINT8   Type;         // NES enemy type
  UINT8   Routine;      // AI routine index
  INT32   WorldX;       // World position 8.8 fixed
  INT32   WorldY;
  INT32   HP;
  INT32   XVelocity;    // 8.8 fixed
  INT32   YVelocity;
  UINT8   SpriteCode;
  UINT8   FacingRight;
  UINT8   AnimFrame;
  UINT16  AnimTimer;
  UINT16  AttackTimer;
  UINT8   DeathTimer;
  // NES enemy variables
  UINT8   Var1, Var2, Var3, Var4;
  UINT16  VarA;         // 16-bit for some enemy types
} ENEMY_SLOT;

extern ENEMY_SLOT gEnemies[MAX_ENEMIES];
extern UINT8 gBossActive;
extern UINT8 gBossPartsAlive;

VOID SpawnLevelEnemies(UINT8 levelNum);
VOID UpdateEnemies(VOID);
VOID DrawEnemies(VOID);
VOID CheckBulletEnemyCollision(VOID);
UINT8 CountBossParts(VOID);

// Soldier generation
VOID SoldierGenerationInit(VOID);
VOID RunSoldierGeneration(VOID);

#endif
