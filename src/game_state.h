#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "types.h"

// Top-level game routines (matching NES GAME_ROUTINE_INDEX)
#define GAME_ROUTINE_COUNT  7

// Level routines (matching NES LEVEL_ROUTINE_INDEX)
#define LEVEL_ROUTINE_COUNT 11

extern UINT8 gGameRoutineIndex;
extern UINT8 gLevelRoutineIndex;
extern UINT8 gFrameCounter;
extern UINT8 gDemoMode;  // 1 during demo playback

// Initialize the game state machine
VOID GameStateInit(VOID);

// Execute one frame of the current game routine
VOID RunGameRoutine(VOID);

// Advance to the next level routine
VOID AdvanceLevelRoutine(VOID);

// Set a specific level routine (used for state transitions)
VOID SetLevelRoutine(UINT8 index);

#endif
