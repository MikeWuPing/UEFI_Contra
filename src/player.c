// Contra player system - follows NES reference code behavior
// Player X is SCREEN-RELATIVE (0-255). World position = playerX + scrollX.

#include "player.h"
#include "sprites.h"
#include "level.h"
#include "bullet.h"

PLAYER_STATE gPlayers[MAX_PLAYERS];
UINT8        gPlayerMode = 0;

// Scroll point: when player reaches this screen X, scroll moves the world instead
#define SCROLL_POINT_X  128  // NES horizontal_scroll_point_tbl: $80
#define RIGHT_EDGE_X    230  // NES level_right_edge_x_pos_tbl outdoor: $e6
#define LEFT_EDGE_X      26  // NES level_left_edge_x_pos_tbl outdoor: $1a

// === ANIMATION TABLES (from NES bank2.asm) ===

static const UINT8 gWalkSprites[6] = {
  0x02, 0x03, 0x04, 0x05, 0x03, 0x06
};

static const UINT8 gJumpSprites[4] = {
  0x08, 0x09, 0x08, 0x09
};

static const UINT8 gJumpFlips[4] = {
  0x00, 0x00, 0x03, 0x03
};

static const UINT8 gDeathSprites[5] = {
  0x0a, 0x0b, 0x0a, 0x0b, 0x0c
};

static const UINT8 gDeathFlips[5] = {
  0x00, 0x00, 0x03, 0x03, 0x00
};

#define SPRITE_STAND   0x0f
#define SPRITE_CROUCH  0x17
#define SPRITE_AIM_UP  0x16

#define ANIM_SPEED_WALK     8
#define ANIM_SPEED_JUMP     5
#define ANIM_SPEED_DIE      8

VOID PlayerInit(UINT8 playerIdx)
{
  PLAYER_STATE *p = &gPlayers[playerIdx];
  ZeroMem(p, sizeof(PLAYER_STATE));

  // NES Contra: player drops from sky at level start
  p->X = (60) << 8;
  p->Y = (30) << 8;  // Drop from sky, lands on first platform below canopy
  p->FacingRight = (playerIdx == 0);
  p->Lives = 2;  // NES Contra: 2 extra lives (3 total including current)
  p->CurrentWeapon = 0;  // Start with default weapon
  p->InvincibilityTimer = 120;  // Brief invincibility on spawn (2 seconds)
  p->AnimSequence = ANIM_IDLE;
  p->AnimFrame = 0;
  p->AnimFrameTimer = 0;
  p->CollBoxLeft   = -6;
  p->CollBoxRight  =  6;
  p->CollBoxTop    = -32;
  p->CollBoxBottom =  0;
  p->GameOverStatus = 0;
}

