#ifndef SPRITES_H
#define SPRITES_H

#include "types.h"

// Maximum number of sprite codes (matching NES sprite table size)
#define MAX_SPRITES 256

// Sprite frame descriptor
typedef struct {
  UINT32 *Pixels;   // Pointer to pixel data (BGR, transparent=0x00000000)
  UINT16  Width;
  UINT16  Height;
} SPRITE_FRAME;

// Map sprite code (0-255) to sprite frame data
extern SPRITE_FRAME gSpriteFrames[MAX_SPRITES];

// Initialize sprite system: open filesystem, load all .spr files
EFI_STATUS SpriteSystemInit(VOID);

// Cleanup: free all sprite pixel data
VOID SpriteSystemShutdown(VOID);

// Draw a sprite to the game buffer at (x,y) with optional flip and tint
// x,y = top-left position in game-space (256x240 coordinates)
// flipFlags: bit 0 = horizontal flip, bit 1 = vertical flip
// Clipping is done to the game buffer boundaries (0..GAME_WIDTH, 0..GAME_HEIGHT)
VOID DrawSprite(
  UINT32 spriteCode,
  INT32  x,
  INT32  y,
  UINT8  flipFlags,
  UINT32 colorMask   // 0xFFFFFFFF = no tint; use for palette/invincibility effects
);

// Convenience: draw sprite without flip or tint
static inline VOID DrawSpriteSimple(UINT32 spriteCode, INT32 x, INT32 y) {
  DrawSprite(spriteCode, x, y, 0, 0xFFFFFFFF);
}

#endif
