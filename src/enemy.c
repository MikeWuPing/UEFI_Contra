// Contra enemy system - authentic NES enemy types and spawn data from bank2.asm

#include "enemy.h"
#include "bullet.h"
#include "level.h"
#include "sprites.h"
#include "game_state.h"
#include "weapon.h"

ENEMY_SLOT gEnemies[MAX_ENEMIES];
UINT8      gBossActive    = 0;
UINT8      gBossPartsAlive = 0;

// === AUTHENTIC LEVEL 1 ENEMY SPAWN DATA (from bank2.asm) ===
// Format: 3 bytes per enemy: [X pos] [type+repeat] [Y+attr]
// X = screen pixel X position
// Type+repeat: high 2 bits = repeat count (0-3), low 6 bits = enemy type
// Y+attr: high 5 bits = Y position, low 3 bits = attribute
// Terminator: $ff

static const UINT8 gLv1SpawnData[] = {
  // screen_00: 6 enemies + $ff
  0x10, 0x05, 0x60,  // Soldier X=$10 Y=$60 attr=000 (run left, no shoot)
  0x40, 0x05, 0x60,  // Soldier X=$40 Y=$60 attr=000
  0x50, 0x06, 0xc0,  // Sniper  X=$50 Y=$c0 attr=000 (stand, 3 bullets)
  0x60, 0x02, 0xa1,  // PillBox X=$60 Y=$a0 attr=001 (M gun inside)
  0x80, 0x05, 0x60,  // Soldier X=$80 Y=$60 attr=000
  0xf0, 0x03, 0x40,  // FlyingCapsule X=$f0 Y=$40 attr=000 (R weapon)
  0xff,
  // screen_01: 1 enemy + $ff
  0x90, 0x06, 0xc0,  // Sniper X=$90 Y=$c0 attr=000
  0xff,
  // screen_02: 1 enemy + $ff
  0x20, 0x12, 0x80,  // ExplodingBridge X=$20 Y=$80
  0xff,
  // screen_03: 1 enemy + $ff
  0x40, 0x12, 0x80,  // ExplodingBridge X=$40 Y=$80
  0xff,
  // screen_04: 4 enemies + $ff
  0x00, 0x04, 0xa0,  // RotatingGun X=$00 Y=$a0 attr=000 (1 bullet per attack)
  0x10, 0x06, 0x60,  // Sniper X=$10 Y=$60 attr=000
  0x50, 0x06, 0x61,  // Sniper X=$50 Y=$60 attr=001 (crouch, 1 bullet)
  0x60, 0x03, 0x43,  // FlyingCapsule X=$60 Y=$40 attr=011 (S weapon)
  0xff,
  // screen_05: 3 enemies + $ff
  0x20, 0x06, 0x41,  // Sniper X=$20 Y=$40 attr=001 (crouch)
  0x40, 0x02, 0xa2,  // PillBox X=$40 Y=$a0 attr=010 (M gun)
  0x80, 0x04, 0x80,  // RotatingGun X=$80 Y=$80 attr=000
  0xff,
  // screen_06: 1 enemy + $ff
  0x40, 0x04, 0x80,  // RotatingGun X=$40 Y=$80 attr=000
  0xff,
  // screen_07: 2 enemies + $ff
  0x20, 0x07, 0xa0,  // RedTurret X=$20 Y=$a0 attr=000 (rock bg)
  0xa0, 0x07, 0x41,  // RedTurret X=$a0 Y=$40 attr=001 (forest bg)
  0xff,
  // screen_08: 2 enemies + $ff
  0x00, 0x02, 0xc3,  // PillBox X=$00 Y=$c0 attr=011
  0x50, 0x06, 0x80,  // Sniper X=$50 Y=$80 attr=000
  0xff,
  // screen_09: 2 base + 1 repeat + $ff (byte $43 = repeat 1, type 03)
  0x10, 0x43, 0x40,  // FlyingCapsule X=$10 Y=$40 attr=000, repeat=1
  0xb4,              //  repeat: Y=$b0 attr=100
  0xe0, 0x07, 0x81,  // RedTurret X=$e0 Y=$80 attr=001
  0xff,
  // screen_0a: 1 enemy + $ff
  0xc0, 0x04, 0xc0,  // RotatingGun X=$c0 Y=$c0 attr=000
  0xff,
  // screen_0b: BOSS SCREEN - 5 enemies + $ff
  0x40, 0x04, 0xc3,  // RotatingGun X=$40 Y=$c0 attr=011 (3 bullets)
  0xa8, 0x10, 0x81,  // BombTurret X=$a8 Y=$80 attr=001 (black bg)
  0xb1, 0x11, 0xb0,  // PlatedDoor X=$b1 Y=$b0 attr=000
  0xb4, 0x06, 0x52,  // Sniper X=$b4 Y=$50 attr=010 (boss screen sniper)
  0xc0, 0x10, 0x80,  // BombTurret X=$c0 Y=$80 attr=000 (wall bg)
  0xff,
  // screen_0c: no enemies
  0xff,
};

