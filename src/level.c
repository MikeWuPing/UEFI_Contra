// Contra level system - NES 2bpp tile + palette background

#include "level.h"
#include "nes_palette.h"
#include "tiles_full.h"
#include "game_state.h"

LEVEL_STATE gLevelState;

// === NES 2bpp tile cache ===
static Pixel gTileCache[4][256][64];
static UINT8 gTilesReady = 0;

// Runtime palette (can be modified for cycling animation)
static UINT8 gActiveBgPalettes[4][4];

// NES palette cycle state (matching Level 1: water + red flash)
// LEVEL_PALETTE_CYCLE_INDEXES = {$05, $08, $05, $08} → water animation
// level_palette_2_index     = {$04, $5c, $04, $5d} → red flash
static const UINT8 gPaletteCycleWater[4] = { 0x05, 0x08, 0x05, 0x08 };
static const UINT8 gPaletteCycleRedFlash[4] = { 0x04, 0x5c, 0x04, 0x5d };

// Full game_palettes table from NES (index → {c1,c2,c3} NES color values)
// Each entry adds 0x0F (black) as the 0th color at runtime
static const UINT8 gGamePaletteTbl[][3] = {
  {0x37,0x12,0x0F},{0x36,0x16,0x0F},{0x19,0x29,0x08},{0x28,0x18,0x08}, // 00-03
  {0x16,0x30,0x10},{0x11,0x21,0x30},{0x16,0x20,0x00},{0x20,0x00,0x0F}, // 04-07
  {0x11,0x30,0x21},{0x10,0x00,0x09},{0x00,0x0C,0x20},{0x0A,0x1A,0x00}, // 08-0B
  {0x28,0x18,0x07},{0x1C,0x2C,0x0C},{0x0C,0x1C,0x2C},{0x2C,0x0C,0x1C}, // 0C-0F
  {0x10,0x2B,0x0F},{0x06,0x00,0x08},{0x20,0x00,0x08},{0x06,0x00,0x08}, // 10-13
  {0x16,0x00,0x08},{0x26,0x00,0x08},{0x20,0x00,0x06},{0x20,0x00,0x16}, // 14-17
  {0x20,0x00,0x26},{0x0C,0x18,0x09},{0x20,0x18,0x09},{0x06,0x18,0x09}, // 18-1B
  {0x16,0x18,0x09},{0x26,0x18,0x09},{0x20,0x18,0x06},{0x20,0x18,0x16}, // 1C-1F
  {0x20,0x18,0x26},{0x20,0x22,0x02},{0x20,0x26,0x16},                   // 20-22
  // Extended entries for cycle lookup (0x5C-0x5D)
  // 0x5C: DARK_RED_06, WHITE_30, LT_GRAY_10
  // 0x5D: LT_RED_26, WHITE_30, LT_GRAY_10
};

// Look up a game_palettes entry and copy to active palette (pal[0]=0x0F black)
static VOID LoadPaletteEntry(UINT8 palIdx, UINT8 *dst)
{
  dst[0] = 0x0F;  // Always black (NES palette entry 0)
  if (palIdx == 0x5C) {
    dst[1] = 0x06; dst[2] = 0x30; dst[3] = 0x10;  // DARK_RED, WHITE, LT_GRAY
  } else if (palIdx == 0x5D) {
    dst[1] = 0x26; dst[2] = 0x30; dst[3] = 0x10;  // LT_RED, WHITE, LT_GRAY
  } else if (palIdx < sizeof(gGamePaletteTbl)/3) {
    dst[1] = gGamePaletteTbl[palIdx][0];
    dst[2] = gGamePaletteTbl[palIdx][1];
    dst[3] = gGamePaletteTbl[palIdx][2];
  } else {
    // Fallback: use palette 0
    dst[1] = 0x19; dst[2] = 0x29; dst[3] = 0x08;
  }
}

// Initialize active palettes from level header and apply palette cycling
VOID UpdatePaletteCycle(VOID)
{
  static UINT8 lastCycle = 0xFF;
  UINT8 cycle = (gFrameCounter / 8) & 0x03;  // NES: increments every 8 frames

  if (cycle == lastCycle) return;
  lastCycle = cycle;

  UINT8 waterIdx = gPaletteCycleWater[cycle];
  UINT8 redIdx   = gPaletteCycleRedFlash[cycle];

  LoadPaletteEntry(waterIdx, gActiveBgPalettes[3]);  // BG palette 3 = water
  LoadPaletteEntry(redIdx,   gActiveBgPalettes[2]);  // BG palette 2 = red flash

  // Invalidate tile cache so it rebuilds with new colors
  gTilesReady = 0;
}

// Initialize the active palette from static base palettes
static VOID InitActivePalettes(VOID)
{
  for (UINT8 p = 0; p < 4; p++)
    for (UINT8 c = 0; c < 4; c++)
      gActiveBgPalettes[p][c] = gBgPalettes[p][c];
}

