#include "types.h"
#include "input.h"
#include "sprites.h"
#include "player.h"
#include "game_state.h"
#include "level.h"
#include "bullet.h"
#include "enemy.h"
#include "weapon.h"
#include "nes_palette.h"
#include "intro_screen.h"

GameState gGame;

// === Intro screen: NES 2bpp tiles with per-attribute palette ===
// Two palette sets from bank6:
//   transition_screen_palettes (CPU $b333) — used during game_routine_00 (logo scroll)
//   intro_background_palette2  (CPU $b3b4) — used when Bill/Lance appear (routine 01-03)
// Palettes 0-2 are identical between both sets; only palette 3 differs.
// Palette 0: black, lt_gray(0x10), lt_olive(0x28), med_red(0x16) → red/orange tones
// Palette 1: black, white(0x30), lt_gray(0x10), med_pink(0x15) → white/pink
// Palette 2: black, lt_gray(0x10), lt_olive(0x28), pale_olive(0x38) → olive
// Palette 3 (transition): black, black, black, black → no backdrop (routine 00)
#define INTRO_PAL0 { 0x0F, 0x10, 0x28, 0x16 }
#define INTRO_PAL1 { 0x0F, 0x30, 0x10, 0x15 }
#define INTRO_PAL2 { 0x0F, 0x10, 0x28, 0x38 }
#define INTRO_PAL3_TRANSITION { 0x0F, 0x0F, 0x0F, 0x0F }
// Palette 3 (intro2): black, white($30), pale_red($36), peach($26) → Bill/Lance body
// NES BG3: index2=$36(pale red=skin tone), index3=$26(peach=darker shading)
#define INTRO_PAL3_INTRO2     { 0x0F, 0x30, 0x36, 0x26 }
static const UINT8 gIntroPalTransition[4][4] = {
  INTRO_PAL0, INTRO_PAL1, INTRO_PAL2, INTRO_PAL3_TRANSITION
};
static const UINT8 gIntroPalIntro2[4][4] = {
  INTRO_PAL0, INTRO_PAL1, INTRO_PAL2, INTRO_PAL3_INTRO2
};
static Pixel gIntroTileCache[4][256][64];
static UINT8 gIntroCacheReady = 0;   // 0 = not built; 1 = transition; 2 = intro2

// Select the active palette set based on game routine
static const UINT8 (*GetIntroPalette(VOID))[4]
{
  return (gGameRoutineIndex == 0) ? gIntroPalTransition : gIntroPalIntro2;
}

static VOID BuildIntroTileCache(VOID)
{
  UINT8 which = (gGameRoutineIndex == 0) ? 1 : 2;
  if (gIntroCacheReady == which) return;
  gIntroCacheReady = 0;  // invalidate during build

  const UINT8 (*pal)[4] = GetIntroPalette();
  for (UINT8 p = 0; p < 4; p++) {
    for (UINT16 t = 0; t < 256; t++) {
      const UINT8 *src = gIntroRawTiles[t];
      for (UINT8 r = 0; r < 8; r++) {
        UINT8 p0 = src[r], p1 = src[r+8];
        for (UINT8 c = 0; c < 8; c++) {
          UINT8 ci = ((p1 >> (7-c)) & 1) << 1 | ((p0 >> (7-c)) & 1);
          gIntroTileCache[p][t][r*8+c] = gNesPaletteBGR[pal[p][ci]];
        }
      }
    }
  }
  gIntroCacheReady = which;
}

// Get palette from attribute table for a tile at (col, row)
static UINT8 IntroAttrPalette(UINT8 col, UINT8 row)
{
  UINT8 attrIdx = (row / 4) * 8 + (col / 4);
  if (attrIdx >= 64) return 0;
  UINT8 attr = gIntroAttr[attrIdx];
  UINT8 shift = ((col / 2) & 1) * 2 + ((row / 2) & 1) * 4;
  return (attr >> shift) & 0x03;
}

static VOID DrawIntroNametable(VOID)
{
  BuildIntroTileCache(); // cache state check is done inside

  for (UINT8 row = 0; row < INTRO_NT_H; row++) {
    for (UINT8 col = 0; col < INTRO_NT_W; col++) {
      UINT8 tileIdx = gIntroNT[row * INTRO_NT_W + col];
      if (tileIdx == 0) continue;

      INT32 dx = col * 8;
      INT32 dy = row * 8;
      UINT8 pal = IntroAttrPalette(col, row);
      Pixel *tile = gIntroTileCache[pal][tileIdx];

      for (UINT8 r = 0; r < 8; r++) {
        INT32 sy = dy + r;
        if (sy < 0 || sy >= GAME_HEIGHT) continue;
        Pixel *dst = &gGameBuffer[sy * GAME_WIDTH + dx];
        Pixel *src = &tile[r * 8];
        INT32 n = 8;
        if (dx < 0) { n += dx; src += (-dx); dst += (-dx); }
        if (dx + 8 > GAME_WIDTH) n = GAME_WIDTH - (dx > 0 ? dx : 0);
        if (n > 0) CopyMem(dst, src, n * sizeof(Pixel));
      }
    }
  }
}

