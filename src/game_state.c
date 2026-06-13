#include "game_state.h"
#include "player.h"
#include "sprites.h"
#include "level.h"
#include "bullet.h"
#include "enemy.h"
#include "weapon.h"

UINT8 gGameRoutineIndex  = 0;
UINT8 gLevelRoutineIndex = 0;
UINT8 gFrameCounter      = 0;
UINT8 gDemoMode          = 0;

// Forward declarations of all game/level routines
static VOID GameRoutine00(VOID);
static VOID GameRoutine01(VOID);
static VOID GameRoutine02(VOID);
static VOID GameRoutine03(VOID);
static VOID GameRoutine04(VOID);
static VOID GameRoutine05(VOID);
static VOID GameRoutine06(VOID);

static VOID LevelRoutine00(VOID);
static VOID LevelRoutine01(VOID);
static VOID LevelRoutine02(VOID);
static VOID LevelRoutine03(VOID);
static VOID LevelRoutine04(VOID);
static VOID LevelRoutine05(VOID);
static VOID LevelRoutine06(VOID);
static VOID LevelRoutine07(VOID);
static VOID LevelRoutine08(VOID);
static VOID LevelRoutine09(VOID);
static VOID LevelRoutine0a(VOID);

// Function pointer tables
typedef VOID (*ROUTINE_FUNC)(VOID);

static ROUTINE_FUNC gGameRoutines[GAME_ROUTINE_COUNT] = {
  GameRoutine00, GameRoutine01, GameRoutine02,
  GameRoutine03, GameRoutine04, GameRoutine05, GameRoutine06
};

static ROUTINE_FUNC gLevelRoutines[LEVEL_ROUTINE_COUNT] = {
  LevelRoutine00, LevelRoutine01, LevelRoutine02,
  LevelRoutine03, LevelRoutine04, LevelRoutine05,
  LevelRoutine06, LevelRoutine07, LevelRoutine08,
  LevelRoutine09, LevelRoutine0a
};

// === GAME ROUTINE IMPLEMENTATIONS ===

static UINT16 gIntroTimer = 0;
static UINT8  gTitleScrollSpeed = 0;

static VOID GameRoutine00(VOID)
{
  // NES: intro logo scroll animation takes ~3 seconds, then Bill/Lance appear.
  // During scroll, transition_screen_palettes are active (BG3 = all black).
  // After scroll completes, intro_background_palette2 is loaded (BG3 = red/white).
  // We simulate this with a timer: stay in routine 00 for ~3 seconds
  // so the black background is visible, then transition to routine 01.
  gIntroTimer++;
  if (gIntroTimer > 180) {  // ~3 seconds at 60fps
    gIntroTimer = 0;
    gGameRoutineIndex = 1;  // Switch to player select (intro2 palette active)
  }
  // Allow skipping with START
  if (gController1.Pressed & INPUT_START) {
    gIntroTimer = 0;
    gGameRoutineIndex = 1;
  }
}

static VOID GameRoutine01(VOID)
{
  // NES: Player select screen with auto-demo timeout
  gDemoMode = 0;
  gIntroTimer++;

  // UP/DOWN to select 1P or 2P
  if (gController1.Pressed & INPUT_UP)   gPlayerMode = 0;
  if (gController1.Pressed & INPUT_DOWN) gPlayerMode = 1;

  // START → begin game
  if (gController1.Pressed & INPUT_START) {
    gIntroTimer = 0;
    PlayerInit(0);
    if (gPlayerMode == 1) PlayerInit(1);
    gGameRoutineIndex = 4;
    return;
  }

  // Auto-demo after ~8 seconds of inactivity (matching NES)
  if (gIntroTimer > 360) {  // 6 seconds auto-demo
    gIntroTimer = 0;
    gDemoMode = 1;
    // Init demo player
    PlayerInit(0);
    if (gPlayerMode == 1) PlayerInit(1);
    gGameRoutineIndex = 4;  // Start gameplay in demo mode
    return;
  }
}

static VOID GameRoutine02(VOID)
{
  // NES: Demo mode - switch to gameplay path with demo flag
  gDemoMode = 1;
  gIntroTimer = 0;           // Reset timer for demo duration tracking
  gGameRoutineIndex = 4;     // Clear state -> gameplay
}

static VOID GameRoutine03(VOID)
{
  // NES: Flash screen transition
  // Port: Quick transition
  gIntroTimer++;
  if (gIntroTimer > 30) {
    gIntroTimer = 0;
    gGameRoutineIndex = 4;
  }
}

static VOID GameRoutine04(VOID)
{
  // NES: Clear level memory and player-specific memory
  // Port: Initialize/clear player state for new game
  DEBUG((DEBUG_INFO, "[Game] Routine 04: Clear state\n"));
  ClearWeaponItems();
  PlayerInit(0);
  if (gPlayerMode == 1) {
    PlayerInit(1);
  }
  gGameRoutineIndex = 5; // Enter core gameplay
  gLevelRoutineIndex = 0; // Start level routines
}