// Screen offsets into gLv1SpawnData
static UINT16 gScreenDataOffset[13] = { 0 };  // computed at init

// Track which screens have been activated
static UINT8 gScreensActive[13];
static UINT8 gScreenCount = 0;

// === SOLDIER GENERATION SYSTEM ===
#define SOLDIER_GEN_TIMER_INIT  0x90  // Level 1: 144 base delay

static UINT8  gSoldierGenRoutine = 0;
static UINT16 gSoldierGenTimer  = 0;
static UINT8  gSoldierGenX      = 0;
static UINT8  gSoldierGenY      = 0;
static UINT8  gScreenGenSoldiers = 0;
static UINT8  gLastGenScreen    = 0;

// X positions for generated soldiers (from gen_soldier_initial_x_pos)
static const UINT8 gGenSoldierX[16] = {
  0xFA,0x0A,0xFA,0xFA,0x0A,0xFA,0x0A,0xFA,
  0x0A,0x0A,0x0A,0xFA,0xFA,0x0A,0x0A,0xFA
};

// Initial soldier attribute table (direction + shoot behavior)
static const UINT8 gGenSoldierAttrs[28] = {
  0x00,0x00,0x00,0x00,  // 0: no shoot
  0x00,0x00,0x00,0x04,  // 4: 25% shoot
  0x00,0x00,0x04,0x04,  // 8: 50% shoot
  0x00,0x04,0x04,0x04,  // 12: 75% shoot
  0x04,0x04,0x04,0x04,  // 16: all shoot
  0x00,0x00,0x00,0x08,  // 20: no shoot, bit3
  0x00,0x00,0x04,0x08,  // 24: mixed, bit3
};

// Soldier sprites
static const UINT8 gSoldierWalkFrames[6] = { 0x3b,0x3c,0x3d,0x3f,0x3c,0x3e };
static const UINT8 gSniperSprites[4]     = { 0x40,0x29,0x2c,0x2d };

// === HELPERS ===

static ENEMY_SLOT *FreeSlot(void) {
  for (UINT32 i = 0; i < MAX_ENEMIES; i++)
    if (!gEnemies[i].Active) return &gEnemies[i];
  return NULL;
}

static INT32 GroundY(INT32 wx) {
  for (INT32 y = 80; y < GAME_HEIGHT; y++) {
    UINT8 c = GetBgCollision(wx, y);
    if (c == 1 || c == 3) return y - 1;
  }
  return GAME_HEIGHT - 17;
}

// Compute screen offsets for gLv1SpawnData at init
static VOID ComputeScreenOffsets(VOID)
{
  if (gScreenCount > 0) return;
  UINT16 off = 0;
  UINT8  screen = 0;
  while (screen < 13 && off < sizeof(gLv1SpawnData)) {
    gScreenDataOffset[screen] = off;
    while (off < sizeof(gLv1SpawnData) && gLv1SpawnData[off] != 0xff) {
      UINT8 typeByte = gLv1SpawnData[off + 1];
      UINT8 repeat  = typeByte >> 6;
      off += 3 + repeat;  // 3 bytes base + 1 per repeat
    }
    off++;  // skip $ff terminator
    screen++;
  }
  gScreenCount = screen;
}