// Render the current game state to the game buffer
static VOID RenderGame(VOID)
{
  ClearGameBuffer(COLOR_BLACK);

  // Title screen / intro (game_routine_00-03)
  if (gGameRoutineIndex <= 3) {
    if (gGameRoutineIndex <= 1) {
      // Draw the NES intro screen from ROM data
      DrawIntroNametable();

      // Player select: cursor + Bill/Lance hair/shirt
      if (gGameRoutineIndex == 1) {
        INT32 cursorY = gPlayerMode ? 178 : 162;
        // sprite_aa has tile offsets (-8,-8); reference ~(8,8) in image
        DrawSprite(0xaa, 36, cursorY - 4, 0, 0xFFFFFFFF);  // cursor (yellow falcon), +2 down
        // sprite_ab NES tile offsets: X=-24..+36, Y=-3..+52
        // PNG ref point ~(24,3); draw at (179-24, 119-3)
        DrawSprite(0xab, 156, 121, 0, 0xFFFFFFFF);      // Bill & Lance hair/shirt (22-entry composite), +1 right, -1 up
      }

      // Overlay dark bars at top and bottom for HUD
      FillRect(0, 0, GAME_WIDTH, 10, COLOR_BLACK);
      FillRect(0, 230, GAME_WIDTH, 10, COLOR_BLACK);

      if (gController2.Pressed & INPUT_START) { gPlayerMode = 1; gGameRoutineIndex = 1; }
    } else if (gGameRoutineIndex == 2) {
      // Routine 02: Demo mode - show "DEMO" in corner
      FillRect(0, 0, GAME_WIDTH, 10, COLOR_BLACK);
      FillRect(4, 2, 32, 6, COLOR_RED);
      if (gController1.Pressed & INPUT_START) gGameRoutineIndex = 1;
    } else {
      // Routine 03: Flash/screen transition
      if ((gFrameCounter & 0x04)) {
        FillRect(0, 0, GAME_WIDTH, GAME_HEIGHT, COLOR_WHITE);
      }
    }
  }

  // Gameplay (game_routine_04+)
  if (gGameRoutineIndex >= 4) {
    DrawBackground();

    // Boss wall uses authentic screen 12 tiles (no synthetic overlay needed)

    DrawWeaponItems();
    DrawPlayerBullets();
    DrawEnemyBullets();
    DrawEnemies();
    DrawPlayer(&gPlayers[0]);
    if (gPlayerMode == 1) DrawPlayer(&gPlayers[1]);

    // HUD
    static const Pixel wc[7] = { COLOR_WHITE,COLOR_RED,COLOR_ORANGE,COLOR_YELLOW,COLOR_CYAN,COLOR_GREEN,COLOR_MAGENTA };
    FillRect(0, 0, GAME_WIDTH, 10, COLOR_BLACK);

    // P1 lives: blue medal sprite
    for (INT32 l = 0; l < gPlayers[0].Lives; l++)
      DrawSprite(0xE0, 8 + l * 14, 2, 0, 0xFFFFFFFF);

    // Score indicator (colored bar, length = score/2000 capped at 60px)
    {
      UINT32 scorePx = gPlayers[0].Score / 2000;
      if (scorePx > 60) scorePx = 60;
      if (scorePx > 0) FillRect(44, 2, scorePx, 6, COLOR_YELLOW);
    }

    // Weapon indicator (hidden - not shown in original HUD)
    // UINT8 w = gPlayers[0].CurrentWeapon;
    // if (w <= 6) FillRect(GAME_WIDTH - 14, 2, 10, 6, wc[w]);


    // Level intro (routines 01-03): brief transition, no text overlay
    if (gLevelRoutineIndex >= 1 && gLevelRoutineIndex <= 3) {
      // Quick darken
      for (INT32 row = 10; row < GAME_HEIGHT; row += 6) {
        for (INT32 col = 0; col < GAME_WIDTH; col += 6) {
          Pixel *p = &gGameBuffer[row * GAME_WIDTH + col];
          UINT32 bgr = *p;
          UINT32 r = ((bgr >> 16) & 0xFF) >> 1;
          UINT32 g = ((bgr >> 8) & 0xFF) >> 1;
          UINT32 b = (bgr & 0xFF) >> 1;
          *p = (r << 16) | (g << 8) | b;
        }
      }
    }

    // Boss defeat / Level complete overlay (routines 05, 08, 09)
    if (gLevelRoutineIndex >= 5 && gLevelRoutineIndex <= 5) {
      // Level complete - show "STAGE CLEAR"
      FillRect(58, 90, 140, 36, COLOR_BLACK);
      FillRect(60, 92, 136, 32, COLOR_GREEN);
      // Simple text blocks
      FillRect(70, 100, 8, 16, COLOR_WHITE);   // S
      FillRect(82, 100, 8, 16, COLOR_WHITE);
      FillRect(96, 100, 8, 16, COLOR_WHITE);
      FillRect(110, 100, 8, 16, COLOR_WHITE);
      FillRect(126, 100, 8, 16, COLOR_WHITE);
      FillRect(140, 100, 8, 16, COLOR_WHITE);
      FillRect(156, 100, 8, 16, COLOR_WHITE);
    }

    if (gLevelRoutineIndex == 8) {
      // Boss defeated - flash red
      if (gFrameCounter & 0x08) {
        FillRect(0, 10, GAME_WIDTH, GAME_HEIGHT - 10, COLOR_RED);
      }
    }

    if (gLevelRoutineIndex == 9) {
      // End-of-level sequence - no special rendering needed
      // Player walks right automatically
    }

    // Game Over screen (level routines 06, 07)
    if (gLevelRoutineIndex >= 6 && gLevelRoutineIndex <= 7) {
      // Dark overlay
      for (INT32 row = 0; row < GAME_HEIGHT; row += 4) {
        for (INT32 col = 0; col < GAME_WIDTH; col += 4) {
          Pixel *p = &gGameBuffer[row * GAME_WIDTH + col];
          UINT32 bgr = *p;
          UINT32 r = ((bgr >> 16) & 0xFF) >> 1;  // 50% dark
          UINT32 g = ((bgr >> 8) & 0xFF) >> 1;
          UINT32 b = (bgr & 0xFF) >> 1;
          *p = (r << 16) | (g << 8) | b;
        }
      }

      // "GAME OVER" text
      FillRect(58, 90, 140, 36, COLOR_BLACK);
      FillRect(60, 92, 136, 32, COLOR_RED);
      // Large block-letter GAME OVER using FillRect
      // G
      FillRect(70, 96, 16, 4, COLOR_WHITE);
      FillRect(70, 96, 4, 24, COLOR_WHITE);
      FillRect(70, 116, 16, 4, COLOR_WHITE);
      FillRect(82, 108, 4, 8, COLOR_WHITE);
      // O
      FillRect(90, 96, 16, 4, COLOR_WHITE);
      FillRect(90, 96, 4, 24, COLOR_WHITE);
      FillRect(90, 116, 16, 4, COLOR_WHITE);
      FillRect(102, 96, 4, 24, COLOR_WHITE);
      // V
      FillRect(112, 96, 4, 24, COLOR_WHITE);
      FillRect(124, 96, 4, 24, COLOR_WHITE);
      // E
      FillRect(134, 96, 16, 4, COLOR_WHITE);
      FillRect(134, 96, 4, 24, COLOR_WHITE);
      FillRect(134, 106, 12, 4, COLOR_WHITE);
      FillRect(134, 116, 16, 4, COLOR_WHITE);
      // R
      FillRect(154, 96, 4, 24, COLOR_WHITE);
      FillRect(154, 96, 12, 4, COLOR_WHITE);
      FillRect(162, 96, 4, 12, COLOR_WHITE);
      FillRect(154, 106, 12, 4, COLOR_WHITE);
      FillRect(158, 110, 4, 6, COLOR_WHITE);
      FillRect(162, 114, 4, 6, COLOR_WHITE);

      if (gLevelRoutineIndex == 6) {
        // "PRESS START TO CONTINUE"
        if (gFrameCounter & 0x20) {
          FillRect(68, 140, 120, 14, COLOR_BLACK);
          FillRect(70, 142, 116, 10, COLOR_GREEN);
        }
      }
      if (gLevelRoutineIndex == 7) {
        FillRect(78, 140, 100, 14, COLOR_BLACK);
        FillRect(80, 142, 96, 10, COLOR_RED);
      }
    }
  }

  // Version v0.58 at bottom-right
  {
    INT32 bx = GAME_WIDTH - 26, by = GAME_HEIGHT - 7;
    // v
    FillRect(bx, by, 1, 3, COLOR_WHITE); FillRect(bx+2, by, 1, 3, COLOR_WHITE);
    FillRect(bx+1, by+2, 1, 2, COLOR_WHITE); bx+=4;
    // 0
    FillRect(bx, by, 3, 1, COLOR_WHITE); FillRect(bx, by+4, 3, 1, COLOR_WHITE);
    FillRect(bx, by+1, 1, 3, COLOR_WHITE); FillRect(bx+2, by+1, 1, 3, COLOR_WHITE); bx+=4;
    // .
    FillRect(bx+1, by+4, 1, 1, COLOR_WHITE); bx+=3;
    // 6
    FillRect(bx, by, 3, 1, COLOR_WHITE);
    FillRect(bx, by+1, 1, 1, COLOR_WHITE);
    FillRect(bx, by+2, 3, 1, COLOR_WHITE);
    FillRect(bx, by+3, 1, 1, COLOR_WHITE);
    FillRect(bx+2, by+3, 1, 1, COLOR_WHITE);
    FillRect(bx, by+4, 3, 1, COLOR_WHITE); bx+=4;
    // 0
    FillRect(bx, by, 3, 1, COLOR_WHITE); FillRect(bx, by+4, 3, 1, COLOR_WHITE);
    FillRect(bx, by+1, 1, 3, COLOR_WHITE); FillRect(bx+2, by+1, 1, 3, COLOR_WHITE);
  }

  ScaleAndPresent();
}

