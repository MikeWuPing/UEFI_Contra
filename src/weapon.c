#include "weapon.h"
#include "player.h"
#include "level.h"
#include "sprites.h"
#include "game_state.h"

#define MAX_WEAPON_ITEMS 6

typedef struct {
  INT32  wx, wy;
  UINT8  wtype;
  UINT8  active;
  UINT8  spr;
  INT32  vy;
} WEAPON_ITEM;

static WEAPON_ITEM gWeaponItems[MAX_WEAPON_ITEMS];

// Weapon sprite codes: R,M,F,S,L,B
static const UINT8 gWeaponSprites[7] = { 0x33, 0x34, 0x31, 0x2f, 0x32, 0x30, 0x33 };

VOID UpdateWeaponItems(VOID)
{
  if (gDemoMode) {
    for (UINT32 i = 0; i < MAX_WEAPON_ITEMS; i++) gWeaponItems[i].active = 0;
    return;
  }

  INT32 playerWorldX = (gPlayers[0].X >> 8) + (INT32)gLevelState.scrollX;
  INT32 playerWorldY = gPlayers[0].Y >> 8;

  for (UINT32 i = 0; i < MAX_WEAPON_ITEMS; i++) {
    if (!gWeaponItems[i].active) continue;

    // Flying items bob up and down
    if (gWeaponItems[i].vy == 0) {
      INT32 bobIdx = (gFrameCounter + i * 21) & 0x3F;
      if (bobIdx < 16) gWeaponItems[i].wy -= 1;
      else if (bobIdx > 32 && bobIdx < 48) gWeaponItems[i].wy += 1;
      // Despawn if off screen
      INT32 sx = gWeaponItems[i].wx - (INT32)gLevelState.scrollX;
      if (sx < -32 || sx > GAME_WIDTH + 32) {
        gWeaponItems[i].active = 0;
      }
    } else {
      // Falling items
      gWeaponItems[i].wy += gWeaponItems[i].vy;
      if (gWeaponItems[i].wy >= GAME_HEIGHT - 17)
        gWeaponItems[i].wy = GAME_HEIGHT - 17;
    }

    // Pickup check
    if (playerWorldX >= gWeaponItems[i].wx - 20 &&
        playerWorldX <= gWeaponItems[i].wx + 20 &&
        playerWorldY >= gWeaponItems[i].wy - 32 &&
        playerWorldY <= gWeaponItems[i].wy + 8) {

      UINT8 wt = gWeaponItems[i].wtype;
      if (wt == 6) {
        // Rapid fire: add rapid modifier to current weapon
        gPlayers[0].CurrentWeapon |= 0x10;
      } else if (wt == 5) {
        // Barrier: invincibility for ~20 seconds
        gPlayers[0].InvincibilityTimer = 200;
      } else {
        // Standard weapon
        gPlayers[0].CurrentWeapon = wt;
      }
      gPlayers[0].FireTimer = 0;
      gWeaponItems[i].active = 0;
    }
  }
}

VOID DrawWeaponItems(VOID)
{
  for (UINT32 i = 0; i < MAX_WEAPON_ITEMS; i++) {
    if (!gWeaponItems[i].active) continue;
    INT32 sx = gWeaponItems[i].wx - (INT32)gLevelState.scrollX;
    UINT8 spr = gWeaponItems[i].spr;
    SPRITE_FRAME *frame = &gSpriteFrames[spr];
    INT32 sy = gWeaponItems[i].wy;
    if (frame->Pixels && frame->Height > 0) {
      DrawSprite(spr, sx - frame->Width/2, sy - frame->Height, 0, 0xFFFFFFFF);
    } else {
      DrawSprite(spr, sx - 8, sy - 16, 0, 0xFFFFFFFF);
    }
  }
}

VOID SpawnWeaponDrop(INT32 worldX, INT32 worldY, UINT8 weaponType)
{
  for (UINT32 i = 0; i < MAX_WEAPON_ITEMS; i++) {
    if (!gWeaponItems[i].active) {
      gWeaponItems[i].wx = worldX;
      gWeaponItems[i].wy = worldY;
      gWeaponItems[i].wtype = weaponType;
      gWeaponItems[i].spr   = (weaponType <= 6) ? gWeaponSprites[weaponType] : 0x33;
      gWeaponItems[i].active = 1;
      gWeaponItems[i].vy     = 1;  // Fall down
      return;
    }
  }
}

VOID ClearWeaponItems(VOID)
{
  ZeroMem(gWeaponItems, sizeof(gWeaponItems));
}