// Parse and spawn enemies for a single screen
static VOID SpawnScreenEnemies(UINT8 screenNum)
{
  if (screenNum >= 13) return;
  if (!gScreenCount) ComputeScreenOffsets();
  if (screenNum >= gScreenCount) return;

  if (gScreensActive[screenNum]) return;
  gScreensActive[screenNum] = 1;

  UINT16 off = gScreenDataOffset[screenNum];
  if (off >= sizeof(gLv1SpawnData)) return;

  INT32 wxBase = screenNum * 256;
  UINT8 spawnCount = 0;

  while (off < sizeof(gLv1SpawnData) && gLv1SpawnData[off] != 0xff) {
    UINT8 xByte    = gLv1SpawnData[off];
    UINT8 typeByte = gLv1SpawnData[off + 1];
    UINT8 yByte    = gLv1SpawnData[off + 2];

    UINT8 etype  = typeByte & 0x3F;
    UINT8 repeat = typeByte >> 6;
    UINT8 yPos   = (yByte >> 3) & 0x1F;
    UINT8 attr   = yByte & 0x07;

    // Spawn the base enemy + repeats
    for (UINT8 r = 0; r <= repeat; r++) {
      UINT8 thisY, thisAttr;
      if (r == 0) {
        thisY = yPos;
        thisAttr = attr;
      } else {
        // Read repeat byte
        UINT8 rByte = gLv1SpawnData[off + 2 + r];
        thisY    = (rByte >> 3) & 0x1F;
        thisAttr = rByte & 0x07;
      }

      ENEMY_SLOT *e = FreeSlot();
      if (!e) { spawnCount++; continue; }

      INT32 wx = wxBase + xByte;
      INT32 wy = thisY * 8;  // Y is in 8-pixel units
      INT32 gy = (etype == 0x03 || etype == 0x11 || etype == 0x10 || etype == 0x04)
                 ? wy : GroundY(wx);

      e->Active      = 1;
      e->Type        = etype;
      e->Routine     = 0;
      e->WorldX      = wx << 8;
      e->WorldY      = (etype == 0x03 || etype == 0x10 || etype == 0x11) ? wy << 8 : gy << 8;
      e->FacingRight = 0;
      e->AnimFrame   = 0;
      e->AnimTimer   = 0;
      e->AttackTimer = (UINT16)((gFrameCounter + spawnCount * 17) % 60);
      e->DeathTimer  = 0;
      e->Var1 = 0; e->Var2 = 0; e->Var3 = 0; e->Var4 = 0;
      e->XVelocity = 0; e->YVelocity = 0;

      switch (etype) {
        case 0x05: // Soldier
          e->HP = 1;
          e->SpriteCode = 0x3b;
          e->FacingRight = (attr & 1) ? 1 : 0;  // bit0 = direction
          e->Var2 = e->FacingRight;  // direction
          break;
        case 0x06: // Sniper
          e->HP = 1;
          e->SpriteCode = 0x40;
          e->Var1 = attr;  // sniper type
          break;
        case 0x04: // Rotating Gun
          e->HP = 8;
          e->SpriteCode = 0x40;
          e->Var2 = attr;  // bullets per attack
          break;
        case 0x07: // Red Turret
          e->HP = 8;
          e->SpriteCode = 0x40;
          e->Var1 = attr;  // bg type (0=rock, 1=forest)
          e->WorldY = (wy - 16) << 8;  // Start hidden underground
          break;
        case 0x10: // Bomb Turret (boss)
          e->HP = 16;
          e->SpriteCode = 0x40;
          e->Var1 = attr;  // bg type (0=wall, 1=black)
          gBossPartsAlive++;
          break;
        case 0x11: // Plated Door (boss target)
          e->HP = 32;
          e->SpriteCode = 0x40;
          gBossPartsAlive++;
          break;
        case 0x02: // Pill Box
          e->HP = 1;
          e->SpriteCode = 0x2e;  // closed box
          e->Var1 = attr;  // weapon type inside
          break;
        case 0x03: // Flying Capsule
          e->HP = 1;
          e->SpriteCode = 0x4d;  // capsule sprite
          e->Var1 = attr;  // weapon type inside
          e->YVelocity = 0;  // Float
          break;
        case 0x12: // Exploding Bridge
          e->HP = 99;  // Indestructible until triggered
          e->SpriteCode = 0;
          break;
      }
      spawnCount++;
    }

    off += 3 + repeat;
  }
}

