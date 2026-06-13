#ifndef LEVEL_H
#define LEVEL_H

#include "types.h"
#include "level_data.h"

// Level state
typedef struct {
  UINT8   currentLevel;      // 0-7
  UINT8   screenNumber;      // Current screen index
  UINT16  scrollX;           // Horizontal scroll offset (pixels)
  UINT16  scrollY;           // Vertical scroll offset (pixels)

  // From current level header
  UINT8   locationType;      // 0=outdoor, 1=indoor, 0xFF=indoor boss
  UINT8   scrollType;        // 0=horizontal, 1=vertical
  UINT8   collisionIdx1;     // Tiles below this = floor
  UINT8   collisionIdx0;     // Tiles >= this and < collisionIdx2 = empty
  UINT8   collisionIdx2;     // Tiles >= this = solid (water if < 0xFF)

  // Palette references from header
  UINT8   bgPaletteIdx[4];
  UINT8   spritePaletteIdx[4];
} LEVEL_STATE;

extern LEVEL_STATE gLevelState;

// Initialize level 1 (Jungle)
VOID LevelInit(UINT8 levelNum);

// Add delta to scroll (like NES FRAME_SCROLL). Positive = scroll right.
VOID ScrollUpdate(INT32 delta);

// Convert world X to screen X
INT32 WorldToScreenX(INT32 worldX);

// Draw the visible portion of the level background
VOID DrawBackground(VOID);

// Get collision code at a world-space coordinate
// Returns: 0=empty, 1=floor, 2=water, 3=solid
UINT8 GetBgCollision(INT32 worldX, INT32 worldY);

// Get the tile index at a world-space coordinate
UINT8 GetTileIndex(INT32 worldX, INT32 worldY);

// Apply NES palette cycling (water shimmer + red flash every 8 frames)
VOID UpdatePaletteCycle(VOID);

#endif