static VOID GameRoutine05(VOID)
{
  // NES: Core game logic - dispatch to level routine
  gLevelRoutines[gLevelRoutineIndex]();
}

static VOID GameRoutine06(VOID)
{
  // NES: Ending sequence after defeating final boss
  // Port: Placeholder
  DEBUG((DEBUG_INFO, "[Game] Routine 06: Ending\n"));
  // Loop back to title (for now)
  gGameRoutineIndex = 0;
}

// === LEVEL ROUTINE IMPLEMENTATIONS ===

static VOID LevelRoutine00(VOID)
{
  // NES: Load level header data, decompress super-tile indexes
  DEBUG((DEBUG_INFO, "[Level] Routine 00: Load level data\n"));
  LevelInit(0);
  SpawnLevelEnemies(0);
  AdvanceLevelRoutine();
}

static VOID LevelRoutine01(VOID)
{
  // Brief transition before level starts
  static UINT8 timer = 0;
  timer++;
  if (timer > 15) {
    timer = 0;
    AdvanceLevelRoutine();
  }
}

static VOID LevelRoutine02(VOID)
{
  static UINT8 timer = 0;
  timer++;
  if (timer > 10) {
    timer = 0;
    AdvanceLevelRoutine();
  }
}

static VOID LevelRoutine03(VOID)
{
  static UINT8 timer = 0;
  timer++;
  if (timer > 5) {
    timer = 0;
    AdvanceLevelRoutine();
  }
}

static VOID LevelRoutine04(VOID)
{
  // NES: CORE GAMEPLAY FRAME

  // Demo mode: auto-play
  if (gDemoMode) {
    gIntroTimer++;

    // Check for START to break out
    if (gController1.Pressed & INPUT_START) {
      gDemoMode = 0;
      gIntroTimer = 0;
      gGameRoutineIndex = 1;  // Go to player select
      return;
    }

    // Timeout: return to title after ~8 seconds
    if (gIntroTimer > 3840) {  // ~64 seconds demo play
      gDemoMode = 0;
      gIntroTimer = 0;
      gGameRoutineIndex = 0;
      return;
    }

    // NES demo: replay recorded controller input
    // Simulate: walk right, jump occasionally, fire periodically
    INT32 phase = gIntroTimer % 180;
    ZeroMem(&gController1, sizeof(gController1));

    if (phase < 80) {
      gController1.Current |= INPUT_RIGHT;
    } else if (phase < 90) {
      gController1.Current |= INPUT_RIGHT;
      gController1.Current |= INPUT_A;  // A = jump
    } else if (phase < 110) {
      gController1.Current |= INPUT_RIGHT;
    } else if (phase < 150) {
      gController1.Current |= INPUT_RIGHT;
      if ((gIntroTimer % 10) == 0) gController1.Current |= INPUT_B;  // B = fire
    } else {
      gController1.Current |= INPUT_RIGHT;
      if ((gIntroTimer % 15) == 0) gController1.Current |= INPUT_B;
    }

    gController1.Pressed = gController1.Current & ~gController1.Previous;
    gController1.Previous = gController1.Current;
  }

  // Update both players
  UpdatePlayer(&gPlayers[0], &gController1);
  if (gPlayerMode == 1) {
    UpdatePlayer(&gPlayers[1], &gController2);
  }

  // Update bullets
  UpdatePlayerBullets();
  UpdateEnemyBullets();

  CheckBulletEnemyCollision();
  CheckEnemyBulletPlayerCollision();

  // Weapon items: update physics and spawn
  UpdateWeaponItems();

  UpdateEnemies();

  // Scroll is handled inside UpdatePlayer via ScrollUpdate(delta)

  // Handle player death
  if (gPlayers[0].DeathFlag == 2) {
    if (gPlayers[0].Lives > 0) {
      gPlayers[0].DeathFlag = 0;
      gPlayers[0].X = (60) << 8;
      // Find ground at current position
      {
        INT32 wx = 60 + (INT32)gLevelState.scrollX;
        INT32 gy = GAME_HEIGHT - 17;
        for (INT32 sy = GAME_HEIGHT - 1; sy > 100; sy--)
          if (GetBgCollision(wx, sy) == 1 || GetBgCollision(wx, sy) == 3) { gy = sy - 1; break; }
        gPlayers[0].Y = gy << 8;
      }
      gPlayers[0].InvincibilityTimer = 120;
      gPlayers[0].YVelocity = 0;
      gPlayers[0].OnGround = 0;
    } else {
      gPlayers[0].DeathFlag = 0;
      gPlayers[0].GameOverStatus = 1;
      SetLevelRoutine(0x06);
    }
  }

  if (gPlayerMode == 1 && gPlayers[1].DeathFlag == 2) {
    if (gPlayers[1].Lives > 0) {
      gPlayers[1].DeathFlag = 0;
      gPlayers[1].X = (60) << 8;
      {
        INT32 wx = 60 + (INT32)gLevelState.scrollX;
        INT32 gy = GAME_HEIGHT - 17;
        for (INT32 sy = GAME_HEIGHT - 1; sy > 100; sy--)
          if (GetBgCollision(wx, sy) == 1 || GetBgCollision(wx, sy) == 3) { gy = sy - 1; break; }
        gPlayers[1].Y = gy << 8;
      }
      gPlayers[1].InvincibilityTimer = 120;
      gPlayers[1].YVelocity = 0;
      gPlayers[1].OnGround = 0;
    } else {
      gPlayers[1].DeathFlag = 0;
      gPlayers[1].GameOverStatus = 1;
      if (gPlayers[0].GameOverStatus) SetLevelRoutine(0x06);
    }
  }

  // Check boss defeat
  if (gBossActive && CountBossParts() == 0) {
    gBossActive = 0;
    SetLevelRoutine(0x08);  // Boss defeated animation
  }
}