// === SOLDIER GENERATION ===

VOID SoldierGenerationInit(VOID)
{
  gSoldierGenRoutine = 0;
  gSoldierGenTimer  = SOLDIER_GEN_TIMER_INIT;
  gSoldierGenX      = 0;
  gSoldierGenY      = 0;
  gScreenGenSoldiers = 0;
  gLastGenScreen    = 0;
}

static INT32 FindGroundYFrom(INT32 wx, INT32 startY, INT32 step, INT32 maxSteps)
{
  INT32 y = startY;
  for (INT32 i = 0; i < maxSteps; i++) {
    if (y < 0 || y >= GAME_HEIGHT) break;
    UINT8 c = GetBgCollision(wx, y);
    if (c == 1 || c == 3) return y - 1;
    y += step;
  }
  return -1;
}

VOID RunSoldierGeneration(VOID)
{
  // Only for level 1 outdoor (soldier generation disabled on indoor/base levels)
  INT32 curScreen = (INT32)gLevelState.scrollX / 256;

  // Track screen changes
  if (curScreen != gLastGenScreen) {
    gScreenGenSoldiers = 0;
    gLastGenScreen = (UINT8)curScreen;
  }

  switch (gSoldierGenRoutine) {
    case 0: // Initialize timer
      gSoldierGenTimer = SOLDIER_GEN_TIMER_INIT;
      gSoldierGenRoutine = 1;
      break;

    case 1: // Decrement timer, find spawn position
      if (gSoldierGenTimer > 0) {
        gSoldierGenTimer -= 2;  // NES: decrement by 2 per frame
      }
      if (gSoldierGenTimer > 0) break;

      // Timer elapsed - find position
      {
        // Random X position (left or right edge)
        UINT8 idx = (gFrameCounter + (curScreen * 7)) & 0x0F;
        gSoldierGenX = gGenSoldierX[idx];

        // Special case: early level 1, soldiers only from right
        if (curScreen < 3 && gScreenGenSoldiers < 30) {
          gSoldierGenX = 0xFC;  // Always from right edge
        }

        // Find ground at this X position
        INT32 startY = (gFrameCounter & 1) ? (gPlayers[0].Y >> 8) : 0x10;
        INT32 wx = curScreen * 256 + gSoldierGenX;

        INT32 gy = -1;
        // Search from different starting points
        for (INT32 attempt = 0; attempt < 3 && gy < 0; attempt++) {
          switch ((gFrameCounter + attempt) % 3) {
            case 0: gy = FindGroundYFrom(wx, 0x10, 0x10, 15); break;
            case 1: gy = FindGroundYFrom(wx, 0xE0, -0x10, 15); break;
            case 2: gy = FindGroundYFrom(wx, startY, -0x10, 15); break;
          }
        }

        if (gy >= 0 && gy > 0x40 && gy < 0xE0) {
          gSoldierGenY = (UINT8)gy;
          gSoldierGenRoutine = 2;
        } else {
          // Reset timer and try again
          gSoldierGenTimer = SOLDIER_GEN_TIMER_INIT;
        }
      }
      break;

    case 2: // Create the soldier
      {
        ENEMY_SLOT *e = FreeSlot();
        if (e) {
          // Determine attributes based on frame counter (randomized behavior)
          UINT8 attrIdx = (gFrameCounter & 3);
          // Level 1 early screens: soldiers don't shoot yet
          UINT8 baseAttrIdx = (curScreen < 5) ? 0 : ((curScreen < 9) ? 4 : 8);
          UINT8 attr = gGenSoldierAttrs[baseAttrIdx + attrIdx];

          INT32 wx = curScreen * 256 + gSoldierGenX;
          INT32 gy = gSoldierGenY;

          e->Active      = 1;
          e->Type        = 0x05;
          e->Routine     = 0;
          e->WorldX      = wx << 8;
          e->WorldY      = gy << 8;
          e->HP          = 1;
          e->SpriteCode  = 0x3b;
          e->FacingRight = (gSoldierGenX < 128) ? 1 : 0;  // Face away from edge
          e->AnimFrame   = 0;
          e->AnimTimer   = 0;
          e->AttackTimer = (UINT16)(gFrameCounter % 60);
          e->DeathTimer  = 0;
          e->Var1 = 0;
          e->Var2 = e->FacingRight;  // Direction to run
          e->Var3 = (attr & 0x04) ? 3 : 0;  // Shoot behavior
          e->Var4 = 0;  // Turn count
          e->XVelocity = 0;
          e->YVelocity = 0;

          gScreenGenSoldiers++;
        }
        // Reset for next soldier
        gSoldierGenTimer = SOLDIER_GEN_TIMER_INIT;
        gSoldierGenRoutine = 0;
      }
      break;
  }
}