static VOID BuildTileCache(VOID)
{
  if (gTilesReady) return;
  for (UINT8 p = 0; p < 4; p++) {
    for (UINT16 t = 0; t < TILE_COUNT; t++) {
      const UINT8 *src = gRawTiles[t];
      for (UINT8 r = 0; r < 8; r++) {
        UINT8 p0 = src[r], p1 = src[r+8];
        for (UINT8 c = 0; c < 8; c++) {
          UINT8 bit = 7 - c;
          UINT8 ci = ((p1 >> bit) & 1) << 1 | ((p0 >> bit) & 1);
          // Use active (possibly cycled) palette
          gTileCache[p][t][r*8+c] = gNesPaletteBGR[gActiveBgPalettes[p][ci]];
        }
      }
    }
  }
  gTilesReady = 1;
}

static VOID DrawTileNes(INT32 x, INT32 y, UINT8 tileIdx, UINT8 attrPal)
{
  if (!gTilesReady) BuildTileCache();
  if (tileIdx >= TILE_COUNT) return;
  UINT8 p = (attrPal <= 3) ? attrPal : 1;

  for (INT32 r = 0; r < 8; r++) {
    INT32 dy = y + r;
    if (dy < 0 || dy >= GAME_HEIGHT) continue;
    Pixel *dst = &gGameBuffer[dy * GAME_WIDTH + x];
    Pixel *rowSrc = &gTileCache[p][tileIdx][r * 8];
    INT32 n = 8;
    if (x < 0) { n += x; rowSrc += (-x); dst += (-x); }
    if (x + 8 > GAME_WIDTH) n = GAME_WIDTH - (x > 0 ? x : 0);
    if (n > 0) CopyMem(dst, rowSrc, n * sizeof(Pixel));
  }
}

// Draw a 32x32 super-tile using authentic NES per-quadrant palette from gLevel1PaletteData
// Format: each byte = 4x 2-bit palette codes (BR,BL,TR,TL = bits 7-6,5-4,3-2,1-0)
static VOID DrawSuperTile(INT32 x, INT32 y, UINT8 superTileIdx, UINT8 stRow, UINT8 screenNum)
{
  if (superTileIdx >= LEVEL1_SUPERTILE_COUNT) return;
  const UINT8 *st = &gLevel1Supertiles[superTileIdx * 16];

  // Get authentic per-quadrant palette from level data
  // Palette array covers all 242 super-tile entries (extended with 0xFF fallbacks)
  UINT8 palByte = gLevel1PaletteData[superTileIdx];
  // Extract 4 quadrants (NES attribute table order: TL, TR, BL, BR)
  UINT8 quadPal[4];
  quadPal[0] = (palByte >> 0) & 0x03;  // TL
  quadPal[1] = (palByte >> 2) & 0x03;  // TR
  quadPal[2] = (palByte >> 4) & 0x03;  // BL
  quadPal[3] = (palByte >> 6) & 0x03;  // BR

  (VOID)stRow;
  (VOID)screenNum;

  for (INT32 row = 0; row < 4; row++) {
    for (INT32 col = 0; col < 4; col++) {
      UINT8 idx = st[row * 4 + col];
      if (idx == 0x00) continue;
      UINT8 quad = (row / 2) * 2 + (col / 2);
      DrawTileNes(x + col * 8, y + row * 8, idx, quadPal[quad]);
    }
  }
}

// === COLLISION SYSTEM ===
#define SUPERTILE_PX 32
#define TILE_PX 8
static UINT8 gColIdx1 = 0x0B;
static UINT8 gColIdx0 = 0x06;
static UINT8 gColIdx2 = 0xF9;

static UINT8  gBgCollisionData[2048];
static UINT16 gTotalCollisionCols = 0;

static UINT8 TileCollisionCode(UINT8 tileIdx)
{
  if (tileIdx == 0x00) return 0;
  if (tileIdx < gColIdx1) return 1;
  if (tileIdx < gColIdx0) return 0;
  if (tileIdx >= gColIdx2) return (tileIdx == 0xFF) ? 3 : 2;
  return 1;
}

static VOID BuildCollisionMap(VOID)
{
  gTotalCollisionCols = LEVEL1_SCREEN_COUNT * 16;
  ZeroMem(gBgCollisionData, sizeof(gBgCollisionData));
  for (UINT16 screen = 0; screen < LEVEL1_SCREEN_COUNT; screen++) {
    const UINT8 *sd = gLevel1Screens[screen];
    for (UINT8 stRow = 0; stRow < SCREEN_ROWS; stRow++) {
      for (UINT8 stCol = 0; stCol < SCREEN_COLS; stCol++) {
        UINT8 stIdx = sd[stRow * SCREEN_COLS + stCol];
        if (stIdx >= LEVEL1_SUPERTILE_COUNT) continue;
        const UINT8 *tiles = &gLevel1Supertiles[stIdx * 16];
        for (INT32 qr = 0; qr < 2; qr++) {
          for (INT32 qc = 0; qc < 2; qc++) {
            UINT8 maxCode = 0;
            for (INT32 tr = 0; tr < 2; tr++)
              for (INT32 tc = 0; tc < 2; tc++) {
                UINT8 tile = tiles[(qr*2+tr)*4 + (qc*2+tc)];
                UINT8 code = TileCollisionCode(tile);
                if (code > maxCode) maxCode = code;
              }
            UINT16 gc = screen * 16 + stCol * 2 + qc;
            UINT8  gr = stRow * 2 + qr;
            if (gr >= 16 || gc >= gTotalCollisionCols) continue;
            UINT16 bpr = gTotalCollisionCols / 4;
            UINT16 bi = gr * bpr + (gc >> 2);
            UINT8  f = gc & 3;
            UINT8  s = (3 - f) * 2;
            UINT8  m = ~(3 << s);
            gBgCollisionData[bi] = (gBgCollisionData[bi] & m) | (maxCode << s);
          }
        }
      }
    }
  }
}

