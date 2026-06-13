#ifndef CONTRA_TYPES_H
#define CONTRA_TYPES_H

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Protocol/GraphicsOutput.h>

// Game native resolution (NES)
#define GAME_WIDTH     256
#define GAME_HEIGHT    240

// Maximum integer scale factor (2x, 3x, 4x)
#define MAX_SCALE       4

// BGR pixel format for UEFI GOP: 0x00BBGGRR
typedef UINT32 Pixel;

// Colors in EFI_GRAPHICS_OUTPUT_BLT_PIXEL LE format: 0xAARRGGBB
// Byte 3 (AA) = reserved/alpha, 0xFF = opaque
#define COLOR_BLACK       0xFF000000
#define COLOR_WHITE       0xFFFFFFFF
#define COLOR_RED         0xFFFF0000
#define COLOR_GREEN       0xFF00FF00
#define COLOR_BLUE        0xFF0000FF
#define COLOR_YELLOW      0xFFFFFF00
#define COLOR_CYAN        0xFF00FFFF
#define COLOR_MAGENTA     0xFFFF00FF
#define COLOR_GRAY        0xFF808080
#define COLOR_DARK_GRAY   0xFF404040
#define COLOR_DARK_BLUE   0xFF000080
#define COLOR_ORANGE      0xFFFFA500
#define COLOR_LIGHT_BLUE  0xFF4040FF

// Game state structure
typedef struct {
  UINT32  frameCount;
  BOOLEAN running;
  BOOLEAN paused;
} GameState;

// GOP globals (defined in render.c)
extern EFI_GRAPHICS_OUTPUT_PROTOCOL  *gGOP;
extern UINT32                         gScreenWidth;
extern UINT32                         gScreenHeight;
extern Pixel                         *gBackBuffer;   // Full-screen buffer
extern Pixel                         *gGameBuffer;   // 256x240 game buffer
extern Pixel                         *gScaledBuffer; // Scaled output buffer
extern UINT32                         gScale;         // Current scale factor (1-4)
extern INT32                          gOffsetX;       // X offset to center
extern INT32                          gOffsetY;       // Y offset to center

// Global variables (defined in main.c)
extern GameState gGame;

// Function declarations - render.c
EFI_STATUS GraphicsInit(VOID);
VOID       GraphicsShutdown(VOID);
VOID       ClearGameBuffer(Pixel color);
VOID       PutPixel(UINT32 x, UINT32 y, Pixel color);
VOID       FillRect(INT32 x, INT32 y, UINT32 w, UINT32 h, Pixel color);
VOID       ScaleAndPresent(VOID);

// Function declarations - main.c
VOID ContraMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);

#endif