// === PUBLIC API ===

VOID SpawnLevelEnemies(UINT8 levelNum)
{
  ZeroMem(gEnemies, sizeof(gEnemies));
  ZeroMem(gScreensActive, sizeof(gScreensActive));
  gBossActive = 0;
  gBossPartsAlive = 0;
  gScreenCount = 0;
  ComputeScreenOffsets();

  // Pre-spawn screen 0 enemies (visible from start)
  if (levelNum == 0) {
    SpawnScreenEnemies(0);
  }

  SoldierGenerationInit();
}

UINT8 CountBossParts(VOID)
{
  UINT8 c = 0;
  for (UINT32 i = 0; i < MAX_ENEMIES; i++)
    if (gEnemies[i].Active && (gEnemies[i].Type == 0x10 || gEnemies[i].Type == 0x11)) c++;
  gBossPartsAlive = c;
  return c;
}

// === ENEMY AI ===

VOID UpdateEnemies(VOID)
{
  INT32 pw = (gPlayers[0].X >> 8) + (INT32)gLevelState.scrollX;
  INT32 py = gPlayers[0].Y >> 8;

  // Spawn enemies for visible screens
  INT32 curScreen = (INT32)gLevelState.scrollX / 256;
  for (INT32 s = curScreen; s <= curScreen + 1; s++) {
    if (s >= 0 && s < 13) SpawnScreenEnemies((UINT8)s);
  }

  // Boss detection: screen >= 11 (0x0b is the boss screen)
  if (curScreen >= 10 && !gBossActive) {
    // Check if any boss enemies are spawned
    for (UINT32 i = 0; i < MAX_ENEMIES; i++) {
      if (gEnemies[i].Active && (gEnemies[i].Type == 0x10 || gEnemies[i].Type == 0x11)) {
        gBossActive = 1;
        break;
      }
    }
  }

  // Run soldier generation
  RunSoldierGeneration();

  // Update each enemy
  for (UINT32 i = 0; i < MAX_ENEMIES; i++) {
    ENEMY_SLOT *e = &gEnemies[i];
    if (!e->Active) continue;

    // Death timer / flash
    if (e->DeathTimer > 0) {
      e->DeathTimer--;
      if (e->HP <= 0 && e->DeathTimer == 0) {
        e->Active = 0;
        continue;
      }
      continue;
    }
    if (e->HP <= 0) {
      e->Active = 0;
      continue;
    }

    INT32 ex = e->WorldX >> 8;
    INT32 ey = e->WorldY >> 8;
    INT32 dist = pw - ex;
    INT32 adist = (dist > 0) ? dist : -dist;

    // Common: face toward player
    if (e->Type != 0x11) {  // Plated door doesn't turn
      e->FacingRight = (dist >= 0) ? 1 : 0;
    }

    switch (e->Type) {
      // === SOLDIER (type 05) ===
      case 0x05:
        {
          INT32 dir = e->FacingRight ? 1 : -1;

          // Walk
          if (adist > 80) {
            e->WorldX += dir * 0x0040;  // Walk speed
          }

          // Animation
          if (e->AnimTimer > 0) {
            e->AnimTimer--;
          } else {
            e->AnimTimer = 8;
            e->AnimFrame = (e->AnimFrame + 1) % 6;
            e->SpriteCode = gSoldierWalkFrames[e->AnimFrame];
          }

          // Shoot at player when close enough
          if (adist < 150) {
            if (e->AttackTimer > 0) {
              e->AttackTimer--;
            } else {
              e->AttackTimer = 50 + (gFrameCounter % 40);
              e->SpriteCode = 0x40;  // Shooting sprite
              EnemyFire(ex + dir * 8, ey, e->FacingRight);
            }
          }

          // Turn at edges
          if (e->Var4 < 2) {
            UINT8 edgeCol = GetBgCollision(ex + dir * 16, ey);
            if (edgeCol == 0) {
              // Reached edge, turn around
              e->FacingRight = !e->FacingRight;
              e->Var2 = e->FacingRight;
              e->Var4++;
            }
          }

          // Standing ground
          if (e->YVelocity >= 0) {
            INT32 gnd = GroundY(ex);
            if (ey < gnd) {
              e->WorldY += 0x0080;  // Fall to ground
            } else if (ey > gnd + 2) {
              e->WorldY = gnd << 8;  // Snap to ground
            }
          }
        }
        break;

      // === SNIPER (type 06) ===
      case 0x06:
        {
          e->SpriteCode = 0x40;  // Shooting pose

          // Aim at player
          if (e->AttackTimer > 0) {
            e->AttackTimer--;
          } else {
            // Fire sequence
            UINT8 sniperType = e->Var1 & 0x07;
            if (sniperType == 0) {
              // Stand and fire 3 bullets
              e->AttackTimer = 30 + (gFrameCounter % 20);
              EnemyFireAimed(ex, ey);
              EnemyFireAimed(ex, ey + 4);
              EnemyFireAimed(ex, ey - 4);
            } else if (sniperType == 1) {
              // Crouch and fire 1 bullet
              e->AttackTimer = 40 + (gFrameCounter % 30);
              EnemyFireAimed(ex, ey);
            } else {
              // Boss screen sniper: 3 bullets
              e->AttackTimer = 25 + (gFrameCounter % 20);
              EnemyFireAimed(ex, ey);
              EnemyFireAimed(ex, ey + 6);
              EnemyFireAimed(ex, ey - 6);
            }
          }
        }
        break;

      // === ROTATING GUN (type 04) ===
      case 0x04:
        {
          e->SpriteCode = 0x40;  // Turret sprite

          if (e->AttackTimer > 0) {
            e->AttackTimer--;
          } else {
            UINT8 bullets = (e->Var2 & 3) + 1;  // 1-4 bullets per attack
            if (bullets > 3) bullets = 3;
            e->AttackTimer = 35 + (gFrameCounter % 25);
            for (UINT8 b = 0; b < bullets; b++) {
              EnemyFireAimed(ex, ey);
            }
          }
        }
        break;

      // === RED TURRET (type 07) ===
      case 0x07:
        {
          // Rise from ground when player is near
          INT32 groundY = GroundY(ex);
          INT32 targetY = groundY - 24;  // Rise 24px above ground

          if (adist < 120) {
            // Active: rise up
            if (ey > targetY + 2) {
              e->WorldY -= 0x0100;  // Rise
            }
            e->SpriteCode = 0x40;

            if (e->AttackTimer > 0) {
              e->AttackTimer--;
            } else {
              e->AttackTimer = 28 + (gFrameCounter % 20);
              EnemyFireAimed(ex, ey);
            }
          } else {
            // Retract: go back underground
            if (ey < groundY - 2) {
              e->WorldY += 0x0080;
            }
            e->SpriteCode = 0x3f;
          }
        }
        break;

      // === BOMB TURRET (type 0x10, Level 1 Boss) ===
      case 0x10:
        {
          // Track player Y for aiming
          INT32 tarty = py - 20;
          if (ey < tarty) e->WorldY += 0x0080;
          else if (ey > tarty + 15) e->WorldY -= 0x0080;

          e->SpriteCode = (gFrameCounter & 4) ? 0x40 : 0x3f;

          if (e->AttackTimer > 0) {
            e->AttackTimer--;
          } else {
            e->AttackTimer = 20;  // Aggressive fire rate
            // Fire cannonball in arc toward player
            EnemyFireAimed(ex, ey);
          }
        }
        break;

      // === PLATED DOOR (type 0x11, Level 1 Boss) ===
      case 0x11:
        {
          e->SpriteCode = 0x40;  // Static target
          // Doesn't fire, just sits there as target
          // HP=32, takes many hits to destroy
        }
        break;

      // === PILL BOX / WEAPON BOX (type 02) ===
      case 0x02:
        {
          e->SpriteCode = 0x2e;  // Closed box

          // Open when player is near and pressing B (fire)
          if (adist < 60) {
            if (e->Var2 == 0) {
              // Check if player is pressing fire
              if (gController1.Current & INPUT_B) {
                e->Var2 = 1;  // Opening
              }
            }
            if (e->Var2 >= 1) {
              e->SpriteCode = 0x2e;  // Open box
              e->HP = 0;  // Can be destroyed now
            }
          }
        }
        break;

      // === FLYING CAPSULE (type 03) ===
      case 0x03:
        {
          e->SpriteCode = 0x4d;

          // Sine wave float pattern
          INT32 phase = (gFrameCounter + (e->WorldX >> 8)) & 0x3F;
          if (phase < 16) e->WorldY -= 0x0040;
          else if (phase > 32 && phase < 48) e->WorldY += 0x0040;

          // Drift right slowly
          e->WorldX += 0x0060;

          // Wrap around or despawn
          if ((e->WorldX >> 8) > (INT32)(gLevelState.scrollX + GAME_WIDTH + 64)) {
            e->Active = 0;
          }
        }
        break;
    }

    // Keep enemies within level bounds
    if ((e->WorldX >> 8) < 0) e->WorldX = 0;
    if ((e->WorldX >> 8) > LEVEL1_SCREEN_COUNT * 256 - 16)
      e->WorldX = (LEVEL1_SCREEN_COUNT * 256 - 16) << 8;
    if ((e->WorldY >> 8) < 0) e->WorldY = 0;
    if ((e->WorldY >> 8) > GAME_HEIGHT - 10) e->WorldY = (GAME_HEIGHT - 10) << 8;
  }
}

