#ifndef PLAYER_H
#define PLAYER_H

#include "types.h"
#include "input.h"

#define MAX_PLAYERS 2

// Animation sequences (matching Contra NES player sprite sequences)
#define ANIM_IDLE      0
#define ANIM_WALK      1
#define ANIM_JUMP      2
#define ANIM_CROUCH    3
#define ANIM_DIE       4
#define ANIM_AIM_UP    5
#define ANIM_AIM_DOWN  6
#define ANIM_COUNT     7

// Physics constants (8.8 fixed point)
#define GRAVITY          0x001C    // 28 in 8.8
#define JUMP_VELOCITY    0x0380    // 896 in 8.8 = 3.5 px/frame
#define WALK_SPEED       0x0100    // 256 in 8.8 = 1.0 px/frame
#define MAX_FALL_SPEED   0x0400    // 1024 in 8.8 = 4.0 px/frame
#define FRICTION         0x0080    // Deceleration when releasing direction

// Player state structure
typedef struct {
  // Position (8.8 fixed point: upper 24 bits = integer, lower 8 bits = fraction)
  INT32  X;
  INT32  Y;
  INT32  XVelocity;        // 8.8 fixed point
  INT32  YVelocity;        // 8.8 fixed point
  INT32  XFracAccum;       // Fractional accumulator for X
  INT32  YFracAccum;       // Fractional accumulator for Y

  // Movement state
  UINT8  JumpStatus;       // 0=grounded, 1=jumping up, 2=falling
  UINT8  AimDirection;     // 0=forward, 1=up, 2=diag-up, 3=down
  UINT8  FacingRight;      // Boolean
  UINT8  Moving;           // Boolean
  UINT8  Crouching;        // Boolean
  UINT8  InWater;          // Boolean
  UINT8  OnLadder;         // Boolean

  // Animation state
  UINT8  AnimSequence;     // Current animation sequence
  UINT8  AnimFrame;        // Current frame within sequence
  UINT16 AnimFrameTimer;   // Ticks until next frame

  // Combat state
  UINT8  CurrentWeapon;    // 0=default, 1=M, 2=F, 3=S, 4=L, 5=B
  UINT16 FireTimer;        // Cooldown between shots
  UINT8  InvincibilityTimer;    // After being hit
  UINT8  RecoilTimer;           // Knockback timer
  UINT8  DeathFlag;             // 0=alive, 1=dying, 2=dead
  UINT8  RespawnTimer;

  // Lives and score
  UINT8  Lives;
  UINT32 Score;
  UINT8  GameOverStatus;

  // Collision box (relative to X/Y position, in pixels)
  INT32  CollBoxLeft;
  INT32  CollBoxRight;
  INT32  CollBoxTop;
  INT32  CollBoxBottom;

  // Sprite code for current frame (computed during update)
  UINT8  SpriteCode;
  UINT8  SpriteFlip;

  // Ground check
  UINT8  OnGround;
} PLAYER_STATE;

extern PLAYER_STATE gPlayers[MAX_PLAYERS];
extern UINT8        gPlayerMode;  // 0=single, 1=two-player

// Initialize player state for a new game
VOID PlayerInit(UINT8 playerIdx);

// Update player physics, input, and animation for one frame
VOID UpdatePlayer(PLAYER_STATE *p, CONTROLLER_STATE *ctrl);

// Draw the player sprite to the game buffer
VOID DrawPlayer(PLAYER_STATE *p);

#endif
