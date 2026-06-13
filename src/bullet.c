// Contra weapon/bullet system with multiple weapon types

#include "bullet.h"
#include "level.h"
#include "sprites.h"

BULLET_SLOT gPlayerBullets[MAX_PLAYER_BULLETS];
BULLET_SLOT gEnemyBullets[MAX_ENEMY_BULLETS];

// Weapon fire rates (frames between shots) and max bullet counts
static const UINT8 gWeaponCooldown[7] = { 15, 8, 10, 12, 20, 6, 0 };
static const UINT8 gWeaponMaxBullets[7] = { 2, 4, 2, 6, 2, 2, 0 };

// Count active bullets for a player
static UINT8 CountMyBullets(UINT8 owner) {
  UINT8 count = 0;
  for (UINT32 i = 0; i < MAX_PLAYER_BULLETS; i++)
    if (gPlayerBullets[i].Active && gPlayerBullets[i].Owner == owner) count++;
  return count;
}

// Find a free bullet slot
static BULLET_SLOT *FindFreeSlot(void) {
  for (UINT32 i = 0; i < MAX_PLAYER_BULLETS; i++)
    if (!gPlayerBullets[i].Active) return &gPlayerBullets[i];
  return NULL;
}

// Spawn a single bullet
static VOID SpawnBullet(UINT8 owner, INT32 worldX, INT32 worldY,
                         INT32 vx, INT32 vy, UINT8 spriteCode, UINT16 lifetime)
{
  BULLET_SLOT *s = FindFreeSlot();
  if (s == NULL) return;
  s->Active = 1;
  s->Owner = owner;
  s->X = worldX << 8;
  s->Y = worldY << 8;
  s->XVelocity = vx;
  s->YVelocity = vy;
  s->SpriteCode = spriteCode;
  s->Timer = lifetime;
  s->FacingRight = (vx >= 0) ? 1 : 0;
}

VOID PlayerFire(UINT8 playerIdx, PLAYER_STATE *p)
{
  UINT8 weapon = p->CurrentWeapon;
  if (weapon > 6) weapon = 0;
  if (CountMyBullets(playerIdx) >= gWeaponMaxBullets[weapon]) return;

  INT32 worldX = (p->X >> 8) + (INT32)gLevelState.scrollX;
  INT32 gunOffset = p->Crouching ? 8 : 24;   // Crouch: 8px above feet
  INT32 worldY = (p->Y >> 8) - gunOffset;
  INT32 dir = p->FacingRight ? 1 : -1;
  UINT16 life = 60;

  // Bullet sprites from NES bank6.asm weapon_bullet_sprite_code_tbl:
  //   Default: 0x1e, M: 0x1f, F: 0x22, S: 0x1f (grows to 0x20,0x21), L: 0x24
  switch (weapon) {
    case 0: // Default rifle
      SpawnBullet(playerIdx, worldX, worldY, dir * BULLET_SPEED, 0, 0x1e, life);
      break;

    case 1: // Machine Gun (M) - 3-way, red bubble sprite 0x1f
      SpawnBullet(playerIdx, worldX, worldY, dir * BULLET_SPEED, 0, 0x1f, life);
      SpawnBullet(playerIdx, worldX, worldY, dir * BULLET_SPEED, -0x0080, 0x1f, life);
      SpawnBullet(playerIdx, worldX, worldY, dir * BULLET_SPEED,  0x0080, 0x1f, life);
      break;

    case 2: // Fire (F) - arcing fireballs, sprite 0x22
      SpawnBullet(playerIdx, worldX, worldY, dir * 0x0200, 0, 0x22, 50);
      SpawnBullet(playerIdx, worldX, worldY, dir * 0x0200, -0x0100, 0x22, 40);
      break;

    case 3: // Spread (S) - 5-way fan, sprite 0x1f
      SpawnBullet(playerIdx, worldX, worldY, dir * 0x0300,  0x0000, 0x1f, life);
      SpawnBullet(playerIdx, worldX, worldY, dir * 0x0300, -0x0040, 0x1f, life);
      SpawnBullet(playerIdx, worldX, worldY, dir * 0x0300,  0x0040, 0x1f, life);
      SpawnBullet(playerIdx, worldX, worldY, dir * 0x0300, -0x0100, 0x1f, life);
      SpawnBullet(playerIdx, worldX, worldY, dir * 0x0300,  0x0100, 0x1f, life);
      break;

    case 4: // Laser (L) - fast penetrating, sprite 0x24 (horizontal beam)
      SpawnBullet(playerIdx, worldX, worldY, dir * 0x0600, 0, 0x24, 25);
      break;

    case 5: // Rapid (R) - very fast single, sprite 0x1e
      SpawnBullet(playerIdx, worldX, worldY, dir * 0x0600, 0, 0x1e, 20);
      break;

    case 6: // Barrier (B) - invincibility, no bullet
      if (p->InvincibilityTimer < 240) p->InvincibilityTimer = 240;
      break;
  }
}

VOID UpdatePlayerBullets(VOID)
{
  for (UINT32 i = 0; i < MAX_PLAYER_BULLETS; i++) {
    BULLET_SLOT *b = &gPlayerBullets[i];
    if (!b->Active) continue;

    b->X += b->XVelocity;
    b->Y += b->YVelocity;

    // F weapon: add Y oscillation (gravity effect)
    if (b->SpriteCode == 0x24) {
      b->YVelocity += 0x0010;  // Slight gravity
    }

    if (b->Timer > 0) b->Timer--; else { b->Active = 0; continue; }

    INT32 worldX = b->X >> 8;
    INT32 worldY = b->Y >> 8;
    if (worldX < 0 || worldX >= LEVEL1_SCREEN_COUNT * 256 ||
        worldY < 0 || worldY >= GAME_HEIGHT) {
      b->Active = 0; continue;
    }
    // Laser penetrates walls; others stop on solid
    UINT8 col = GetBgCollision(worldX, worldY);
    if (col == 3 && b->SpriteCode != 0x24) {
      b->Active = 0;
    }
  }
}