VOID DrawEnemies(VOID)
{
  for (UINT32 i = 0; i < MAX_ENEMIES; i++) {
    ENEMY_SLOT *e = &gEnemies[i];
    if (!e->Active) continue;

    INT32 sx = (e->WorldX >> 8) - (INT32)gLevelState.scrollX;
    INT32 sy = (e->WorldY >> 8);

    if (sx < -32 || sx > GAME_WIDTH + 32) continue;

    // Death flash: skip on odd frames, draw explosion
    if (e->DeathTimer > 0) {
      if (e->DeathTimer & 1) continue; // flash effect
      // Draw explosion sprite during death sequence
      // NES uses explosion types: small ring (0x9D-0x9F) for soldiers,
      // cloudy (0x35-0x3A) for larger enemies
      UINT8 expSprite = 0x9D; // small explosion default
      if (e->DeathTimer > 4) expSprite = 0x9E;
      if (e->DeathTimer > 2) expSprite = 0x9D;
      DrawSprite(expSprite, sx - 4, sy - 8, 0, 0xFFFFFFFF);
      continue;
    }

    SPRITE_FRAME *f = &gSpriteFrames[e->SpriteCode];
    if (!f->Pixels || !f->Height) continue;

    sx -= f->Width / 2;
    sy -= f->Height;

    UINT8 flip = e->FacingRight ? 1 : 0;
    DrawSprite(e->SpriteCode, sx, sy, flip, 0xFFFFFFFF);
  }
}