VOID LevelInit(UINT8 levelNum)
{
  ZeroMem(&gLevelState, sizeof(gLevelState));
  gLevelState.currentLevel = levelNum;
  gLevelState.scrollX = 0;
  const UINT8 *hdr = gLevelHeaders[levelNum];
  gColIdx1 = hdr[9];   // COLLISION_CODE_1: tiles < this = floor
  gColIdx0 = hdr[10];  // COLLISION_CODE_0: tiles >= col1 and < this = empty
  gColIdx2 = hdr[11];  // COLLISION_CODE_2: tiles >= col0 and < this = water, >= = solid
  BuildCollisionMap();
  InitActivePalettes();
  gTilesReady = 0;
}

VOID ScrollUpdate(INT32 delta)
{
  INT32 ns = (INT32)gLevelState.scrollX + delta;
  INT32 maxS = LEVEL1_SCREEN_COUNT * 256 - GAME_WIDTH;
  if (maxS < 0) maxS = 0;
  if (ns < 0) ns = 0;
  if (ns > maxS) ns = maxS;
  gLevelState.scrollX = (UINT16)ns;
}

INT32 WorldToScreenX(INT32 worldX) { return worldX - (INT32)gLevelState.scrollX; }

VOID DrawBackground(VOID)
{
  // Apply NES palette cycling animation (water shimmer + red flash)
  UpdatePaletteCycle();

  INT32 sx = (INT32)gLevelState.scrollX;
  INT32 startScreen = sx / 256;
  INT32 pixelOffset = sx % 256;
  for (INT32 screen = 0; screen < 2; screen++) {
    INT32 si = startScreen + screen;
    if (si >= LEVEL1_SCREEN_COUNT) break;
    const UINT8 *sd = gLevel1Screens[si];
    for (INT32 row = 0; row < SCREEN_ROWS; row++) {
      for (INT32 col = 0; col < SCREEN_COLS; col++) {
        UINT8 stIdx = sd[row * SCREEN_COLS + col];
        INT32 dx = screen * 256 + col * SUPERTILE_PX - pixelOffset;
        INT32 dy = row * SUPERTILE_PX;
        if (dx >= GAME_WIDTH || dx + SUPERTILE_PX <= 0) continue;
        DrawSuperTile(dx, dy, stIdx, (UINT8)row, (UINT8)si);
      }
    }
  }
}

UINT8 GetBgCollision(INT32 worldX, INT32 worldY)
{
  if (worldX < 0 || worldY < 0 || worldY >= GAME_HEIGHT) return 0;
  UINT32 uy = (UINT32)worldY;
  // Use pre-computed BG_COLLISION_DATA (max code per 16x16 cell, matching NES)
  UINT32 ux = (UINT32)worldX;
  UINT16 cellCol = (UINT16)(ux / 16);
  UINT16 cellRow = (UINT16)(uy / 16);
  if (cellCol >= gTotalCollisionCols || cellRow >= 16) return 0;
  UINT16 bpr = gTotalCollisionCols >> 2;
  UINT16 bi = cellRow * bpr + (cellCol >> 2);
  if (bi >= sizeof(gBgCollisionData)) return 0;
  UINT8 field = cellCol & 3;
  UINT8 shift = (3 - field) * 2;
  return (gBgCollisionData[bi] >> shift) & 0x03;
}

UINT8 GetTileIndex(INT32 worldX, INT32 worldY)
{
  UINT32 ux = (UINT32)(worldX & 0xFFFF);
  UINT32 uy = (UINT32)(worldY & 0xFFFF);
  UINT16 sn = (UINT16)(ux / 256);
  UINT8 lx = (UINT8)(ux % 256);
  UINT8 ly = (UINT8)uy;
  if (sn >= LEVEL1_SCREEN_COUNT) return 0xFF;
  UINT8 sc = lx / SUPERTILE_PX;
  UINT8 sr = ly / SUPERTILE_PX;
  if (sc >= SCREEN_COLS || sr >= SCREEN_ROWS) return 0xFF;
  UINT8 si = gLevel1Screens[sn][sr * SCREEN_COLS + sc];
  if (si >= LEVEL1_SUPERTILE_COUNT) return 0xFF;
  UINT8 tc = (lx % SUPERTILE_PX) / TILE_PX;
  UINT8 tr = (ly % SUPERTILE_PX) / TILE_PX;
  return gLevel1Supertiles[si * 16 + tr * 4 + tc];
}