VOID ContraMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS  Status;
  EFI_EVENT   timerEvent;
  UINTN       index;

  Print(L"\n=== Contra UEFI Game ===\n");

  // 1. Initialize graphics
  Status = GraphicsInit();
  if (EFI_ERROR(Status)) {
    Print(L"Error: Graphics init failed (status=0x%x)\n", Status);
    return;
  }
  // Disable UEFI console cursor so it doesn't show as an artifact
  gST->ConOut->EnableCursor(gST->ConOut, FALSE);
  Print(L"Display: %dx%d, Scale: %dx\n", gScreenWidth, gScreenHeight, gScale);

  // 2. Initialize sprite system (load .spr files from disk)
  Print(L"Loading sprites...\n");
  Status = SpriteSystemInit();
  if (EFI_ERROR(Status)) {
    Print(L"Warning: Sprite loading failed, sprites won't show\n");
  }

  // 3. Initialize input
  InputInit();

  // 4. Initialize game state machine and level
  GameStateInit();
  LevelInit(0);  // Load level 1 data so background renders during title
  gGame.running  = TRUE;
  gGame.paused   = FALSE;
  gGame.frameCount = 0;

  // 5. Create frame timer (60 FPS = 16667 microseconds)
  Status = SystemTable->BootServices->CreateEvent(
    EVT_TIMER,
    TPL_CALLBACK,
    NULL,
    NULL,
    &timerEvent
  );
  if (EFI_ERROR(Status)) {
    Print(L"Error: Cannot create timer (status=0x%x)\n", Status);
    SpriteSystemShutdown();
    GraphicsShutdown();
    return;
  }

  Status = SystemTable->BootServices->SetTimer(
    timerEvent,
    TimerPeriodic,
    166667  // ~16.67ms * 10000 (timer units are 100ns)
  );
  if (EFI_ERROR(Status)) {
    Print(L"Error: Cannot set timer (status=0x%x)\n", Status);
    SystemTable->BootServices->CloseEvent(timerEvent);
    SpriteSystemShutdown();
    GraphicsShutdown();
    return;
  }

  Print(L"=== Starting Game Loop ===\n");
  Print(L"Controls: Arrows=Move  Z=Fire  X=Jump  Enter=Start  P=Pause  ESC=Quit\n");

  // 6. Main game loop
  while (gGame.running) {
    // Wait for next frame
    Status = SystemTable->BootServices->WaitForEvent(
      1,
      &timerEvent,
      &index
    );
    if (EFI_ERROR(Status)) continue;

    gGame.frameCount++;

    // Poll input every frame
    PollInput();

    // Check quit (ESC key)
    {
      EFI_INPUT_KEY Key;
      EFI_STATUS keyStatus = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
      if (!EFI_ERROR(keyStatus)) {
        if (Key.ScanCode == 0x0017) { // ESC
          gGame.running = FALSE;
          break;
        }
        if (Key.UnicodeChar == L'q' || Key.UnicodeChar == L'Q') {
          gGame.running = FALSE;
          break;
        }
        if (Key.UnicodeChar == L'p' || Key.UnicodeChar == L'P') {
          gGame.paused = !gGame.paused;
        }
      }
    }

    if (!gGame.paused) {
      // Execute game state machine
      RunGameRoutine();

      // Render frame
      RenderGame();
    }
  }

  // 7. Cleanup
  SystemTable->BootServices->CloseEvent(timerEvent);
  SpriteSystemShutdown();
  GraphicsShutdown();

  Print(L"\n=== Contra UEFI Game Exit (frame %d) ===\n", gGame.frameCount);
}