VOID CheckBulletEnemyCollision(VOID)
{
  for (UINT32 b = 0; b < MAX_PLAYER_BULLETS; b++) {
    if (!gPlayerBullets[b].Active) continue;
    INT32 bx = gPlayerBullets[b].X >> 8;
    INT32 by = gPlayerBullets[b].Y >> 8;

    for (UINT32 e = 0; e < MAX_ENEMIES; e++) {
      if (!gEnemies[e].Active || gEnemies[e].DeathTimer > 0) continue;
      INT32 ex = gEnemies[e].WorldX >> 8;
      INT32 ey = gEnemies[e].WorldY >> 8;

      // Hitbox size varies by type
      INT32 hw = 10, hh = 28;
      switch (gEnemies[e].Type) {
        case 0x10: hw = 14; hh = 28; break;  // Bomb turret
        case 0x11: hw = 16; hh = 32; break;  // Plated door
        case 0x04: hw = 12; hh = 24; break;  // Rotating gun
        case 0x07: hw = 12; hh = 24; break;  // Red turret
        case 0x02: hw = 10; hh = 16; break;  // Pill box
        case 0x03: hw = 12; hh = 16; break;  // Flying capsule
      }

      if (bx >= ex - hw && bx <= ex + hw && by >= ey - hh && by <= ey + 2) {
        // Pill box: only damage when open
        if (gEnemies[e].Type == 0x02 && gEnemies[e].Var2 == 0) {
          gPlayerBullets[b].Active = 0;
          break;
        }

        gEnemies[e].HP--;
        gEnemies[e].DeathTimer = 6;
        gPlayerBullets[b].Active = 0;

        if (gEnemies[e].HP <= 0) {
          INT32 dropX = gEnemies[e].WorldX >> 8;
          INT32 dropY = gEnemies[e].WorldY >> 8;
          UINT8 etype = gEnemies[e].Type;

          // Score based on enemy type
          static const UINT32 scores[] = { 500, 100, 500, 500, 500, 500, 500, 1000, 0,0,0,0,0,0,0,0, 2000, 5000, 300 };
          if (etype < 19) gPlayers[0].Score += scores[etype];

          // Weapon drops
          if (etype == 0x03) {
            // Flying capsule: always drops weapon
            SpawnWeaponDrop(dropX, dropY, gEnemies[e].Var1);
          } else if (etype == 0x02) {
            // Pill box: drops weapon when destroyed while open
            if (gEnemies[e].Var2 >= 1)
              SpawnWeaponDrop(dropX, dropY, gEnemies[e].Var1);
          } else if (etype == 0x10 || etype == 0x11) {
            // Boss enemies: always drop weapon
            if (gEnemies[e].Var1 <= 6)
              SpawnWeaponDrop(dropX, dropY, gEnemies[e].Var1);
            else
              SpawnWeaponDrop(dropX, dropY, (gFrameCounter % 6) + 1);
          } else if ((gFrameCounter & 0x03) == 0) {
            // Other enemies: 25% chance
            SpawnWeaponDrop(dropX, dropY, (gFrameCounter % 6) + 1);
          }
        }
        break;
      }
    }
  }
}