static VOID LevelRoutine05(VOID)
{
  // NES: Level complete - advance to next level or loop
  // For now: show completion, then restart level
  static UINT8 timer = 0;
  timer++;
  if (timer > 180) {  // 3 seconds
    timer = 0;
    // Restart the level (no multiple levels yet)
    gLevelState.scrollX = 0;
    SpawnLevelEnemies(0);
    SetLevelRoutine(0);
  }
}

static VOID LevelRoutine06(VOID)
{
  // NES: Game over screen - CONTINUE/END
  static UINT8 timer = 0;
  timer++;

  if ((gController1.Pressed & INPUT_START) && timer > 60) {
    timer = 0;
    gPlayers[0].Lives = 3;
    gPlayers[0].Score = 0;
    gPlayers[0].GameOverStatus = 0;
    gPlayers[0].CurrentWeapon = 0;
    gPlayers[0].DeathFlag = 0;
    gPlayers[0].X = (60) << 8;
    {
      INT32 wx = 60;
      INT32 gy = GAME_HEIGHT - 17;
      for (INT32 sy = GAME_HEIGHT - 1; sy > 100; sy--)
        if (GetBgCollision(wx, sy) == 1 || GetBgCollision(wx, sy) == 3) { gy = sy - 1; break; }
      gPlayers[0].Y = gy << 8;
    }
    gPlayers[0].InvincibilityTimer = 120;
    gPlayers[0].OnGround = 0;
    gLevelState.scrollX = 0;
    SpawnLevelEnemies(0);
    SetLevelRoutine(0);
    return;
  }

  if (timer > 600) {
    timer = 0;
    gGameRoutineIndex = 0;
    gLevelRoutineIndex = 0;
    gPlayers[0].GameOverStatus = 0;
    gPlayers[0].Lives = 3;
    gPlayers[0].CurrentWeapon = 0;
    gPlayers[0].DeathFlag = 0;
  }
}

static VOID LevelRoutine07(VOID)
{
  // NES: Game over, no continues
  static UINT8 timer = 0;
  timer++;
  if (timer > 120) {
    timer = 0;
    gGameRoutineIndex = 0;
    gLevelRoutineIndex = 0;
    gPlayers[0].GameOverStatus = 0;
    gPlayers[0].Lives = 3;
    gPlayers[0].CurrentWeapon = 0;
    gPlayers[0].DeathFlag = 0;
  }
}

static VOID LevelRoutine08(VOID)
{
  // NES: Boss destroyed animation - flash screen
  static UINT8 timer = 0;
  timer++;
  if (timer > 90) {  // 1.5 second flash
    timer = 0;
    AdvanceLevelRoutine(); // -> 09
  }
}

static VOID LevelRoutine09(VOID)
{
  // NES: End-of-level sequence - player walks right, level scrolls out
  static UINT8 timer = 0;
  timer++;

  // Auto-scroll to the right
  if (timer < 60) {
    ScrollUpdate(2);
  }

  // Make player walk right
  gPlayers[0].XVelocity = WALK_SPEED;
  gPlayers[0].FacingRight = 1;
  gPlayers[0].Moving = 1;
  UpdatePlayer(&gPlayers[0], &gController1);

  if (timer > 150) {  // 2.5 seconds
    timer = 0;
    SetLevelRoutine(5); // -> 05 (level complete init)
  }
}

static VOID LevelRoutine0a(VOID)
{
  // NES: Game over delay
  static UINT8 timer = 0;
  timer++;
  if (timer > 60) {
    timer = 0;
    SetLevelRoutine(5);
  }
}

// === PUBLIC API ===

VOID GameStateInit(VOID)
{
  gGameRoutineIndex  = 0;
  gLevelRoutineIndex = 0;
  gFrameCounter      = 0;
  gPlayerMode        = 0;
  gDemoMode          = 0;
}

VOID RunGameRoutine(VOID)
{
  gFrameCounter++;
  gGameRoutines[gGameRoutineIndex]();
}

VOID AdvanceLevelRoutine(VOID)
{
  gLevelRoutineIndex++;
  if (gLevelRoutineIndex >= LEVEL_ROUTINE_COUNT) {
    gLevelRoutineIndex = 0;
  }
}

VOID SetLevelRoutine(UINT8 index)
{
  gLevelRoutineIndex = index;
}
