#ifndef NES_PALETTE_H
#define NES_PALETTE_H

#include "types.h"

// NES has 64 colors in its master palette
#define NES_PALETTE_SIZE 64

// 4 background palettes + 4 sprite palettes, 4 colors each
#define PALETTE_COLORS      4
#define BG_PALETTE_COUNT    4
#define SPRITE_PALETTE_COUNT 4
#define TOTAL_PALETTE_SIZE  ((BG_PALETTE_COUNT + SPRITE_PALETTE_COUNT) * PALETTE_COLORS)

// gNesPaletteBGR[i] = BGR color for NES palette index i (0x00-0x3F)
extern const UINT32 gNesPaletteBGR[NES_PALETTE_SIZE];

// Convert NES palette index (0x00-0x3F) to BGR UINT32
static inline UINT32 NesColorBGR(UINT8 nesIdx) {
  return gNesPaletteBGR[nesIdx & 0x3F];
}

#endif