VOID DrawPlayerBullets(VOID)
{
  for (UINT32 i = 0; i < MAX_PLAYER_BULLETS; i++) {
    BULLET_SLOT *b = &gPlayerBullets[i];
    if (!b->Active) continue;

    INT32 screenX = (b->X >> 8) - (INT32)gLevelState.scrollX;
    INT32 screenY = (b->Y >> 8);
    UINT8 flip = b->FacingRight ? 0 : 1;
    DrawSprite(b->SpriteCode, screenX, screenY, flip, 0xFFFFFFFF);
  }
}

// === ENEMY BULLETS ===

VOID EnemyFire(INT32 worldX, INT32 worldY, UINT8 facingRight)
{
  BULLET_SLOT *slot = NULL;
  for (UINT32 i = 0; i < MAX_ENEMY_BULLETS; i++) {
    if (!gEnemyBullets[i].Active) { slot = &gEnemyBullets[i]; break; }
  }
  if (slot == NULL) return;

  slot->Active      = 1;
  slot->FacingRight = facingRight;
  slot->Timer       = 90;
  slot->X           = worldX << 8;
  slot->Y           = (worldY - 20) << 8;
  slot->XVelocity   = facingRight ? ENEMY_BULLET_SPEED : -ENEMY_BULLET_SPEED;
  slot->YVelocity   = 0;
  slot->SpriteCode  = 0x1e;
}

// Enemy fire aimed at player
VOID EnemyFireAimed(INT32 worldX, INT32 worldY)
{
  INT32 playerWorldX = (gPlayers[0].X >> 8) + (INT32)gLevelState.scrollX;
  INT32 playerWorldY = gPlayers[0].Y >> 8;
  INT32 dx = playerWorldX - worldX;
  INT32 dy = (playerWorldY - 20) - worldY;

  // Normalize direction and scale to bullet speed
  INT32 adx = dx > 0 ? dx : -dx;
  INT32 ady = dy > 0 ? dy : -dy;
  INT32 mag = adx + ady;
  if (mag < 1) mag = 1;

  BULLET_SLOT *slot = NULL;
  for (UINT32 i = 0; i < MAX_ENEMY_BULLETS; i++) {
    if (!gEnemyBullets[i].Active) { slot = &gEnemyBullets[i]; break; }
  }
  if (slot == NULL) return;

  slot->Active      = 1;
  slot->FacingRight = (dx > 0) ? 1 : 0;
  slot->Timer       = 90;
  slot->X           = worldX << 8;
  slot->Y           = (worldY - 20) << 8;
  slot->XVelocity   = (dx * (INT32)ENEMY_BULLET_SPEED) / mag;
  slot->YVelocity   = (dy * (INT32)ENEMY_BULLET_SPEED) / mag;
  slot->SpriteCode  = 0x1e;
}

VOID UpdateEnemyBullets(VOID)
{
  for (UINT32 i = 0; i < MAX_ENEMY_BULLETS; i++) {
    BULLET_SLOT *b = &gEnemyBullets[i];
    if (!b->Active) continue;
    b->X += b->XVelocity;
    if (b->Timer > 0) b->Timer--; else { b->Active = 0; continue; }
    INT32 worldX = b->X >> 8;
    if (worldX < 0 || worldX >= LEVEL1_SCREEN_COUNT * 256) { b->Active = 0; continue; }
    if (GetBgCollision(worldX, b->Y >> 8) == 3) { b->Active = 0; }
  }
}

VOID DrawEnemyBullets(VOID) {
  for (UINT32 i = 0; i < MAX_ENEMY_BULLETS; i++) {
    BULLET_SLOT *b = &gEnemyBullets[i];
    if (!b->Active) continue;
    INT32 sx = (b->X >> 8) - (INT32)gLevelState.scrollX;
    INT32 sy = b->Y >> 8;
    DrawSprite(b->SpriteCode, sx, sy, b->FacingRight ? 0 : 1, 0xFFFFFFFF);
  }
}

VOID CheckEnemyBulletPlayerCollision(VOID) {
  for (UINT32 b = 0; b < MAX_ENEMY_BULLETS; b++) {
    if (!gEnemyBullets[b].Active) continue;
    INT32 bx = gEnemyBullets[b].X >> 8;
    INT32 by = gEnemyBullets[b].Y >> 8;
    for (UINT32 p = 0; p < MAX_PLAYERS; p++) {
      PLAYER_STATE *pl = &gPlayers[p];
      if (pl->DeathFlag || pl->InvincibilityTimer > 0) continue;
      INT32 px = (pl->X >> 8) + (INT32)gLevelState.scrollX;
      INT32 py = pl->Y >> 8;
      INT32 hitTop = pl->Crouching ? (py - 12) : (py - 28);
      if (bx >= px - 8 && bx <= px + 8 && by >= hitTop && by <= py) {
        gEnemyBullets[b].Active = 0;
        if (pl->Lives > 0) {
          pl->Lives--;
          pl->DeathFlag = 1;
          pl->AnimFrame = 0;
          pl->AnimFrameTimer = 0;
          pl->AnimSequence = ANIM_DIE;
          pl->YVelocity = -0x0100;
          pl->OnGround = 0;
        }
        break;
      }
    }
  }
}