VOID UpdatePlayer(PLAYER_STATE *p, CONTROLLER_STATE *ctrl)
{
  if (p->DeathFlag) {
    // Apply gravity during death
    p->YVelocity += GRAVITY;
    if (p->YVelocity > MAX_FALL_SPEED) p->YVelocity = MAX_FALL_SPEED;
    p->Y += p->YVelocity;

    p->AnimFrameTimer++;
    if (p->AnimFrameTimer >= ANIM_SPEED_DIE) {
      p->AnimFrameTimer = 0;
      p->AnimFrame++;
      if (p->AnimFrame >= 5) { p->AnimFrame = 4; p->DeathFlag = 2; }
    }
    return;
  }

  // Fire weapon with weapon-specific cooldown
  if (p->FireTimer > 0) p->FireTimer--;
  if ((ctrl->Current & INPUT_B) && p->FireTimer == 0) {
    PlayerFire(0, p);
    // Cooldown depends on weapon type
    static const UINT8 cooldowns[7] = { 15, 7, 10, 12, 20, 5, 0 };
    UINT8 w = p->CurrentWeapon;
    p->FireTimer = (w <= 6) ? cooldowns[w] : 15;
  }

  if (p->InvincibilityTimer > 0) p->InvincibilityTimer--;
  if (p->RecoilTimer > 0)       p->RecoilTimer--;

  // === HORIZONTAL MOVEMENT (NES calc_player_x_vel) ===
  // Crouching prevents horizontal movement
  if (!p->Crouching) {
    if (ctrl->Current & INPUT_LEFT) {
      p->XVelocity = -WALK_SPEED;
      p->FacingRight = 0;
      p->Moving = 1;
    } else if (ctrl->Current & INPUT_RIGHT) {
      p->XVelocity = WALK_SPEED;
      p->FacingRight = 1;
      p->Moving = 1;
    } else {
      if (p->XVelocity > FRICTION)      p->XVelocity -= FRICTION;
      else if (p->XVelocity < -FRICTION) p->XVelocity += FRICTION;
      else                               p->XVelocity = 0;
      p->Moving = 0;
    }
  } else {
    p->XVelocity = 0;
    p->Moving = 0;
  }

  // === VERTICAL MOVEMENT ===
  // Can't jump while crouching (NES behavior)
  if ((ctrl->Pressed & INPUT_A) && p->OnGround && !p->InWater && !p->Crouching) {
    p->YVelocity = -JUMP_VELOCITY;
    p->OnGround = 0;
    p->JumpStatus = 1;
    p->AnimFrameTimer = 0;
    p->AnimFrame = 0;
  }

  if (!p->OnGround) {
    p->YVelocity += GRAVITY;
    if (p->YVelocity > MAX_FALL_SPEED) p->YVelocity = MAX_FALL_SPEED;
    if (p->YVelocity > 0)              p->JumpStatus = 2;
  }

  // === AIM / CROUCH (hold-to-crouch for keyboard playability) ===
  // Hold DOWN to crouch (while on ground); release to stand
  if (p->OnGround && !p->Crouching) {
    if (ctrl->Pressed & INPUT_DOWN) {
      p->Crouching = 1;
    }
  }
  if (p->Crouching) {
    // Cancel crouch on: release DOWN, movement, jump, or leaving ground
    if (!(ctrl->Current & INPUT_DOWN) ||
        (ctrl->Current & (INPUT_LEFT | INPUT_RIGHT | INPUT_UP)) ||
        (ctrl->Pressed & INPUT_A) || !p->OnGround) {
      p->Crouching = 0;
    }
    p->AimDirection = p->Crouching ? 3 : 0;
  } else {
    p->AimDirection = (ctrl->Current & INPUT_UP) ? 1 : 0;
  }

  // === HORIZONTAL POSITION / SCROLL (NES set_frame_scroll_if_appropriate) ===
  if (p->XVelocity > 0) {
    INT32 screenX = p->X >> 8;
    INT32 newScreenX = screenX + (p->XVelocity >> 8);
    INT32 maxScroll = LEVEL1_SCREEN_COUNT * 256 - GAME_WIDTH;

    if (newScreenX >= SCROLL_POINT_X && (INT32)gLevelState.scrollX < maxScroll) {
      ScrollUpdate(p->XVelocity >> 8);  // Convert 8.8 fixed to pixels
      p->XVelocity = 0;
    } else if (newScreenX > RIGHT_EDGE_X) {
      p->XVelocity = 0;
    } else {
      p->X += p->XVelocity;
    }
  } else if (p->XVelocity < 0) {
    INT32 screenX = p->X >> 8;
    INT32 newScreenX = screenX + (p->XVelocity >> 8);

    if (newScreenX <= LEFT_EDGE_X && gLevelState.scrollX > 0) {
      ScrollUpdate(p->XVelocity >> 8);  // Convert 8.8 fixed to pixels
      p->XVelocity = 0;
    } else if (newScreenX < LEFT_EDGE_X && gLevelState.scrollX == 0) {
      p->XVelocity = 0;
    } else {
      p->X += p->XVelocity;
    }
  }

  // === Y POSITION ===
  if (!p->OnGround) p->Y += p->YVelocity;

  // === GROUND CHECK (collision uses world coords) ===
  {
    INT32 worldX = (p->X >> 8) + (INT32)gLevelState.scrollX;
    INT32 pixelY = p->Y >> 8;

    // Check 3 points across player width for ground at feet level
    UINT8 col = 0;
    INT32 checkX[3] = { worldX + p->CollBoxLeft, worldX, worldX + p->CollBoxRight };
    for (INT32 i = 0; i < 3; i++) {
      UINT8 c = GetBgCollision(checkX[i], pixelY);
      if (c == 1 || c == 3) { col = c; break; }
    }

    if (col == 1 || col == 3) {
      // Ground found at feet - player is standing in ground, push up
      if (!p->OnGround) {
        p->Y = (pixelY - 1) << 8;
        p->YVelocity = 0;
      }
      p->OnGround = 1;
      p->JumpStatus = 0;
    } else {
      p->OnGround = 0;
    }
  }

  if (p->Y < (16 << 8)) { p->Y = 16 << 8; p->YVelocity = 0; }

  // === ANIMATION (from NES bank2.asm set_player_sprite) ===
  if (!p->OnGround && p->JumpStatus != 0) {
    p->AnimFrameTimer++;
    if (p->AnimFrameTimer >= ANIM_SPEED_JUMP) {
      p->AnimFrameTimer = 0;
      p->AnimFrame = (p->AnimFrame + 1) % 4;
    }
    p->AnimSequence = ANIM_JUMP;
  } else if (p->Crouching) {
    p->AnimSequence = ANIM_CROUCH;
    p->AnimFrame = 0;
  } else if (p->AimDirection == 1 && !p->Moving) {
    p->AnimSequence = ANIM_AIM_UP;
    p->AnimFrame = 0;
  } else if (p->Moving) {
    p->AnimFrameTimer++;
    if (p->AnimFrameTimer >= ANIM_SPEED_WALK) {
      p->AnimFrameTimer = 0;
      p->AnimFrame = (p->AnimFrame + 1) % 6;
    }
    p->AnimSequence = ANIM_WALK;
  } else {
    p->AnimSequence = ANIM_IDLE;
    p->AnimFrame = 0;
  }

  // Select sprite code and flip
  switch (p->AnimSequence) {
    case ANIM_WALK:
      p->SpriteCode = gWalkSprites[p->AnimFrame % 6];
      p->SpriteFlip = p->FacingRight ? 0 : 0x01;
      break;
    case ANIM_JUMP:
      p->SpriteCode = gJumpSprites[p->AnimFrame % 4];
      p->SpriteFlip = gJumpFlips[p->AnimFrame % 4];
      if (!p->FacingRight) p->SpriteFlip ^= 0x01;
      break;
    case ANIM_CROUCH:
      p->SpriteCode = SPRITE_CROUCH;
      p->SpriteFlip = p->FacingRight ? 0 : 0x01;
      break;
    case ANIM_DIE:
      p->SpriteCode = gDeathSprites[p->AnimFrame % 5];
      p->SpriteFlip = gDeathFlips[p->AnimFrame % 5];
      if (!p->FacingRight) p->SpriteFlip ^= 0x01;
      break;
    case ANIM_AIM_UP:
      p->SpriteCode = SPRITE_AIM_UP;
      p->SpriteFlip = p->FacingRight ? 0 : 0x01;
      break;
    default:
      p->SpriteCode = SPRITE_STAND;
      p->SpriteFlip = p->FacingRight ? 0 : 0x01;
      break;
  }
}

VOID DrawPlayer(PLAYER_STATE *p)
{
  if (p->DeathFlag == 2) return;
  if (p->InvincibilityTimer > 0 && (p->InvincibilityTimer & 0x04)) return;

  SPRITE_FRAME *frame = &gSpriteFrames[p->SpriteCode];
  if (frame->Pixels == NULL || frame->Height == 0) return;

  // Player X is screen-relative, just draw at that position
  INT32 screenX = (p->X >> 8) - (frame->Width / 2);
  INT32 screenY = (p->Y >> 8) - frame->Height;

  if (screenX + (INT32)frame->Width < 0 || screenX >= GAME_WIDTH) return;

  DrawSprite(p->SpriteCode, screenX, screenY, p->SpriteFlip, 0xFFFFFFFF);
}
